#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// NoiseBlanker.h  –  Look-ahead interpolating impulse blanker
// Removes lightning, static, and ignition noise at the IF level
// before the CQUAM decoder for cleanest audio output
// ─────────────────────────────────────────────────────────────────────────────
#include <complex>
#include <cstddef>
#include <vector>

class NoiseBlanker
{
public:
    NoiseBlanker();

    void setEnabled(bool enabled);
    void setThresholdDb(float dB);              // dB above running envelope
    void setMaxBlankDuration(std::size_t samples); // safety limit

    // Process complex IF samples in-place
    void process(std::complex<float>* samples, std::size_t count);

    bool isEnabled() const { return enabled_; }

private:
    bool   enabled_            = true;
    float  thresholdDb_        = 12.0f;   // 12 dB above envelope = impulse
    std::size_t maxBlankDuration_ = 64;   // safety limit (~1.3 ms @ 48 kHz)

    // Running envelope (exponential moving average of magnitude)
    float  envelope_           = 0.0f;
    float  envelopeAlpha_      = 0.01f;   // slow adaptation

    // State for blanking
    bool   inBlank_            = false;
    std::size_t blankStartIdx_ = 0;
    std::complex<float> preBlankSample_{0.0f, 0.0f};

    // Hold-off to prevent immediate retrigger
    std::size_t holdOffCounter_ = 0;
    std::size_t holdOffSamples_ = 32;     // ~0.67 ms @ 48 kHz
};