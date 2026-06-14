// ─────────────────────────────────────────────────────────────────────────────
// NoiseBlanker.cpp  –  Look-ahead interpolating impulse blanker
// ─────────────────────────────────────────────────────────────────────────────
#include "NoiseBlanker.h"
#include <algorithm>
#include <cmath>

NoiseBlanker::NoiseBlanker() = default;

void NoiseBlanker::setEnabled(bool enabled)
{
    enabled_ = enabled;
    inBlank_ = false;
    holdOffCounter_ = 0;
}

void NoiseBlanker::setThresholdDb(float dB)
{
    thresholdDb_ = dB;
}

void NoiseBlanker::setMaxBlankDuration(std::size_t samples)
{
    maxBlankDuration_ = samples;
}

void NoiseBlanker::process(std::complex<float>* samples, std::size_t count)
{
    if (!enabled_ || count == 0)
        return;

    for (std::size_t i = 0; i < count; ++i) {
        const float mag = std::abs(samples[i]);

        // Update running envelope (slow attack, slow decay)
        if (mag > envelope_) {
            envelope_ = envelope_ * (1.0f - envelopeAlpha_ * 0.1f) + mag * envelopeAlpha_ * 0.1f;
        } else {
            envelope_ = envelope_ * (1.0f - envelopeAlpha_) + mag * envelopeAlpha_;
        }

        // Hold-off period after a blank
        if (holdOffCounter_ > 0) {
            --holdOffCounter_;
            continue;
        }

        const float threshold = envelope_ * std::pow(10.0f, thresholdDb_ / 20.0f);

        if (!inBlank_) {
            // Look for impulse start
            if (mag > threshold) {
                inBlank_ = true;
                blankStartIdx_ = i;
                preBlankSample_ = (i > 0) ? samples[i - 1] : samples[i];
            }
        } else {
            // Continue blanking until impulse ends or max duration reached
            const std::size_t blankLen = i - blankStartIdx_;

            if (mag <= threshold || blankLen >= maxBlankDuration_) {
                // End of impulse — interpolate across the region
                const std::complex<float> postBlankSample = samples[i];

                for (std::size_t j = blankStartIdx_; j < i; ++j) {
                    const float t = static_cast<float>(j - blankStartIdx_) / static_cast<float>(blankLen);
                    samples[j] = preBlankSample_ * (1.0f - t) + postBlankSample * t;
                }

                inBlank_ = false;
                holdOffCounter_ = holdOffSamples_;
            }
        }
    }

    // If still in blank at end of buffer, interpolate to last known good sample
    if (inBlank_) {
        const std::size_t blankLen = count - blankStartIdx_;
        if (blankLen > 0) {
            for (std::size_t j = blankStartIdx_; j < count; ++j) {
                const float t = static_cast<float>(j - blankStartIdx_) / static_cast<float>(blankLen);
                samples[j] = preBlankSample_ * (1.0f - t) + preBlankSample_ * t;
            }
        }
        inBlank_ = false;
        holdOffCounter_ = holdOffSamples_;
    }
}