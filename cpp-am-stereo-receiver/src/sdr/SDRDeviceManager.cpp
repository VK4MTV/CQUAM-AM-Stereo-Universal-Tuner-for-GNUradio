// ─────────────────────────────────────────────────────────────────────────────
// SDRDeviceManager.cpp  –  Auto-detect and configure any SoapySDR device
// ─────────────────────────────────────────────────────────────────────────────
#include "SDRDeviceManager.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Errors.hpp>
#include <SoapySDR/Logger.hpp>

// ── Priority order (lower = higher priority) ─────────────────────────────────
int SDRDeviceManager::devicePriority(const std::string& driver)
{
    if (driver == "rtlsdr")  return 0;
    if (driver == "hackrf")  return 1;
    if (driver == "uhd")     return 2;
    if (driver == "airspy")  return 3;
    return 99;
}

SDRDeviceManager::SDRDeviceManager() = default;

SDRDeviceManager::~SDRDeviceManager()
{
    stopStream();
    close();
}

// ── Enumerate ─────────────────────────────────────────────────────────────────
std::vector<SDRDeviceInfo> SDRDeviceManager::enumerateDevices()
{
    std::vector<SDRDeviceInfo> result;

    const auto kwargsList = SoapySDR::Device::enumerate();
    for (const auto& kwargs : kwargsList) {
        SDRDeviceInfo info;

        // Extract driver key
        auto it = kwargs.find("driver");
        info.driver = (it != kwargs.end()) ? it->second : "unknown";

        // Build args string (SoapySDR KWArgs → "key=val,key2=val2")
        std::string args;
        for (const auto& kv : kwargs) {
            if (!args.empty()) args += ",";
            args += kv.first + "=" + kv.second;
        }
        info.args = args;

        // Build label
        auto lbl = kwargs.find("label");
        info.label = (lbl != kwargs.end()) ? lbl->second : info.driver;

        // RTL-SDR specifics
        if (info.driver == "rtlsdr") {
            info.sampleRate = 1'000'000.0;
            info.directSamp = true;
        } else if (info.driver == "hackrf") {
            info.sampleRate = 2'000'000.0;
            info.directSamp = false;
        } else {
            info.sampleRate = 1'000'000.0;
            info.directSamp = false;
        }

        result.push_back(info);
        std::cout << "[SDR] Found: " << info.label
                  << " (driver=" << info.driver << ")\n";
    }

    // Sort by priority
    std::sort(result.begin(), result.end(), [](const SDRDeviceInfo& a, const SDRDeviceInfo& b) {
        return devicePriority(a.driver) < devicePriority(b.driver);
    });

    return result;
}

// ── Auto-open ─────────────────────────────────────────────────────────────────
bool SDRDeviceManager::autoOpen()
{
    const auto devices = enumerateDevices();
    if (devices.empty()) {
        std::cerr << "[SDR] No SoapySDR devices found.\n";
        return false;
    }

    const auto& best = devices.front();
    std::cout << "[SDR] Auto-selecting: " << best.label << "\n";
    return open(best.args, best.directSamp);
}

// ── Open a specific device ────────────────────────────────────────────────────
bool SDRDeviceManager::open(const std::string& args, bool directSamp)
{
    if (device_) close();

    try {
        device_ = SoapySDR::Device::make(args);
        if (!device_) {
            std::cerr << "[SDR] SoapySDR::Device::make returned null.\n";
            return false;
        }

        // Populate activeInfo_
        activeInfo_.args       = args;
        activeInfo_.directSamp = directSamp;
        auto kwargs = SoapySDR::KwargsFromString(args);
        auto it = kwargs.find("driver");
        activeInfo_.driver = (it != kwargs.end()) ? it->second : "unknown";
        auto lbl = kwargs.find("label");
        activeInfo_.label = (lbl != kwargs.end()) ? lbl->second : activeInfo_.driver;
        activeInfo_.sampleRate = (activeInfo_.driver == "rtlsdr") ? 1'000'000.0 : 2'000'000.0;

        applyDeviceDefaults(device_, activeInfo_);
        std::cout << "[SDR] Opened: " << activeInfo_.label << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[SDR] Error opening device: " << e.what() << "\n";
        device_ = nullptr;
        return false;
    }
}

