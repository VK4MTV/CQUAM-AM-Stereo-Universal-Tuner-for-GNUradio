// ─────────────────────────────────────────────────────────────────────────────
// DSPPipeline.cpp
// ─────────────────────────────────────────────────────────────────────────────
#include "DSPPipeline.h"
#include <algorithm>

DSPPipeline::DSPPipeline()
    : xlator_   (-100'000.0, SDR_RATE)
    , resampler1_(RESAMP1_INTERP, RESAMP1_DECIM)
    , lpf_      (IF_RATE, 10'000.0, 2'000.0)
    , decoder_  (IF_RATE, 5'000.0, 50.0, false)
    , resamplerL_(RESAMP2_INTERP, RESAMP2_DECIM)
    , resamplerR_(RESAMP2_INTERP, RESAMP2_DECIM)
{
    // Pre-allocate intermediate buffers
    // Worst case: 4096 IQ in → ×12/125 + margin IF samples
    ifBuf_.resize(8192);
    lBuf_.resize(8192);
    rBuf_.resize(8192);
    lAudio_.resize(8192);
    rAudio_.resize(8192);
    interleaved_.resize(16384);
}

void DSPPipeline::setAudioBandwidth(double hz)
{
    lpf_.setCutoff(hz);
}

void DSPPipeline::setNotchFreq(double hz)
{
    decoder_.setNotchFreq(hz);
}

void DSPPipeline::setMonoMode(bool mono)
{
    decoder_.setMonoMode(mono);
}

void DSPPipeline::processIQ(const std::complex<float>* samples, std::size_t count)
{
    // ── 1. Work on a local copy to allow frequency translation in-place ───────
    // Avoid VLA; use the ifBuf_ as scratch for the xlated IQ
    if (count > ifBuf_.size()) ifBuf_.resize(count * 2);

    std::copy(samples, samples + count, ifBuf_.data());

    // ── 2. Frequency translation: -100 kHz ───────────────────────────────────
    xlator_.process(ifBuf_.data(), count);

    // ── 3. Rational resampler: 1 MHz → 96 kHz ────────────────────────────────
    const std::size_t maxIf = (count * RESAMP1_INTERP) / RESAMP1_DECIM + 64;
    if (maxIf > lBuf_.size()) {
        lBuf_.resize(maxIf);
        rBuf_.resize(maxIf);
    }

    // Reuse ifBuf_ output area
    std::vector<std::complex<float>> ifOut(maxIf);
    const std::size_t ifCount = resampler1_.process(ifBuf_.data(), count,
                                                     ifOut.data(), maxIf);

    // ── 4. Low-pass filter (audio bandwidth shaping) ─────────────────────────
    lpf_.process(ifOut.data(), ifCount);

    // ── 5. C-QUAM decoder → L and R float channels ───────────────────────────
    if (ifCount > lBuf_.size()) {
        lBuf_.resize(ifCount);
        rBuf_.resize(ifCount);
    }
    decoder_.process(ifOut.data(), lBuf_.data(), rBuf_.data(), ifCount);

    // ── 6. Resample to 48 kHz (96 kHz → 48 kHz, factor 1/2) ─────────────────
    const std::size_t maxAudio = (ifCount * RESAMP2_INTERP) / RESAMP2_DECIM + 16;
    if (maxAudio > lAudio_.size()) {
        lAudio_.resize(maxAudio);
        rAudio_.resize(maxAudio);
    }
    const std::size_t lFrames = resamplerL_.process(lBuf_.data(), ifCount,
                                                     lAudio_.data(), maxAudio);
    const std::size_t rFrames = resamplerR_.process(rBuf_.data(), ifCount,
                                                     rAudio_.data(), maxAudio);

    const std::size_t frames = std::min(lFrames, rFrames);
    if (frames == 0 || !audioCallback_) return;

    // ── 7. Interleave stereo and hand off to audio output ────────────────────
    if (frames * 2 > interleaved_.size())
        interleaved_.resize(frames * 2);

    for (std::size_t i = 0; i < frames; ++i) {
        interleaved_[2 * i]     = lAudio_[i];
        interleaved_[2 * i + 1] = rAudio_[i];
    }

    audioCallback_(interleaved_.data(), frames);
}
