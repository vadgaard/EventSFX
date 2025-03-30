//=======================================================================
/** SoundInterface.cpp
 * Adapted from the QuickVoice plugin by Sei4or
 */
//=======================================================================

#include "pch.h"
#include "SoundInterface/SoundManager.h"

#include <xaudio2.h>

#include <wrl.h>
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>

// Converts a sound to mono for easier 3D playback
template <class T>
void ConvertAudioFileToMono (std::shared_ptr<AudioFile<T>> audioFile) {
    if (audioFile->isMono()) {
        return; // Already mono
    }

    int numChannels = audioFile->getNumChannels();
    int numSamplesPerChannel = audioFile->getNumSamplesPerChannel();

    // Create a new buffer for mono audio
    typename AudioFile<T>::AudioBuffer monoBuffer(1, std::vector<T>(numSamplesPerChannel, 0));

    // Mix down all channels to mono
    for (int i = 0; i < numSamplesPerChannel; i++) {
        T mixedSample = 0;
        for (int channel = 0; channel < numChannels; channel++) {
            mixedSample += audioFile->samples[channel][i];
        }
        monoBuffer[0][i] = mixedSample / numChannels; // Average the samples
    }

    // Set the new mono buffer
    audioFile->setAudioBuffer(monoBuffer);
    audioFile->setNumChannels(1); // Update the number of channels to mono
}

namespace SoundInterface
{
    SoundManager::SoundManager()
        : VoiceManager(*this)
    {}

    SoundManager::~SoundManager()
    {
        if (this->XAudio2)
        {
            this->XAudio2->Release();
            this->XAudio2 = nullptr;
        }
    }

    HRESULT SoundManager::Initialize(std::wstring& outputId, const float volume)
    {
        // Set output ID
        this->OutputId = outputId;

        // Initialize XAudio2
        HRESULT hr = XAudio2Create(&this->XAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr))
        {
            DEBUGLOG("FAILED TO CREATE XAUDIO2 ENGINE. HRESULT: {}", hr);
            return hr;
        }

        // Consolidate audio device
        AudioDevice device;
        this->ConsolidateAudioDevices(device);
        outputId = device.Id;

        const LPCWSTR deviceId = device.Id == LDEFAULT_OUTPUT_DEVICE_ID
                                     ? nullptr
                                     : device.Id.c_str();

        // Create the mastering voice
        hr = XAudio2->CreateMasteringVoice(
            &this->MasterVoice,
            XAUDIO2_DEFAULT_CHANNELS,
            XAUDIO2_DEFAULT_SAMPLERATE,
            0, deviceId, nullptr,
            AudioCategory_GameEffects);
        if (FAILED(hr))
        {
            DEBUGLOG("FAILED TO CREATE MASTERING VOICE. HRESULT: {}", hr);
            return hr;
        }

        // Set the volume
        hr = this->MasterVoice->SetVolume(volume);
        if (FAILED(hr))
        {
            DEBUGLOG("FAILED TO SET MASTER VOLUME. HRESULT: {}", hr);
        }

        // Initialize 3D Audio
        DWORD speakers;
        hr = this->MasterVoice->GetChannelMask(&speakers);
        if (FAILED(hr))
        {
            DEBUGLOG("FAILED TO GET CHANNEL MASK. HRESULT: {}", hr);
            return hr;
        }
        hr = X3DAudioInitialize(speakers, X3DAUDIO_SPEED_OF_SOUND, this->X3DAudioHandle);
        if (FAILED(hr))
        {
            DEBUGLOG("FAILED TO INITIALIZE X3DAUDIO. HRESULT: {}", hr);
            return hr;
        }

