#pragma once

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include "bakkesmod/plugin/bakkesmodplugin.h"

#include <string>
#include <vector>
#include <array>
#include <functional>
#include <memory>
#include <random>
#include <algorithm>
#include <chrono>
#include <cmath>

#include "IMGUI/imgui.h"
#include "IMGUI/imgui_stdlib.h"
#include "IMGUI/imgui_searchablecombo.h"
#include "IMGUI/imgui_rangeslider.h"

#include "logging.h"
#include "Utils.h"

#include "Events.h"
#include "PluginSettings.h"
#include "CameraInfo.h"

// Globals
extern std::shared_ptr<PluginSettings>     globalPluginSettings; // For global access to plugin settings
extern std::shared_ptr<CameraInfo>         globalCameraInfo;     // Global access to camera settings; probably not necessary
extern std::shared_ptr<CVarManagerWrapper> globalCVarManager;    // CVar manager
extern std::shared_ptr<GameWrapper>        globalGameWrapper;	  // Game wrapper

// Notifiers
#define PLAY_SOUND_NOTIFIER	              "eventsfx_play"
#define SET_PLUGIN_VOLUME_NOTIFIER        "eventsfx_set_volume"
#define TOGGLE_SOUNDTRACKING_NOTIFIER     "eventsfx_toggle_soundtracking"
#define ENABLE_SOUNDTRACKING_NOTIFIER     "eventsfx_enable_soundtracking"
#define DISABLE_SOUNDTRACKING_NOTIFIER    "eventsfx_disable_soundtracking"
#define TOGGLE_PLING_NOTIFIER             "eventsfx_toggle_pling"
#define ENABLE_PLING_NOTIFIER             "eventsfx_enable_pling"
#define DISABLE_PLING_NOTIFIER            "eventsfx_disable_pling"
#define PLAY_EVENT_NOTIFIER_PARTIAL       "eventsfx_play_"
#define TOGGLE_EVENT_NOTIFIER_PARTIAL     "eventsfx_toggle_"
#define ENABLE_EVENT_NOTIFIER_PARTIAL     "eventsfx_enable_"
#define DISABLE_EVENT_NOTIFIER_PARTIAL    "eventsfx_disable_"
#define SET_EVENT_SOUND_NOTIFIER_PARTIAL  "eventsfx_set_sound_"
#define SET_EVENT_VOLUME_NOTIFIER_PARTIAL "eventsfx_set_volume_"
#define SET_EVENT_DELAY_NOTIFIER_PARTIAL  "eventsfx_set_delay_"
#define SAVE_SETTINGS_NOTIFIER            "eventsfx_save_settings"
#define LOAD_SETTINGS_NOTIFIER            "eventsfx_load_settings"
