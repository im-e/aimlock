#pragma once
#include "Engine/Graphics/Core.h"
#include "Engine/Maths/Vec3.h"
#include "Engine/Maths/Vec4.h"
#include "Engine/Maths/Matrix.h"
#include <string>
#include <vector>

struct Vertex
{
    Vec3 position;   //x, y, z      -> 12 bytes
    Vec3 normal;     //nx, ny, nz   -> 12 bytes
    Color color;     //r, g, b, a   -> 16 bytes  (40 bytes per vertex)
};

//one part of a model (a GEMMesh), e.g. a tree's trunk or its leaves
//all parts share the mesh's vertex and index buffers, this just says which slice to draw
struct SubMesh
{
    unsigned int startIndex = 0; //first index of this part in the index buffer
    unsigned int indexCount = 0; //how many indices it uses
    int baseVertex = 0;          //added to each index, so every part's indices can start at 0
};

class Mesh
{
public:
    ID3D12Resource* vertexBuffer = nullptr; //memory on the gpu that stores verticies
    D3D12_VERTEX_BUFFER_VIEW vbView = {}; //defines how to access data when drawing
    unsigned int vertexCount = 0; //amount of verticies

    ID3D12Resource* indexBuffer = nullptr; //which vertices make each triangle
    D3D12_INDEX_BUFFER_VIEW ibView = {};
    unsigned int indexCount = 0;

    std::vector<SubMesh> subMeshes; //one per part - each will get its own texture later

    void init(Core* core, const Vertex* vertices, unsigned int numVertices,
        const unsigned int* indices, unsigned int numIndices);
    bool loadGEM(Core* core, const std::string& filename); //load a .gem model and upload it
    void draw(Core* core); //draws every part - constant buffers are bound by the caller before this
    void drawSubMesh(Core* core, unsigned int index); //draws one part (for per-part materials later)
    void release();

    static D3D12_INPUT_LAYOUT_DESC inputLayout();

private:
    //creates a buffer in gpu memory and fills it with data via the upload heap
    void bindBuffers(Core* core);
    static ID3D12Resource* createGPUBuffer(Core* core, const void* data, UINT size, D3D12_RESOURCE_STATES finalState);

};
