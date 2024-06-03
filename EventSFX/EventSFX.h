#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"

#include "SoundInterface/SoundManager.h"
#include "Params.h"

// Version information
#include "version.h"
constexpr auto plugin_version =
    stringify(VERSION_MAJOR) "."
    stringify(VERSION_MINOR) "."
    stringify(VERSION_PATCH) "."
    stringify(VERSION_BUILD);

class EventSfx final :
    public BakkesMod::Plugin::BakkesModPlugin,
    public SettingsWindowBase
{
public:
    /* General */

    // Loading and unloading
    void onLoad() override;
    void onUnload() override;

    // Initialization
    void RunSetup();
    void InitializeNotifiers();
    void InitializeHooks();
    bool InitializeAudio();

    // Deinitialization
    void UnrunSetup();
    void DeinitializeAudio();
    void DeinitializeHooks() const;

    /* Settings */

    [[nodiscard]] inline
    bool SaveSettings(const std::string& filename = DEFAULT_SETTINGS_FILE) const;
    bool LoadSettings(const std::string& filename = DEFAULT_SETTINGS_FILE);
    void ApplySettings(const std::wstring& oldOutputId); // NOTE(sushi): For only changing output device when necessary
    void ApplyDefaultSettings();

    /* On events */

    void OnBump(CarWrapper carWrapper, const BumpParams* bumpData);
    void OnStatTickerMessage(const StatTickerParams* statParams);

    /* Audio */

    // Playing sound from filename
    inline void PlaySoundFile(
        const std::string&                    soundId,
        const SoundInterface::PlaybackParams& params = std::nullopt,
        float                                 volume = 1.0f);

    // Playing sound from event type
    void PlayEventSound(
        RlEvents::Kind                        eventId,
        const SoundInterface::PlaybackParams& params = std::nullopt);

    // Helpers
    inline void PlayBumpSfx(const Vector& location, bool fromMenu = false);
    inline void PlayDemoSfx(const Vector& location, bool fromMenu = false);
    inline void PlayPlayerGoalSfx();
    inline void PlayTeamGoalSfx();
    inline void PlayConcedeSfx();
    inline void PlaySaveSfx();
    inline void PlayWinSfx();
    inline void PlayLossSfx();
    inline void PlayAssistSfx();

    // GUI rendering
    void DrawMapAndPositions(
        const ImVec2& areaSize,
        const ImVec2& cameraPos);

    void DrawCameraAndCarIcons(
        ImDrawList*   drawList,
        const ImVec2& cameraPos,
        const ImVec2& carPos,
        const ImVec2& areaSize) const;

    void RenderSettings() override;

    void GenerateSoundFileCombo(
        RlEvents::Kind eventId,
        SoundSettings& soundSettings);

    /* Hooks */

    void HookSoundTracking() const;
    void UnhookSoundTracking() const;

    /* Fields */

    // Error state
    std::shared_ptr<bool> Enabled = std::make_shared<bool>(false);

    // Global camera info. Don't ask.
    std::shared_ptr<CameraInfo> CamInfo = std::make_shared<CameraInfo>();

    // Active settings
    std::shared_ptr<PluginSettings> Settings = std::make_shared<PluginSettings>();

    // Timing out bumps
    using TimeStampMap = std::map<uintptr_t, std::chrono::steady_clock::time_point>;
    TimeStampMap              NotedBumps;
    std::chrono::milliseconds BumpTimeout = std::chrono::milliseconds(200);

    // The sound interface
    SoundInterface::SoundManager SoundManager;

    // For recommended volume
    std::optional<float> LastMasterVolume   = std::nullopt;
    std::optional<float> LastGameplayVolume = std::nullopt;

    // For timing out the loss event
    bool TmpLossesDisabled = false;
};
