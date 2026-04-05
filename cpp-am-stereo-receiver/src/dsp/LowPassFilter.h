#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// LowPassFilter.h  –  FIR low-pass filter with Hamming window
// Processes complex<float> samples (complex filtering for the IF stage)
// ─────────────────────────────────────────────────────────────────────────────
#include <complex>
#include <vector>
#include <cstddef>

class LowPassFilter
{
public:
    // sample_rate: input sample rate (Hz)
    // cutoff_hz:   -3dB cutoff frequency (Hz)
    // transition:  transition bandwidth (Hz)  – default 2000 Hz
    LowPassFilter(double sample_rate = 120'000.0,
                  double cutoff_hz   = 10'000.0,
                  double transition  = 2'000.0);

    // Update the cutoff; rebuilds taps
    void setCutoff(double cutoff_hz);

    // Filter a single complex sample
    std::complex<float> process(std::complex<float> sample);

    // Filter a block of complex samples in-place
    void process(std::complex<float>* samples, std::size_t count);

    int numTaps() const { return static_cast<int>(taps_.size()); }

private:
    void buildTaps();

    double sampleRate_;
    double cutoffHz_;
    double transitionHz_;

    std::vector<float>              taps_;
    std::vector<std::complex<float>> delay_;  // circular delay line
    int                              delayIdx_;
};
