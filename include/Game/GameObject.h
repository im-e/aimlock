#pragma once
#include "Engine/Graphics/Mesh.h"
#include "Engine/Maths/Vec3.h"
#include "Engine/Maths/Matrix.h"
#include "Engine/Maths/Color.h"
#include <string>

//something placed in the world: which mesh to draw, and where and how it sits
//the mesh is only a pointer, so lots of objects can share one mesh without loading it twice
class GameObject
{
public:
    std::string name;
    Mesh* mesh = nullptr;
    Vec3 position = Vec3(0, 0, 0);
    Vec3 rotation = Vec3(0, 0, 0); //radians: x = tilt forward/back, y = turn left/right, z = roll
    Vec3 scale = Vec3(1, 1, 1);
    Color colour = Color(0.8f, 0.8f, 0.8f, 1.0f); //base colour, lit by the shader

    GameObject() {}
    GameObject(const std::string& _name, Mesh* _mesh, Vec3 _position, Vec3 _scale = Vec3(1, 1, 1),
        Vec3 _rotation = Vec3(0, 0, 0), Color _colour = Color(0.8f, 0.8f, 0.8f, 1.0f))
        : name(_name), mesh(_mesh), position(_position), rotation(_rotation), scale(_scale), colour(_colour) {
    }

    //model space -> world space
    //read right to left: scale it, then rotate it, then move it into place
    Matrix getWorld() const
    {
        return Matrix::translation(position)
            * Matrix::rotateY(rotation.y)
            * Matrix::rotateX(rotation.x)
            * Matrix::rotateZ(rotation.z)
            * Matrix::scaling(scale);
    }
};
