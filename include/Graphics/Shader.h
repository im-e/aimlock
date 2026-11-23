#pragma once
#include "Core.h"
#include <string>

class Shader
{
public:
    ID3DBlob* vs = nullptr; //compiled vertex shader
    ID3DBlob* ps = nullptr; //compiled pixel shader

    bool load(const std::wstring& filename);

private:
    ID3DBlob* compile(const std::wstring& filename, const char* entryPoint, const char* target);
};