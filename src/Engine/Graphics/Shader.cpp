#include "Engine/Graphics/Shader.h"
#include <d3d12shader.h> //ID3D12ShaderReflection
#include <filesystem>

#pragma comment(lib, "dxguid.lib")

ID3DBlob* Shader::compile(const std::wstring& filename, const char* entryPoint, const char* target)
{
    UINT flags = 0;
#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION; //so renderdoc/pix can step through the hlsl
#endif

    ID3DBlob* code = nullptr;
    ID3DBlob* errors = nullptr;
    HRESULT hr = D3DCompileFromFile(filename.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint, target, flags, 0, &code, &errors);

    if (errors)
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());  //compiler messages -> Output window
        errors->Release();
    }
    if (FAILED(hr))
    {
        MessageBoxA(NULL, "Shader failed to compile - check the Output window", entryPoint, MB_OK);
        return nullptr;
    }
    return code;
}

//compiled shaders are saved next to the source as e.g. shader.hlsl.VSMain.debug.cso
//next run, if the .cso is newer than the .hlsl it's loaded instead of compiling again
//note: only the main file's date is checked, so if you #include another file and change it, touch the main file
ID3DBlob* Shader::loadOrCompile(const std::wstring& filename, const char* entryPoint, const char* target)
{
#ifdef _DEBUG
    const wchar_t* config = L".debug.cso";
#else
    const wchar_t* config = L".release.cso";
#endif
    std::wstring cachePath = filename + L"." + std::wstring(entryPoint, entryPoint + strlen(entryPoint)) + config;

    std::error_code ec1, ec2;
    auto sourceTime = std::filesystem::last_write_time(filename, ec1);
    auto cacheTime = std::filesystem::last_write_time(cachePath, ec2);
    if (!ec1 && !ec2 && cacheTime >= sourceTime)
    {
        ID3DBlob* cached = nullptr;
        if (SUCCEEDED(D3DReadFileToBlob(cachePath.c_str(), &cached)))
            return cached;
    }

    ID3DBlob* code = compile(filename, entryPoint, target);
    if (code)
        D3DWriteBlobToFile(code, cachePath.c_str(), TRUE); //if saving fails we just compile again next time
    return code;
}

bool Shader::load(Core* core, const std::wstring& filename)
{
    //                   filename entrypoint shader target
    vs = loadOrCompile(filename, "VSMain", "vs_5_1");
    ps = loadOrCompile(filename, "PSMain", "ps_5_1");
    if (!vs || !ps)
        return false;

    //look inside the compiled code to find the constant buffers and their variables
    reflect(core, vs, vsConstantBuffers, VS_CBV_REGISTER, "VS");
    reflect(core, ps, psConstantBuffers, PS_CBV_REGISTER, "PS");
    return true;
}

void Shader::reflect(Core* core, ID3DBlob* shader, std::vector<ConstantBuffer>& buffers, UINT expectedRegister, const char* stage)
{
    ID3D12ShaderReflection* reflection = nullptr;
    CHECK(D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&reflection)));

    D3D12_SHADER_DESC desc;
    reflection->GetDesc(&desc);

    for (UINT i = 0; i < desc.ConstantBuffers; i++)
    {
        ID3D12ShaderReflectionConstantBuffer* constantBuffer = reflection->GetConstantBufferByIndex(i);
        D3D12_SHADER_BUFFER_DESC cbDesc;
        constantBuffer->GetDesc(&cbDesc);
        if (cbDesc.Type != D3D_CT_CBUFFER)
            continue; //tbuffers etc

        ConstantBuffer buffer;
        buffer.name = cbDesc.Name;
        bool used = false;

        for (UINT j = 0; j < cbDesc.Variables; j++)
        {
            ID3D12ShaderReflectionVariable* var = constantBuffer->GetVariableByIndex(j);
            D3D12_SHADER_VARIABLE_DESC vDesc;
            var->GetDesc(&vDesc);

            ConstantBufferVariable bufferVariable;
            bufferVariable.offset = vDesc.StartOffset;
            bufferVariable.size = vDesc.Size;
            buffer.constantBufferData.insert({ vDesc.Name, bufferVariable });

            if (vDesc.uFlags & D3D_SVF_USED)
                used = true;
        }

        //which register is it bound to? it has to match the root signature slot for this stage
        D3D12_SHADER_INPUT_BIND_DESC bindDesc = {};
        bool bound = SUCCEEDED(reflection->GetResourceBindingDescByName(cbDesc.Name, &bindDesc));
        if (!bound || bindDesc.BindPoint != expectedRegister)
        {
            if (used) //only complain if the shader actually reads it
            {
                char msg[256];
                sprintf_s(msg, "Shader (%s): cbuffer '%s' is not on register(b%u) so it won't be bound - the root signature only has b%u for this stage\n",
                    stage, cbDesc.Name, expectedRegister, expectedRegister);
                OutputDebugStringA(msg);
            }
            continue;
        }

        //cbDesc.Size is the real size including hlsl's padding, so don't add the variable sizes up yourself
        buffer.init(core, cbDesc.Size);
        buffers.push_back(buffer);
    }

    reflection->Release();
}

void Shader::updateConstant(std::vector<ConstantBuffer>& buffers, const std::string& variableName, const void* data, const char* stage)
{
    bool found = false;
    for (ConstantBuffer& cb : buffers)
        if (cb.update(variableName, data))
            found = true;

    if (!found && warned.insert(variableName).second)
    {
        std::string msg = std::string("Shader (") + stage + "): no constant called '" + variableName +
            "' - check the spelling, or the compiler may have removed it because it's unused\n";
        OutputDebugStringA(msg.c_str());
    }
}

void Shader::updateConstantVS(const std::string& variableName, const void* data)
{
    updateConstant(vsConstantBuffers, variableName, data, "VS");
}

void Shader::updateConstantPS(const std::string& variableName, const void* data)
{
    updateConstant(psConstantBuffers, variableName, data, "PS");
}

void Shader::apply(Core* core)
{
    ID3D12GraphicsCommandList4* cmd = core->getCommandList();
    for (ConstantBuffer& cb : vsConstantBuffers)
    {
        cb.upload();
        cmd->SetGraphicsRootConstantBufferView(VS_CBV_ROOT_INDEX, cb.getGPUAddress());
        cb.next();
    }
    for (ConstantBuffer& cb : psConstantBuffers)
    {
        cb.upload();
        cmd->SetGraphicsRootConstantBufferView(PS_CBV_ROOT_INDEX, cb.getGPUAddress());
        cb.next();
    }
}

void Shader::release()
{
    safeRelease(vs);
    safeRelease(ps);
    for (ConstantBuffer& cb : vsConstantBuffers) cb.release();
    for (ConstantBuffer& cb : psConstantBuffers) cb.release();
    vsConstantBuffers.clear();
    psConstantBuffers.clear();
}
