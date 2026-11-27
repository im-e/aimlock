#pragma once
#include "Engine/Graphics/Core.h"
#include "Engine/Maths/Vec3.h"
#include "Engine/Maths/Matrix.h"
#include <string>
#include <vector>

struct Vertex
{
    Vec3 position; //12 bytes
    Vec3 normal;   //12 bytes -> 24 bytes per vertex
    //uvs and tangents go here once textures are in
};

//one part of a model (a GEMMesh in the file)
//every part shares the mesh's vertex and index buffers, this just says which slice to draw
struct SubMesh
{
    unsigned int startIndex = 0; //first index of this part in the index buffer
    unsigned int indexCount = 0;
    int baseVertex = 0;          //added to every index, so each part's indices can start at 0
};

class Mesh
{
public:
    ID3D12Resource* vertexBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vbView = {}; //tells the input assembler where the vertices are and how big each one is
    unsigned int vertexCount = 0;

    ID3D12Resource* indexBuffer = nullptr; //which vertices make up each triangle
    D3D12_INDEX_BUFFER_VIEW ibView = {};
    unsigned int indexCount = 0;

    std::vector<SubMesh> subMeshes; //one per part, each one can have its own material later

    void init(Core* core, const Vertex* vertices, unsigned int numVertices,
        const unsigned int* indices, unsigned int numIndices);
    bool loadGEM(Core* core, const std::string& filename);
    void createSphere(Core* core, unsigned int rings, unsigned int segments); //unit sphere (radius 1) for targets

    void draw(Core* core); //draws every part, the caller binds the constant buffers first
    void drawSubMesh(Core* core, unsigned int index);
    void release();

    static D3D12_INPUT_LAYOUT_DESC inputLayout();

private:
    void bindBuffers(Core* core);
    static ID3D12Resource* createGPUBuffer(Core* core, const void* data, UINT size, D3D12_RESOURCE_STATES finalState);
};
