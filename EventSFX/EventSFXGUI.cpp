#include "pch.h"
#include "EventSFX.h"

// Global guides
#define ITEM_WIDTH		(ImGui::GetFontSize() * 18.0f)
#define HALF_ITEM_WIDTH (ITEM_WIDTH / 2 - ITEM_SEP / 2)
#define ITEM_SEP		(ImGui::GetFontSize() /  2.0f)
#define CMD_WIDTH		(ImGui::GetFontSize() * 26.0f)

// Play buttons
constexpr float PLAY_BUTTON_WIDTH = 80.0f;

// Combos
bool isComboOpen = false;

// Alignment
inline void SameLineColumn()
{
    ImGui::SameLine(ITEM_WIDTH, ITEM_SEP);
}

inline void SameLineHalfColumn()
{
    ImGui::SameLine(HALF_ITEM_WIDTH, ITEM_SEP);
}

// For storing clicked map positions
struct ClickedPosition
{
    ImVec2                                Pos;
    std::chrono::steady_clock::time_point Timestamp;
    std::chrono::milliseconds             Duration;
    std::chrono::milliseconds             Delay;
};

namespace
{
    ImVec2                                lastPosition;
    std::optional<float>                  lastSpeed;
    std::vector<ClickedPosition>          clickedPositions;
    std::chrono::steady_clock::time_point lastPositionTime;
    std::chrono::milliseconds             lastPositionDuration;
}

constexpr auto MIN_INFO_DURATION = std::chrono::milliseconds(1500);

inline void NoteClick(EventSfx* pPlugin, const RlEvents::Kind eventId, const Vector& location)
{
    lastSpeed = std::nullopt;

    // Store position for rendering
    lastPosition     = ImVec2(location.X, location.Y);
    lastPositionTime = std::chrono::steady_clock::now();

    const SoundSettings& soundSettings = pPlugin->Settings->Sounds[eventId];
    const std::string&   soundId       = soundSettings.SoundId;

    const double durationSeconds      = pPlugin->SoundManager.GetSoundDuration(soundId);
    const int    durationMilliseconds = std::lround(durationSeconds * 1000.0f);
    const auto   duration             = std::chrono::milliseconds(durationMilliseconds);

    const int  delayMilliseconds = std::lround(soundSettings.Delay * 1000.0f);
    const auto delay             = std::chrono::milliseconds(delayMilliseconds);

    clickedPositions.push_back({
        lastPosition,
        std::chrono::steady_clock::now(),
        duration,
        delay
    });

    lastPositionDuration = max(MIN_INFO_DURATION, duration + delay);
}

// For info text
constexpr float INFO_FONT_SIZE = 12.0f;

// Forward declaration
void DrawArrow(
    ImDrawList*   drawList,
    const ImVec2& position,
    float         iconSize);

void DrawCar(
    ImDrawList*   drawList,
    const ImVec2& position,
    float         iconSize);

