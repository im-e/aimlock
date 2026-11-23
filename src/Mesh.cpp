#include <Mesh.h>
#include <cstring>

D3D12_INPUT_LAYOUT_DESC Mesh::inputLayout()
{
    static D3D12_INPUT_ELEMENT_DESC elements[] =
    {
        //name       index  format                         slot  byte offset
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOUR",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    return { elements, _countof(elements) };
}

void Mesh::init(Core* core, const Vertex* vertices, unsigned int count)
{
    //get the vertex count
    vertexCount = count;
    UINT size = sizeof(Vertex) * count;

    //
    D3D12_HEAP_PROPERTIES heapprops = {};
    heapprops.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapprops.CreationNodeMask = 1;
    heapprops.VisibleNodeMask = 1;


    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;          //buffers have no pixel format
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; //required for buffers

    core->device->CreateCommittedResource(&heapprops, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));

    //copy the vertices in
    void* data = nullptr;
    D3D12_RANGE readRange = { 0, 0 };   // "I won't read this back on the CPU"
    vertexBuffer->Map(0, &readRange, &data);
    memcpy(data, vertices, size);
    vertexBuffer->Unmap(0, nullptr);

    //A view: where the data is, how big each vertex is, how big the whole thing is
    vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbView.StrideInBytes = sizeof(Vertex);
    vbView.SizeInBytes = size;
}

void Mesh::draw(Core* core)
{
    ID3D12GraphicsCommandList4* cmd = core->getCommandList();
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // every 3 vertices = 1 triangle
    cmd->IASetVertexBuffers(0, 1, &vbView);
    cmd->DrawInstanced(vertexCount, 1, 0, 0);
}