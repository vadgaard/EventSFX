//=======================================================================
/** SourceVoiceManager.cpp
 * Adapted from the QuickVoice plugin by Sei4or
 */
//=======================================================================

#include "pch.h"
#include "SoundInterface/SourceVoiceManager.h"

#include <ranges>

#include "SoundInterface/SoundManager.h"

namespace
{
    /*
     * Converts a Vector to an X3DAudioVector
     *
     * Vector (Input):
     * - X increases away from screen
     * - Y increases to the right
     * - Z increases upwards
     *
     * X3DAudioVector (Output):
     * - X increases to the right
     * - Y increases upwards
     * - Z increases towards the screen
     */

    X3DAUDIO_VECTOR
    VectorToX3DAudioVector(const Vector& vector, const bool normalize = false)
    {
        auto result = Vector(-vector.X, vector.Z, vector.Y);

        if (normalize)
        {
            result.normalize();
        }
        else
        {
            result /= 100.0f;
        }

        return {result.X, result.Y, result.Z};
    }

    X3DAUDIO_ROTATION
    RotatorToX3DAudioRotation(const Rotator& rotation)
    {
        // Calculate the forward vector based on yaw and pitch
        const Vector gameFront      = RotatorToVector(rotation);
        const Quat   gameQuaternion = RotatorToQuat(rotation);

        // Compute the up vector based on roll
        const auto   worldUp = Vector(0, 0, 1);
        const Vector gameUp  = RotateVectorWithQuat(worldUp, gameQuaternion);

        // Convert to X3DAudioVectors
        X3DAUDIO_VECTOR forward = VectorToX3DAudioVector(gameFront, true);
        X3DAUDIO_VECTOR up      = VectorToX3DAudioVector(gameUp, true);

        return std::make_pair(forward, up);
    }
}

namespace SoundInterface
{
    class SourceVoiceCallback final
        : public IXAudio2VoiceCallback
    {
    public:
        SourceVoiceCallback(
            const VoiceIndex    index,
            const ReadyQuePtr&  readyIndices,
            const ActiveMapPtr& activeIndices)
            : Index(index),
              ReadyIndices(readyIndices),
              ActiveIndices(activeIndices)
        {}

        void OnStreamEnd() override
        {
            this->ReadyIndices->push_back(this->Index);
            this->ActiveIndices->erase(this->Index);
        }

        // Inherited via IXAudio2VoiceCallback
        void OnVoiceProcessingPassStart(UINT32 bytesRequired) override
        {}

        void OnVoiceProcessingPassEnd() override
        {}

        void OnBufferStart(void* pBufferContext) override
        {}

        void OnBufferEnd(void* pBufferContext) override
        {}

        void OnLoopEnd(void* pBufferContext) override
        {}

        void OnVoiceError(void* pBufferContext, HRESULT error) override
        {}

    private:
        VoiceIndex   Index;
        ReadyQuePtr  ReadyIndices;
        ActiveMapPtr ActiveIndices;
    };

    SourceVoiceManager::SourceVoiceManager(SoundManager& soundManager)
        : Manager(soundManager),
          ActiveIndices(std::make_shared<ActiveMap>())
    {}

    VoiceIndex SourceVoiceManager::GetReadySourceVoiceIndex(
        const WAVEFORMATEX* wfx)
    {
        HRESULT hr = S_OK;

        // Construct the key from the provided WAVEFORMATEX
        const AudioFormatKey key = {
            wfx->nChannels,
            wfx->nSamplesPerSec,
            wfx->wBitsPerSample
        };

        // Check if there's already a deque for this format, if not, create one
        if (!this->ReadyIndexQues.contains(key))
        {
            this->ReadyIndexQues[key] = std::make_shared<ReadyQue>();
        }

        VoiceIndex sourceVoiceIndex;

        // Check the deque to see if there's a voice ready to be reused
        if (const auto& readyIndices = this->ReadyIndexQues[key]; readyIndices->empty())
        {
            // Need to create a new source voice
            sourceVoiceIndex = static_cast<VoiceIndex>(this->SourceVoices.size());

            // Define the callback
            auto callback =
                new SourceVoiceCallback(
                    sourceVoiceIndex,
                    readyIndices,
                    this->ActiveIndices);

            /*
            // Setup reverb effect
            XAUDIO2_EFFECT_CHAIN effects;
            IUnknown* pReverbEffect = nullptr;

            // Create the reverb effect
            HRESULT hr = XAudio2CreateReverb(&pReverbEffect, 0);
            if (FAILED(hr)) {
                DEBUGLOG("COULD NOT CREATE REVERB EFFECT. HRESULT: {}", hr);
                return hr;
            }

            // Prepare the effect descriptor
            XAUDIO2_EFFECT_DESCRIPTOR effectDesc;
            effectDesc.InitialState = true;
            effectDesc.OutputChannels = wfx->Channels;  // Match the channel layout of the source
            effectDesc.pEffect = pReverbEffect;

            // Prepare the effect chain
            XAUDIO2_EFFECT_CHAIN effectChain;
            effectChain.EffectCount = 1;
            effectChain.pEffectDescriptors = &effectDesc;
            */

            // Create the new voice
            IXAudio2SourceVoice* sourceVoice;
            hr = this->Manager.XAudio2->CreateSourceVoice(
                &sourceVoice, wfx, 0,
                XAUDIO2_DEFAULT_FREQ_RATIO,
                callback); // , nullptr, & effectChain);

            if (FAILED(hr))
            {
                DEBUGLOG("FAILED TO CREATE SOURCE VOICE. HRESULT: {}", hr);
                // pReverbEffect->Release();
                delete callback;
                throw hr;
            }

            // Define new output matrix
            XAUDIO2_VOICE_DETAILS details;
            this->Manager.MasterVoice->GetVoiceDetails(&details);

            OutputMatrix outputMatrix;
            outputMatrix.Size = details.InputChannels * XAUDIO2_NUM_SRC_CHANNELS;
            outputMatrix.Data = new FLOAT32[outputMatrix.Size];

            // Store the new source voice and callback
            this->SourceVoiceCallbacks.emplace_back(callback);
            this->SourceVoices.push_back(sourceVoice);
            this->OutputMatrices.push_back(outputMatrix);
            // this->voiceEffects.push_back(pReverbEffect);
        }
        else
        {
            // Reuse an existing source voice
            sourceVoiceIndex = readyIndices->front();
            readyIndices->pop_front();
        }

        return sourceVoiceIndex;
    }

