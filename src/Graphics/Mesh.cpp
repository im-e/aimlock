#include "Graphics/Mesh.h"
#include <cstring>  //memcpy, memset (GEMLoader needs it but doesn't include it)
#include <cstddef>  //offsetof
#include <map>      //GEMLoader uses std::map but doesn't include it
#include <vector>
#include "External/GEMLoader.h"

D3D12_INPUT_LAYOUT_DESC Mesh::inputLayout()
{
    static D3D12_INPUT_ELEMENT_DESC elements[] =
    {
        //name       index  format                         slot  byte offset (worked out by the compiler)
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex, normal),   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOUR",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, color),   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    return { elements, _countof(elements) };
}

void Mesh::init(Core* core, const Vertex* vertices, unsigned int count)
{
    //get the vertex count
    vertexCount = count;
    UINT size = sizeof(Vertex) * count;

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
    D3D12_RANGE readRange = { 0, 0 };   //"i wont read this back on the cpu"
    vertexBuffer->Map(0, &readRange, &data);
    memcpy(data, vertices, size);
    vertexBuffer->Unmap(0, nullptr);

    //view
    vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbView.StrideInBytes = sizeof(Vertex);
    vbView.SizeInBytes = size;
}

bool Mesh::loadGEM(Core* core, const std::string& filename)
{
    std::vector<GEMLoader::GEMMesh> gemMeshes;
    GEMLoader::GEMModelLoader loader;
    loader.load(filename, gemMeshes);

    //expand the index list into a flat list: every 3 vertices = 1 triangle
    std::vector<Vertex> vertices;
    for (GEMLoader::GEMMesh& gm : gemMeshes)
    {
        for (unsigned int index : gm.indices)
        {
            GEMLoader::GEMVec3 p, n;
            if (!gm.verticesStatic.empty())
            {
                p = gm.verticesStatic[index].position;
                n = gm.verticesStatic[index].normal;
            }
            else //for animated models
            {
                p = gm.verticesAnimated[index].position;
                n = gm.verticesAnimated[index].normal;
            }

            Vertex v;
            v.position = Vec3(p.x, p.y, p.z);
            v.normal = Vec3(n.x, n.y, n.z);
            //debug colour: show the normal as a colour (-1..1 mapped to 0..1)
            v.color = Color(n.x * 0.5f + 0.5f, n.y * 0.5f + 0.5f, n.z * 0.5f + 0.5f, 1.0f);
            vertices.push_back(v);
        }
    }

    if (vertices.empty())
        return false;

    init(core, vertices.data(), (unsigned int)vertices.size());
    return true;
}

void Mesh::draw(Core* core, const Matrix& wvp)
{
    ID3D12GraphicsCommandList4* cmd = core->getCommandList();
    cmd->SetGraphicsRoot32BitConstants(0, 16, wvp.m, 0);   //root parameter 0, 16 floats, from wvp.m, starting at 0
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->IASetVertexBuffers(0, 1, &vbView);
    cmd->DrawInstanced(vertexCount, 1, 0, 0);
}