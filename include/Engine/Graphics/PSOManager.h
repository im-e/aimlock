#pragma once
#include "Engine/Graphics/Core.h"
#include "Engine/Graphics/Shader.h"
#include <string>
#include <unordered_map>

//pipeline state objects hold almost every setting the gpu needs to draw
//they're immutable, so we make each one once at startup and look it up by name
class PSOManager
{
public:
    std::unordered_map<std::string, ID3D12PipelineState*> psos;

    //does nothing if a pso with this name already exists
    void createPSO(Core* core, const std::string& name, ID3DBlob* vs, ID3DBlob* ps, D3D12_INPUT_LAYOUT_DESC layout);
    void createPSO(Core* core, const std::string& name, Shader& shader, D3D12_INPUT_LAYOUT_DESC layout);

    void bind(Core* core, const std::string& name);
    void release();
};
