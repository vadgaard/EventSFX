#include "pch.h"
#include "EventSFX.h"

BAKKESMOD_PLUGIN(EventSfx, "EventSFX", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CameraInfo>         globalCameraInfo;
std::shared_ptr<PluginSettings>     globalPluginSettings;
std::shared_ptr<CVarManagerWrapper> globalCVarManager;
std::shared_ptr<GameWrapper>        globalGameWrapper;


/* GENERAL */

void EventSfx::onLoad()
{
    // Globals
    globalCVarManager    = this->cvarManager;
    globalGameWrapper    = this->gameWrapper;
    globalCameraInfo     = this->CamInfo;
    globalPluginSettings = this->Settings;

    // Camera info
    this->CamInfo->Distance = this->gameWrapper->GetSettings().GetCameraSettings().Distance;
    this->CamInfo->Height   = this->gameWrapper->GetSettings().GetCameraSettings().Height;
    this->CamInfo->Pitch    = this->gameWrapper->GetSettings().GetCameraSettings().Pitch;

    LOG("LOADING EventSFX");

    // Load the settings
    if (!this->Settings->Load())
    {
        this->Settings->SetDefaults();
    }

    // Run the plugin setup
    this->RunSetup();

    if (!*this->Enabled)
    {
        LOG("EventSFX COULD NOT LOAD");
        LOG("TRY RESTARTING OR SOMETHING");
    }
    else
    {
        LOG("EventSFX WAS LOADED");
    }
}

void EventSfx::onUnload()
{
    if (*this->Enabled)
    {
        LOG("ATTEMPTING TO SAVE SETTINGS..");
        // Save the settings
        if (!this->Settings->Save())
        {
            LOG("COULD NOT SAVE SETTINGS.");
        }

        // Unrun the plugin setup
        LOG("UNLOADING.. :(");
        this->UnrunSetup();
    }
}

void EventSfx::RunSetup()
{
    *this->Enabled = true;
    if (!this->InitializeAudio())
    {
        *this->Enabled = false;
        LOG("COULD NOT INITIALIZE AUDIO");
        return;
    }
    this->InitializeNotifiers();
    this->InitializeHooks();
}

void EventSfx::UnrunSetup()
{
    *this->Enabled = false;
    this->DeinitializeHooks();
    this->DeinitializeAudio();
}


/* NOTIFIERS */

void EventSfx::InitializeNotifiers()
{
    // Notifier: Play any sound with the plugin
    this->cvarManager->registerNotifier(
        PLAY_SOUND_NOTIFIER,
        [this](const std::vector<std::string>& args)
        {
            if (args.size() < 2) return;

            const std::string soundId = args[1];

            if (args.size() > 2)
            {
                const std::string volumeStr = args[2];

                try
                {
                    const int volumePercentage = std::stof(volumeStr);
                    if (volumePercentage >= MIN_SOUND_VOLUME_PERCENTAGE
                        && volumePercentage <= MAX_SOUND_VOLUME_PERCENTAGE)
                    {
                        const float volume = volumePercentage / 100.0f;
                        this->PlaySoundFile(soundId, std::nullopt, volume);
                    }
                    else
                    {
                        LOG("INVALID ARGUMENT: VOLUME SHOULD BE BETWEEN {} AND {}.", MIN_SOUND_VOLUME_PERCENTAGE,
                            MAX_SOUND_VOLUME_PERCENTAGE);
                    }
                }
                catch ([[maybe_unused]] const std::invalid_argument& e)
                {
                    LOG("INVALID ARGUMENT: COULD NOT CONVERT '" + volumeStr + "' TO AN INT.");
                }
            }
            else
            {
                this->PlaySoundFile(soundId);
            }
        },
        "Play any sound file in " + Utils::WStringToString(SoundInterface::GetSoundsFolder()),
        PERMISSION_ALL
    );

    // Notifier: Set the volume
    this->cvarManager->registerNotifier(
        SET_PLUGIN_VOLUME_NOTIFIER,
        [this](const std::vector<std::string>& args)
        {
            if (args.size() < 2) return;

            const std::string volumeStr = args[1];

            try
            {
                const int volumePercentage = std::stof(volumeStr);
                if (volumePercentage >= MIN_PLUGIN_VOLUME_PERCENTAGE
                    && volumePercentage <= MAX_PLUGIN_VOLUME_PERCENTAGE)
                {
                    const float volume     = volumePercentage / 100.0f;
                    this->Settings->Volume = volume;
                }
                else
                {
                    LOG("INVALID ARGUMENT: VOLUME SHOULD BE BETWEEN {} AND {}.", MIN_PLUGIN_VOLUME_PERCENTAGE,
                        MAX_PLUGIN_VOLUME_PERCENTAGE);
                }
            }
            catch ([[maybe_unused]] const std::invalid_argument& e)
            {
                LOG("INVALID ARGUMENT: COULD NOT CONVERT '" + volumeStr + "' TO AN INT.");
            }
        },
        "Set the plugin volume",
        PERMISSION_ALL
    );

    // Notifier: Toggle 3D sound tracking
    this->cvarManager->registerNotifier(
        TOGGLE_SOUNDTRACKING_NOTIFIER,
        [this](std::vector<std::string>)
        {
            if (this->Settings->SoundTrackingEnabled)
            {
                this->Settings->SoundTrackingEnabled = false;
                this->UnhookSoundTracking();
            }
            else
            {
                this->Settings->SoundTrackingEnabled = true;
                this->HookSoundTracking();
            }
        },
        "Toggle 3D sound tracking",
        PERMISSION_ALL
    );

    // Notifier: Enable 3D sound tracking
    this->cvarManager->registerNotifier(
        ENABLE_SOUNDTRACKING_NOTIFIER,
        [this](std::vector<std::string>)
        {
            if (!this->Settings->SoundTrackingEnabled)
            {
                this->Settings->SoundTrackingEnabled = true;
                this->HookSoundTracking();
            }
        },
        "Enable 3D sound tracking",
        PERMISSION_ALL
    );

    // Notifier: Disable 3D sound tracking
    this->cvarManager->registerNotifier(
        DISABLE_SOUNDTRACKING_NOTIFIER,
        [this](std::vector<std::string>)
        {
            if (this->Settings->SoundTrackingEnabled)
            {
                this->Settings->SoundTrackingEnabled = false;
                this->UnhookSoundTracking();
            }
        },
        "Disable 3D sound tracking",
        PERMISSION_ALL
    );

    // Notifier: Toggle crossbar pling
    this->cvarManager->registerNotifier(
        TOGGLE_PLING_NOTIFIER,
        [this](std::vector<std::string>)
        {
            this->Settings->CrossbarPlingEnabled = !this->Settings->CrossbarPlingEnabled;
        },
        "Toggle default crossbar pling",
        PERMISSION_ALL
    );

    // Notifier: Disable crossbar pling
    this->cvarManager->registerNotifier(
        DISABLE_PLING_NOTIFIER,
        [this](std::vector<std::string>)
        {
            this->Settings->CrossbarPlingEnabled = false;
        },
        "Disable default crossbar pling",
        PERMISSION_ALL
    );

    // Notifier: Enable crossbar pling
    this->cvarManager->registerNotifier(
        DISABLE_PLING_NOTIFIER,
        [this](std::vector<std::string>)
        {
            this->Settings->CrossbarPlingEnabled = true;
        },
        "Enable default crossbar pling",
        PERMISSION_ALL
    );

    for (int i = 0; i < this->Settings->Sounds.size(); ++i)
    {
        auto eventId = static_cast<RlEvents::Kind>(i);

        // Notifier: Toggle rl_events
        this->cvarManager->registerNotifier(
            TOGGLE_EVENT_NOTIFIER_PARTIAL + GetEventStringId(eventId),
            [this, i](std::vector<std::string>)
            {
                this->Settings->Sounds[i].IsEnabled = !this->Settings->Sounds[i].IsEnabled;
            },
            "Toggle " + GetEventLabel(eventId),
            PERMISSION_ALL
        );

        // Notifier: Enable rl_events
        this->cvarManager->registerNotifier(
            ENABLE_EVENT_NOTIFIER_PARTIAL + GetEventStringId(eventId),
            [this, i](std::vector<std::string>)
            {
                this->Settings->Sounds[i].IsEnabled = true;
            },
            "Enable " + GetEventLabel(eventId),
            PERMISSION_ALL
        );

        // Notifier: Disable rl_events
        this->cvarManager->registerNotifier(
            DISABLE_EVENT_NOTIFIER_PARTIAL + GetEventStringId(eventId),
            [this, i](std::vector<std::string>)
            {
                this->Settings->Sounds[i].IsEnabled = false;
            },
            "Disable " + GetEventLabel(eventId),
            PERMISSION_ALL
        );

        // Notifier: Set sound for event
        this->cvarManager->registerNotifier(
            SET_EVENT_SOUND_NOTIFIER_PARTIAL + GetEventStringId(eventId),
            [this, i](const std::vector<std::string>& args)
            {
                if (args.size() < 2) return;

                std::string soundId = args[1];

                HRESULT hr = this->SoundManager.LoadSound(soundId, true);
                if (FAILED(hr))
                {
                    LOG("LOAD FAILED. SOUNDID: {}, HRESULT: {}", soundId, hr);
                }
                else
                {
                    // Set the sound ID
                    this->Settings->Sounds[i].SoundId = soundId;
                }
            },
            "Set custom " + GetEventLabel(eventId) + " sound",
            PERMISSION_ALL
        );

        // Notifier: Play event sounds
        this->cvarManager->registerNotifier(
            PLAY_EVENT_NOTIFIER_PARTIAL + GetEventStringId(eventId),
            [this, eventId](std::vector<std::string>)
            {
                this->PlayEventSound(eventId);
            },
            "Play " + GetEventLabel(eventId) + " sound",
            PERMISSION_ALL
        );

        // Notifier: Set event volume
        this->cvarManager->registerNotifier(
            SET_EVENT_VOLUME_NOTIFIER_PARTIAL + GetEventStringId(eventId),
            [this, i](const std::vector<std::string>& args)
            {
                if (args.size() < 2) return;

                const std::string volumeStr = args[1];

                try
                {
                    const int volumePercentage = std::stof(volumeStr);
                    if (volumePercentage >= MIN_SOUND_VOLUME_PERCENTAGE
                        && volumePercentage <= MAX_SOUND_VOLUME_PERCENTAGE)
                    {
                        const float volume               = volumePercentage / 100.0f;
                        this->Settings->Sounds[i].Volume = volume;
                    }
                    else
                    {
                        LOG("INVALID ARGUMENT: VOLUME SHOULD BE BETWEEN {} AND {}.", MIN_SOUND_VOLUME_PERCENTAGE,
                            MAX_SOUND_VOLUME_PERCENTAGE);
                    }
                }
                catch ([[maybe_unused]] const std::invalid_argument& e)
                {
                    LOG("INVALID ARGUMENT: COULD NOT CONVERT '" + volumeStr + "' TO AN INT.");
                }
            },
            "Set the " + GetEventLabel(eventId) + " submix volume",
            PERMISSION_ALL
        );

        // Notifier: Set event delay
        this->cvarManager->registerNotifier(
            SET_EVENT_DELAY_NOTIFIER_PARTIAL + GetEventStringId(eventId),
            [this, i](const std::vector<std::string>& args)
            {
                if (args.size() < 2) return;

                const std::string delayStr = args[1];

                try
                {
                    const float delay = std::stof(delayStr);
                    if (delay >= MIN_SOUND_DELAY && delay <= MAX_SOUND_DELAY)
                    {
                        this->Settings->Sounds[i].Delay = delay;
                    }
                    else
                    {
                        LOG("INVALID ARGUMENT: DELAY SHOULD BE BETWEEN {} AND {}.", MIN_SOUND_DELAY, MAX_SOUND_DELAY);
                    }
                }
                catch ([[maybe_unused]] const std::invalid_argument& e)
                {
                    LOG("INVALID ARGUMENT: COULD NOT CONVERT '" + delayStr + "' TO A FLOAT.");
                }
            },
            "Set the " + GetEventLabel(eventId) + " delay",
            PERMISSION_ALL
        );
    }

    // Notifier: Save settings
    this->cvarManager->registerNotifier(
        SAVE_SETTINGS_NOTIFIER,
        [this](std::vector<std::string> args)
        {
            if (args.size() < 2)
            {
                args.push_back(DEFAULT_SETTINGS_FILE);
            }

            if (!this->SaveSettings(args[1]))
            {
                LOG("FAILED TO SAVE SETTINGS");
            }
        },
        "Save plugin settings",
        PERMISSION_ALL
    );

    // Notifier: Load settings
    this->cvarManager->registerNotifier(
        LOAD_SETTINGS_NOTIFIER,
        [this](std::vector<std::string> args)
        {
            if (args.size() < 2)
            {
                args.push_back(DEFAULT_SETTINGS_FILE);
            }

            this->LoadSettings(args[1]);
        },
        "Load plugin settings",
        PERMISSION_ALL
    );
}


/* HOOKS */

void EventSfx::InitializeHooks()
{
    // NotedBumps
    this->gameWrapper->HookEventWithCaller<CarWrapper>(
        "Function TAGame.Car_TA.IsBumperHit",
        [this](const CarWrapper& caller, void* params, std::string)
        {
            this->OnBump(caller, static_cast<BumpParams*>(params));
        });

    // Other stats
    this->gameWrapper->HookEventWithCallerPost<ServerWrapper>(
        "Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
        [this](ServerWrapper, void* params, std::string)
        {
            this->OnStatTickerMessage(static_cast<StatTickerParams*>(params));
        });

    // Crossbar hit
    this->gameWrapper->HookEventWithCaller<ActorWrapper>(
        "Function TAGame.GoalCrossbarVolumeManager_TA.TriggerHit",
        [this](auto caller, void* params, ...)
        {
            this->OnCrossbarTrigger(static_cast<CrossbarParams*>(params));
        });

    // Recommended volume
    this->gameWrapper->HookEventWithCaller<CarWrapper>(
        "Function TAGame.SoundSettingsManager_TA.SetSoundVolume",
        [this](CarWrapper, void* params, std::string)
        {
            const auto gameplayVolume = static_cast<float*>(params);
            this->LastGameplayVolume  = *gameplayVolume;
        });

    this->gameWrapper->HookEventWithCaller<CarWrapper>(
        "Function TAGame.GFxData_Settings_TA.SetMasterVolume",
        [this](CarWrapper, void* params, std::string)
        {
            float* pMasterVolume   = static_cast<float*>(params) + 2;
            this->LastMasterVolume = *pMasterVolume;
        });

    // Camera info
    this->gameWrapper->HookEventPost(
        "Function TAGame.GFxData_Settings_TA.SetCameraDistance",
        [this](std::string)
        {
            this->CamInfo->Distance = this->gameWrapper->GetSettings().GetCameraSettings().Distance;
            DEBUGLOG("DISTANCE SET TO: {}", this->CamInfo->Distance);
        });
    this->gameWrapper->HookEventPost(
        "Function TAGame.GFxData_Settings_TA.SetCameraHeight",
        [this](std::string)
        {
            this->CamInfo->Height = this->gameWrapper->GetSettings().GetCameraSettings().Height;
            DEBUGLOG("HEIGHT SET TO: {}", this->CamInfo->Height);
        });
    this->gameWrapper->HookEventPost(
        "Function TAGame.GFxData_Settings_TA.SetCameraAngle",
        [this](std::string)
        {
            this->CamInfo->Pitch = this->gameWrapper->GetSettings().GetCameraSettings().Pitch;
            DEBUGLOG("PITCH SET TO: {}", this->CamInfo->Pitch);
        });

    // Hook sound tracking if enabled
    if (this->Settings->SoundTrackingEnabled)
    {
        this->HookSoundTracking();
    }
}

void EventSfx::DeinitializeHooks() const
{
    this->UnhookSoundTracking();
    this->gameWrapper->UnhookEventPost("Function TAGame.GFxData_Settings_TA.SetCameraAngle");
    this->gameWrapper->UnhookEventPost("Function TAGame.GFxData_Settings_TA.SetCameraHeight");
    this->gameWrapper->UnhookEventPost("Function TAGame.GFxData_Settings_TA.SetCameraDistance");
    this->gameWrapper->UnhookEvent("Function TAGame.GFxData_Settings_TA.SetMasterVolume");
    this->gameWrapper->UnhookEvent("Function TAGame.SoundSettingsManager_TA.SetSoundVolume");
    this->gameWrapper->UnhookEvent("Function TAGame.GFxHUD_TA.HandleStatTickerMessage");
    this->gameWrapper->UnhookEvent("Function TAGame.Car_TA.EventBumpedCar");
}


/* AUDIO */

bool EventSfx::InitializeAudio()
{
    // Get relevant settings
    auto&      outputId = this->Settings->OutputId;
    const auto volume   = this->Settings->Volume;

    // Initialize sound manager
    HRESULT hr = this->SoundManager.Initialize(outputId, volume);
    if (FAILED(hr))
    {
        LOG("FAILED TO INITIALIZE THE SOUND MANAGER. HRESULT: {}", hr);
        return false;
    }

    // Preload sounds
    this->SoundManager.PreloadSounds();

    return true;
}

void EventSfx::DeinitializeAudio()
{
    this->SoundManager.Unload();
}

void EventSfx::HookSoundTracking() const
{
    this->gameWrapper->HookEvent(
        "Function Engine.GameViewportClient.Tick",
        [this](std::string eventName)
        {
            this->SoundManager.Update3D();
        });
}

void EventSfx::UnhookSoundTracking() const
{
    this->gameWrapper->UnhookEvent("Function Engine.GameViewportClient.Tick");
}


/* EVENTS */

void EventSfx::OnBump(CarWrapper carWrapper, const BumpParams* bumpData)
{
    if (carWrapper.IsNull()) return;

    const bool isEnabled = this->Settings->Sounds[RlEvents::Kind::Bump].IsEnabled;
    if (!isEnabled) return;

    const auto now = std::chrono::steady_clock::now();

    for (auto& [carId, time] : this->NotedBumps)
    {
        if (now - time > this->BumpTimeout)
        {
            this->NotedBumps.erase(carId);
        }
    }

    const auto carId = carWrapper.memory_address;
    if (this->NotedBumps.contains(carId)) return;

    this->NotedBumps[carId] = now;

    this->PlayBumpSfx(bumpData->HitLocation);

    if (!bumpData->OtherCar) return;

    this->NotedBumps[bumpData->OtherCar] = now;

    /*
    // this->tmpBumpsDisabled = true;

    this->PlayBumpSFX(bumpData->HitLocation);

    gameWrapper->SetTimeout([this](GameWrapper* l){
            this->tmpBumpsDisabled = false;
        }, 0.1f);
        */
}

void EventSfx::OnStatTickerMessage(const StatTickerParams* statParams)
{
    auto receiver  = PriWrapper(statParams->Receiver);
    auto victim    = PriWrapper(statParams->Victim);
    auto statEvent = StatEventWrapper(statParams->StatEvent);

    // Check if there is a receiver
    if (!receiver) return;

    // Find the primary player's PRI
    PlayerControllerWrapper playerController = this->gameWrapper->GetPlayerController();

    if (!playerController)
    {
        DEBUGLOG("NULL CONTROLLER.");
        return;
    }

    PriWrapper playerPri = playerController.GetPRI();

    if (!playerPri)
    {
        DEBUGLOG("NULL PLAYER PRI.");
        return;
    }

    // Check what is enabled
    const bool demosEnabled       = this->Settings->Sounds[RlEvents::Kind::Demo].IsEnabled;
    const bool playerGoalsEnabled = this->Settings->Sounds[RlEvents::Kind::PlayerGoal].IsEnabled;
    const bool teamGoalsEnabled   = this->Settings->Sounds[RlEvents::Kind::TeamGoal].IsEnabled;
    const bool concedesEnabled    = this->Settings->Sounds[RlEvents::Kind::Concede].IsEnabled;
    const bool savesEnabled       = this->Settings->Sounds[RlEvents::Kind::Save].IsEnabled;
    const bool winsEnabled        = this->Settings->Sounds[RlEvents::Kind::Win].IsEnabled;
    const bool lossesEnabled      = this->Settings->Sounds[RlEvents::Kind::Loss].IsEnabled;
    const bool assistsEnabled     = this->Settings->Sounds[RlEvents::Kind::Assist].IsEnabled;

    // Anyone
    if (statEvent.GetEventName() == "Demolish" && demosEnabled)
    {
        this->PlayDemoSfx(victim.GetCar().GetLocation());
        return;
    }

    // Player
    if (playerPri.memory_address == receiver.memory_address)
    {
        if (statEvent.GetEventName() == "Goal" && playerGoalsEnabled)
        {
            this->PlayPlayerGoalSfx();
            return;
        }
        if (statEvent.GetEventName() == "OwnGoal" && concedesEnabled)
        {
            this->PlayConcedeSfx();
            return;
        }
        if (statEvent.GetEventName() == "Save" && savesEnabled)
        {
            this->PlaySaveSfx();
            return;
        }
        if (statEvent.GetEventName() == "Win" && winsEnabled)
        {
            this->PlayWinSfx();
            return;
        }
        if (statEvent.GetEventName() == "Assist"
            && assistsEnabled && !teamGoalsEnabled)
        {
            this->PlayAssistSfx();
            return;
        }
    }

    // Player's team
    if (playerPri.GetTeam().GetTeamIndex() == receiver.GetTeam().GetTeamIndex())
    {
        if (statEvent.GetEventName() == "Goal" && teamGoalsEnabled)
        {
            this->PlayTeamGoalSfx();
        }
    }
    // Opponents' team
    else
    {
        if (statEvent.GetEventName() == "Goal" && concedesEnabled)
        {
            this->PlayConcedeSfx();
            return;
        }

        if (statEvent.GetEventName() == "Win" && lossesEnabled)
        {
            if (this->TmpLossesDisabled) return;

            this->TmpLossesDisabled = true;

            this->PlayLossSfx();

            // Time out so it won't play multiple times
            this->gameWrapper->SetTimeout([this](GameWrapper*)
            {
                this->TmpLossesDisabled = true;
            }, 1.0f);
        }
    }
}

void EventSfx::OnCrossbarTrigger(CrossbarParams* crossbarParams)
{
    const bool customEnabled = this->Settings->Sounds[RlEvents::Kind::Crossbar].IsEnabled;

    const Vector location = crossbarParams->HitLocation;

    // Is default pling enabled?
    if (!this->Settings->CrossbarPlingEnabled)
    {
        crossbarParams->HitLocation = { 0, 0, 0 };
    }

    if (!customEnabled) return;

    // Do we have a game state?
    auto state = this->gameWrapper->GetCurrentGameState();
    if (state.IsNull())
    {
        this->PlayCrossbarSfx(location);
        return;
    }

    // Do we have a ball?
    auto ball = state.GetBall();
    if (ball.IsNull())
    {
        this->PlayCrossbarSfx(location);
        return;
    }

    float multiplier;

    // Compute the multiplier
    if (this->Settings->FixedCrossbarVolume)
    {
        multiplier = 1.0;

        // LOG("MULTIPLIER: {}", multiplier);
    }
    else
    {
        float speed = ball.GetVelocity().magnitude();
        multiplier = Utils::ComputeVolumeMultiplier(speed);

        // LOG("SPEED: {} ({}kph), MULTIPLIER: {}", speed, (speed * 60 * 60) / 100 / 1000, multiplier);
    }

    const auto now = std::chrono::steady_clock::now();

    // Is the sound still timed out or too close in volume to the previous?
    if (now - this->LastPling < this->CrossbarTimeout && multiplier / this->lastMultiplier < 3.0f)
        return;

    // Note timestamp etc.
    this->LastPling = now;
    this->lastMultiplier = multiplier;

    // Play the sound
    this->PlayCrossbarSfx(location, multiplier);
}


/* AUDIO */

inline void EventSfx::PlaySoundFile(
    const std::string&                    soundId,
    const SoundInterface::PlaybackParams& params,
    const float                           volume)
{
    HRESULT hr = this->SoundManager.PlaySound(soundId, params, volume);
    if (FAILED(hr))
    {
        LOG("FAILED TO PLAY SOUND ({}). HRESULT: {}", soundId, hr);
    }
}

void EventSfx::PlayEventSound(
    const RlEvents::Kind                  eventId,
    const SoundInterface::PlaybackParams& params,
    float                                 volumeMultiplier)
{
    const SoundSettings& soundSettings = this->Settings->Sounds[static_cast<int>(eventId)];

    float volume = soundSettings.Volume * volumeMultiplier;

    if (constexpr float epsilon = 0.04f; soundSettings.Delay <= epsilon)
    {
        this->PlaySoundFile(soundSettings.SoundId, params, volume);
    }
    else
    {
        this->gameWrapper->SetTimeout(
            [this, soundSettings, params, volume](GameWrapper*)
            {
                this->PlaySoundFile(soundSettings.SoundId, params, volume);
            }, soundSettings.Delay);
    }
}

inline void EventSfx::PlayBumpSfx(const Vector& location, bool fromMenu)
{
    auto params = std::make_pair(location, fromMenu);
    this->PlayEventSound(RlEvents::Kind::Bump, params);
}

inline void EventSfx::PlayDemoSfx(const Vector& location, bool fromMenu)
{
    auto params = std::make_pair(location, fromMenu);
    this->PlayEventSound(RlEvents::Kind::Demo, params);
}

inline void EventSfx::PlayCrossbarSfx(const Vector& location, float multiplier, bool fromMenu)
{
    /*
    for (float testSpeed = 0 ; testSpeed <= 6000.0f ; testSpeed += 250.0f)
    {
        float testMult = computeMultiplier(testSpeed);
        LOG("SPEED {} -> {}", testSpeed, testMult);
    }

    LOG("MULTIPLIER: {}", multiplier);
    */

    auto params = std::make_pair(location, fromMenu);
    this->PlayEventSound(RlEvents::Kind::Crossbar, params, multiplier);
}

inline void EventSfx::PlayPlayerGoalSfx()
{
    this->PlayEventSound(RlEvents::Kind::PlayerGoal);
}

inline void EventSfx::PlayTeamGoalSfx()
{
    this->PlayEventSound(RlEvents::Kind::TeamGoal);
}

inline void EventSfx::PlayConcedeSfx()
{
    this->PlayEventSound(RlEvents::Kind::Concede);
}

inline void EventSfx::PlaySaveSfx()
{
    this->PlayEventSound(RlEvents::Kind::Save);
}

inline void EventSfx::PlayWinSfx()
{
    this->PlayEventSound(RlEvents::Kind::Win);
}

inline void EventSfx::PlayLossSfx()
{
    this->PlayEventSound(RlEvents::Kind::Loss);
}

inline void EventSfx::PlayAssistSfx()
{
    this->PlayEventSound(RlEvents::Kind::Assist);
}


/* SETTINGS */

void EventSfx::ApplySettings(const std::wstring& oldOutputId)
{
    HRESULT hr = this->SoundManager.SetVolume(this->Settings->Volume);
    if (FAILED(hr))
    {
        LOG("FAILED TO SET LOADED VOLUME. HRESULT: {}", hr);
    }

    const auto& newOutputId = this->Settings->OutputId;
    if (oldOutputId != newOutputId)
    {
        hr = this->SoundManager.SetOutputId(newOutputId);
        if (FAILED(hr))
        {
            LOG("FAILED TO SET LOADED OUTPUT ID. HRESULT: {}", hr);
        }
    }

    this->SoundManager.UnloadSounds();
    this->SoundManager.PreloadSounds();
}

void EventSfx::ApplyDefaultSettings()
{
    const std::wstring oldOutputId = this->Settings->OutputId;

    this->Settings->SetDefaults();
    this->ApplySettings(oldOutputId);
}

bool EventSfx::LoadSettings(const std::string& filename)
{
    const std::wstring oldOutputId = this->Settings->OutputId;

    if (!this->Settings->Load(filename))
    {
        return false;
    }

    this->ApplySettings(oldOutputId);

    return true;
}

inline bool EventSfx::SaveSettings(const std::string& filename) const
{
    return this->Settings->Save(filename);
}

/*
struct AGoalCrossbarVolumeManager_TA_execTriggerHit_Parameters
{
    void* Ball;
    void* Crossbar;
    struct Vector HitLoc;
    struct Vector HitNormal;
};


    gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GoalCrossbarVolumeManager_TA.TriggerHit",
                                                   [](auto caller, void* params, ...)
                                                   {
                                                       auto p = reinterpret_cast<
                                                           AGoalCrossbarVolumeManager_TA_execTriggerHit_Parameters*>(
                                                           params);
                                                       p->HitLoc = {0, 0, 0};
                                                   });
*/
