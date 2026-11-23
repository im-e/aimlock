#pragma once
#include "Core.h"
#include "Shader.h"

class PSO //pipeline state object - holds almost every setting the gpu needs to draw
{
public:
    ID3D12PipelineState* pso = nullptr;

    void init(Core* core, Shader& shader, D3D12_INPUT_LAYOUT_DESC layout);
    void bind(Core* core);
};