#pragma once
#include "Engine/Maths/Vec3.h"
#include "Engine/Maths/Matrix.h"
#include "Engine/Maths/MathUtils.h" //PI
#include "Engine/Platform/Window.h"
#include <cmath>

//first person camera: the mouse turns your head, WASD walks along the ground
//press F to toggle flying (handy for looking around the level while building it)
class Camera
{
public:
    Vec3 position = Vec3(0, 1.7f, -8.0f);
    float yaw = 0.0f;   //turning left/right in radians, 0 = looking down +z
    float pitch = 0.0f; //looking up/down in radians

    float moveSpeed = 4.0f;          //metres per second (a brisk walk)
    float sprintMultiplier = 2.0f;   //hold shift
    float lookSensitivity = 0.002f;  //radians per unit of mouse movement
    float eyeHeight = 1.7f;          //how high your eyes are above the ground (y = 0) when walking
    bool flying = false;

    //the direction the camera is looking (used for the view)
    Vec3 forward() const
    {
        return Vec3(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch));
    }

    //forward but flat along the ground, so looking down doesn't make you walk slower or into the floor
    Vec3 flatForward() const
    {
        return Vec3(sinf(yaw), 0.0f, cosf(yaw));
    }

    //flat right vector so strafing never moves you up or down
    Vec3 right() const
    {
        return Vec3(cosf(yaw), 0.0f, -sinf(yaw));
    }

    void update(const Window& win, float dt)
    {
        //mouse look whenever the mouse is locked into the window
        if (win.mouseLocked)
        {
            yaw += win.mouseDeltaX * lookSensitivity;
            pitch -= win.mouseDeltaY * lookSensitivity; //mouse down = look down

            //stop just short of straight up/down, lookAt breaks when forward lines up with the up vector
            const float limit = PI / 2.0f - 0.01f;
            if (pitch > limit) pitch = limit;
            if (pitch < -limit) pitch = -limit;
        }

        //F toggles flying - only on the frame the key goes down, not every frame it's held
        bool flyKey = win.keys['F'];
        if (flyKey && !flyKeyWasDown)
            flying = !flying;
        flyKeyWasDown = flyKey;

        //add up which way the keys are pushing
        Vec3 f = flying ? forward() : flatForward(); //when flying, W goes where you look
        Vec3 r = right();
        Vec3 move(0, 0, 0);
        if (win.keys['W']) move += f;
        if (win.keys['S']) move += -f;
        if (win.keys['D']) move += r;
        if (win.keys['A']) move += -r;
        if (flying && win.keys[VK_SPACE])   move += Vec3(0, 1, 0);
        if (flying && win.keys[VK_CONTROL]) move += Vec3(0, -1, 0);

        //normalise so going diagonally (W + D) isn't faster than going straight
        float length = sqrtf(Dot(move, move));
        if (length > 0.0f)
            move = move / length;

        float speed = moveSpeed * (win.keys[VK_SHIFT] ? sprintMultiplier : 1.0f);
        position += move * (speed * dt); //dt so speed doesn't depend on frame rate

        if (!flying)
            position.y = eyeHeight; //stay on the ground (proper jumping/gravity comes with physics)
    }

    Matrix getView() const
    {
        return Matrix::lookAt(position, position + forward(), Vec3(0, 1, 0));
    }

private:
    bool flyKeyWasDown = false;
};
