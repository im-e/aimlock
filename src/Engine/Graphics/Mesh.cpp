#include "Engine/Graphics/Mesh.h"
#include <cstring>  //memcpy, memset (GEMLoader needs it but doesn't include it)
#include <cstddef>  //offsetof
#include <map>      //GEMLoader uses std::map but doesn't include it
#include <vector>
#include "Engine/External/GEMLoader.h"

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

void Mesh::init(Core* core, const Vertex* vertices, unsigned int numVertices,
    const unsigned int* indices, unsigned int numIndices)
{
    vertexCount = numVertices;
    indexCount = numIndices;

    UINT vbSize = sizeof(Vertex) * numVertices;
    vertexBuffer = createGPUBuffer(core, vertices, vbSize, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbView.StrideInBytes = sizeof(Vertex);
    vbView.SizeInBytes = vbSize;

    UINT ibSize = sizeof(unsigned int) * numIndices;
    indexBuffer = createGPUBuffer(core, indices, ibSize, D3D12_RESOURCE_STATE_INDEX_BUFFER); //different final state
    ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    ibView.Format = DXGI_FORMAT_R32_UINT; //32 bit indices, matches unsigned int
    ibView.SizeInBytes = ibSize;

    //nothing told us about parts (not loaded from a .gem), so treat the whole thing as one
    if (subMeshes.empty())
    {
        SubMesh whole;
        whole.indexCount = numIndices;
        subMeshes.push_back(whole);
    }
}

ID3D12Resource* Mesh::createGPUBuffer(Core* core, const void* data, UINT size, D3D12_RESOURCE_STATES finalState)
{
    D3D12_HEAP_PROPERTIES heapprops = {};
    heapprops.Type = D3D12_HEAP_TYPE_DEFAULT; //vram
    heapprops.CreationNodeMask = 1;
    heapprops.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ID3D12Resource* buffer = nullptr;
    CHECK(core->device->CreateCommittedResource(&heapprops, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&buffer)));
    core->uploadResource(buffer, data, size, finalState);
    return buffer;
}

//turns a position and normal from the gem file into one of our vertices
static Vertex makeVertex(const GEMLoader::GEMVec3& p, const GEMLoader::GEMVec3& n)
{
    Vertex v;
    v.position = Vec3(p.x, p.y, p.z);
    v.normal = Vec3(n.x, n.y, n.z);
    //debug colour: show the normal as a colour (-1..1 mapped to 0..1)
    v.color = Color(n.x * 0.5f + 0.5f, n.y * 0.5f + 0.5f, n.z * 0.5f + 0.5f, 1.0f);
    return v;
}

bool Mesh::loadGEM(Core* core, const std::string& filename)
{
    std::vector<GEMLoader::GEMMesh> gemMeshes;
    GEMLoader::GEMModelLoader loader;
    loader.load(filename, gemMeshes);

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    subMeshes.clear();

    for (GEMLoader::GEMMesh& gm : gemMeshes)
    {
        //remember where this part starts in the shared buffers
        //its indices are kept as they are (starting at 0) and baseVertex shifts them at draw time
        SubMesh part;
        part.startIndex = (unsigned int)indices.size();
        part.indexCount = (unsigned int)gm.indices.size();
        part.baseVertex = (int)vertices.size();
        subMeshes.push_back(part);

        if (!gm.verticesStatic.empty())
            for (GEMLoader::GEMStaticVertex& sv : gm.verticesStatic)
                vertices.push_back(makeVertex(sv.position, sv.normal));
        else //animated models
            for (GEMLoader::GEMAnimatedVertex& av : gm.verticesAnimated)
                vertices.push_back(makeVertex(av.position, av.normal));

        for (unsigned int index : gm.indices)
            indices.push_back(index);
    }

    if (vertices.empty() || indices.empty())
        return false;

    init(core, vertices.data(), (unsigned int)vertices.size(),
        indices.data(), (unsigned int)indices.size());
    return true;
}

void Mesh::bindBuffers(Core* core)
{
    ID3D12GraphicsCommandList4* cmd = core->getCommandList();
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->IASetVertexBuffers(0, 1, &vbView);
    cmd->IASetIndexBuffer(&ibView);
}

void Mesh::draw(Core* core)
{
    bindBuffers(core);
    for (const SubMesh& part : subMeshes)
    {
        //index count, instance count, first index, base vertex, first instance
        core->getCommandList()->DrawIndexedInstanced(part.indexCount, 1, part.startIndex, part.baseVertex, 0);
    }
}

void Mesh::drawSubMesh(Core* core, unsigned int index)
{
    if (index >= subMeshes.size())
        return;
    bindBuffers(core);
    const SubMesh& part = subMeshes[index];
    core->getCommandList()->DrawIndexedInstanced(part.indexCount, 1, part.startIndex, part.baseVertex, 0);
}

void Mesh::release()
{
    safeRelease(vertexBuffer);
    safeRelease(indexBuffer);
    vertexCount = 0;
    indexCount = 0;
    subMeshes.clear();
}
