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

        // Stereo mode
    enum class StereoMode { Auto, ForceStereo, ForceMono };

    // Parameter setters (thread-safe via mutex)
    void setNotchFreq(double hz);
    void setStereoMode(StereoMode mode);
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
    float fs_;
    float zeta_  = 0.707f;
    float omegaN_ = 100.0f;
    float alpha_ = 0.0f, beta_ = 0.0f;

    float  phz_     = 0.0f;
    float  omega2_  = 0.0f;
    float  cosGamma_= 1.0f;
    std::complex<float> vco_{ 1.0f, 0.0f };

    // --- Goertzel 25 Hz pilot ---
    float gCoeff_ = 0.0f;
    float gS1_    = 0.0f;
    float gS2_    = 0.0f;

    // --- Notch filter (biquad, transposed direct form II) ---
    float notchFreq_;
    float notchQ_;
    float nb0_, nb1_, nb2_;
    float na1_, na2_;
    float w1L_, w2L_;
    float w1R_, w2R_;

    // --- DC blocker for Goertzel input (prevents state accumulation) ---
    float dcState_ = 0.0f;

    // --- Mode ---
    StereoMode stereoMode_ = StereoMode::Auto;

    // --- Status (updated atomically) ---
    std::atomic<float> lockLevel_{ 0.f };
    std::atomic<float> pilotMag_ { 0.f };

    // --- Mutex for parameter changes ---
    std::mutex paramMutex_;
};
