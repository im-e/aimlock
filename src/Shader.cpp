#include "Shader.h"

ID3DBlob* Shader::compile(const std::wstring& filename, const char* entryPoint, const char* target)
{
    UINT flags = 0;
#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;  // readable in PIX/RenderDoc
#endif

    ID3DBlob* code = nullptr;
    ID3DBlob* errors = nullptr;
    HRESULT hr = D3DCompileFromFile(filename.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint, target, flags, 0, &code, &errors);

    if (errors)
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());  // compiler messages -> Output window
        errors->Release();
    }
    if (FAILED(hr))
    {
        MessageBoxA(NULL, "Shader failed to compile - check the Output window", entryPoint, MB_OK);
        return nullptr;
    }
    return code;
}

bool Shader::load(const std::wstring& filename)
{
    vs = compile(filename, "VSMain", "vs_5_1");
    ps = compile(filename, "PSMain", "ps_5_1");
    return vs && ps;
}