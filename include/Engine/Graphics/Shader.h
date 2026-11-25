#pragma once
#include "Engine/Graphics/Core.h"
#include "Engine/Graphics/ConstantBuffer.h"
#include <string> //std::wstring
#include <vector>
#include <set>

//the root signature has one cbv per stage, so these must match Core::createRootSignature
//vertex shader cbuffer: register(b0) -> root index 0
//pixel shader cbuffer:  register(b1) -> root index 1
static const UINT VS_CBV_ROOT_INDEX = 0;
static const UINT VS_CBV_REGISTER = 0;
static const UINT PS_CBV_ROOT_INDEX = 1;
static const UINT PS_CBV_REGISTER = 1;

class Shader
{
public:
    ID3DBlob* vs = nullptr; //compiled vertex shader
    ID3DBlob* ps = nullptr; //compiled pixel shader

    //built automatically from the shader code by reflection
    std::vector<ConstantBuffer> vsConstantBuffers;
    std::vector<ConstantBuffer> psConstantBuffers;

    bool load(Core* core, const std::wstring& filename);

    //set a variable by its hlsl name, e.g. updateConstantPS("time", &time)
    void updateConstantVS(const std::string& variableName, const void* data);
    void updateConstantPS(const std::string& variableName, const void* data);

    //copy constant buffers to the gpu and bind them - call after binding the pso, before drawing
    void apply(Core* core);

    void release();

private:
    std::set<std::string> warned; //so a typo is reported once, not every frame

    ID3DBlob* compile(const std::wstring& filename, const char* entryPoint, const char* target);
    ID3DBlob* loadOrCompile(const std::wstring& filename, const char* entryPoint, const char* target);
    void reflect(Core* core, ID3DBlob* shader, std::vector<ConstantBuffer>& buffers, UINT expectedRegister, const char* stage);
    void updateConstant(std::vector<ConstantBuffer>& buffers, const std::string& variableName, const void* data, const char* stage);
};
