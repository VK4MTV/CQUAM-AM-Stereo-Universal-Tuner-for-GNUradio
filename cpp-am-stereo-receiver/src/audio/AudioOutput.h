#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// AudioOutput.h  –  PortAudio stereo output at 48 kHz
// ─────────────────────────────────────────────────────────────────────────────
#include <portaudio.h>
#include <vector>
#include <mutex>
#include <cstddef>
#include "../RingBuffer.h"

class AudioOutput
{
public:
    static constexpr int    SAMPLE_RATE  = 48'000;
    static constexpr int    CHANNELS     = 2;
    static constexpr int    FRAMES_PER_BUF = 512;

    AudioOutput();
    ~AudioOutput();

    bool open();
    void close();
    bool isOpen() const { return stream_ != nullptr; }

    // Push interleaved stereo float32 samples into the ring buffer.
    // Called from the DSP thread.
    void push(const float* interleaved, std::size_t frames);

private:
    static int paCallback(const void* input, void* output,
                          unsigned long frameCount,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData);

    PaStream* stream_ = nullptr;

    // Ring buffer: 65536 frames × 2 channels = 131072 floats
    static constexpr std::size_t RING_FRAMES = 65536;
    RingBuffer<float, RING_FRAMES * CHANNELS> ring_;
};
