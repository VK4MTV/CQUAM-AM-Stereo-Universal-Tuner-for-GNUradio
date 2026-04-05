// ─────────────────────────────────────────────────────────────────────────────
// RmsAgc.cpp
// ─────────────────────────────────────────────────────────────────────────────
#include "RmsAgc.h"

RmsAgc::RmsAgc(float alpha, float reference)
    : alpha_(alpha)
    , reference_(reference)
    , powerAvg_(reference * reference)  // initialise to target power
    , gain_(1.0f)
{
}

void RmsAgc::process(std::complex<float>* samples, std::size_t count)
{
    for (std::size_t i = 0; i < count; ++i) {
        const float re  = samples[i].real();
        const float im  = samples[i].imag();
        const float pwr = re * re + im * im;

        // Exponential moving average of instantaneous power
        powerAvg_ += alpha_ * (pwr - powerAvg_);

        // Gain = reference / RMS;  guard against division by zero
        const float rms = std::sqrt(powerAvg_ < 1e-20f ? 1e-20f : powerAvg_);
        gain_ = reference_ / rms;

        // Apply gain in-place without constructing a new complex object
        samples[i].real(re * gain_);
        samples[i].imag(im * gain_);
    }
}
