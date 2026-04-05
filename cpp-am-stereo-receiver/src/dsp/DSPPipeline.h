#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// DSPPipeline.h  –  Orchestrates the full signal processing chain
//
// Signal path:
//   SDR IQ @ 1 MHz
//   → FreqXlator  (−100 kHz offset)
//   → ComplexResampler (1 MHz → 96 kHz,  interpolate=12, decimate=125)
//   → RmsAgc (alpha=1e-4, reference=0.2)  ← normalises IF envelope
//   → LowPassFilter (variable audio bandwidth)
//   → CQUAMDecoder  (96 kHz → stereo float L/R)
//   → FloatResampler × 2 (96 kHz → 48 kHz, interpolate=1, decimate=2)
//   → AudioOutput (48 kHz stereo)
// ─────────────────────────────────────────────────────────────────────────────
#include <complex>
#include <vector>
#include <functional>
#include <cstddef>

#include "FreqXlator.h"
#include "Resampler.h"
#include "RmsAgc.h"
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
    static constexpr int    IF_RATE     = 96'000;
    static constexpr int    AUDIO_RATE  = 48'000;

    // Resampler ratios (reduced fractions)
    // 1 000 000 / 96 000  →  GCD=8000  →  interp=12, decim=125
    static constexpr int RESAMP1_INTERP = 12;
    static constexpr int RESAMP1_DECIM  = 125;
    // 96 000 / 48 000 = 2/1  →  interp=1, decim=2
    static constexpr int RESAMP2_INTERP = 1;
    static constexpr int RESAMP2_DECIM  = 2;

    FreqXlator      xlator_;         // -100 kHz
    ComplexResampler resampler1_;    // 1 MHz → 96 kHz
    RmsAgc          agc_;           // IF envelope normaliser (alpha=1e-4, ref=0.2)
    LowPassFilter   lpf_;           // audio bandwidth shaping
    CQUAMDecoder    decoder_;       // C-QUAM stereo decode
    FloatResampler  resamplerL_;    // 96 kHz → 48 kHz (left)
    FloatResampler  resamplerR_;    // 96 kHz → 48 kHz (right)

    // Intermediate buffers
    std::vector<std::complex<float>> ifBuf_;
    std::vector<float> lBuf_, rBuf_;
    std::vector<float> lAudio_, rAudio_;
    std::vector<float> interleaved_;

    AudioCallback audioCallback_;
};
