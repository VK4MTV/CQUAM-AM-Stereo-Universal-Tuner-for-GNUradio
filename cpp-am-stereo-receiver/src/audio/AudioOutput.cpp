// ─────────────────────────────────────────────────────────────────────────────
// AudioOutput.cpp  –  PortAudio stereo output at 48 kHz
// ─────────────────────────────────────────────────────────────────────────────
#include "AudioOutput.h"
#include <iostream>
#include <cstring>

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

    PaStreamParameters outParams;
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
        nullptr,        // no input
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
    std::cout << "[Audio] PortAudio stream opened at " << SAMPLE_RATE << " Hz stereo.\n";
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
    ring_.push(interleaved, frames * CHANNELS);
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

    const std::size_t got = self->ring_.pop(out, needed);
    if (got < needed) {
        // Buffer underrun – zero-fill remainder to avoid noise
        std::memset(out + got, 0, (needed - got) * sizeof(float));
    }
    return paContinue;
}
