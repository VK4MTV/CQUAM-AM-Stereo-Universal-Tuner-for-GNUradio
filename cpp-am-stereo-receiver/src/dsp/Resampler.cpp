// ─────────────────────────────────────────────────────────────────────────────
// Resampler.cpp  –  Rational polyphase resampler
//
// Polyphase decomposition of prototype FIR h[n] into L branches:
//   p_k[m] = h[k + m*L],  k = 0..L-1
//
// For each output sample m:
//   phase = (m * D) mod L         → which polyphase branch to apply
//   y[m]  = sum_k p[phase][k] * x[inputIdx - k]
//   After computing y[m]:
//     accumulated = phase + D
//     numNewInputs = accumulated / L
//     phase = accumulated % L
//     Consume numNewInputs samples from input into the circular delay line.
// ─────────────────────────────────────────────────────────────────────────────
#include "Resampler.h"
#include <cmath>
#include <algorithm>

static constexpr double TWO_PI = 2.0 * M_PI;

// Build a windowed-sinc prototype low-pass FIR.
// Cutoff: 0.5 / max(L, D) normalised [0, 0.5].
static std::vector<float> buildSincHamming(int L, int D, int& tapsPerPhase)
{
    const double cutoff = 0.5 / std::max(L, D);
    tapsPerPhase = 16;   // taps per polyphase branch
    const int totalTaps = tapsPerPhase * L;
    std::vector<float> h(totalTaps);
    const int M = totalTaps - 1;

    double sum = 0.0;
    for (int n = 0; n < totalTaps; ++n) {
        const double x = n - M / 2.0;
        double sinc;
        if (std::abs(x) < 1e-9)
            sinc = 2.0 * cutoff;
        else
            sinc = std::sin(TWO_PI * cutoff * x) / (M_PI * x);

        const double win = 0.54 - 0.46 * std::cos(TWO_PI * n / M);
        h[n] = static_cast<float>(sinc * win * L);   // compensate interp gain
        sum += h[n];
    }
    const float norm = static_cast<float>(sum / L);
    for (auto& v : h) v /= norm;
    return h;
}

// Split prototype into L polyphase branches.
static std::vector<std::vector<float>> polyphase(const std::vector<float>& proto, int L)
{
    const int tapsPerPhase = static_cast<int>(proto.size()) / L;
    std::vector<std::vector<float>> poly(L, std::vector<float>(tapsPerPhase, 0.f));
    for (int k = 0; k < L; ++k)
        for (int m = 0; m < tapsPerPhase; ++m)
            if (k + m * L < static_cast<int>(proto.size()))
                poly[k][m] = proto[k + m * L];
    return poly;
}

// ── ComplexResampler ──────────────────────────────────────────────────────────

ComplexResampler::ComplexResampler(int interpolation, int decimation)
    : interpolation_(interpolation), decimation_(decimation)
    , writeIdx_(0), phase_(0)
{
    buildPrototype();
}

void ComplexResampler::buildPrototype()
{
    int tapsPerPhase;
    proto_ = buildSincHamming(interpolation_, decimation_, tapsPerPhase);
    poly_  = polyphase(proto_, interpolation_);
    delay_.assign(tapsPerPhase, {0.f, 0.f});
    writeIdx_ = 0;
    phase_    = 0;
}

std::size_t ComplexResampler::process(const std::complex<float>* in, std::size_t inCount,
                                      std::complex<float>*        out, std::size_t outCapacity)
{
    const int L    = interpolation_;
    const int D    = decimation_;
    const int taps = static_cast<int>(poly_[0].size());
    const int dlen = static_cast<int>(delay_.size());

    std::size_t outIdx = 0;
    std::size_t inIdx  = 0;

    while (outIdx < outCapacity) {
        // Compute output using current polyphase branch.
        // delay_ is a circular buffer; (writeIdx_-1) is the most recent sample.
        // Multiply by 4 to guarantee positive modulo regardless of k (k < dlen).
        std::complex<float> acc{0.f, 0.f};
        const auto& branch = poly_[phase_];
        for (int k = 0; k < taps; ++k) {
            const int di = (writeIdx_ - 1 - k + dlen * 4) % dlen;
            acc += branch[k] * delay_[di];
        }
        out[outIdx++] = acc;

        // Advance phase by D; each full L steps consumes one new input sample.
        const int next = phase_ + D;
        const int numNewInputs = next / L;
        phase_ = next % L;

        // Feed new input samples into the delay line.
        for (int i = 0; i < numNewInputs; ++i) {
            if (inIdx >= inCount) return outIdx;
            delay_[writeIdx_] = in[inIdx++];
            writeIdx_ = (writeIdx_ + 1) % dlen;
        }
    }
    return outIdx;
}

// ── FloatResampler ────────────────────────────────────────────────────────────

FloatResampler::FloatResampler(int interpolation, int decimation)
    : interpolation_(interpolation), decimation_(decimation)
    , writeIdx_(0), phase_(0)
{
    buildPrototype();
}

void FloatResampler::buildPrototype()
{
    int tapsPerPhase;
    proto_ = buildSincHamming(interpolation_, decimation_, tapsPerPhase);
    poly_  = polyphase(proto_, interpolation_);
    delay_.assign(tapsPerPhase, 0.f);
    writeIdx_ = 0;
    phase_    = 0;
}

std::size_t FloatResampler::process(const float* in, std::size_t inCount,
                                    float*        out, std::size_t outCapacity)
{
    const int L    = interpolation_;
    const int D    = decimation_;
    const int taps = static_cast<int>(poly_[0].size());
    const int dlen = static_cast<int>(delay_.size());

    std::size_t outIdx = 0;
    std::size_t inIdx  = 0;

    while (outIdx < outCapacity) {
        float acc = 0.f;
        const auto& branch = poly_[phase_];
        for (int k = 0; k < taps; ++k) {
            // Multiply by 4 to guarantee positive modulo regardless of k (k < dlen).
            const int di = (writeIdx_ - 1 - k + dlen * 4) % dlen;
            acc += branch[k] * delay_[di];
        }
        out[outIdx++] = acc;

        const int next = phase_ + D;
        const int numNewInputs = next / L;
        phase_ = next % L;

        for (int i = 0; i < numNewInputs; ++i) {
            if (inIdx >= inCount) return outIdx;
            delay_[writeIdx_] = in[inIdx++];
            writeIdx_ = (writeIdx_ + 1) % dlen;
        }
    }
    return outIdx;
}
