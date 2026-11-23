#pragma once
#include "Core.h"
#include "Maths/Vec3.h"
#include "Maths/Vec4.h"
#include "Maths/Matrix.h"
#include <string>

struct Vertex
{
    Vec3 position;   //x, y, z      -> 12 bytes
    Vec3 normal;     //nx, ny, nz   -> 12 bytes
    Color color;     //r, g, b, a   -> 16 bytes  (40 bytes per vertex)
};

class Mesh
{
public:
    ID3D12Resource* vertexBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vbView = {};
    unsigned int vertexCount = 0;

    void init(Core* core, const Vertex* vertices, unsigned int count);
    bool loadGEM(Core* core, const std::string& filename); //load a .gem model and upload it
    void draw(Core* core, const Matrix& wvp);

    static D3D12_INPUT_LAYOUT_DESC inputLayout();
};