        return hr;
    }

    void SoundManager::Unload()
    {
        this->UnloadSounds();
        this->VoiceManager.Unload();
    }

    std::vector<std::string> SoundManager::ListSoundFiles()
    {
        std::vector<std::string> soundFiles;

        const std::filesystem::path soundsFolder = GetSoundsFolder();
        if (exists(soundsFolder) && is_directory(soundsFolder))
        {
            for (const auto& entry : std::filesystem::directory_iterator(soundsFolder))
            {
                if (entry.is_regular_file()
                    && (entry.path().extension() == ".wav"
                        || entry.path().extension() == ".WAV"
                        || entry.path().extension() == ".aiff"
                        || entry.path().extension() == ".AIFF"))
                {
                    soundFiles.push_back(entry.path().filename().string());
                }
            }
        }

        return soundFiles;
    }

    std::vector<AudioDevice> SoundManager::EnumerateAudioDevices()
    {
        std::vector<AudioDevice> devices;

        AudioDevice defaultDevice;
        defaultDevice.Id   = LDEFAULT_OUTPUT_DEVICE_ID;
        defaultDevice.Name = LDEFAULT_OUTPUT_DEVICE_NAME;
        devices.push_back(defaultDevice);

        Microsoft::WRL::ComPtr<IMMDeviceEnumerator> deviceEnumerator;
        HRESULT                                     hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            IID_PPV_ARGS(&deviceEnumerator));
        if (FAILED(hr))
        {
            DEBUGLOG("COULDN'T CREATE DEVICE ENUMERATOR. HRESULT: {}", hr);
            return devices;
        }

        Microsoft::WRL::ComPtr<IMMDeviceCollection> collection;
        hr = deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
        if (FAILED(hr))
        {
            DEBUGLOG("COULDN'T ENUMERATE AUDIO ENDPOINTS. HRESULT: {}", hr);
            return devices;
        }

        UINT count;
        hr = collection->GetCount(&count);
        if (FAILED(hr))
        {
            DEBUGLOG("FAILED TO GET COLLECTION COUNT. HRESULT: {}", hr);
            return devices;
        }

        for (UINT i = 0; i < count; ++i)
        {
            Microsoft::WRL::ComPtr<IMMDevice> endpoint;
            hr = collection->Item(i, &endpoint);
            if (FAILED(hr))
            {
                DEBUGLOG("FAILED TO GET COLLECTION ITEM. HRESULT: {}", hr);
                continue;
            }

            LPWSTR wstrID = nullptr;
            hr            = endpoint->GetId(&wstrID);
            if (FAILED(hr))
            {
                DEBUGLOG("FAILED TO GET DEVICE ID. HRESULT: {}", hr);
                continue;
            }
            std::wstring deviceId(wstrID);
            CoTaskMemFree(wstrID);

            Microsoft::WRL::ComPtr<IPropertyStore> props;
            hr = endpoint->OpenPropertyStore(STGM_READ, &props);
            if (FAILED(hr))
            {
                DEBUGLOG("FAILED TO OPEN PROPERTY STORE. HRESULT: {}", hr);
                continue;
            }

            PROPVARIANT varName;
            PropVariantInit(&varName);
            hr = props->GetValue(PKEY_Device_FriendlyName, &varName);
            if (FAILED(hr))
            {
                DEBUGLOG("FAILED TO GET PROP VALUE. HRESULT: {}", hr);
                continue;
            }

            AudioDevice device;
            device.Id   = deviceId;
            device.Name = varName.pwszVal;
            devices.push_back(device);

            hr = PropVariantClear(&varName);
            if (FAILED(hr))
            {
                DEBUGLOG("FAILED TO CLEAR PROP VARIANT. HRESULT: {}", hr);
            }
        }

        return devices;
    }

    std::vector<AudioDevice> SoundManager::ConsolidateAudioDevices(
        AudioDevice& outDevice,
        const bool   shouldSetOutput)
    {
        std::vector<AudioDevice> devices = this->EnumerateAudioDevices();

        const auto it = std::ranges::find_if(
            devices,
            [this](const AudioDevice& device)
            {
                return device.Id == this->OutputId;
            });

        std::wstring deviceId   = LDEFAULT_OUTPUT_DEVICE_ID;
        std::wstring deviceName = LDEFAULT_OUTPUT_DEVICE_NAME;

        if (it != devices.end())
        {
            deviceId   = it->Id;
            deviceName = it->Name;
        }
        else
        {
            this->OutputId = deviceId;

            if (shouldSetOutput)
            {
                DEBUGLOG("STORED OUTPUT DEVICE NOT FOUND. RESETTING..");
                HRESULT hr = this->SetOutputId(LDEFAULT_OUTPUT_DEVICE_ID);
                if (FAILED(hr))
                {
                    DEBUGLOG("FAILED TO SET OUTPUT DEVICE. HRESULT: {}", hr);
                }
            }
        }

        outDevice.Id   = deviceId;
        outDevice.Name = deviceName;

        return devices;
    }

    HRESULT SoundManager::SetOutputId(const std::wstring& newId)
    {
        this->OutputId = newId;

        // Destroy previous stuff, if any
        // this->Unload();
        this->VoiceManager.Unload();
        if (this->MasterVoice)
        {
            this->MasterVoice->DestroyVoice();
            this->MasterVoice = nullptr;
        }
        if (this->XAudio2)
        {
            this->XAudio2->Release();
            this->XAudio2 = nullptr;
        }

        // Re-initialize
        return this->Initialize(this->OutputId, this->Volume);
    }

    HRESULT SoundManager::LoadSound(
        const std::string& soundId,
        const bool         force)
    {
        constexpr HRESULT hr = S_OK;

        if (!force && this->LoadedSounds.contains(soundId))
        {
            return hr;
        }

        const std::wstring wFilePath = GetSoundFilePath(soundId);
        const std::string  filePath  = Utils::WStringToString(wFilePath);

        // Load audio file
        auto audioFile = std::make_shared<AudioFile>();
        if (!audioFile->load(filePath))
        {
            DEBUGLOG("FAILED TO LOAD SOUND: {}", soundId);
            return E_FAIL;
        }

        // Force to be mono
        ConvertAudioFileToMono(audioFile);

        // Store a pointer to the audio file
        this->LoadedSounds[soundId] = std::move(audioFile); // std::move(audioFile);

        return hr;
    }

    HRESULT SoundManager::PlaySound(
        const std::string&    soundId,
        const PlaybackParams& params,
        const float           volume)
    {
        HRESULT hr = this->LoadSound(soundId);
        if (FAILED(hr))
        {
            DEBUGLOG("FAILED TO LOAD SOUND: HRESULT: {}", hr);
            return hr;
        }

        const AudioFilePtr audioFile = this->LoadedSounds[soundId];

        WAVEFORMATEX wfx    = {};
        wfx.wFormatTag      = WAVE_FORMAT_IEEE_FLOAT;
        wfx.nChannels       = audioFile->getNumChannels();
        wfx.nSamplesPerSec  = audioFile->getSampleRate();
        wfx.wBitsPerSample  = sizeof(float) * 8;
        wfx.nBlockAlign     = (wfx.nChannels * wfx.wBitsPerSample) / 8;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

        XAUDIO2_BUFFER buffer = {};
        buffer.AudioBytes     = audioFile->getNumSamplesPerChannel() * wfx.nBlockAlign;
        buffer.pAudioData     = reinterpret_cast<const BYTE*>(audioFile->samples[0].data());
        buffer.Flags          = XAUDIO2_END_OF_STREAM;

        IXAudio2SourceVoice* sourceVoice;
        try
        {
            if (params.has_value())
            {
                auto& location = params.value().first;
                auto& fromMenu = params.value().second;

                sourceVoice    = VoiceManager.GetReadySourceVoice(&wfx, location, fromMenu);
            }
            else
            {
                sourceVoice = VoiceManager.GetReadySourceVoice(&wfx);
            }
        }
        catch (HRESULT thrownHr)
        {
            DEBUGLOG("FAILED TO GET READY SOURCE VOICE. HRESULT: {}", thrownHr);
            return thrownHr;
        }

        // Set the volume of source voice
        hr = sourceVoice->SetVolume(2 * volume);

        if (FAILED(hr))
        {
            DEBUGLOG("FAILED TO SET SOURCE VOICE VOLUME. HRESULT: {}", hr);
        }

        // Submit the buffer
        hr = sourceVoice->SubmitSourceBuffer(&buffer);
        if (FAILED(hr))
        {
            DEBUGLOG("FAILED TO SUBMIT SOURCE BUFFER. HRESULT: {}", hr);
            return hr;
        }

        // Play the sound
        hr = sourceVoice->Start(0);
        if (FAILED(hr))
        {
            DEBUGLOG("FAILED TO START SOURCE VOICE. HRESULT: {}", hr);
            return hr;
        }

        return hr;
    }

    void SoundManager::PreloadSounds()
    {
        // Unload previous sounds
        this->UnloadSounds();

        // Load all current sounds
        for (auto& sound : globalPluginSettings->Sounds)
        {
            const std::string& soundId = sound.SoundId;

            HRESULT hr = this->LoadSound(soundId, true);
            if (FAILED(hr))
            {
                DEBUGLOG("FAILED TO PRELOAD SOUND ({}). HRESULT: {}", soundId, hr);
            }
        }
    }

    HRESULT SoundManager::SetVolume(const float newVolume)
    {
        HRESULT hr = this->MasterVoice->SetVolume(newVolume);
        if (FAILED(hr))
        {
            DEBUGLOG("COULDN'T SET THE VOLUME. {}", hr);
        }
        else
        {
            this->Volume = newVolume;
        }

        return hr;
    }

    inline void SoundManager::UnloadSounds()
    {
        this->LoadedSounds.clear();
    }
}
