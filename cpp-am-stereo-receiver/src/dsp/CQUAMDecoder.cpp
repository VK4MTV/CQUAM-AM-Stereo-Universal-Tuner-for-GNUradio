// ─────────────────────────────────────────────────────────────────────────────
// CQUAMDecoder.cpp  –  C++ port of AMradio_epy_block_5.py
// ─────────────────────────────────────────────────────────────────────────────
#include "CQUAMDecoder.h"
#include <cmath>
#include <algorithm>
#include <mutex>

// SciPy iirnotch equivalent:
//   b, a = iirnotch(f0, Q, fs)
//
// Transfer function:
//   b = [1, -2cos(w0), 1] * (1 + w0/Q)/2   ... simplified
//
// The exact SciPy formula:
//   w0    = 2*pi*f0/fs
//   alpha = sin(w0) / (2*Q)
//   b0    = 1
//   b1    = -2*cos(w0)
//   b2    = 1
//   a0    = 1 + alpha
//   a1    = -2*cos(w0)
//   a2    = 1 - alpha
//   normalise by a0.
static void computeNotchCoeffs(double f0, double Q, double fs,
                                float& b0, float& b1, float& b2,
                                float& a1, float& a2)
{
    const double w0    = 2.0 * M_PI * f0 / fs;
    const double alpha = std::sin(w0) / (2.0 * Q);
    const double a0    = 1.0 + alpha;
    b0 =  static_cast<float>( 1.0          / a0);
    b1 =  static_cast<float>(-2.0 * std::cos(w0) / a0);
    b2 =  static_cast<float>( 1.0          / a0);
    a1 =  static_cast<float>(-2.0 * std::cos(w0) / a0);
    a2 =  static_cast<float>((1.0 - alpha) / a0);
}

// ── Constructor ───────────────────────────────────────────────────────────────
CQUAMDecoder::CQUAMDecoder(double sampleRate, double notchFreq, double notchQ, bool /*legacyMono*/)
    : fs_(static_cast<float>(sampleRate))
    , notchFreq_(static_cast<float>(notchFreq))
    , notchQ_(static_cast<float>(notchQ))
    , w1L_(0), w2L_(0), w1R_(0), w2R_(0)
    , dcState_(0.0f)
    , stereoMode_(StereoMode::Auto)
{
    updatePLLGains();
    updateNotchCoeffs();

    // Goertzel coefficient for 25 Hz pilot
    gCoeff_ = 2.0f * std::cos(2.0f * static_cast<float>(M_PI) * 25.0f / fs_);
}

// ── PLL gains ─────────────────────────────────────────────────────────────────
void CQUAMDecoder::updatePLLGains()
{
    const float T     = 1.0f / fs_;
    const float denom = 1.0f + 2.0f * zeta_ * omegaN_ * T + (omegaN_ * T) * (omegaN_ * T);
    alpha_ = (2.0f * zeta_ * omegaN_ * T) / denom;
    beta_  = ((omegaN_ * T) * (omegaN_ * T)) / denom;
}

// ── Notch coefficients ────────────────────────────────────────────────────────
void CQUAMDecoder::updateNotchCoeffs()
{
    computeNotchCoeffs(notchFreq_, notchQ_, fs_, nb0_, nb1_, nb2_, na1_, na2_);
    w1L_ = w2L_ = w1R_ = w2R_ = 0.0f;
    dcState_ = 0.0f;
}

// ── Setters (thread-safe) ─────────────────────────────────────────────────────
void CQUAMDecoder::setNotchFreq(double hz)
{
    std::lock_guard<std::mutex> lk(paramMutex_);
    if (notchFreq_ != hz) {
        notchFreq_ = hz;
        updateNotchCoeffs();
    }
}

void CQUAMDecoder::setStereoMode(StereoMode mode)
{
    std::lock_guard<std::mutex> lk(paramMutex_);
    stereoMode_ = mode;
}

void CQUAMDecoder::setSampleRate(double hz)
{
    std::lock_guard<std::mutex> lk(paramMutex_);
    fs_ = static_cast<float>(hz);
    updatePLLGains();
    updateNotchCoeffs();
    gCoeff_ = 2.0f * std::cos(2.0f * static_cast<float>(M_PI) * 25.0f / fs_);
}

