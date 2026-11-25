#include "Engine/Graphics/ShaderManager.h"

Shader* ShaderManager::load(Core* core, const std::string& name, const std::wstring& filename)
{
    //already loaded? hand back the one we have
    auto it = shaders.find(name);
    if (it != shaders.end())
        return &it->second;

    //load straight into the map so the constant buffers aren't copied around
    Shader& shader = shaders[name];
    if (!shader.load(core, filename))
    {
        shader.release(); //one half may have compiled
        shaders.erase(name);
        return nullptr;
    }
    return &shader;
}

Shader* ShaderManager::get(const std::string& name)
{
    auto it = shaders.find(name);
    if (it == shaders.end())
    {
        OutputDebugStringA(("ShaderManager: no shader called " + name + "\n").c_str());
        return nullptr;
    }
    return &it->second;
}

void ShaderManager::release()
{
    for (auto& pair : shaders)
        pair.second.release();
    shaders.clear();
}
