#pragma once
#include "Engine/Graphics/Core.h"
#include <string>
#include <map>
#include "Engine/Graphics/Shader.h"

//keeps every shader by name so the same file is never compiled twice
class ShaderManager
{
public:
    std::map<std::string, Shader> shaders;

    //compiles the file the first time (or loads its cached .cso), returns the stored one after that
    //returns nullptr if it fails to compile
    Shader* load(Core* core, const std::string& name, const std::wstring& filename);
    Shader* get(const std::string& name); //nullptr if it hasn't been loaded
    void release();
};
