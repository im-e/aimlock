#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <vector>
#include "Maths/Color.h"

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler.lib")

class GPUFence
{
public:
    ID3D12Fence* fence = nullptr;
    HANDLE eventHandle = nullptr;
    UINT64 value = 0;

    void create(ID3D12Device5* device)
    {
        device->CreateFence(value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        eventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    void signal(ID3D12CommandQueue* queue) { queue->Signal(fence, ++value); }
    void wait()
    {
        if (fence->GetCompletedValue() < value)
        {
            fence->SetEventOnCompletion(value, eventHandle);
            WaitForSingleObject(eventHandle, INFINITE);
        }
    }
    ~GPUFence()
    {
        CloseHandle(eventHandle);
        fence->Release();
    }
};

class Barrier //barriers ensure resources are in the right state
{
public:
    static void add(ID3D12Resource* res, D3D12_RESOURCE_STATES first,
        D3D12_RESOURCE_STATES second, ID3D12GraphicsCommandList4* commandList)
    {
        D3D12_RESOURCE_BARRIER rb = {};
        rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        rb.Transition.pResource = res;
        rb.Transition.StateBefore = first;
        rb.Transition.StateAfter = second;
        rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &rb);
    }
};

class Core
{
public:
    static const int BUFFER_COUNT = 2;

    //core interfaces
    IDXGIAdapter1* adapter = nullptr;
    ID3D12Device5* device = nullptr;
    ID3D12CommandQueue* graphicsQueue = nullptr;
    ID3D12CommandQueue* copyQueue = nullptr;
    ID3D12CommandQueue* computeQueue = nullptr;
    IDXGISwapChain3* swapchain = nullptr;
    ID3D12RootSignature* rootSignature = nullptr;

    //allocator and command list
    ID3D12CommandAllocator* graphicsCommandAllocator[BUFFER_COUNT] = {};
    ID3D12GraphicsCommandList4* graphicsCommandList[BUFFER_COUNT] = {};

    //back buffers
    ID3D12DescriptorHeap* backbufferHeap = nullptr;
    ID3D12Resource* backbuffers[BUFFER_COUNT] = {};

    //depth buffer
    ID3D12DescriptorHeap* dsvHeap = nullptr;
    ID3D12Resource* dsv = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};

    GPUFence graphicsQueueFence[BUFFER_COUNT];

    D3D12_VIEWPORT viewport = {};
    D3D12_RECT scissorRect = {};
    int width = 0;
    int height = 0;

    void init(HWND hwnd, int _width, int _height);

    void resetCommandList();
    ID3D12GraphicsCommandList4* getCommandList();
    void runCommandList();

    void beginFrame(Color clearColor);
    void finishFrame();
    void flushGraphicsQueue();

private:
    void findAdapter(IDXGIFactory6* factory);
    void createQueues();
    void createSwapchain(IDXGIFactory6* factory, HWND hwnd);
    void createCommandLists();
    void createBackbufferViews();
    void createDepthBuffer();
    void createRootSignature();
};