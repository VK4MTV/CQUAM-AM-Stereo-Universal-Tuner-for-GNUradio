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
                                double& b0, double& b1, double& b2,
                                double& a1, double& a2)
{
    const double w0    = 2.0 * M_PI * f0 / fs;
    const double alpha = std::sin(w0) / (2.0 * Q);
    const double a0    = 1.0 + alpha;
    b0 =  1.0          / a0;
    b1 = -2.0 * std::cos(w0) / a0;
    b2 =  1.0          / a0;
    a1 = -2.0 * std::cos(w0) / a0;
    a2 = (1.0 - alpha) / a0;
}

// ── Constructor ───────────────────────────────────────────────────────────────
CQUAMDecoder::CQUAMDecoder(double sampleRate, double notchFreq, double notchQ, bool monoMode)
    : fs_(sampleRate)
    , notchFreq_(notchFreq)
    , notchQ_(notchQ)
    , w1L_(0), w2L_(0), w1R_(0), w2R_(0)
    , monoMode_(monoMode)
{
    updatePLLGains();
    updateNotchCoeffs();

    // Goertzel coefficient for 25 Hz pilot
    gCoeff_ = 2.0 * std::cos(2.0 * M_PI * 25.0 / fs_);
}

// ── PLL gains ─────────────────────────────────────────────────────────────────
void CQUAMDecoder::updatePLLGains()
{
    const double T     = 1.0 / fs_;
    const double denom = 1.0 + 2.0 * zeta_ * omegaN_ * T + (omegaN_ * T) * (omegaN_ * T);
    alpha_ = (2.0 * zeta_ * omegaN_ * T) / denom;
    beta_  = ((omegaN_ * T) * (omegaN_ * T)) / denom;
}

// ── Notch coefficients ────────────────────────────────────────────────────────
void CQUAMDecoder::updateNotchCoeffs()
{
    computeNotchCoeffs(notchFreq_, notchQ_, fs_, nb0_, nb1_, nb2_, na1_, na2_);
    w1L_ = w2L_ = w1R_ = w2R_ = 0.0;
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

void CQUAMDecoder::setMonoMode(bool mono)
{
    std::lock_guard<std::mutex> lk(paramMutex_);
    monoMode_ = mono;
}

void CQUAMDecoder::setSampleRate(double hz)
{
    std::lock_guard<std::mutex> lk(paramMutex_);
    fs_ = hz;
    updatePLLGains();
    updateNotchCoeffs();
    gCoeff_ = 2.0 * std::cos(2.0 * M_PI * 25.0 / fs_);
}

// ── Main processing loop ──────────────────────────────────────────────────────
void CQUAMDecoder::process(const std::complex<float>* in,
                           float* lOut, float* rOut,
                           std::size_t n)
{
    // Take parameter snapshot under lock (cheap copy)
    double alpha, beta, gCoeff, nb0, nb1, nb2, na1, na2;
    bool   monoMode;
    {
        std::lock_guard<std::mutex> lk(paramMutex_);
        alpha    = alpha_;
        beta     = beta_;
        gCoeff   = gCoeff_;
        nb0      = nb0_; nb1 = nb1_; nb2 = nb2_;
        na1      = na1_; na2 = na2_;
        monoMode = monoMode_;
    }

    // Working state (local for speed)
    double omega2   = omega2_;
    double cosGamma = cosGamma_;
    std::complex<double> vco = vco_;

    double s1 = gS1_, s2 = gS2_;
    double lockLvl = lockLevel_.load(std::memory_order_relaxed);

    double w1l = w1L_, w2l = w2L_;
    double w1r = w1R_, w2r = w2R_;

    for (std::size_t i = 0; i < n; ++i) {
        // 1. Complex demodulation via VCO rotation
        const std::complex<double> bb = std::complex<double>(in[i].real(), in[i].imag()) * vco;
        const double I = bb.real();
        const double Q = bb.imag();

        // 2. Fast envelope approximation (max-abs + 0.4*min-abs)
        const double absI = std::abs(I);
        const double absQ = std::abs(Q);
        const double env  = (absI > absQ)
                          ? absI + 0.4 * absQ + 1e-9
                          : absQ + 0.4 * absI + 1e-9;

        // 3. PLL error detector
        const double det = Q / env;
        omega2   += beta * det;
        cosGamma += 0.005 * ((I / env) - cosGamma);

        // 4. VCO phase update
        const double dPhz = alpha * det + omega2;
        vco *= std::complex<double>(std::cos(dPhz), -std::sin(dPhz));

        // Renormalise VCO every 512 samples to prevent magnitude drift
        if ((i & 511) == 0) {
            const double mag = std::abs(vco);
            if (mag > 1e-9) vco /= mag;
        }

        // 5. Stereo extraction
        const double LpR  = (env * cosGamma) - 1.0;
        const double LmR  = (std::abs(cosGamma) > 1e-9) ? Q / cosGamma : 0.0;
        const double rawL = 0.5 * (LpR + LmR);
        const double rawR = 0.5 * (LpR - LmR);

        // 6. Goertzel filter for 25 Hz pilot detection
        const double s0 = LmR + gCoeff * s1 - s2;
        s2 = s1; s1 = s0;

        // 7. Carrier lock monitor (slow IIR)
        const double lockInput = (env > 1e-9) ? std::max(0.0, I / env) : 0.0;
        lockLvl += 0.001 * (lockInput - lockLvl);

        // 8. Biquad notch filter – left channel (Direct Form II transposed)
        const double wn0l = rawL - na1 * w1l - na2 * w2l;
        lOut[i] = static_cast<float>(nb0 * wn0l + nb1 * w1l + nb2 * w2l);
        w2l = w1l; w1l = wn0l;

        // 9. Biquad notch filter – right channel
        const double wn0r = rawR - na1 * w1r - na2 * w2r;
        rOut[i] = static_cast<float>(nb0 * wn0r + nb1 * w1r + nb2 * w2r);
        w2r = w1r; w1r = wn0r;
    }

    // Write back state
    omega2_   = omega2;
    cosGamma_ = cosGamma;
    vco_      = vco;
    gS1_ = s1; gS2_ = s2;
    w1L_ = w1l; w2L_ = w2l;
    w1R_ = w1r; w2R_ = w2r;
    lockLevel_.store(static_cast<float>(lockLvl), std::memory_order_relaxed);

    // Goertzel magnitude (25 Hz pilot power)
    const double power = s1 * s1 + s2 * s2 - s1 * s2 * gCoeff;
    const double mag   = (n > 0) ? std::sqrt(std::max(0.0, power)) / n : 0.0;
    const float  prev  = pilotMag_.load(std::memory_order_relaxed);
    pilotMag_.store(0.9f * prev + 0.1f * static_cast<float>(mag), std::memory_order_relaxed);

    // Mono fold-down
    if (monoMode) {
        for (std::size_t i = 0; i < n; ++i) {
            const float m = 0.5f * (lOut[i] + rOut[i]);
            lOut[i] = rOut[i] = m;
        }
    }
}
