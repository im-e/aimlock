#pragma once
#include "Engine/Graphics/Core.h"
#include "Engine/Platform/Window.h"

class DebugUI
{
public:
    void init(Core* core, Window* win);
    void beginFrame();          //call once per frame, before any ImGui:: calls
    void render(Core* core);    //call after drawing the scene, before core.finishFrame()
    void shutdown();            //after core.flushGraphicsQueue()
};