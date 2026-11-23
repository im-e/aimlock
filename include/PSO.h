#pragma once
#include "Core.h"
#include "Shader.h"

class PSO
{
public:
    ID3D12PipelineState* pso = nullptr;

    void init(Core* core, Shader& shader, D3D12_INPUT_LAYOUT_DESC layout);
    void bind(Core* core);
};