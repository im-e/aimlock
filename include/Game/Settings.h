#pragma once
#include <string>

//a game's "yaw" is how many degrees one mouse count turns you at sens 1
//picking the game you play means you can type your normal sens straight in
struct YawPreset
{
    const char* name;
    float degreesPerCount;
};

static const YawPreset YAW_PRESETS[] =
{
    { "CS2/Apex",    0.022f  },
    { "Valorant",    0.07f   },
    { "Overwatch 2", 0.0066f },
};
static const int YAW_PRESET_COUNT = sizeof(YAW_PRESETS) / sizeof(YAW_PRESETS[0]);

//player settings, saved to settings.txt so they stick between launches
struct Settings
{
    float sensitivity = 1.0f;
    int yawPreset = 0;            //index into YAW_PRESETS
    int dpi = 800;                //only used to work out cm/360, the game itself just sees mouse counts
    float horizontalFov = 103.0f; //degrees

    float yawPerCount() const { return YAW_PRESETS[yawPreset].degreesPerCount; }
    const char* yawName() const { return YAW_PRESETS[yawPreset].name; }

    //how far you move the mouse for a full 360 turn
    //this is the number people actually compare between games and mice, since it doesn't care about dpi or sens on their own
    float cmPer360() const
    {
        float countsPer360 = 360.0f / (sensitivity * yawPerCount());
        float inches = countsPer360 / (float)dpi;
        return inches * 2.54f;
    }

    bool load(const std::string& path); //false if the file isn't there yet (defaults are kept)
    void save(const std::string& path) const;
};
