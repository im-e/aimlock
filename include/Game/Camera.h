#pragma once
#include "Engine/Maths/Vec3.h"
#include "Engine/Maths/Matrix.h"
#include "Engine/Maths/MathUtils.h"
#include "Engine/Platform/Window.h"
#include <cmath>

//the player's view in the aim trainer
//you never move, you just stand in one spot and turn, so all this does is mouse look
class Camera
{
public:
    Vec3 position = Vec3(0.0f, 1.7f, 0.0f); //eye height above the floor
    float yaw = 0.0f;   //left/right in radians, 0 = looking down +z
    float pitch = 0.0f; //up/down in radians

    //sensitivity works the same as CS/Apex (0.022 degrees per mouse count at sens 1)
    //so people can bring their in game sens straight over
    float sensitivity = 5.0f;
    float yawPerCount = 0.022f;

    float horizontalFov = 103.0f; //degrees, same as valorant's fixed fov
    float nearPlane = 0.1f;
    float farPlane = 200.0f;

    Vec3 forward() const
    {
        return Vec3(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch));
    }

    void update(const Window& win)
    {
        if (!win.mouseLocked)
            return; //cursor is free in menus, so it shouldn't turn the view

        float radiansPerCount = MathUtils::toRadians(sensitivity * yawPerCount);
        yaw += win.mouseDeltaX * radiansPerCount;
        pitch -= win.mouseDeltaY * radiansPerCount; //mouse down = look down

        //stop just short of straight up/down, lookAt breaks when forward lines up with the up vector
        const float limit = PI / 2.0f - 0.01f;
        if (pitch > limit) pitch = limit;
        if (pitch < -limit) pitch = -limit;
    }

    void resetView()
    {
        yaw = 0.0f;
        pitch = 0.0f;
    }

    Matrix getView() const
    {
        return Matrix::lookAt(position, position + forward(), Vec3(0, 1, 0));
    }

    //games quote horizontal fov, but the projection matrix wants vertical, so convert using the aspect ratio
    Matrix getProjection(float aspect) const
    {
        float halfHorizontal = MathUtils::toRadians(horizontalFov) * 0.5f;
        float verticalFov = 2.0f * atanf(tanf(halfHorizontal) / aspect);
        return Matrix::perspective(MathUtils::toDegrees(verticalFov), aspect, nearPlane, farPlane);
    }
};
