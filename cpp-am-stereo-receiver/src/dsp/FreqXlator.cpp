// ─────────────────────────────────────────────────────────────────────────────
// FreqXlator.cpp
// ─────────────────────────────────────────────────────────────────────────────
#include "FreqXlator.h"
#include <cmath>

static constexpr double TWO_PI = 2.0 * M_PI;

FreqXlator::FreqXlator(double offset_hz, double sample_rate)
    : offsetHz_(offset_hz)
    , sampleRate_(sample_rate)
    , phase_(0.0)
{
    updateIncrement();
}

void FreqXlator::updateIncrement()
{
    phaseInc_ = TWO_PI * offsetHz_ / sampleRate_;
}

void FreqXlator::setOffset(double offset_hz)
{
    offsetHz_ = offset_hz;
    updateIncrement();
}

void FreqXlator::setSampleRate(double sample_rate)
{
    sampleRate_ = sample_rate;
    updateIncrement();
}

void FreqXlator::process(std::complex<float>* samples, std::size_t count)
{
    for (std::size_t i = 0; i < count; ++i) {
        const float cos_p = static_cast<float>(std::cos(phase_));
        const float sin_p = static_cast<float>(std::sin(phase_));
        samples[i] *= std::complex<float>(cos_p, sin_p);

        phase_ += phaseInc_;
        // Keep phase in [-π, π] to avoid floating-point drift
        if (phase_ > M_PI)       phase_ -= TWO_PI;
        else if (phase_ < -M_PI) phase_ += TWO_PI;
    }
}
