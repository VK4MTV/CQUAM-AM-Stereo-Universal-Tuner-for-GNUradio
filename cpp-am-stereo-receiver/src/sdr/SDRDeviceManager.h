#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// SDRDeviceManager.h  –  Auto-detect and configure any SoapySDR device
// ─────────────────────────────────────────────────────────────────────────────
#include <string>
#include <vector>
#include <functional>
#include <complex>
#include <SoapySDR/Device.hpp>

struct SDRDeviceInfo {
    std::string driver;     // e.g. "rtlsdr", "hackrf", "uhd"
    std::string label;      // human-readable label
    std::string args;       // SoapySDR kwargs string
    double      sampleRate; // recommended sample rate
    bool        directSamp; // RTL-SDR direct sampling mode
};

class SDRDeviceManager
{
public:
    SDRDeviceManager();
    ~SDRDeviceManager();

    // Enumerate all connected SoapySDR devices (sorted by priority)
    std::vector<SDRDeviceInfo> enumerateDevices();

    // Open the best available device (or show picker if none)
    // Returns true on success
    bool autoOpen();

    // Open a specific device by its args string
    bool open(const std::string& args, bool directSamp = false);

    // Close the device
    void close();

    bool isOpen() const { return device_ != nullptr; }
    const SDRDeviceInfo& activeDevice() const { return activeInfo_; }

    // Set tuning parameters
    void setFrequency(double hz);
    void setSampleRate(double hz);
    void setGain(double dB);

    // Start/stop streaming
    // callback receives a buffer of complex<float> samples
    using SamplesCallback = std::function<void(const std::complex<float>*, std::size_t)>;
    void startStream(SamplesCallback cb);
    void stopStream();

private:
    void applyDeviceDefaults(SoapySDR::Device* dev, const SDRDeviceInfo& info);
    static int devicePriority(const std::string& driver);

    SoapySDR::Device*   device_    = nullptr;
    SoapySDR::Stream*   rxStream_  = nullptr;
    SDRDeviceInfo       activeInfo_;
    SamplesCallback     callback_;
    bool                streaming_ = false;
};
