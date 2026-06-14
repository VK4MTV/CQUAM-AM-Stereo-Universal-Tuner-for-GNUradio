// ─────────────────────────────────────────────────────────────────────────────
// AudioOutput.cpp  –  PortAudio stereo output at 48 kHz
// ─────────────────────────────────────────────────────────────────────────────
#include "AudioOutput.h"
#include <portaudio.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

AudioOutput::AudioOutput()
{
    const PaError err = Pa_Initialize();
    if (err != paNoError)
        std::cerr << "[Audio] Pa_Initialize failed: " << Pa_GetErrorText(err) << "\n";
}

AudioOutput::~AudioOutput()
{
    close();
    Pa_Terminate();
}

bool AudioOutput::open()
{
    if (stream_) return true;

    PaStreamParameters outParams{};
    outParams.device           = Pa_GetDefaultOutputDevice();
    if (outParams.device == paNoDevice) {
        std::cerr << "[Audio] No default output device found.\n";
        return false;
    }
    outParams.channelCount          = CHANNELS;
    outParams.sampleFormat          = paFloat32;
    outParams.suggestedLatency      = Pa_GetDeviceInfo(outParams.device)->defaultLowOutputLatency;
    outParams.hostApiSpecificStreamInfo = nullptr;

    const PaError err = Pa_OpenStream(
        &stream_,
        nullptr,
        &outParams,
        SAMPLE_RATE,
        FRAMES_PER_BUF,
        paClipOff,
        paCallback,
        this
    );

    if (err != paNoError) {
        std::cerr << "[Audio] Pa_OpenStream failed: " << Pa_GetErrorText(err) << "\n";
        stream_ = nullptr;
        return false;
    }

    Pa_StartStream(stream_);
    const auto* info = Pa_GetStreamInfo(stream_);
    std::cout << "[Audio] PortAudio stream opened at " << SAMPLE_RATE << " Hz stereo.\n";
    if (info) {
        std::cout << "[Audio][DIAG] InputLatency=" << info->inputLatency
                  << "s OutputLatency=" << info->outputLatency << "s\n";
    }
    std::cout << "[Audio][DIAG] RingBuffer capacity: " << (RING_FRAMES * CHANNELS) << " samples\n";
    return true;
}

void AudioOutput::close()
{
    if (stream_) {
        Pa_StopStream(stream_);
        Pa_CloseStream(stream_);
        stream_ = nullptr;
    }
}

void AudioOutput::push(const float* interleaved, std::size_t frames)
{
    ring_.push_force(interleaved, frames * CHANNELS);   // <-- changed to force
}

int AudioOutput::paCallback(const void* /*input*/, void* output,
                            unsigned long frameCount,
                            const PaStreamCallbackTimeInfo* /*timeInfo*/,
                            PaStreamCallbackFlags /*statusFlags*/,
                            void* userData)
{
    auto* self = static_cast<AudioOutput*>(userData);
    auto* out  = static_cast<float*>(output);
    const std::size_t needed = frameCount * CHANNELS;

    // --- Priming: Wait until buffer reaches 25% before outputting real audio ---
    if (!self->primed_) {
        if (self->ring_.size() >= (RING_FRAMES / 4 * CHANNELS)) {
            self->primed_ = true;
            std::cout << "[Audio] Buffer primed at 25%, starting audio output.\n";
        } else {
            // Output silence while waiting for buffer to fill
            std::memset(out, 0, needed * sizeof(float));
            return paContinue;
        }
    }

    std::size_t got = self->ring_.pop(out, needed);

    // --- Simple rate regulation: correct when buffer > 50% ---
    // Large buffer (5.5s) + infrequent correction = minimal audible artifacts
    const std::size_t postPopFill = self->ring_.size();

    if (postPopFill > (self->CORRECTION_THRESHOLD * CHANNELS) && got >= CHANNELS * 64) {
        // Buffer > 75% - drop 32 stereo pairs (64 samples) to catch drift
        // With 22s buffer at 75% threshold, this correction happens ~once every 4 minutes
        got -= CHANNELS * 64;
    }

    if (got < needed) {
        // Hold last valid sample during underrun (prevents clicks)
        for (std::size_t i = got; i < needed; i += CHANNELS) {
            out[i]     = self->lastSample_[0];
            if (i + 1 < needed)
                out[i + 1] = self->lastSample_[1];
        }
        // Mark that we are in an underrun state so we can fade back in
        self->inUnderRun_ = true;

        // --- DIAGNOSTIC: Log underrun ---
        ++self->underrunCount_;
        if (self->underrunCount_ <= 10 || (self->underrunCount_ % 100) == 0) {
            std::cerr << "[Audio][GLITCH] Underrun #" << self->underrunCount_
                      << " (needed=" << needed << ", got=" << got << ")\n";
        }
    }

    // Update lastSample_ from the most recent valid data we wrote
    if (needed >= 2) {
        self->lastSample_[0] = out[needed - 2];
        self->lastSample_[1] = out[needed - 1];
    }

    // Gentle linear fade-in after recovering from underrun (first 64 samples)
    if (self->inUnderRun_ && got == needed) {
        constexpr int fadeLen = 64;
        for (int i = 0; i < fadeLen && i < static_cast<int>(frameCount); ++i) {
            const float g = static_cast<float>(i) / fadeLen;
            out[i*2]     *= g;
            out[i*2 + 1] *= g;
        }
        self->inUnderRun_ = false;
    }

    // --- DIAGNOSTIC: Update counters ---
    self->totalFramesRead_    += (got / CHANNELS);

    // Periodic status report every ~5 seconds
    constexpr std::size_t REPORT_INTERVAL = 240000;
    if ((self->totalFramesRead_ / REPORT_INTERVAL) != (self->lastReportTime_ / REPORT_INTERVAL)) {
        self->lastReportTime_ = self->totalFramesRead_;
        const int fillPct = static_cast<int>(self->ring_.size() * 100 / (RING_FRAMES * CHANNELS));
        std::cout << "[Audio][STATS] Underruns: " << self->underrunCount_
                  << " | Buffer: " << fillPct << "% (target 20-40%, correcting >35%)\n";
    }

    return paContinue;
}
