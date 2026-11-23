
#pragma once
#include "Core.h"

struct Vertex
{
    float position[3];   //x, y, z      -> 12 bytes
    float colour[4];     //r, g, b, a   -> 16 bytes
};

class Mesh
{
public:
    ID3D12Resource* vertexBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vbView = {};
    unsigned int vertexCount = 0;

    void init(Core* core, const Vertex* vertices, unsigned int count);
    void draw(Core* core);

    static D3D12_INPUT_LAYOUT_DESC inputLayout();
};