    IXAudio2SourceVoice* SourceVoiceManager::GetReadySourceVoice(
        const WAVEFORMATEX* wfx,
        const Vector&       location,
        const bool          fromMenu)
    {
        const auto  readyIndex   = this->GetReadySourceVoiceIndex(wfx);
        const auto  sourceVoice  = this->SourceVoices[readyIndex];
        const auto& outputMatrix = this->OutputMatrices[readyIndex];

        // Set initial 3D stuff?
        const auto emitterLocation = VectorToX3DAudioVector(location);
        const auto listenerInfo    = GetListenerInfo(fromMenu);

        HRESULT hr = this->Apply3D(sourceVoice, outputMatrix, emitterLocation, listenerInfo);
        if (FAILED(hr))
        {
            DEBUGLOG("COULD NOT APPLY 3D. HRESULT: {}", hr);
        }

        if (!fromMenu)
        {
            (*this->ActiveIndices)[readyIndex] = emitterLocation;
        }

#if DEBUG_LOG
        voice_index readies = 0;
        for (const auto& entry : this->readyIndexQues) {
            const AudioFormatKey& key = entry.first;
            const std::shared_ptr<std::deque<voice_index>>& readyIndices = entry.second;

            DEBUGLOG("{} {} {} <- Audio format key", key.Channels, key.SamplesPerSec, key.BitsPerSample);
            DEBUGLOG("{} <- Num ready source voices at key", readyIndices->size());
            readies += readyIndices->size();
        }
        DEBUGLOG("\n"
            "{} <- Num ready  source voices\n"
            "{} <- Num source voice indices\n"
            "{} <- Num active source voice indices\n"
            "{} <- Num callbacks\n"
            "{} <- Num output matrices\n",
            readies,
            this->sourceVoices.size(),
            this->ActiveIndices->size(),
            this->sourceVoiceCallbacks.size(),
            this->outputMatrices.size());
#endif

        return sourceVoice;
    }

    HRESULT SourceVoiceManager::ResetOutputMatrix(const VoiceIndex sourceVoiceIndex) const
    {
        const auto  sourceVoice  = this->SourceVoices[sourceVoiceIndex];
        const auto& [size, data] = this->OutputMatrices[sourceVoiceIndex];

        for (int i = 0; i < size; i++)
        {
            data[i] = 1.1f / size;
        }

        HRESULT hr = sourceVoice->SetOutputMatrix(
            this->Manager.MasterVoice,
            XAUDIO2_NUM_SRC_CHANNELS,
            size,
            data
        );

        if (FAILED(hr))
        {
            DEBUGLOG("FAILED TO RESET OUTPUT MATRIX. HRESULT: {}", hr);
        }

        return hr;
    }

    X3DAUDIO_VEC_ROT SourceVoiceManager::GetListenerInfo(const bool isStationary)
    {
        Vector  listenerLocation = {0, 0, globalCameraInfo->Height};
        Rotator listenerRotation = {
            static_cast<int>(globalCameraInfo->Pitch + 0.5f),
            RlAngle90, 0
        };

        auto camera = globalGameWrapper->GetCamera();
        if (!isStationary && !camera.IsNull() &&
            (globalGameWrapper->IsInGame() ||
                globalGameWrapper->IsInOnlineGame()))
        {
            listenerLocation = camera.GetLocation();
            listenerRotation = camera.GetCameraRotation();
        }

        X3DAUDIO_VECTOR   listenerLocationX3D = VectorToX3DAudioVector(listenerLocation);
        X3DAUDIO_ROTATION listenerRotationX3D = RotatorToX3DAudioRotation(listenerRotation);
        return std::make_pair(listenerLocationX3D, listenerRotationX3D);
    }

