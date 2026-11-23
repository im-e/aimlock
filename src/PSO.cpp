#include "PSO.h"

void PSO::init(Core* core, Shader& shader, D3D12_INPUT_LAYOUT_DESC layout)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

    // Which inputs and which programs
    desc.pRootSignature = core->rootSignature;
    desc.InputLayout = layout;
    desc.VS = { shader.vs->GetBufferPointer(), shader.vs->GetBufferSize() };
    desc.PS = { shader.ps->GetBufferPointer(), shader.ps->GetBufferSize() };

    // Rasterizer: how triangles become pixels
    desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;   // WIREFRAME is fun to try
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;    // draw both sides for now
    desc.RasterizerState.FrontCounterClockwise = FALSE;
    desc.RasterizerState.DepthClipEnable = TRUE;

    // Blending: off, just overwrite the pixel
    desc.BlendState.RenderTarget[0].BlendEnable = FALSE;
    desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // Depth test: keep the nearest pixel
    desc.DepthStencilState.DepthEnable = TRUE;
    desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    desc.DepthStencilState.StencilEnable = FALSE;

    // What we're drawing and what we're drawing into
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;   // must match your RTV exactly
    desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;                 // must match your depth buffer
    desc.SampleMask = UINT_MAX;
    desc.SampleDesc.Count = 1;

    core->device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
}

void PSO::bind(Core* core)
{
    core->getCommandList()->SetGraphicsRootSignature(core->rootSignature);
    core->getCommandList()->SetPipelineState(pso);
}