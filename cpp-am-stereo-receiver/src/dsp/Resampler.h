#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Resampler.h  –  Rational polyphase resampler
// Supports complex<float> (SDR stage) and float (audio stage)
// ─────────────────────────────────────────────────────────────────────────────
#include <complex>
#include <vector>
#include <cstddef>

// ── Complex resampler (1 MHz → 120 kHz) ──────────────────────────────────────
class ComplexResampler
{
public:
    // interpolation / decimation must be co-prime (reduce beforehand)
    ComplexResampler(int interpolation, int decimation);

    // Process input block; returns number of output samples written
    std::size_t process(const std::complex<float>* in,  std::size_t inCount,
                        std::complex<float>*        out, std::size_t outCapacity);

private:
    void buildPrototype();

    int interpolation_;
    int decimation_;

    std::vector<float> proto_;             // prototype low-pass FIR
    std::vector<std::vector<float>> poly_; // polyphase branches [L][tapsPerPhase]

    std::vector<std::complex<float>> delay_; // circular delay line
    int writeIdx_;   // next write position in delay_
    int phase_;      // current polyphase branch index (0 .. interpolation_-1)
};

// ── Float (stereo) resampler (120 kHz → 48 kHz) ──────────────────────────────
class FloatResampler
{
public:
    FloatResampler(int interpolation, int decimation);

    // Process one channel; returns number of output samples written
    std::size_t process(const float* in,  std::size_t inCount,
                        float*        out, std::size_t outCapacity);

private:
    void buildPrototype();

    int interpolation_;
    int decimation_;

    std::vector<float> proto_;
    std::vector<std::vector<float>> poly_;

    std::vector<float> delay_;  // circular delay line
    int writeIdx_;
    int phase_;
};