// ── Apply device-specific defaults ───────────────────────────────────────────
void SDRDeviceManager::applyDeviceDefaults(SoapySDR::Device* dev, const SDRDeviceInfo& info)
{
    if (info.driver == "rtlsdr") {
        // Direct sampling mode 2 (Q branch) for HF/SW reception with RTL-SDR V4
        try {
            dev->writeSetting("direct_samp", "2");
            std::cout << "[SDR] RTL-SDR: direct sampling mode 2 enabled.\n";
        } catch (...) {
            std::cerr << "[SDR] RTL-SDR: could not set direct_samp (driver may not support it).\n";
        }
        dev->setSampleRate(SOAPY_SDR_RX, 0, 1'000'000.0);
        dev->setGainMode(SOAPY_SDR_RX, 0, true);   // AGC
    } else if (info.driver == "hackrf") {
        dev->setSampleRate(SOAPY_SDR_RX, 0, 2'000'000.0);
        dev->setGainMode(SOAPY_SDR_RX, 0, false);
        dev->setGain(SOAPY_SDR_RX, 0, 30.0);
    } else {
        // Generic defaults
        dev->setSampleRate(SOAPY_SDR_RX, 0, info.sampleRate);
        dev->setGainMode(SOAPY_SDR_RX, 0, true);
    }
}

// ── Close ─────────────────────────────────────────────────────────────────────
void SDRDeviceManager::close()
{
    if (device_) {
        SoapySDR::Device::unmake(device_);
        device_ = nullptr;
        std::cout << "[SDR] Device closed.\n";
    }
}

// ── Parameter setters ─────────────────────────────────────────────────────────
void SDRDeviceManager::setFrequency(double hz)
{
    if (device_)
        device_->setFrequency(SOAPY_SDR_RX, 0, hz);
}

void SDRDeviceManager::setSampleRate(double hz)
{
    if (device_)
        device_->setSampleRate(SOAPY_SDR_RX, 0, hz);
}

void SDRDeviceManager::setGain(double dB)
{
    if (device_) {
        device_->setGainMode(SOAPY_SDR_RX, 0, false);
        device_->setGain(SOAPY_SDR_RX, 0, dB);
    }
}

// ── Start streaming ───────────────────────────────────────────────────────────
void SDRDeviceManager::startStream(SamplesCallback cb)
{
    if (!device_ || streaming_) return;

    callback_  = std::move(cb);
    rxStream_  = device_->setupStream(SOAPY_SDR_RX, SOAPY_SDR_CF32);
    device_->activateStream(rxStream_);
    streaming_ = true;

    std::thread([this]() {
        constexpr int SAMPLES_PER_READ = 4096;
        std::vector<std::complex<float>> buf(SAMPLES_PER_READ);
        void* bufs[] = { buf.data() };
        int   flags  = 0;
        long long timeNs = 0;

        while (streaming_) {
            const int ret = device_->readStream(rxStream_, bufs, SAMPLES_PER_READ, flags, timeNs, 100'000);
            if (ret > 0 && callback_) {
                callback_(buf.data(), static_cast<std::size_t>(ret));
            } else if (ret == SOAPY_SDR_OVERFLOW) {
                // benign – continue
            } else if (ret < 0 && ret != SOAPY_SDR_TIMEOUT) {
                std::cerr << "[SDR] readStream error: " << SoapySDR::errToStr(ret) << "\n";
            }
        }

        device_->deactivateStream(rxStream_);
        device_->closeStream(rxStream_);
        rxStream_ = nullptr;
    }).detach();
}

// ── Stop streaming ────────────────────────────────────────────────────────────
void SDRDeviceManager::stopStream()
{
    streaming_ = false;
    // The streaming thread closes the stream on exit; give it a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}
