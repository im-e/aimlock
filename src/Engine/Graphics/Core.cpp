#include "Engine/Graphics/Core.h"

void Core::init(HWND hwnd, int _width, int _height)
{
    width = _width;
    height = _height;

#ifdef _DEBUG
    //turn on the debug layer
    ID3D12Debug* debug = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
    {
        debug->EnableDebugLayer();
        debug->Release();
    }
#endif
    //dxgi is the part of windows that knows about gpus and displays
    //the factory is how you get in, it lists the gpus and makes the swapchain
    IDXGIFactory6* factory = nullptr;
    CHECK(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));

    findAdapter(factory);

    //pretty much everything after this gets created through the device
    CHECK(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device)));

    createQueues();
    createSwapchain(factory, hwnd);
    factory->Release(); //not needed once the swapchain exists

    createCommandLists();
    createBackbufferViews();
    createDepthBuffer();

    //a fence is a 64 bit counter both the cpu and gpu can see, used to wait for the gpu to catch up
    for (int i = 0; i < BUFFER_COUNT; i++)
        graphicsQueueFence[i].create(device);

    //draw to the whole window
    viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
    scissorRect = { 0, 0, width, height };

    createRootSignature();
}

void Core::findAdapter(IDXGIFactory6* factory)
{
    std::vector<IDXGIAdapter1*> adapters;
    IDXGIAdapter1* adapterf = nullptr;
    //list every gpu, then keep the one with the most dedicated vram (the real gpu on a laptop)
    for (UINT i = 0; factory->EnumAdapters1(i, &adapterf) != DXGI_ERROR_NOT_FOUND; i++)
        adapters.push_back(adapterf);

    SIZE_T maxVideoMemory = 0;
    size_t useAdapterIndex = 0;
    for (size_t i = 0; i < adapters.size(); i++)
    {
        DXGI_ADAPTER_DESC desc;
        adapters[i]->GetDesc(&desc);
        if (desc.DedicatedVideoMemory > maxVideoMemory)
        {
            maxVideoMemory = desc.DedicatedVideoMemory;
            useAdapterIndex = i;
        }
    }
    adapter = adapters[useAdapterIndex];

    //release the ones we're not using
    for (size_t i = 0; i < adapters.size(); i++)
        if (i != useAdapterIndex) adapters[i]->Release();
}

void Core::createQueues()
{
    //a queue is the gpu's inbox, finished command lists go in and the gpu works through them in order
    D3D12_COMMAND_QUEUE_DESC desc = {};
    //direct = drawing
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    CHECK(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&graphicsQueue)));
    //copy = uploading textures/meshes in the background
    desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    CHECK(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&copyQueue)));
    //compute = compute shaders
    desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    CHECK(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&computeQueue)));
}

void Core::createSwapchain(IDXGIFactory6* factory, HWND hwnd)
{
    //the swapchain is the images attached to the window. one is on screen (front buffer) while
    //we draw into the other (back buffer), then they swap when the frame is done. double buffering
    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.Width = width;
    scDesc.Height = height;
    scDesc.SampleDesc.Count = 1; //flip model swapchains can't be multisampled, msaa will go on an off-screen target
    scDesc.SampleDesc.Quality = 0;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //the back buffers will be drawn into, so they need to be render targets
    scDesc.BufferCount = BUFFER_COUNT;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    IDXGISwapChain1* swapChain1 = nullptr;
    CHECK(factory->CreateSwapChainForHwnd(graphicsQueue, hwnd, &scDesc, NULL, NULL, &swapChain1));
    CHECK(swapChain1->QueryInterface(IID_PPV_ARGS(&swapchain))); //upgrade 1 -> 3
    swapChain1->Release();
}

