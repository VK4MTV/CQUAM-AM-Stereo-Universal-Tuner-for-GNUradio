#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// AudioOutput.h  –  PortAudio stereo output at 48 kHz
// ─────────────────────────────────────────────────────────────────────────────
#include <portaudio.h>
#include <vector>
#include <mutex>
#include <cstddef>
#include "../RingBuffer.h"

// ... existing includes ...

class AudioOutput
{
public:
    static constexpr int    SAMPLE_RATE  = 48'000;
    static constexpr int    CHANNELS     = 2;
    static constexpr int    FRAMES_PER_BUF = 2048;  // Increased from 512 for DSP spike tolerance

    AudioOutput();
    ~AudioOutput();

    bool open();
    void close();
    bool isOpen() const { return stream_ != nullptr; }

    void push(const float* interleaved, std::size_t frames);

private:
    static int paCallback(const void* input, void* output,
                          unsigned long frameCount,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData);

    PaStream* stream_ = nullptr;

    // Very large buffer (~22s) to make corrections very infrequent
    // Correction happens ~once every 4 minutes, tick is barely noticeable
    static constexpr std::size_t RING_FRAMES = 1048576;  // 1M frames * 2ch = 2M samples (~22s)
    RingBuffer<float, RING_FRAMES * CHANNELS> ring_;   // note: explicit template now matches

    bool primed_ = false;  // true once buffer reaches 25% fill for first time

    // Correction threshold: correct only when buffer > 75%
    // With 22s buffer, this gives plenty of headroom
    static constexpr std::size_t CORRECTION_THRESHOLD = RING_FRAMES * 3 / 4;  // 75%

    // For smooth underrun handling
    float lastSample_[2] = {0.0f, 0.0f};
    bool  inUnderRun_    = false;

    // --- Diagnostic counters ---
    std::size_t underrunCount_      = 0;
    std::size_t totalFramesWritten_ = 0;
    std::size_t totalFramesRead_    = 0;
    std::size_t lastReportTime_     = 0;   // simple frame-based reporting
};