// ── Main processing loop ──────────────────────────────────────────────────────
void CQUAMDecoder::process(const std::complex<float>* in,
                           float* lOut, float* rOut,
                           std::size_t n)
{
    // Take parameter snapshot under lock (cheap copy)
    float alpha, beta, gCoeff, nb0, nb1, nb2, na1, na2;
        StereoMode stereoMode;
    {
        std::lock_guard<std::mutex> lk(paramMutex_);
        alpha      = alpha_;
        beta       = beta_;
        gCoeff     = gCoeff_;
        nb0        = nb0_; nb1 = nb1_; nb2 = nb2_;
        na1        = na1_; na2 = na2_;
        stereoMode = stereoMode_;
    }

    // Working state (local for speed)
    float omega2   = omega2_;
    float cosGamma = cosGamma_;
    std::complex<float> vco = vco_;

    float s1 = gS1_, s2 = gS2_;
    float lockLvl = lockLevel_.load(std::memory_order_relaxed);

    float w1l = w1L_, w2l = w2L_;
    float w1r = w1R_, w2r = w2R_;

    for (std::size_t i = 0; i < n; ++i) {
        // 1. Complex demodulation via VCO rotation
        const std::complex<float> bb = std::complex<float>(in[i].real(), in[i].imag()) * vco;
        const float I = bb.real();
        const float Q = bb.imag();

        // 2. Fast envelope approximation (max-abs + 0.4*min-abs)
        const float absI = std::abs(I);
        const float absQ = std::abs(Q);
        const float env  = (absI > absQ)
                          ? absI + 0.4f * absQ + 1e-9f
                          : absQ + 0.4f * absI + 1e-9f;

        // 3. PLL error detector
        const float det = Q / env;
        omega2   += beta * det;
        cosGamma += 0.005f * ((I / env) - cosGamma);

        // 4. VCO phase update
        const float dPhz = alpha * det + omega2;
        vco *= std::complex<float>(std::cos(dPhz), -std::sin(dPhz));

        // Renormalise VCO every 512 samples to prevent magnitude drift
        if ((i & 511) == 0) {
            const float mag = std::abs(vco);
            if (mag > 1e-9f) vco /= mag;
        }

        // 5. Stereo extraction
        const float LpR  = (env * cosGamma) - 1.0f;
        const float LmR_raw  = (std::abs(cosGamma) > 1e-9f) ? Q / cosGamma : 0.0f;

        // DC blocker (1-pole high-pass, ~1.5 Hz cutoff) to prevent Goertzel state accumulation
        constexpr float dcAlpha = 0.9999f;
        const float LmR = LmR_raw - dcState_;
        dcState_ = dcAlpha * dcState_ + (1.0f - dcAlpha) * LmR_raw;

        const float rawL = 0.5f * (LpR + LmR);
        const float rawR = 0.5f * (LpR - LmR);

        // 6. Goertzel filter for 25 Hz pilot detection
        const float s0 = LmR + gCoeff * s1 - s2;
        s2 = s1; s1 = s0;

        // 7. Carrier lock monitor (slow IIR)
        const float lockInput = (env > 1e-9f) ? std::max(0.0f, I / env) : 0.0f;
        lockLvl += 0.001f * (lockInput - lockLvl);

        // 8. Biquad notch filter – left channel (Direct Form II transposed)
        const float wn0l = rawL - na1 * w1l - na2 * w2l;
        lOut[i] = nb0 * wn0l + nb1 * w1l + nb2 * w2l;
        w2l = w1l; w1l = wn0l;

        // 9. Biquad notch filter – right channel
        const float wn0r = rawR - na1 * w1r - na2 * w2r;
        rOut[i] = nb0 * wn0r + nb1 * w1r + nb2 * w2r;
        w2r = w1r; w1r = wn0r;
    }

    // Write back state
    omega2_   = omega2;
    cosGamma_ = cosGamma;
    vco_      = vco;
    gS1_ = s1; gS2_ = s2;
    w1L_ = w1l; w2L_ = w2l;
    w1R_ = w1r; w2R_ = w2r;
    lockLevel_.store(lockLvl, std::memory_order_relaxed);

    // Goertzel magnitude (25 Hz pilot power)
    const float power = s1 * s1 + s2 * s2 - s1 * s2 * gCoeff;
    const float mag   = (n > 0) ? std::sqrt(std::max(0.0f, power)) / static_cast<float>(n) : 0.0f;
    const float  prev  = pilotMag_.load(std::memory_order_relaxed);
    pilotMag_.store(0.9f * prev + 0.1f * mag, std::memory_order_relaxed);

        // Stereo mode handling (decide once per block)
        const bool wantMono = (stereoMode == StereoMode::ForceMono) ||
                              (stereoMode == StereoMode::Auto && !pilotDetected());

        if (wantMono) {
            for (std::size_t i = 0; i < n; ++i) {
                const float m = 0.5f * (lOut[i] + rOut[i]);
                lOut[i] = rOut[i] = m;
            }
        }
        // ForceStereo: keep decoded stereo
    }

