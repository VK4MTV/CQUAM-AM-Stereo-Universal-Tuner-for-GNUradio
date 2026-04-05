#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// RmsAgc.h  –  RMS Automatic Gain Control for complex IF signals
//
// Tracks the RMS envelope of the input using an exponential moving average and
// applies a gain to keep the RMS near a target reference level.  Matches the
// behaviour of satellites.hier.rms_agc (alpha=1e-4, reference=0.2) used in
// the Python flowgraph before the C-QUAM decoder.
// ─────────────────────────────────────────────────────────────────────────────
#include <complex>
#include <cstddef>
#include <cmath>

class RmsAgc
{
public:
    // alpha:     smoothing factor for the RMS estimator (e.g. 1e-4)
    // reference: target RMS output level (e.g. 0.2)
    explicit RmsAgc(float alpha = 1e-4f, float reference = 0.2f);

    // Process a block of complex samples in-place
    void process(std::complex<float>* samples, std::size_t count);

    void setAlpha(float alpha)         { alpha_ = alpha; }
    void setReference(float reference) { reference_ = reference; }

    float currentGain() const { return gain_; }

private:
    float alpha_;
    float reference_;
    float powerAvg_;   // exponential moving average of |x|^2
    float gain_;       // current applied gain
};
