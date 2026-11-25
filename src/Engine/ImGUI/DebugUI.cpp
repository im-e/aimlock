#include "Engine/ImGUI/DebugUI.h"
#include "Engine/External/imgui/imgui.h"
#include "Engine/External/imgui/imgui_impl_win32.h"
#include "Engine/External/imgui/imgui_impl_dx12.h"
#include <vector>

//imgui needs somewhere to put its font texture's view (an SRV), and the heap must be shader visible
static ID3D12DescriptorHeap* srvHeap = nullptr;
static UINT srvIncrement = 0;
static std::vector<UINT> freeSlots; //simple free list so slots can be reused if imgui rebuilds its font

static const UINT SRV_HEAP_SIZE = 64;

//imgui calls these when it wants a slot / is done with one
static void allocSrv(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu)
{
    UINT slot = freeSlots.back();
    freeSlots.pop_back();
    outCpu->ptr = srvHeap->GetCPUDescriptorHandleForHeapStart().ptr + (SIZE_T)slot * srvIncrement;
    outGpu->ptr = srvHeap->GetGPUDescriptorHandleForHeapStart().ptr + (UINT64)slot * srvIncrement;
}

static void freeSrv(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE)
{
    UINT slot = (UINT)((cpu.ptr - srvHeap->GetCPUDescriptorHandleForHeapStart().ptr) / srvIncrement);
    freeSlots.push_back(slot);
}

void DebugUI::init(Core* core, Window* win)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = SRV_HEAP_SIZE;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; //the gpu reads it while drawing
    CHECK(core->device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap)));
    srvIncrement = core->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    for (UINT i = SRV_HEAP_SIZE; i > 0; i--)
        freeSlots.push_back(i - 1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(win->hwnd);

    ImGui_ImplDX12_InitInfo info;
    info.Device = core->device;
    info.CommandQueue = core->graphicsQueue;        //used to upload the font texture
    info.NumFramesInFlight = Core::BUFFER_COUNT;
    info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //must match your render target view
    info.DSVFormat = DXGI_FORMAT_D32_FLOAT;          //your depth buffer is bound while imgui draws
    info.SrvDescriptorHeap = srvHeap;
    info.SrvDescriptorAllocFn = allocSrv;
    info.SrvDescriptorFreeFn = freeSrv;
    ImGui_ImplDX12_Init(&info);
}

void DebugUI::beginFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void DebugUI::render(Core* core)
{
    ImGui::Render();
    ID3D12DescriptorHeap* heaps[] = { srvHeap };
    core->getCommandList()->SetDescriptorHeaps(1, heaps); //imgui's shader reads its font through this heap
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), core->getCommandList());
}

void DebugUI::shutdown()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    safeRelease(srvHeap);
}