void Core::createCommandLists()
{
    //the command list is what you record instructions on (clear this, draw that)
    //the allocator is the memory those instructions actually get written into
    //while the gpu is reading frame n we're already writing frame n+1, and you can't write over memory
    //the gpu is still reading, so each back buffer gets its own allocator and list
    for (int i = 0; i < BUFFER_COUNT; i++)
    {
        CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&graphicsCommandAllocator[i])));
        CHECK(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&graphicsCommandList[i])));
    }
}

void Core::createBackbufferViews()
{
    //a texture on its own is just memory, a view tells the gpu how to use it
    //views live in a descriptor heap, which is basically an array of them
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = BUFFER_COUNT;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    CHECK(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&backbufferHeap)));

    D3D12_CPU_DESCRIPTOR_HANDLE handle = backbufferHeap->GetCPUDescriptorHandleForHeapStart();
    UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    //write a view for each swapchain image, then step the handle on one slot
    //the slot size has to come from the device because it's different on every gpu
    for (int i = 0; i < BUFFER_COUNT; i++)
    {
        CHECK(swapchain->GetBuffer(i, IID_PPV_ARGS(&backbuffers[i])));
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //the gpu applies gamma correction when writing
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        device->CreateRenderTargetView(backbuffers[i], &rtvDesc, handle);
        handle.ptr += rtvSize;
    }
}

void Core::createDepthBuffer()
{
    //the depth buffer stores how far away each pixel is, so nearer things hide things behind them
    //same idea as the back buffers: a heap slot for the view, the texture itself, then the view
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    CHECK(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));
    dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CLEAR_VALUE depthClearValue = {};
    depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthClearValue.DepthStencil.Depth = 1.0f;
    depthClearValue.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heapprops = {};
    heapprops.Type = D3D12_HEAP_TYPE_DEFAULT; //vram that only the gpu can touch, the fastest option
    heapprops.CreationNodeMask = 1;
    heapprops.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.Width = width;
    dsvDesc.Height = height;
    dsvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dsvDesc.DepthOrArraySize = 1;
    dsvDesc.MipLevels = 1;
    dsvDesc.SampleDesc.Count = 1;
    dsvDesc.SampleDesc.Quality = 0;
    dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    dsvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    //committed resource = the texture and a heap for it in one call (the easy way from the slides)
    CHECK(device->CreateCommittedResource(&heapprops, D3D12_HEAP_FLAG_NONE, &dsvDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&dsv)));
    device->CreateDepthStencilView(dsv, &depthStencilDesc, dsvHandle);
}

void Core::createRootSignature()
{
    //root parameters are the slots the shaders read from
    //0: vertex shader constant buffer (b0), 1: pixel shader constant buffer (b1)
    //textures will need descriptor tables and samplers adding here
    D3D12_ROOT_PARAMETER params[2] = {};

    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].Descriptor.ShaderRegister = 0; //register(b0)
    params[0].Descriptor.RegisterSpace = 0;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[1].Descriptor.ShaderRegister = 1; //register(b1)
    params[1].Descriptor.RegisterSpace = 0;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = _countof(params);
    desc.pParameters = params;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* serialized = nullptr;
    ID3DBlob* errors = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors);
    if (errors) //print why the root signature is invalid before CHECK stops the program
        OutputDebugStringA((char*)errors->GetBufferPointer());
    CHECK(hr);
    CHECK(device->CreateRootSignature(0, serialized->GetBufferPointer(),
        serialized->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

    serialized->Release();
    if (errors) errors->Release();
}

void Core::uploadResource(ID3D12Resource* dstResource, const void* data, unsigned int size,
    D3D12_RESOURCE_STATES targetState, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* texFootprint)
{
    //temporary buffer in the upload heap (cpu can write it, gpu can read it)
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = size;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ID3D12Resource* uploadBuffer = nullptr;
    CHECK(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer)));

    //copy the cpu data into it
    void* mapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 }; //we won't read it back
    CHECK(uploadBuffer->Map(0, &readRange, &mapped));
    memcpy(mapped, data, size);
    uploadBuffer->Unmap(0, nullptr);

    //get the gpu to copy it across into the fast default heap resource
    resetCommandList();
    if (texFootprint != nullptr) //textures, will need looping over every mip once those are in
    {
        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = uploadBuffer;
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint = *texFootprint;

        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = dstResource;
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = 0;

        getCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    }
    else //buffers (vertices, indices etc)
    {
        getCommandList()->CopyBufferRegion(dstResource, 0, uploadBuffer, 0, size);
    }

    //switch it from "being copied into" to whatever it's actually for
    Barrier::add(dstResource, D3D12_RESOURCE_STATE_COPY_DEST, targetState, getCommandList());

    //submit and wait, the upload buffer can only go once the gpu is done with it
    runCommandList();
    flushGraphicsQueue();
    uploadBuffer->Release();
}

