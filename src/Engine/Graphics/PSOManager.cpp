#include "Engine/Graphics/PSOManager.h"

void PSOManager::createPSO(Core* core, const std::string& name, ID3DBlob* vs, ID3DBlob* ps, D3D12_INPUT_LAYOUT_DESC layout)
{
    //avoid creating the same state twice
    if (psos.find(name) != psos.end())
        return;

    //fills in the D3D12_GRAPHICS_PIPELINE_STATE_DESC
    //each part maps onto a stage of the pipeline
    //input assembler -> vertex shader -> rasteriser -> pixel shader -> depth test/blend -> render target
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

    //which inputs and which programs
    desc.pRootSignature = core->rootSignature;
    desc.InputLayout = layout;
    desc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    desc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };

    //rasterizer settings
    D3D12_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D12_FILL_MODE_SOLID; //can try wireframe
    rasterDesc.CullMode = D3D12_CULL_MODE_BACK;  //cull from the back
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.MultisampleEnable = FALSE;
    rasterDesc.AntialiasedLineEnable = FALSE;
    rasterDesc.ForcedSampleCount = 0;
    rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    desc.RasterizerState = rasterDesc;

    //depth test
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencilDesc.StencilEnable = FALSE;
    //valid stencil values even though it's off, so turning it on later can't use garbage
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
        { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
    depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depthStencilDesc.FrontFace = defaultStencilOp;
    depthStencilDesc.BackFace = defaultStencilOp;
    desc.DepthStencilState = depthStencilDesc;

    //blending: off, but every render target gets valid defaults (src * 1 + dest * 0)
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlend =
    {
        FALSE, FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL
    };
    for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
        blendDesc.RenderTarget[i] = defaultRenderTargetBlend;
    desc.BlendState = blendDesc;

    //what we're drawing and what we're drawing into
    desc.SampleMask = UINT_MAX;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //matches the rtv
    desc.DSVFormat = DXGI_FORMAT_D32_FLOAT; //matches depth buffer
    desc.SampleDesc.Count = 1;

    //create the state and store it under its name
    ID3D12PipelineState* pso = nullptr;
    CHECK(core->device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso)));
    psos.insert({ name, pso });
}

void PSOManager::createPSO(Core* core, const std::string& name, Shader& shader, D3D12_INPUT_LAYOUT_DESC layout)
{
    createPSO(core, name, shader.vs, shader.ps, layout);
}

void PSOManager::bind(Core* core, const std::string& name)
{
    //find rather than psos[name], which would quietly insert a null pso for a typo
    auto it = psos.find(name);
    if (it == psos.end())
    {
        OutputDebugStringA(("PSOManager: no pso called " + name + "\n").c_str());
        return;
    }
    core->getCommandList()->SetGraphicsRootSignature(core->rootSignature);
    core->getCommandList()->SetPipelineState(it->second);
}

void PSOManager::release()
{
    for (auto& pair : psos)
        pair.second->Release();
    psos.clear();
}