    HRESULT SourceVoiceManager::Apply3D(
        IXAudio2SourceVoice*    sourceVoice,
        const OutputMatrix&     outputMatrix,
        const X3DAUDIO_VECTOR&  emitterLocation,
        const X3DAUDIO_VEC_ROT& listenerInfo) const
    {
        X3DAUDIO_EMITTER emitter    = {};
        emitter.Position            = emitterLocation;
        emitter.ChannelCount        = XAUDIO2_NUM_SRC_CHANNELS;
        emitter.CurveDistanceScaler = 4.0f;

        X3DAUDIO_LISTENER listener = {};
        listener.Position          = listenerInfo.first;
        auto [front, top]          = listenerInfo.second;
        listener.OrientFront       = front;
        listener.OrientTop         = top;

#if DEBUG_LOG
        DEBUGLOG("========FROM SOURCEVOICEMANAGER=========");
        DEBUGLOG("X3D EMITTER__POSITION: {}, {}, {}", emitter.Position.x, emitter.Position.y, emitter.Position.z);
        DEBUGLOG("X3D LISTENER POSITION: {}, {}, {}", listener.Position.x, listener.Position.y, listener.Position.z);
        DEBUGLOG("X3D LISTENER_FRONT___: {}, {}, {}", listener.OrientFront.x, listener.OrientFront.y,
                 listener.OrientFront.z);
        DEBUGLOG("X3D LISTENER_TOP_____: {}, {}, {}", listener.OrientTop.x, listener.OrientTop.y, listener.OrientTop.z);
#endif

        X3DAUDIO_DSP_SETTINGS dspSettings = {nullptr};
        dspSettings.SrcChannelCount       = XAUDIO2_NUM_SRC_CHANNELS;
        dspSettings.DstChannelCount       = outputMatrix.Size;
        dspSettings.pMatrixCoefficients   = outputMatrix.Data;

        X3DAudioCalculate(
            this->Manager.X3DAudioHandle,
            &listener, &emitter,
            X3DAUDIO_CALCULATE_MATRIX,
            &dspSettings
        );

        HRESULT hr = sourceVoice->SetOutputMatrix(
            this->Manager.MasterVoice,
            dspSettings.SrcChannelCount,
            dspSettings.DstChannelCount,
            dspSettings.pMatrixCoefficients
        );

        if (FAILED(hr))
        {
            DEBUGLOG("FAILED TO SET OUTPUT MATRIX. HRESULT: {}", hr);
        }

        return hr;
    }

    void SourceVoiceManager::Update3D() const
    {
        if (this->ActiveIndices->empty()) return;

        const X3DAUDIO_VEC_ROT listenerInfo = GetListenerInfo();

        HRESULT hr = S_OK;
        for (const auto& [index, location] : *this->ActiveIndices)
        {
            IXAudio2SourceVoice* sourceVoice  = this->SourceVoices[index];
            OutputMatrix         outputMatrix = this->OutputMatrices[index];

            hr = this->Apply3D(sourceVoice, outputMatrix, location, listenerInfo);
            if (FAILED(hr))
            {
                DEBUGLOG("FAILED TO APPLY 3D. HRESULT: {}", hr);
            }
        }
    }

    void SourceVoiceManager::Unload()
    {
        this->ActiveIndices->clear();

        for (const auto& val : this->ReadyIndexQues | std::views::values)
        {
            val->clear();
        }
        this->ReadyIndexQues.clear();

        for (auto& voice : this->SourceVoices)
        {
            if (voice != nullptr)
            {
                HRESULT hr = voice->Stop();
                if (FAILED(hr))
                {
                    DEBUGLOG("FAILED TO STOP VOICE. HRESULT: {}", hr);
                }
                hr = voice->FlushSourceBuffers();
                if (FAILED(hr))
                {
                    DEBUGLOG("FAILED TO FLUSH SOURCE VOICE BUFFERS. HRESULT: {}", hr);
                }
                voice->DestroyVoice();
                voice = nullptr;
            }
        }
        this->SourceVoices.clear();

        for (const auto& [_, data] : this->OutputMatrices)
        {
            delete[] data;
        }
        this->OutputMatrices.clear();

        for (auto& callback : this->SourceVoiceCallbacks)
        {
            delete callback;
            callback = nullptr;
        }
        this->SourceVoiceCallbacks.clear();

        /*
        for (auto& effect : this->voiceEffects) {
            effect->Release();
            effect = nullptr;
        }
        this->voiceEffects.clear();
        */
    }
}
