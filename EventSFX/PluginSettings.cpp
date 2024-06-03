#include "pch.h"
#include "PluginSettings.h"

#include "nlohmann/json.hpp"
#include "utils/parser.h"

#include <fstream>

// ReSharper disable once CppInconsistentNaming
using json = nlohmann::json;

inline std::filesystem::path GetSettingsFolder()
{
    return globalGameWrapper->GetDataFolder();
}

inline std::filesystem::path GetSettingsFilePath(const std::string& filename)
{
    return globalGameWrapper->GetDataFolder() / filename;
}

// Define how to serialize SoundSettings
// ReSharper disable once CppInconsistentNaming
void to_json(json& j, const SoundSettings& s)
{
    j = json{
        {"sound_id", s.SoundId},
        {"enabled", s.IsEnabled},
        {"delay", to_string_with_precision(s.Delay, 2)},
        {"volume", to_string_with_precision(s.Volume, 2)}
    };
}

// Define how to deserialize SoundSettings
// ReSharper disable once CppInconsistentNaming
void from_json(const json& j, SoundSettings& s)
{
    j.at("sound_id").get_to(s.SoundId);
    j.at("enabled").get_to(s.IsEnabled);
    s.Delay  = get_safe_float(j["delay"]);
    s.Volume = get_safe_float(j["volume"]);

    if (s.Volume < MIN_SOUND_VOLUME) s.Volume = MIN_SOUND_VOLUME;
    if (s.Volume > MAX_SOUND_VOLUME) s.Volume = MAX_SOUND_VOLUME;

    if (s.Delay < MIN_SOUND_DELAY) s.Delay = MIN_SOUND_DELAY;
    if (s.Delay > MAX_SOUND_DELAY) s.Delay = MAX_SOUND_DELAY;
}

// Define JSON serialization for PluginSettings
// ReSharper disable once CppInconsistentNaming
void to_json(json& j, const PluginSettings& p)
{
    j = json{
        {"volume", to_string_with_precision(p.Volume, 2)},
        {"output_id", Utils::WStringToString(p.OutputId)},
        {"soundtracking_enabled", p.SoundTrackingEnabled},
        {"previews_enabled", p.PreviewsEnabled},
        {"sounds", p.Sounds}
    };
}

// Define JSON deserialization for PluginSettings
// ReSharper disable once CppInconsistentNaming
void from_json(const json& j, PluginSettings& p)
{
    p.Volume   = get_safe_float(j["volume"]);
    p.OutputId = Utils::StringToWString(j["output_id"]);
    j.at("soundtracking_enabled").get_to(p.SoundTrackingEnabled);
    j.at("previews_enabled").get_to(p.PreviewsEnabled);
    j.at("sounds").get_to(p.Sounds);

    if (p.Volume < MIN_PLUGIN_VOLUME) p.Volume = MIN_PLUGIN_VOLUME;
    if (p.Volume > MAX_PLUGIN_VOLUME) p.Volume = MAX_PLUGIN_VOLUME;
}

PluginSettings::PluginSettings()
{
    this->SetDefaults();
}

void PluginSettings::SetDefaults()
{
    this->Volume                             = 1.0f;
    this->OutputId                           = L"default";
    this->SoundTrackingEnabled               = true;
    this->PreviewsEnabled                    = true;
    this->Sounds[RlEvents::Kind::Bump]       = {"bonk.wav", true, 0.0f, 1.0f};
    this->Sounds[RlEvents::Kind::Demo]       = {"sm64_mario_so_long_bowser.wav", true, 0.0f, 1.0f};
    this->Sounds[RlEvents::Kind::Win]        = {"sm64_mario_game_over.wav", true, 0.1f, 1.0f};
    this->Sounds[RlEvents::Kind::Loss]       = {"sm64_mario_lost_a_life.wav", true, 0.1f, 1.0f};
    this->Sounds[RlEvents::Kind::PlayerGoal] = {"sm64_mario_waha.wav", true, 0.1f, 1.0f};
    this->Sounds[RlEvents::Kind::TeamGoal]   = {"sm64_mario_lets_go.wav", true, 0.1f, 1.0f};
    this->Sounds[RlEvents::Kind::Concede]    = {"sm64_mario_mamma-mia.wav", true, 0.1f, 1.0f};
    this->Sounds[RlEvents::Kind::Save]       = {"sm64_mario_hoohoo.wav", true, 0.1f, 1.0f};
    this->Sounds[RlEvents::Kind::Assist]     = {"sm64_mario_haha.wav", true, 0.1f, 1.0f};
}

bool PluginSettings::Load(const std::string& filename)
{
    const auto filepath = GetSettingsFilePath(filename);

    try
    {
        if (std::ifstream file(filepath); file.is_open())
        {
            json j;
            file >> j;
            file.close();

            *this = j.get<PluginSettings>();

            return true;
        }
        return false;
    }
    catch ([[maybe_unused]] const std::exception& e)
    {
        return false;
    }
}

bool PluginSettings::Save(const std::string& filename)
{
    const std::filesystem::path filepath = GetSettingsFilePath(filename);

    try
    {
        const json j = *this;
        if (std::ofstream file(filepath); file.is_open())
        {
            file << j.dump(4);
            file.close();

            return true;
        }
        return false;
    }
    catch ([[maybe_unused]] const std::exception& e)
    {
        return false;
    }
}