void EventSfx::RenderSettings()
{
    // Don't proceed if plugin is not enabled
    if (!*this->Enabled)
    {
        ImGui::TextUnformatted("EventSFX is not enabled. Try restarting the plugin.");
        return;
    }

    // Write user guide
    ImGui::Separator();
    if (ImGui::TreeNode("Guide for the user"))
    {
        ImGui::BulletText(
            "Place your custom .wav or .aiff files in the sounds folder (*BAKKESMODPATH*\\bakkesmod\\data\\EventSFX\\).");
        ImGui::BulletText(
            "The plugin settings are saved to, and loaded from, the data folder (*BAKKESMODPATH*\\bakkesmod\\data\\).");
        ImGui::Indent();
        ImGui::BulletText("'%s' is automatically loaded on startup and saved on shutdown.", DEFAULT_SETTINGS_FILE);
        ImGui::BulletText("However, other filenames can be used through a console command (see below).");
        ImGui::Unindent();
        ImGui::BulletText(
            "When 3D sound tracking is enabled, in-game 3D sounds will be adjusted continuously to reflect their positions relative to the camera.");
        ImGui::BulletText(
            "When fixed crossbar volume is disabled, the volume of the custom crossbar sound will increase with the speed of the ball.");
        ImGui::BulletText("The recommended plugin volume is your master volume multiplied by your gameplay volume.");
        ImGui::Indent();
        ImGui::BulletText(
            "Manually adjust your Rocket League master volume to make the plugin detect these and recommend a volume for the plugin.");
        ImGui::Unindent();
        ImGui::BulletText("You may route the EventSFX audio through a non-default output device.");
        ImGui::Indent();
        ImGui::BulletText(
            "This is useful for using the plugin as a soundboard, for example when talking to people on Discord.\n(This requires e.g.VoiceMeeter to route the audio accordingly).");
        ImGui::Unindent();
        ImGui::BulletText(
            "For each event you can set the sound file, individual volume, and delay. The checkbox toggles the event.");
        ImGui::BulletText("Use the interactive map or the play buttons to test the audio levels.");
        ImGui::BulletText(
            "The plugin also provides the following console commands:\n"
            "(<arg> means mandatory; [arg:val] means optional with some default value)");
        ImGui::Indent();
        ImGui::Indent();

        // General
        //ImGui::Bullet();
        ImGui::TextColored(
            ImVec4(1.0f, 0.0f, 1.0f, 1.0f),
            PLAY_SOUND_NOTIFIER "  <file> [subvolume:100]");
        ImGui::SameLine(CMD_WIDTH + ITEM_SEP);
        ImGui::Text(
            "Plays <file> in the sounds folder at [subvolume] if the latter is between %d and %d.",
            MIN_SOUND_VOLUME_PERCENTAGE, MAX_SOUND_VOLUME_PERCENTAGE);
        //ImGui::Bullet();
        ImGui::TextColored(
            ImVec4(1.0f, 0.0f, 1.0f, 1.0f),
            SET_PLUGIN_VOLUME_NOTIFIER "  <volume>");
        ImGui::SameLine(CMD_WIDTH + ITEM_SEP);
        ImGui::Text(
            "Sets the plugin master volume to <volume> if it's between %d and %d.",
            MIN_PLUGIN_VOLUME_PERCENTAGE, MAX_PLUGIN_VOLUME_PERCENTAGE);
        //ImGui::Bullet();
        ImGui::TextColored(
            ImVec4(1.0f, 0.0f, 1.0f, 1.0f),
            "eventsfx_(toggle|enable|disable)_soundtracking");
        ImGui::SameLine(CMD_WIDTH + ITEM_SEP);
        ImGui::TextUnformatted(
            "Toggles, enables, and disables 3D sound tracking, respectively.");
        ImGui::TextColored(
            ImVec4(1.0f, 0.0f, 1.0f, 1.0f),
            "eventsfx_(toggle|enable|disable)_pling");
        ImGui::SameLine(CMD_WIDTH + ITEM_SEP);
        ImGui::TextUnformatted(
            "Toggles, enables, and disables the default crossbar pling, respectively.");

        // Events
        //ImGui::Bullet();
        ImGui::TextColored(
            ImVec4(1.0f, 0.0f, 1.0f, 1.0f),
            "eventsfx_(toggle|enable|disable)_<event>");
        ImGui::SameLine(CMD_WIDTH + ITEM_SEP);
        ImGui::TextUnformatted(
            "Toggles, enables, and disables <event>, respectively.");
        //ImGui::Bullet();
        ImGui::TextColored(
            ImVec4(1.0f, 0.0f, 1.0f, 1.0f),
            SET_EVENT_SOUND_NOTIFIER_PARTIAL "<event>");
        ImGui::SameLine(CMD_WIDTH + ITEM_SEP);
        ImGui::TextUnformatted(
            "Sets the sound file associated with <event>.");
        //ImGui::Bullet();
        ImGui::TextColored(
            ImVec4(1.0f, 0.0f, 1.0f, 1.0f),
            PLAY_EVENT_NOTIFIER_PARTIAL "<event>");
        ImGui::SameLine(CMD_WIDTH + ITEM_SEP);
        ImGui::TextUnformatted(
            "Plays the sound associated with <event>.");
        //ImGui::Bullet();
        ImGui::TextColored(
            ImVec4(1.0f, 0.0f, 1.0f, 1.0f),
            SET_EVENT_VOLUME_NOTIFIER_PARTIAL "<event> " " <subvolume>");
        ImGui::SameLine(CMD_WIDTH + ITEM_SEP);
        ImGui::Text(
            "Sets the submix volume of <event> to <subvolume> if the latter is between %d and %d.",
            MIN_SOUND_VOLUME_PERCENTAGE, MAX_SOUND_VOLUME_PERCENTAGE);
        //ImGui::Bullet();
        ImGui::TextColored(
            ImVec4(1.0f, 0.0f, 1.0f, 1.0f),
            SET_EVENT_DELAY_NOTIFIER_PARTIAL "<event> " " <delay>");
        ImGui::SameLine(CMD_WIDTH + ITEM_SEP);
        ImGui::Text(
            "Sets the delay of <event> to <delay> if the latter is between %.2f and %.2f.",
            MIN_SOUND_DELAY, MAX_SOUND_DELAY);

        // Storage
        //ImGui::Bullet();
        ImGui::TextColored(
            ImVec4(1.0f, 0.0f, 1.0f, 1.0f),
            SAVE_SETTINGS_NOTIFIER "  [file:" DEFAULT_SETTINGS_FILE "]");
        ImGui::SameLine(CMD_WIDTH + ITEM_SEP);
        ImGui::Text(
            "Saves the current settings to [file] in the data folder.");
        //ImGui::Bullet();
        ImGui::TextColored(
            ImVec4(1.0f, 0.0f, 1.0f, 1.0f),
            LOAD_SETTINGS_NOTIFIER "  [file:" DEFAULT_SETTINGS_FILE "]");
        ImGui::SameLine(CMD_WIDTH + ITEM_SEP);
        ImGui::Text(
            "Loads the settings stored in [file] in the data folder.");
        ImGui::Unindent();
        ImGui::Unindent();
        ImGui::BulletText(
            "Contact me on Discord");
        ImGui::Indent();
        ImGui::Indent();
        ImGui::TextUnformatted("@vadgaard");
        ImGui::Unindent();
        ImGui::TextUnformatted("if you have any questions or comments.");
        ImGui::Unindent();
        ImGui::TreePop();
    }

    ImGui::Separator();


    /*
     * SETTINGS AREA
     */

    ImGui::BeginChild(
        "GeneralArea",
        ImVec2(ITEM_WIDTH * 2 + ITEM_SEP * 2, 620),
        false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Save/load settings
    if (ImGui::Button("Save " DEFAULT_SETTINGS_FILE))
    {
        if (this->SaveSettings())
        {
            this->gameWrapper->Toast(
                "Settings were saved",
                "Good for you :^)",
                "default", 5.0, ToastType_OK);

            LOG("SETTINGS WERE SAVED.");
        }
        else
        {
            this->gameWrapper->Toast(
                "Failed to save settings",
                "Try restarting or something",
                "default", 5.0, ToastType_Warning);

            LOG("FAILED TO SAVE SETTINGS.");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load " DEFAULT_SETTINGS_FILE))
    {
        if (this->LoadSettings())
        {
            this->gameWrapper->Toast(
                "Settings were loaded",
                "Have fun! :^)",
                "default", 5.0, ToastType_OK);

            LOG("SETTINGS WERE LOADED.");
        }
        else
        {
            this->gameWrapper->Toast(
                "Failed to load settings",
                "Try restarting or something",
                "default", 5.0, ToastType_Warning);

            LOG("FAILED TO LOAD SETTINGS.");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Revert to defaults"))
    {
        this->ApplyDefaultSettings();
    }

    // Sound tracking
    bool soundTrackingEnabled = this->Settings->SoundTrackingEnabled;

    if (ImGui::Checkbox("3D tracking", &soundTrackingEnabled))
    {
        this->Settings->SoundTrackingEnabled = soundTrackingEnabled;

        if (soundTrackingEnabled)
        {
            this->HookSoundTracking();
        }
        else
        {
            this->UnhookSoundTracking();
        }
    }
    ImGui::SameLine();

    // Sound tracking
    bool previewsEnabled = this->Settings->PreviewsEnabled;

    if (ImGui::Checkbox("Preview", &previewsEnabled))
    {
        this->Settings->PreviewsEnabled = previewsEnabled;
    }
    ImGui::SameLine();

    // RL crossbar pling
    bool plingEnabled = this->Settings->CrossbarPlingEnabled;

    if (ImGui::Checkbox("Crossbar pling", &plingEnabled))
    {
        this->Settings->CrossbarPlingEnabled = plingEnabled;
    }
    ImGui::SameLine();

    // Fixed crossbar volume
    bool fixedVolumeEnabled = this->Settings->FixedCrossbarVolume;

    if (ImGui::Checkbox("Fixed crossbar volume", &fixedVolumeEnabled))
    {
        this->Settings->FixedCrossbarVolume = fixedVolumeEnabled;
    }

    // Volume
    ImGui::Separator();

    ImGui::TextUnformatted("Plugin master volume");
    SameLineColumn();
    ImGui::TextUnformatted("Plugin output device");

    float pluginVolume           = this->Settings->Volume;
    int   pluginVolumePercentage = std::lround(pluginVolume * 100.0f);

    ImGui::PushItemWidth(ITEM_WIDTH);

    if (ImGui::SliderInt("##mastervolume", &pluginVolumePercentage, MIN_PLUGIN_VOLUME * 100.0f,
                         MAX_PLUGIN_VOLUME * 100.0f, "Volume: %d%%"))
    {
        float volume = static_cast<float>(pluginVolumePercentage) / 100.0f;

        // Set volume
        HRESULT hr = this->SoundManager.SetVolume(volume);
        if (FAILED(hr))
        {
            this->gameWrapper->Toast(
                "Failed to set volume",
                "Try restarting or something",
                "default", 5.0, ToastType_Warning);

            LOG("FAILED TO SET VOLUME. HRESULT: {}", hr);
        }
        else
        {
            this->Settings->Volume = volume;
        }
    }
    ImGui::PopItemWidth();

    // Preview sound when releasing slider
    if (this->Settings->PreviewsEnabled && ImGui::IsItemDeactivated())
    {
        this->gameWrapper->Execute(
            [this](GameWrapper*)
            {
                // Play the sound
                this->PlayPlayerGoalSfx();
            });
    }

    SameLineColumn();

    // Audio device
    SoundInterface::AudioDevice              outputDevice;
    std::vector<SoundInterface::AudioDevice> devices =
        this->SoundManager.ConsolidateAudioDevices(outputDevice, true);
    std::string outputName = Utils::WStringToString(outputDevice.Name);

    ImGui::PushItemWidth(ITEM_WIDTH);
    if (ImGui::BeginCombo("##Output device", outputName.c_str()))
    {
        for (const auto& device : devices)
        {
            std::string name = Utils::WStringToString(device.Name);

            if (ImGui::Selectable(name.c_str(), outputDevice.Id == device.Id))
            {
                DEBUGLOG(L"SELECTED DEVICE: {} ({})", device.Name, device.Id);

                HRESULT hr = this->SoundManager.SetOutputId(device.Id);
                if (FAILED(hr))
                {
                    this->gameWrapper->Toast(
                        "Failed to set output device",
                        "Try restarting or something",
                        "default", 5.0, ToastType_Warning);

                    LOG("FAILED TO SET OUTPUT DEVICE. HRESULT: {}", hr);
                }
                else
                {
                    this->Settings->OutputId = device.Id;
                }

                // Preview player goal
                if (this->Settings->PreviewsEnabled)
                {
                    this->gameWrapper->Execute(
                        [this](GameWrapper*)
                        {
                            // Play the sound
                            this->PlayPlayerGoalSfx();
                        });
                }
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    // Recommended volume
    bool hasRecommendedVolume = this->LastMasterVolume.has_value()
        && this->LastGameplayVolume.has_value();

    if (hasRecommendedVolume)
    {
        // Compute recommended volume
        const float recommendedVolume = this->LastMasterVolume.value() * this->LastGameplayVolume.value();
        const int recommendedVolumePercentage = std::lround(recommendedVolume * 100.0f);

        // Make button active if not at recommended volume
        if (pluginVolumePercentage != recommendedVolumePercentage)
        {
            std::string label =
                "Apply recommended volume (" + std::to_string(recommendedVolumePercentage) + "%)";

            ImGui::PushStyleColor(ImGuiCol_Button, static_cast<ImVec4>(ImColor::HSV(7 / 7.0f, 0.6f, 0.6f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, static_cast<ImVec4>(ImColor::HSV(7 / 7.0f, 0.7f, 0.7f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, static_cast<ImVec4>(ImColor::HSV(7 / 7.0f, 0.8f, 0.8f)));
            if (ImGui::Button(label.c_str(), ImVec2(ITEM_WIDTH, ImGui::GetFrameHeight())))
            {
                const float volume = static_cast<float>(recommendedVolumePercentage) / 100.0f;
                HRESULT     hr     = this->SoundManager.SetVolume(volume);
                if (FAILED(hr))
                {
                    this->gameWrapper->Toast(
                        "Failed to set volume",
                        "Try restarting or something",
                        "default", 5.0, ToastType_Warning);

                    LOG("FAILED TO SET VOLUME. HRESULT: {}", hr);
                }
                else
                {
                    this->Settings->Volume = volume;
                }

                // Preview player goal sound
                if (this->Settings->PreviewsEnabled)
                {
                    this->gameWrapper->Execute(
                        [this](GameWrapper*)
                        {
                            // Play the sound
                            this->PlayPlayerGoalSfx();
                        });
                }
            }
        }
        // Disable button if already at recommended volume
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, static_cast<ImVec4>(ImColor::HSV(0, 0, .2f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, static_cast<ImVec4>(ImColor::HSV(0, 0, .2f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, static_cast<ImVec4>(ImColor::HSV(0, 0, .2f)));
            ImGui::PushStyleColor(ImGuiCol_Text, static_cast<ImVec4>(ImColor::HSV(0, 0, .6f)));
            ImGui::Button("Volume already at recommended", ImVec2(ITEM_WIDTH, ImGui::GetFrameHeight()));
            ImGui::PopStyleColor(1);
        }
    }
    // Disable button if recommended volume not available
    else
    {
        std::string label = "Apply recommended volume (see guide)";

        ImGui::PushStyleColor(ImGuiCol_Button, static_cast<ImVec4>(ImColor::HSV(0, 0, .2f)));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, static_cast<ImVec4>(ImColor::HSV(0, 0, .2f)));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, static_cast<ImVec4>(ImColor::HSV(0, 0, .2f)));
        ImGui::PushStyleColor(ImGuiCol_Text, static_cast<ImVec4>(ImColor::HSV(0, 0, .6f)));
        ImGui::Button(label.c_str(), ImVec2(ITEM_WIDTH, ImGui::GetFrameHeight()));
        ImGui::PopStyleColor(1);
    }
    ImGui::PopStyleColor(3);


    // Sounds
    for (int i = 0; i < this->Settings->Sounds.size(); ++i)
    {
        auto eventId = static_cast<RlEvents::Kind>(i);
        SoundSettings& soundSettings = this->Settings->Sounds[i];

        ImGui::Separator();

        // Is the event 3D?
        bool is3D = IsEvent3D(eventId);

        // Define the identifiers
        std::string volumeLabel = "##soundvolume" + std::to_string(i);
        std::string delayLabel  = "##sounddelay" + std::to_string(i);

        {
            // Generate the combo
            this->GenerateSoundFileCombo(eventId, soundSettings);

            SameLineColumn();

            // Is the sound enabled?
            bool enable = soundSettings.IsEnabled;

            // Decide on visuals
            std::string eventLabel  = GetEventLabel(eventId);
            std::string enableLabel = "Custom " + eventLabel + " sound";
            if (is3D) enableLabel += " (3D)";

            // Create the checkbox
            if (ImGui::Checkbox(enableLabel.c_str(), &enable))
            {
                soundSettings.IsEnabled = enable;
            }
        }

        {
            // Sound volume slider
            int soundVolumePercentage = std::lround(soundSettings.Volume * 100.0f);
            ImGui::PushItemWidth(HALF_ITEM_WIDTH);
            if (ImGui::DragInt(volumeLabel.c_str(), &soundVolumePercentage, 0.5f, MIN_SOUND_VOLUME * 100.0f,
                               MAX_SOUND_VOLUME * 100.0f, "Volume: %d%%"))
            {
                soundSettings.Volume = static_cast<float>(soundVolumePercentage) / 100.0f;
            }
            ImGui::PopItemWidth();

            // Preview sound when releasing slider
            if (this->Settings->PreviewsEnabled && ImGui::IsItemDeactivated())
            {
                // Only define sound params for 3D audio
                if (is3D)
                {
                    // Generate random vector at camera distance
                    Vector location = Utils::GetRandomVector(this->CamInfo->Distance);

                    this->gameWrapper->Execute(
                        [this, eventId, location](GameWrapper*)
                        {
                            // Play the sound
                            std::pair<Vector, bool> params = std::make_pair(location, true);
                            this->PlayEventSound(eventId, params);

                            // Store random position for rendering
                            NoteClick(this, eventId, location);
                        });
                }
                else
                {
                    this->gameWrapper->Execute(
                        [this, eventId](GameWrapper*)
                        {
                            // Play the sound
                            this->PlayEventSound(eventId);
                        });
                }
            }

            SameLineHalfColumn();

            // Sound delay slider
            float delay = soundSettings.Delay;
            ImGui::PushItemWidth(HALF_ITEM_WIDTH);
            if (ImGui::DragFloat(delayLabel.c_str(), &delay, 0.05f, MIN_SOUND_DELAY, MAX_SOUND_DELAY, "Delay: %.2fs"))
            {
                soundSettings.Delay = delay;
            }
            ImGui::PopItemWidth();

            // Play sound when releasing slider
            if (this->Settings->PreviewsEnabled && ImGui::IsItemDeactivated())
            {
                // Only define sound params for 3D audio
                if (is3D)
                {
                    // Generate random vector at camera distance
                    Vector location = Utils::GetRandomVector(this->CamInfo->Distance);

                    this->gameWrapper->Execute(
                        [this, eventId, location](GameWrapper*)
                        {
                            // Play the sound
                            std::pair<Vector, bool> params = std::make_pair(location, true);
                            this->PlayEventSound(eventId, params);

                            // Store random position for rendering
                            NoteClick(this, eventId, location);
                        });
                }
                else
                {
                    this->gameWrapper->Execute(
                        [this, eventId](GameWrapper*)
                        {
                            // Play the sound
                            this->PlayEventSound(eventId);
                        });
                }
            }

            SameLineColumn();

            // Define button text
            std::string buttonText = is3D ? "Play 3D sound" : "Play 2D sound";
            buttonText += "##" + std::to_string(i);

            // Create the play button
            ImGui::PushItemWidth(PLAY_BUTTON_WIDTH);
            ImGui::PushStyleColor(ImGuiCol_Button, static_cast<ImVec4>(ImColor::HSV(3 / 7.0f, 0.6f, 0.6f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, static_cast<ImVec4>(ImColor::HSV(3 / 7.0f, 0.7f, 0.7f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, static_cast<ImVec4>(ImColor::HSV(3 / 7.0f, 0.8f, 0.8f)));
            if (ImGui::Button(buttonText.c_str()))
            {
                // Only define sound params for 3D audio
                if (is3D)
                {
                    // Generate random vector at camera distance
                    Vector location = Utils::GetRandomVector(this->CamInfo->Distance);

                    this->gameWrapper->Execute(
                        [this, eventId, location](GameWrapper*)
                        {
                            // Play the sound
                            std::pair<Vector, bool> params = std::make_pair(location, true);
                            this->PlayEventSound(eventId, params);

                            // Store random position for rendering
                            NoteClick(this, eventId, location);
                        });
                }
                else
                {
                    this->gameWrapper->Execute(
                        [this, eventId](GameWrapper*)
                        {
                            // Play the sound
                            this->PlayEventSound(eventId);
                        });
                }
            }
            ImGui::PopStyleColor(3);
            ImGui::PopItemWidth();
        }
    }

    // Store the height for the next window
    float windowHeight = ImGui::GetCursorPosY();

    // LOG("height: {}", windowHeight);

    // End area
    ImGui::EndChild();
    ImGui::SameLine();


    /*
     * MAP AREA
     */

    auto areaSize = ImVec2(windowHeight * Utils::RlValues::MAP_ASPECT_RATIO, windowHeight);
    ImGui::BeginChild(
        "MapArea",
        areaSize,
        false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Draw the map
    this->DrawMapAndPositions(areaSize, {0, 0});

    // End area
    ImGui::EndChild();
}


/* VISUALIZING THE MAP */

void EventSfx::DrawMapAndPositions(
    const ImVec2& areaSize,
    const ImVec2& cameraPos)
{
    ImVec2      areaTopLeft = ImGui::GetWindowPos();
    ImDrawList* drawList    = ImGui::GetWindowDrawList();

    constexpr auto mapWidth  = Utils::RlValues::MAP_WIDTH;
    constexpr auto mapHeight = Utils::RlValues::MAP_HEIGHT;

    // Normalize player and sound positions from game coordinates to [0,1]
    auto normalizePosition = [&](const ImVec2 pos) -> ImVec2
    {
        const float x = 1.0f - (pos.x + mapWidth / 2.0f) / mapWidth;
        const float y = 1.0f - (pos.y + mapHeight / 2.0f) / mapHeight;
        return {x, y};
    };

    ImVec2 normalizedCameraPos = normalizePosition(cameraPos);

    // Draw the map rectangle
    ImVec2 areaBottomRight(areaTopLeft.x + areaSize.x, areaTopLeft.y + areaSize.y);
    drawList->AddRectFilled(areaTopLeft, areaBottomRight, IM_COL32(138, 240, 166, 150));
    drawList->AddRect(areaTopLeft, areaBottomRight, IM_COL32(255, 255, 255, 255));

    // Calculate the scaled camera position
    auto scaleToArea = [&](const ImVec2 normalizedPos) -> ImVec2
    {
        return {
            areaTopLeft.x + normalizedPos.x * areaSize.x,
            areaTopLeft.y + normalizedPos.y * areaSize.y
        };
    };

    ImVec2 scaledCameraPos = scaleToArea(normalizedCameraPos);

    // Draw camera and car icons
    auto carPosX           = scaledCameraPos.x;
    auto carPosY           = scaledCameraPos.y - this->CamInfo->Distance / mapWidth * areaSize.y;
    auto carPosAboveCamera = ImVec2(carPosX, carPosY);

    // Position of car above the camera
    DrawCameraAndCarIcons(drawList, scaledCameraPos, carPosAboveCamera, areaSize);

    // Detect mouse click within the map area
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        if (mousePos.x >= areaTopLeft.x && mousePos.x <= areaBottomRight.x
            && mousePos.y >= areaTopLeft.y && mousePos.y <= areaBottomRight.y)
        {
            // Normalize for in-game coordinates
            auto  clickedPos  = ImVec2(mousePos.x - areaTopLeft.x, mousePos.y - areaTopLeft.y);
            float normalizedX = clickedPos.x / areaSize.x;
            float normalizedY = clickedPos.y / areaSize.y;

            // Convert back to game coordinates
            Vector location;
            location.X = (1.0f - normalizedX) * 8192.0f - 4096;
            location.Y = (1.0f - normalizedY) * 10240.0f - 5120;
            location.Z = 20.0f;

            this->gameWrapper->Execute(
                [this, location](GameWrapper*)
                {
                    // Play the sound at given position
                    this->PlayBumpSfx(location, true);

                    // Store information
                    NoteClick(this, RlEvents::Kind::Bump, location);
                });
        }
    }
    else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        if (mousePos.x >= areaTopLeft.x && mousePos.x <= areaBottomRight.x
            && mousePos.y >= areaTopLeft.y && mousePos.y <= areaBottomRight.y)
        {
            // Normalize for in-game coordinates
            auto  clickedPos  = ImVec2(mousePos.x - areaTopLeft.x, mousePos.y - areaTopLeft.y);
            float normalizedX = clickedPos.x / areaSize.x;
            float normalizedY = clickedPos.y / areaSize.y;

            // Convert back to game coordinates
            Vector location;
            location.X = (1.0f - normalizedX) * 8192.0f - 4096;
            location.Y = (1.0f - normalizedY) * 10240.0f - 5120;
            location.Z = 20.0f;

            this->gameWrapper->Execute(
                [this, location](GameWrapper*)
                {
                    // Compute random ball speed
                    float speed = Utils::GetRandomFloat(500.0f, 5000.0f);
                    float multiplier = Utils::ComputeVolumeMultiplier(speed);

                    // Play the sound at given position
                    this->PlayCrossbarSfx(location, multiplier, true);

                    // Store information
                    NoteClick(this, RlEvents::Kind::Crossbar, location);
                    lastSpeed = (speed * 60 * 60) / 100 / 1000;
                });
        }
    }

    // Draw the camera distance
    std::string cameraDistanceText =
        "camera distance: " + std::to_string(static_cast<int>(this->CamInfo->Distance)) + "cm";
    auto textPos = ImVec2(areaTopLeft.x + 2, areaTopLeft.y);
    drawList->AddText(
        nullptr,
        INFO_FONT_SIZE,
        textPos,
        IM_COL32(255, 255, 255, 255),
        cameraDistanceText.c_str());
    textPos.y += INFO_FONT_SIZE * 1.2f;

    // Draw legend
    ImVec2 carTextPos = textPos;
    carTextPos.x += 5.0f;
    carTextPos.y += 7.0f;
    DrawCar(drawList, carTextPos, 7.0f);
    carTextPos.x += 6.0f;
    carTextPos.y -= 7.0f;
    std::string carLegendText = ": player";
    drawList->AddText(
        nullptr,
        INFO_FONT_SIZE,
        carTextPos,
        IM_COL32(255, 255, 255, 255),
        carLegendText.c_str());
    textPos.y += INFO_FONT_SIZE;

    ImVec2 arrowTextPos = textPos;
    arrowTextPos.x += 5.0f;
    arrowTextPos.y += 7.0f;
    DrawArrow(drawList, arrowTextPos, 6.0f);
    arrowTextPos.x += 6.0f;
    arrowTextPos.y -= 7.0f;
    std::string arrowLegendText = ": camera";
    drawList->AddText(
        nullptr,
        INFO_FONT_SIZE,
        arrowTextPos,
        IM_COL32(255, 255, 255, 255),
        arrowLegendText.c_str());
    textPos.y += INFO_FONT_SIZE;

    ImVec2 circlePos = textPos;
    circlePos.x += 5.5f;
    circlePos.y += INFO_FONT_SIZE / 2.0f;
    drawList->AddCircle(
        circlePos,
        INFO_FONT_SIZE / 2.7f,
        IM_COL32(20, 20, 20, 100));
    circlePos.x += 5.5f;
    circlePos.y -= INFO_FONT_SIZE / 2.0f;
    std::string circleLegendText = ": camera distance";
    drawList->AddText(
        nullptr,
        INFO_FONT_SIZE,
        circlePos,
        IM_COL32(255, 255, 255, 255),
        circleLegendText.c_str());
    textPos.y += INFO_FONT_SIZE;
    textPos.y += INFO_FONT_SIZE;

    // Draw tutorial
    drawList->AddText(
        nullptr,
        INFO_FONT_SIZE,
        textPos,
        IM_COL32(255, 255, 255, 255),
        "left   click = bump");
    textPos.y += INFO_FONT_SIZE;
    drawList->AddText(
        nullptr,
        INFO_FONT_SIZE,
        textPos,
        IM_COL32(255, 255, 255, 255),
        "right click = crossbar");
    textPos.y += INFO_FONT_SIZE;

    // Update clicked positions: remove expired and draw active
    auto now = std::chrono::steady_clock::now();
    auto it  = clickedPositions.begin();
    while (it != clickedPositions.end())
    {
        // Duration for clicks to stay active
        const auto renderDuration = it->Duration + it->Delay;
        const auto opaqueDuration = renderDuration / 4 + it->Delay;

        auto playedDuration = now - it->Timestamp;

        if (playedDuration > renderDuration)
        {
            it = clickedPositions.erase(it); // Remove expired click
        }
        else if (playedDuration <= renderDuration)
        {
            float playedDurationSeconds = std::chrono::duration<float>(playedDuration).count();
            float opaqueDurationSeconds = std::chrono::duration<float>(opaqueDuration).count();
            float durationSeconds       = std::chrono::duration<float>(renderDuration).count();
            float delaySeconds          = std::chrono::duration<float>(it->Delay).count();

            // Compute alpha
            int alpha = 255;
            if (playedDuration > opaqueDuration)
            {
                float fractionElapsed = (playedDurationSeconds - opaqueDurationSeconds)
                    / (durationSeconds - opaqueDurationSeconds);
                int tmpAlpha = std::lround(255.0f * (1.0f - fractionElapsed));

                alpha = 0 < tmpAlpha ? tmpAlpha : 0;
            }

            // Compute dot size
            float size = 2.0f;
            if (playedDuration > it->Delay)
            {
                float fractionElapsed = (playedDurationSeconds - delaySeconds)
                    / (durationSeconds - delaySeconds);
                float tmpSize = 7.0f * fractionElapsed + 2.0f;

                size = 0 < tmpSize ? tmpSize : 0;
            }

            // Draw it
            ImVec2 normalizedPos = normalizePosition(it->Pos);
            ImVec2 scaledPos     = scaleToArea(normalizedPos);
            drawList->AddCircleFilled(scaledPos, size, IM_COL32(255, 50, 20, alpha));
            ++it;
        }
    }

    // Draw info for latest click
    auto shownDuration = now - lastPositionTime;

    if (shownDuration <= lastPositionDuration)
    {
        uint8_t numlines = 2;

        std::string locationText =
            "x: "
            + std::to_string(std::lround(lastPosition.x))
            + "cm\ny: "
            + std::to_string(std::lround(lastPosition.y))
            + "cm";

        if (lastSpeed.has_value())
        {
            locationText += "\ns: " + std::to_string(std::lround(lastSpeed.value())) + "kph";
            numlines = 3;
        }

        auto infoPos = ImVec2(textPos.x, areaTopLeft.y + areaSize.y - (numlines + 1) * INFO_FONT_SIZE - 2);

        drawList->AddText(
            nullptr,
            INFO_FONT_SIZE,
            infoPos,
            IM_COL32(255, 255, 255, 255),
            locationText.c_str());

        // Draw in-game click distance
        infoPos.y += INFO_FONT_SIZE * numlines;
        float distance = std::sqrt(std::pow(lastPosition.x, 2)
								 + std::pow(lastPosition.y, 2));
        std::string distanceText = "d: " + std::to_string(std::lround(distance)) + "cm";
        drawList->AddText(
            nullptr,
            INFO_FONT_SIZE,
            infoPos,
            IM_COL32(255, 255, 255, 255),
            distanceText.c_str());
    }
}


void DrawCar(
    ImDrawList*   drawList,
    const ImVec2& position,
    const float   iconSize)
{
    // Smaller size for the wheels
    const float wheelSize = iconSize / 3.5f;

    // Draw the back wheels
    const auto backWheelsMin = ImVec2(position.x - iconSize / 1.9f, position.y + iconSize / 2.0f - wheelSize);
    const auto backWheelsMax = ImVec2(position.x + iconSize / 1.9f + 0.5f, position.y + iconSize / 2 + 0.5f);
    drawList->AddRectFilled(backWheelsMin, backWheelsMax, IM_COL32(0, 0, 0, 255));

    // Draw the front wheels
    const auto frontWheelsMin = ImVec2(position.x - iconSize / 1.9f,
                                       position.y - iconSize / 2.0f + iconSize / 8 + 0.5f);
    const auto frontWheelsMax = ImVec2(position.x + iconSize / 1.9f + 0.5f,
                                       position.y - iconSize / 2 + wheelSize + iconSize / 8 + 0.5f);
    drawList->AddRectFilled(frontWheelsMin, frontWheelsMax, IM_COL32(0, 0, 0, 255));

    // Draw car icon (a simple rectangle above the camera)
    const auto carIconMin = ImVec2(position.x - iconSize / 3.0f, position.y - iconSize / 2.0f);
    const auto carIconMax = ImVec2(position.x + iconSize / 3.0f + 0.5f, position.y + iconSize / 2.0f + 0.5f);
    drawList->AddRectFilled(carIconMin, carIconMax, IM_COL32(255, 255, 255, 255));
    drawList->AddRect(carIconMin, carIconMax, IM_COL32(10, 10, 10, 200), 1.0f);
}

void DrawArrow(
    ImDrawList*   drawList,
    const ImVec2& position,
    const float   iconSize)
{
    // Arrow properties
    const float shaftHeight = iconSize * 0.60f;
    const float shaftWidth  = shaftHeight * 0.40f;
    const float headWidth   = shaftWidth * 4.50f;

    // Shaft of the arrow
    ImVec2 shaftTop = position;
    shaftTop.y -= 1.0f;
    const auto shaftBottom = ImVec2(position.x, position.y + shaftHeight - 1.0f);
    drawList->AddLine(shaftTop, shaftBottom, IM_COL32(10, 10, 10, 255), shaftWidth);

    // Arrowhead
    const float headBottomY = position.y + headWidth / 5.0f - 1.0f + 0.5f;
    const auto  headLeft    = ImVec2(position.x - headWidth / 2.0f, headBottomY);
    const auto  headRight   = ImVec2(position.x + headWidth / 2.0f + 0.5f, headBottomY);
    const auto  headTop     = ImVec2(shaftTop.x, shaftTop.y - (headWidth - (headBottomY - shaftTop.y)) * 0.8f - 0.6f);
    drawList->AddTriangleFilled(headTop, headLeft, headRight, IM_COL32(10, 10, 10, 255));
}

void EventSfx::DrawCameraAndCarIcons(
    ImDrawList*   drawList,
    const ImVec2& cameraPos,
    const ImVec2& carPos,
    const ImVec2& areaSize) const
{
    constexpr auto mapHeight = Utils::RlValues::MAP_HEIGHT;
    // Draw camera distance circle
    const float cameraDistance = this->CamInfo->Distance / mapHeight * areaSize.y;
    drawList->AddCircle(cameraPos, cameraDistance, IM_COL32(30, 30, 20, 70));

    // Icon size should correspond to length of the Octane
    const float iconSize = 144.0f / mapHeight * areaSize.y;

    DrawCar(drawList, carPos, iconSize);
    DrawArrow(drawList, cameraPos, iconSize);

    // Center dot
    // drawList->AddCircleFilled(cameraPos, 1.0f, IM_COL32(240, 240, 240, 200));
}


/* GENERATING COMBOS */

void EventSfx::GenerateSoundFileCombo(
    const RlEvents::Kind eventId,
    SoundSettings&       soundSettings)
{
    const std::string identifier = "##sound" + std::to_string(eventId);

    const std::string soundFile = soundSettings.SoundId;

    static std::vector<std::string> soundFiles;
    static bool                     needToUpdateList = true;

    ImGui::PushItemWidth(ITEM_WIDTH);
    if (ImGui::BeginCombo(identifier.c_str(), soundFile.c_str()))
    {
        if (!isComboOpen)
        {
            if (needToUpdateList)
            {
                soundFiles       = this->SoundManager.ListSoundFiles();
                needToUpdateList = false;
            }
            isComboOpen = true;
        }

        for (const auto& soundFileName : soundFiles)
        {
            std::string soundId = soundFileName;

            const bool isSelected = (soundFile == soundFileName);
            if (ImGui::Selectable(soundFileName.c_str(), isSelected))
            {
                // Load sound
                HRESULT hr = this->SoundManager.LoadSound(soundId, true);
                if (FAILED(hr))
                {
                    // Toast
                    this->gameWrapper->Toast(
                        "Failed to load sound",
                        "Try converting it to e.g. 16-bit PCM",
                        "default", 5.0, ToastType_Warning);

                    LOG("LOAD FAILED. SOUNDID: {}, HRESULT: {}", soundId, hr);
                }
                else
                {
                    // Set the sound ID
                    soundSettings.SoundId = soundId;
                }

                // Preview the sound
                if (this->Settings->PreviewsEnabled)
                {
                    this->gameWrapper->SetTimeout(
                        [this, eventId](GameWrapper*)
                        {
                            this->PlayEventSound(eventId);
                        },
                        std::min(0.1f, soundSettings.Delay));
                }
            }

            // Set the initial focus to the current selection
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
    else
    {
        isComboOpen      = false;
        needToUpdateList = true;
    }
    ImGui::PopItemWidth();
}
