#pragma once
#include "Engine/Graphics/Core.h"
#include <cstring>
#include <string>
#include <map>
#include <vector>

//where one variable lives inside a constant buffer (found by reflecting the shader)
struct ConstantBufferVariable
{
    unsigned int offset = 0; //bytes from the start of the buffer
    unsigned int size = 0;   //bytes
};

class ConstantBuffer
{
public:
    std::string name; //cbuffer name in the hlsl
    std::map<std::string, ConstantBufferVariable> constantBufferData; //variable name -> offset + size

    ID3D12Resource* constantBuffer = nullptr;
    unsigned char* buffer = nullptr; //gpu-visible memory, mapped once and kept mapped
    std::vector<unsigned char> cpuData; //cpu copy of the whole buffer, written by update()
    unsigned int dataSizeInBytes = 0; //real size of the cbuffer
    unsigned int cbSizeInBytes = 0; //one slot, rounded up to 256
    unsigned int maxDrawCalls = 0; //number of slots in the ring
    unsigned int offsetIndex = 0; //slot in use

    void init(Core* core, unsigned int sizeInBytes, unsigned int _maxDrawCalls = 1024)
    {
        dataSizeInBytes = sizeInBytes;
        cbSizeInBytes = (sizeInBytes + 255) & ~255; //cbv addresses must be 256 byte aligned
        maxDrawCalls = _maxDrawCalls;
        offsetIndex = 0;
        cpuData.assign(sizeInBytes, 0);

        D3D12_HEAP_PROPERTIES heapprops = {};
        heapprops.Type = D3D12_HEAP_TYPE_UPLOAD; //cpu writes every frame, so upload heap
        heapprops.CreationNodeMask = 1;
        heapprops.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = (UINT64)cbSizeInBytes * maxDrawCalls;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        CHECK(core->device->CreateCommittedResource(&heapprops, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer)));

        D3D12_RANGE readRange = { 0, 0 };
        CHECK(constantBuffer->Map(0, &readRange, (void**)&buffer)); //never unmapped
    }

    bool has(const std::string& variableName) const
    {
        return constantBufferData.find(variableName) != constantBufferData.end();
    }

    //update one named variable, no need to worry about alignment or padding
    //data must point to at least as many bytes as the hlsl variable uses
    bool update(const std::string& variableName, const void* data)
    {
        auto it = constantBufferData.find(variableName);
        if (it == constantBufferData.end())
            return false;
        memcpy(cpuData.data() + it->second.offset, data, it->second.size);
        return true;
    }

    //update the whole buffer from a cpu struct
    void update(const void* data, unsigned int sizeInBytes)
    {
        memcpy(cpuData.data(), data, sizeInBytes < dataSizeInBytes ? sizeInBytes : dataSizeInBytes);
    }

    //copy the cpu copy into the current slot - call right before binding
    //copying the whole thing each draw means variables you didn't touch keep their last value
    //instead of whatever was left in that slot 1024 draws ago
    void upload()
    {
        memcpy(buffer + (offsetIndex * cbSizeInBytes), cpuData.data(), dataSizeInBytes);
    }

    //gpu address of the current slot
    D3D12_GPU_VIRTUAL_ADDRESS getGPUAddress() const
    {
        return constantBuffer->GetGPUVirtualAddress() + ((UINT64)offsetIndex * cbSizeInBytes);
    }

    //move on so the next draw doesn't overwrite data the gpu hasn't read yet
    void next()
    {
        offsetIndex++;
        if (offsetIndex >= maxDrawCalls)
            offsetIndex = 0;
    }

    void release()
    {
        safeRelease(constantBuffer);
        buffer = nullptr;
    }
};
