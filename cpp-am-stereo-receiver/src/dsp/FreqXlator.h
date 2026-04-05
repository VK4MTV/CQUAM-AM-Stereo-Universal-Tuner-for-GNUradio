#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// FreqXlator.h  –  Complex frequency translation via phase-accumulator NCO
// ─────────────────────────────────────────────────────────────────────────────
#include <complex>
#include <cstddef>

class FreqXlator
{
public:
    // offset_hz: signed offset applied to input (negative shifts down)
    // sample_rate: input sample rate in Hz
    explicit FreqXlator(double offset_hz = -100'000.0, double sample_rate = 1'000'000.0);

    // Translate a block of complex samples in-place
    void process(std::complex<float>* samples, std::size_t count);

    void setOffset(double offset_hz);
    void setSampleRate(double sample_rate);

private:
    void updateIncrement();

    double              offsetHz_;
    double              sampleRate_;
    double              phaseInc_;    // radians per sample
    double              phase_;       // current phase accumulator
};