//command list helpers
void Core::resetCommandList()
{
    UINT frameIndex = swapchain->GetCurrentBackBufferIndex();
    //wipes the old commands and starts recording again
    CHECK(graphicsCommandAllocator[frameIndex]->Reset());
    CHECK(graphicsCommandList[frameIndex]->Reset(graphicsCommandAllocator[frameIndex], NULL));
}

ID3D12GraphicsCommandList4* Core::getCommandList()
{
    return graphicsCommandList[swapchain->GetCurrentBackBufferIndex()];
}

void Core::runCommandList()
{
    //close = done recording
    CHECK(getCommandList()->Close());
    ID3D12CommandList* lists[] = { getCommandList() };
    graphicsQueue->ExecuteCommandLists(1, lists); //into the gpu's inbox, it starts working on it from here
}

//every frame

void Core::beginFrame(Color clearColor)
{
    //the cpu is usually a frame ahead of the gpu, so wait until it's finished with the buffer we're about to use
    UINT frameIndex = swapchain->GetCurrentBackBufferIndex(); //0 or 1
    graphicsQueueFence[frameIndex].wait();

    //find this back buffer's view in the heap
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = backbufferHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += frameIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    resetCommandList();
    //switch the buffer from "being shown" to "being drawn to"
    Barrier::add(backbuffers[frameIndex], D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        getCommandList());

    getCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    getCommandList()->RSSetViewports(1, &viewport);
    getCommandList()->RSSetScissorRects(1, &scissorRect);

    //wipe last frame's picture and reset depth to 1 (as far away as possible)
    float color[4] = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
    getCommandList()->ClearRenderTargetView(rtvHandle, color, 0, NULL);
    getCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
}

void Core::finishFrame()
{
    UINT frameIndex = swapchain->GetCurrentBackBufferIndex();
    //back to "ready to show"
    Barrier::add(backbuffers[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT, getCommandList());
    runCommandList();
    graphicsQueueFence[frameIndex].signal(graphicsQueue); //so beginFrame knows when this buffer is free again
    CHECK(swapchain->Present(1, 0)); //1 = vsync on, a toggle for this is on the list since aim trainers want low latency
}

void Core::flushGraphicsQueue()
{
    //put a fence at the very end of the queue and wait for it, so the gpu has finished everything
    graphicsQueueFence[0].signal(graphicsQueue);
    graphicsQueueFence[0].wait();
}

void Core::release()
{
    flushGraphicsQueue(); //never free something the gpu might still be using

    safeRelease(rootSignature);

    safeRelease(dsv);
    safeRelease(dsvHeap);
    for (int i = 0; i < BUFFER_COUNT; i++)
        safeRelease(backbuffers[i]);
    safeRelease(backbufferHeap);

    for (int i = 0; i < BUFFER_COUNT; i++)
    {
        safeRelease(graphicsCommandList[i]);
        safeRelease(graphicsCommandAllocator[i]);
        graphicsQueueFence[i].release();
    }

    safeRelease(swapchain);
    safeRelease(computeQueue);
    safeRelease(copyQueue);
    safeRelease(graphicsQueue);
    safeRelease(device); //last, everything above was made by it
    safeRelease(adapter);
}
