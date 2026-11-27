#include "Engine/Graphics/Mesh.h"
#include "Engine/Maths/MathUtils.h" //PI
#include <cstring>  //memcpy, memset (GEMLoader uses them but doesn't include it)
#include <cstddef>  //offsetof
#include <map>      //same again, GEMLoader needs std::map
#include <vector>
#include "Engine/External/GEMLoader.h"

D3D12_INPUT_LAYOUT_DESC Mesh::inputLayout()
{
    static D3D12_INPUT_ELEMENT_DESC elements[] =
    {
        //semantic  index  format                       slot  byte offset
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal),   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
    indexBuffer = createGPUBuffer(core, indices, ibSize, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    ibView.Format = DXGI_FORMAT_R32_UINT; //32 bit indices to match unsigned int
    ibView.SizeInBytes = ibSize;

    //if nothing set up parts (i.e. it didn't come from a .gem) just treat the whole mesh as one part
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
    core->uploadResource(buffer, data, size, finalState); //goes via a temporary upload buffer
    return buffer;
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
        //its indices stay as they are in the file (starting at 0) and baseVertex offsets them when drawing
        SubMesh part;
        part.startIndex = (unsigned int)indices.size();
        part.indexCount = (unsigned int)gm.indices.size();
        part.baseVertex = (int)vertices.size();
        subMeshes.push_back(part);

        for (GEMLoader::GEMStaticVertex& sv : gm.verticesStatic)
        {
            Vertex v;
            v.position = Vec3(sv.position.x, sv.position.y, sv.position.z);
            v.normal = Vec3(sv.normal.x, sv.normal.y, sv.normal.z);
            vertices.push_back(v);
        }

        for (unsigned int index : gm.indices)
            indices.push_back(index);
    }

    if (vertices.empty() || indices.empty())
        return false;

    init(core, vertices.data(), (unsigned int)vertices.size(),
        indices.data(), (unsigned int)indices.size());
    return true;
}

void Mesh::createSphere(Core* core, unsigned int rings, unsigned int segments)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    //rings go top to bottom (theta 0 -> PI), segments go round the middle (phi 0 -> 2PI)
    //the first and last column of each ring sit in the same place so the seam can have its own uvs later
    for (unsigned int r = 0; r <= rings; r++)
    {
        float theta = PI * (float)r / (float)rings;
        for (unsigned int s = 0; s <= segments; s++)
        {
            float phi = 2.0f * PI * (float)s / (float)segments;
            Vec3 p(sinf(theta) * cosf(phi), cosf(theta), sinf(theta) * sinf(phi));

            Vertex v;
            v.position = p;
            v.normal = p; //on a unit sphere the normal is just the position
            vertices.push_back(v);
        }
    }

    //two triangles per quad, wound clockwise from the outside so back face culling keeps the outside
    unsigned int stride = segments + 1;
    for (unsigned int r = 0; r < rings; r++)
    {
        for (unsigned int s = 0; s < segments; s++)
        {
            unsigned int topLeft = r * stride + s;
            unsigned int bottomLeft = topLeft + stride;

            indices.push_back(topLeft);
            indices.push_back(topLeft + 1);
            indices.push_back(bottomLeft);

            indices.push_back(topLeft + 1);
            indices.push_back(bottomLeft + 1);
            indices.push_back(bottomLeft);
        }
    }

    subMeshes.clear();
    init(core, vertices.data(), (unsigned int)vertices.size(),
        indices.data(), (unsigned int)indices.size());
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
