// ─────────────────────────────────────────────────────────────────────────────
// LowPassFilter.cpp  –  FIR low-pass filter with Hamming window
// ─────────────────────────────────────────────────────────────────────────────
#include "LowPassFilter.h"
#include <cmath>
#include <algorithm>

static constexpr double TWO_PI = 2.0 * M_PI;

LowPassFilter::LowPassFilter(double sample_rate, double cutoff_hz, double transition)
    : sampleRate_(sample_rate)
    , cutoffHz_(cutoff_hz)
    , transitionHz_(transition)
    , delayIdx_(0)
{
    buildTaps();
}

void LowPassFilter::setCutoff(double cutoff_hz)
{
    if (cutoffHz_ == cutoff_hz) return;
    cutoffHz_ = cutoff_hz;
    buildTaps();
}

// Kaiser-estimate for number of taps given transition band and Hamming window
void LowPassFilter::buildTaps()
{
    // Number of taps for Hamming window: N ≈ 8 / (Δf/fs)
    const double delta_f = transitionHz_ / sampleRate_;
    int numTaps = static_cast<int>(std::ceil(8.0 / delta_f));
    if (numTaps % 2 == 0) ++numTaps;   // force odd
    numTaps = std::min(numTaps, 1024); // cap

    const int M = numTaps - 1;
    const double fc = cutoffHz_ / sampleRate_; // normalised [0, 0.5]

    taps_.resize(numTaps);
    double sum = 0.0;
    for (int n = 0; n <= M; ++n) {
        double h;
        if (n == M / 2) {
            h = 2.0 * fc;
        } else {
            h = std::sin(TWO_PI * fc * (n - M / 2.0)) / (M_PI * (n - M / 2.0));
        }
        // Hamming window
        const double w = 0.54 - 0.46 * std::cos(TWO_PI * n / M);
        taps_[n] = static_cast<float>(h * w);
        sum += taps_[n];
    }
    // Normalise for unity pass-band gain
    for (auto& t : taps_) t /= static_cast<float>(sum);

    // Reset delay line
    delay_.assign(numTaps, {0.f, 0.f});
    delayIdx_ = 0;
}

std::complex<float> LowPassFilter::process(std::complex<float> sample)
{
    delay_[delayIdx_] = sample;
    delayIdx_ = (delayIdx_ + 1) % static_cast<int>(taps_.size());

    std::complex<float> acc{0.f, 0.f};
    int idx = delayIdx_;
    const int N = static_cast<int>(taps_.size());
    for (int k = 0; k < N; ++k) {
        idx = (idx == 0) ? N - 1 : idx - 1;
        acc += taps_[k] * delay_[idx];
    }
    return acc;
}

void LowPassFilter::process(std::complex<float>* samples, std::size_t count)
{
    for (std::size_t i = 0; i < count; ++i)
        samples[i] = process(samples[i]);
}
