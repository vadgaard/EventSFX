//=======================================================================
/** SoundInterface.h
 * Adapted from the QuickVoice plugin by Sei4or
 */
//=======================================================================

#pragma once

#include "SoundInterface/SourceVoiceManager.h"
#include "AudioFile.h"

#define DEFAULT_OUTPUT_DEVICE_NAME     "Default"
#define DEFAULT_OUTPUT_DEVICE_ID       "default"
#define LDEFAULT_OUTPUT_DEVICE_NAME   L"Default"
#define LDEFAULT_OUTPUT_DEVICE_ID     L"default"

namespace SoundInterface
{
    struct AudioDevice
    {
        std::wstring Id;
        std::wstring Name;
    };

    static inline LPCWSTR GetSoundsFolder()
    {
        return (globalGameWrapper->GetDataFolder() / "EventSFX").c_str();
    }

    static inline LPCWSTR GetSoundFilePath(const std::string& file)
    {
        return (globalGameWrapper->GetDataFolder() / "EventSFX" / file).c_str();
    }

    using SoundFmt       = float;
    using AudioFile      = AudioFile<SoundFmt>;
    using AudioFilePtr   = std::shared_ptr<AudioFile>;
    using SoundMap       = std::unordered_map<std::string, AudioFilePtr>;
    using PlaybackParams = std::optional<std::pair<Vector, bool>>;

    class SoundManager
    {
        friend class SourceVoiceManager;

    public:
        SoundManager();
        ~SoundManager();

        HRESULT Initialize(std::wstring& outputId, float volume);

        HRESULT LoadSound(
            const std::string& soundId,
            bool               force = false);

        HRESULT PlaySound(
            const std::string&    soundId,
            const PlaybackParams& params = std::nullopt, // For 3D playback
            float                 volume = 1.0f);

        double GetSoundDuration(const std::string& soundId)
        {
            return this->LoadedSounds[soundId]->getLengthInSeconds();
        }

        void Update3D() const
        {
            this->VoiceManager.Update3D();
        }

        void        PreloadSounds();
        inline void UnloadSounds();

        void Unload();

        HRESULT SetVolume(float newVolume);

        static std::vector<std::string> ListSoundFiles();
        static std::vector<AudioDevice> EnumerateAudioDevices();
        std::vector<AudioDevice>        ConsolidateAudioDevices(
            AudioDevice& outDevice,
            bool         shouldSetOutput = false);

        HRESULT SetOutputId(const std::wstring& newId);

    private:
        std::wstring            OutputId = LDEFAULT_OUTPUT_DEVICE_ID;
        float                   Volume   = 1.0;
        SourceVoiceManager      VoiceManager;
        IXAudio2*               XAudio2     = nullptr;
        IXAudio2MasteringVoice* MasterVoice = nullptr;
        X3DAUDIO_HANDLE         X3DAudioHandle;
        SoundMap                LoadedSounds;
    };
}
