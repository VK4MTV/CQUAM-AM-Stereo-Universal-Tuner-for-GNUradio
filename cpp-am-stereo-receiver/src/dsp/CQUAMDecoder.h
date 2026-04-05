#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// CQUAMDecoder.h  –  C++ port of AMradio_epy_block_5.py
//
// Input : complex<float> stream at 120 kHz (IF after resampler + LPF)
// Output: two float streams – L channel and R channel
//
// Algorithm:
//   1. PLL locks onto the AM carrier (zeta=0.707, omegaN=100)
//   2. Complex demodulation via VCO rotation
//   3. C-QUAM stereo extraction: L+R from envelope, L-R from quadrature
//   4. Goertzel filter detects 25 Hz pilot tone
//   5. Biquad IIR notch filter on both channels
// ─────────────────────────────────────────────────────────────────────────────
#include <complex>
#include <cstddef>
#include <atomic>
#include <mutex>

class CQUAMDecoder
{
public:
    explicit CQUAMDecoder(double sampleRate = 120'000.0,
                          double notchFreq  = 5'000.0,
                          double notchQ     = 50.0,
                          bool   monoMode   = false);

    // Process n complex input samples; write n float samples to lOut and rOut
    void process(const std::complex<float>* in,
                 float* lOut, float* rOut,
                 std::size_t n);

    // Parameter setters (thread-safe via mutex)
    void setNotchFreq(double hz);
    void setMonoMode(bool mono);
    void setSampleRate(double hz);

    // Status getters (updated after each process() call)
    float lockLevel()  const { return lockLevel_.load(std::memory_order_relaxed); }
    float pilotMag()   const { return pilotMag_.load(std::memory_order_relaxed); }

    // Threshold helpers matching the Python logic
    bool  pilotDetected() const { return lockLevel() > 0.8f && pilotMag() > 0.001f; }
    bool  carrierLocked() const { return lockLevel() > 0.7f; }

private:
    void updatePLLGains();
    void updateNotchCoeffs();

    // --- PLL state ---
    double fs_;
    double zeta_  = 0.707;
    double omegaN_ = 100.0;
    double alpha_ = 0.0, beta_ = 0.0;

    double  phz_     = 0.0;
    double  omega2_  = 0.0;
    double  cosGamma_= 1.0;
    std::complex<double> vco_{ 1.0, 0.0 };

    // --- Goertzel 25 Hz pilot ---
    double gCoeff_ = 0.0;
    double gS1_    = 0.0;
    double gS2_    = 0.0;

    // --- Notch filter (biquad, transposed direct form II) ---
    double notchFreq_;
    double notchQ_;
    double nb0_, nb1_, nb2_;
    double na1_, na2_;
    double w1L_, w2L_;
    double w1R_, w2R_;

    // --- Mode ---
    bool monoMode_;

    // --- Status (updated atomically) ---
    std::atomic<float> lockLevel_{ 0.f };
    std::atomic<float> pilotMag_ { 0.f };

    // --- Mutex for parameter changes ---
    std::mutex paramMutex_;
};
