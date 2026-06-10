# Testing Guide – AM Stereo Receiver (C-QUAM)

This document is intended for manual testing on **Windows** (primary) and **Linux**.

macOS testing is currently deprioritised due to hardware issues.

---

## 1. Windows Testing (Primary Focus)

### Prerequisites

Install in this order:

1. **Visual Studio 2022**  
   - Workload: “Desktop development with C++”

2. **Qt 6**  
   - Download the Qt Online Installer from https://www.qt.io/download-open-source  
   - Install **Qt 6.x → MSVC 2019 64-bit** (or 2022)

3. **PothosSDR** (recommended)  
   - https://github.com/pothosware/PothosSDR/releases  
   - Provides SoapySDR + RTL-SDR support

4. **PortAudio**  
   - Recommended: `vcpkg install portaudio:x64-windows`  
   - Alternative: prebuilt binaries from http://www.portaudio.com

### Build Steps (Developer Command Prompt)

```cmd
cd cpp-am-stereo-receiver

cmake -B build -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH="C:/Qt/6.x.y/msvc2019_64;C:/PothosSDR"

cmake --build build --config Release
```

Run the application:

```
build\Release\am_stereo_receiver.exe
```

---

## 2. Linux Testing

### Ubuntu 22.04 / Debian 12 (or Raspberry Pi OS)

```bash
sudo apt update
sudo apt install -y cmake g++ ninja-build pkg-config \
                    qt6-base-dev libsoapysdr-dev soapysdr-tools \
                    soapysdr-module-rtlsdr libportaudio2 libportaudio-dev

cd cpp-am-stereo-receiver
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

./build/am_stereo_receiver
```

> **Note**: On Raspberry Pi 5 you may also need to add the RTL-SDR udev rule (see BUILD.md).

---

## 3. Test Checklist

Run the following tests on **both Windows and Linux** when possible.

### Basic Functionality
- [ ] Application launches without crashing
- [ ] Auto-detects RTL-SDR (or other SoapySDR device)
- [ ] Default frequency is 7390 kHz (40 m amateur band)
- [ ] Frequency can be changed via spinbox and slider

### Audio Quality (Most Important on Windows)
- [ ] No constant clicks or pops during normal reception
- [ ] Audio remains clean when changing frequency
- [ ] Underrun recovery (if it occurs) is smooth, not glitchy
- [ ] Volume is reasonable (not extremely quiet or distorted)

### Stereo Modes
- [ ] **Auto** (default): Switches to stereo only when 25 Hz pilot is detected
- [ ] **Force Stereo**: Always outputs stereo, even on weak or no pilot
- [ ] **Force Mono**: Always folds L+R to mono

### Notch Filter
- [ ] Switching between 5 kHz / 9 kHz / 10 kHz / Variable does not cause audio pops
- [ ] Variable notch slider updates the filter frequency live

### Status Indicators
- [ ] “Carrier Lock” LED turns green when tuned to a strong station
- [ ] “Stereo Pilot” LED turns blue when a C-QUAM station with 25 Hz pilot is received
- [ ] LEDs update at a reasonable rate (~100 ms)

### Edge Cases
- [ ] Changing notch frequency while receiving does not cause a click
- [ ] Rapid frequency changes do not crash or freeze the application
- [ ] Closing the window cleanly stops the SDR and audio streams

---

## 4. Known Issues / Gotchas

| Issue | Platform | Workaround / Status |
|-------|----------|---------------------|
| Occasional clicks on very long underruns | Windows | Being monitored – current fade-in helps |
| RTL-SDR clock drift vs sound card | All | Currently mitigated by resampler + ring buffer |
| Qt6 not found during CMake | Windows | Set `CMAKE_PREFIX_PATH` to your Qt install |
| SoapySDR not found | All | Install PothosSDR (Windows) or libsoapysdr-dev (Linux) |

---

## 5. Reporting Results

When reporting test results, please include:

- Operating System + version
- SDR device used (e.g. RTL-SDR Blog v3)
- Whether audio was clean or had clicks/pops
- Which stereo modes were tested
- Any crashes or unexpected behaviour

Thank you for helping improve cross-platform stability!