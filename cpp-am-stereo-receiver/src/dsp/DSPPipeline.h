#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// DSPPipeline.h  –  Orchestrates the full signal processing chain
//
// Signal path:
//   SDR IQ @ 1 MHz
//   → FreqXlator  (−100 kHz offset)
//   → ComplexResampler (1 MHz → 120 kHz,  interpolate=3, decimate=25)
//   → LowPassFilter (variable audio bandwidth)
//   → CQUAMDecoder  (120 kHz → stereo float L/R)
//   → FloatResampler × 2 (120 kHz → 48 kHz, interpolate=2, decimate=5)
//   → AudioOutput (48 kHz stereo)
// ─────────────────────────────────────────────────────────────────────────────
#include <complex>
#include <vector>
#include <functional>
#include <cstddef>

#include "FreqXlator.h"
#include "Resampler.h"
#include "LowPassFilter.h"
#include "CQUAMDecoder.h"

class DSPPipeline
{
public:
    // audioCallback is called with interleaved stereo float32 samples @ 48 kHz
    using AudioCallback = std::function<void(const float*, std::size_t frames)>;

    DSPPipeline();

    void setAudioCallback(AudioCallback cb) { audioCallback_ = std::move(cb); }

    // Feed raw IQ samples from the SDR (1 MHz, complex<float>)
    void processIQ(const std::complex<float>* samples, std::size_t count);

    // ── Parameter setters (thread-safe via CQUAMDecoder's own mutex) ─────────
    void setFrequency(double /*hz*/) {} // handled at SDR level; pipeline is offset-based
    void setAudioBandwidth(double hz);
    void setNotchFreq(double hz);
    void setMonoMode(bool mono);

    // ── Status ────────────────────────────────────────────────────────────────
    float lockLevel()       const { return decoder_.lockLevel(); }
    float pilotMag()        const { return decoder_.pilotMag(); }
    bool  pilotDetected()   const { return decoder_.pilotDetected(); }
    bool  carrierLocked()   const { return decoder_.carrierLocked(); }

private:
    static constexpr int    SDR_RATE    = 1'000'000;
    static constexpr int    IF_RATE     = 120'000;
    static constexpr int    AUDIO_RATE  = 48'000;

    // Resampler ratios (reduced fractions)
    // 1 000 000 / 120 000 = 25/3  →  interp=3, decim=25
    static constexpr int RESAMP1_INTERP = 3;
    static constexpr int RESAMP1_DECIM  = 25;
    // 120 000 / 48 000 = 5/2  →  interp=2, decim=5
    static constexpr int RESAMP2_INTERP = 2;
    static constexpr int RESAMP2_DECIM  = 5;

    FreqXlator      xlator_;         // -100 kHz
    ComplexResampler resampler1_;    // 1 MHz → 120 kHz
    LowPassFilter   lpf_;           // audio bandwidth shaping
    CQUAMDecoder    decoder_;       // C-QUAM stereo decode
    FloatResampler  resamplerL_;    // 120 kHz → 48 kHz (left)
    FloatResampler  resamplerR_;    // 120 kHz → 48 kHz (right)

    // Intermediate buffers
    std::vector<std::complex<float>> ifBuf_;
    std::vector<float> lBuf_, rBuf_;
    std::vector<float> lAudio_, rAudio_;
    std::vector<float> interleaved_;

    AudioCallback audioCallback_;
};
