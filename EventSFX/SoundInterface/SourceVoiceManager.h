//=======================================================================
/** SourceVoiceManager.h
 * Adapted from the QuickVoice plugin by Sei4or
 */
//=======================================================================

#pragma once

#include <xaudio2.h>
// #include <XAudio2fx.h>
// #include <xapofx.h>
#include <x3daudio.h>
#pragma comment(lib, "XAUDIO2_8.lib")

#define XAUDIO2_NUM_SRC_CHANNELS 1

using X3DAUDIO_ROTATION = std::pair<X3DAUDIO_VECTOR, X3DAUDIO_VECTOR>;
using X3DAUDIO_VEC_ROT  = std::pair<X3DAUDIO_VECTOR, X3DAUDIO_ROTATION>;

namespace SoundInterface
{
    struct OutputMatrix
    {
        unsigned int Size;
        FLOAT32*     Data;
    };

    struct AudioFormatKey
    {
        WORD  Channels;
        DWORD SamplesPerSec;
        WORD  BitsPerSample;

        bool operator<(const AudioFormatKey& other) const
        {
            return std::tie(Channels, SamplesPerSec, BitsPerSample) <
                std::tie(other.Channels, other.SamplesPerSec, other.BitsPerSample);
        }
    };

    class SoundManager;
    class SourceVoiceCallback;

    using VoiceIndex   = short unsigned int;
    using VoiceVec     = std::vector<IXAudio2SourceVoice*>;
    using ReadyQue     = std::deque<VoiceIndex>;
    using ReadyQuePtr  = std::shared_ptr<ReadyQue>;
    using FmtQueMap    = std::map<AudioFormatKey, std::shared_ptr<ReadyQue>>;
    using ActiveMap    = std::map<VoiceIndex, X3DAUDIO_VECTOR>;
    using MatrixVec    = std::vector<OutputMatrix>;
    using ActiveMapPtr = std::shared_ptr<ActiveMap>;
    using CallbackVec  = std::vector<SourceVoiceCallback*>;
    // using effect_vec     = std::vector<IUnknown*>;

    class SourceVoiceManager
    {
    public:
        explicit SourceVoiceManager(SoundManager& soundManager);

        VoiceIndex GetReadySourceVoiceIndex(const WAVEFORMATEX* wfx);

        // For 3D playback
        IXAudio2SourceVoice* GetReadySourceVoice(
            const WAVEFORMATEX* wfx,
            const Vector&       location,
            bool                fromMenu = false);

        // For 2D playback
        IXAudio2SourceVoice* GetReadySourceVoice(
            const WAVEFORMATEX* pWfx)
        {
            const auto readyIndex   = this->GetReadySourceVoiceIndex(pWfx);
            const auto pSourceVoice = this->SourceVoices[readyIndex];
            // ReSharper disable once CppExpressionWithoutSideEffects
            this->ResetOutputMatrix(readyIndex);
            return pSourceVoice;
        }

        HRESULT ResetOutputMatrix(VoiceIndex sourceVoiceIndex) const;

        static X3DAUDIO_VEC_ROT GetListenerInfo(bool isStationary = false);
        HRESULT                 Apply3D(
            IXAudio2SourceVoice*    sourceVoice,
            const OutputMatrix&     outputMatrix,
            const X3DAUDIO_VECTOR&  emitterLocation,
            const X3DAUDIO_VEC_ROT& listenerInfo) const;
        void Update3D() const;
        void Unload();

    private:
        SoundManager& Manager; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        VoiceVec      SourceVoices;
        ActiveMapPtr  ActiveIndices;
        FmtQueMap     ReadyIndexQues;
        CallbackVec   SourceVoiceCallbacks;
        MatrixVec     OutputMatrices;
        // effect_vec     voiceEffects;
    };
}
