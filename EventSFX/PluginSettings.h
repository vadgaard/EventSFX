#pragma once

constexpr float MIN_PLUGIN_VOLUME = 0.0f;
constexpr float MAX_PLUGIN_VOLUME = 1.0f;
constexpr float MIN_SOUND_VOLUME  = 0.0f;
constexpr float MAX_SOUND_VOLUME  = 5.0f;
constexpr float MIN_SOUND_DELAY   = 0.0f;
constexpr float MAX_SOUND_DELAY   = 5.0f;

#define MIN_PLUGIN_VOLUME_PERCENTAGE std::lround(MIN_PLUGIN_VOLUME * 100.0f)
#define MAX_PLUGIN_VOLUME_PERCENTAGE std::lround(MAX_PLUGIN_VOLUME * 100.0f)
#define MIN_SOUND_VOLUME_PERCENTAGE  std::lround(MIN_SOUND_VOLUME  * 100.0f)
#define MAX_SOUND_VOLUME_PERCENTAGE  std::lround(MAX_SOUND_VOLUME  * 100.0f)

#define DEFAULT_SETTINGS_FILE "eventsfx.json"

inline std::filesystem::path GetSettingsFolder();

inline std::filesystem::path GetSettingsFilePath(const std::string& filename = DEFAULT_SETTINGS_FILE);

struct SoundSettings
{
    std::string SoundId;
    bool        IsEnabled;
    float       Delay;
    float       Volume;
};

class PluginSettings
{
public:
    float                        Volume;
    std::wstring                 OutputId;
    bool                         SoundTrackingEnabled;
    bool                         PreviewsEnabled;
    std::array<SoundSettings, 9> Sounds;

    PluginSettings();

    void SetDefaults();
    bool Load(const std::string& filename = DEFAULT_SETTINGS_FILE);
    bool Save(const std::string& filename = DEFAULT_SETTINGS_FILE);
};
