#include "Game/Settings.h"
#include <fstream>
#include <sstream>

//settings.txt is just "name value" on each line, so it's easy to edit by hand too
bool Settings::load(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
        return false;

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream in(line);
        std::string key;
        if (!(in >> key) || key[0] == '#')
            continue;

        if (key == "sensitivity")        in >> sensitivity;
        else if (key == "yaw_preset")    in >> yawPreset;
        else if (key == "dpi")           in >> dpi;
        else if (key == "fov")           in >> horizontalFov;
    }

    //don't trust the file completely, someone might have typed something silly into it
    if (yawPreset < 0 || yawPreset >= YAW_PRESET_COUNT) yawPreset = 0;
    if (sensitivity <= 0.0f) sensitivity = 1.0f;
    if (dpi <= 0) dpi = 800;
    if (horizontalFov < 60.0f || horizontalFov > 130.0f) horizontalFov = 103.0f;
    return true;
}

void Settings::save(const std::string& path) const
{
    std::ofstream file(path);
    file << "# aimlock settings\n";
    file << "sensitivity " << sensitivity << "\n";
    file << "yaw_preset " << yawPreset << "   # " << yawName() << "\n";
    file << "dpi " << dpi << "\n";
    file << "fov " << horizontalFov << "\n";
}
