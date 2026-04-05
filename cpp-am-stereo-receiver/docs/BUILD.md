# Building AM Stereo Receiver

## Dependencies

| Library     | Version     | Purpose                        |
|-------------|-------------|--------------------------------|
| Qt6         | ≥ 6.2       | GUI framework                  |
| SoapySDR    | ≥ 0.8       | SDR hardware abstraction layer |
| PortAudio   | ≥ 19        | Cross-platform audio output    |
| CMake       | ≥ 3.16      | Build system                   |

---

## Linux (Ubuntu 22.04 / Debian 12)

```bash
# Install build tools
sudo apt update
sudo apt install -y cmake g++ ninja-build pkg-config

# Install Qt6
sudo apt install -y qt6-base-dev

# Install SoapySDR
sudo apt install -y libsoapysdr-dev soapysdr-tools
# For RTL-SDR support:
sudo apt install -y soapysdr-module-rtlsdr librtlsdr-dev

# Install PortAudio
sudo apt install -y libportaudio2 libportaudio-dev

# Clone and build
git clone https://github.com/VK4MTV/CQUAM-AM-Stereo-Universal-Tuner-for-GNUradio.git
cd CQUAM-AM-Stereo-Universal-Tuner-for-GNUradio/cpp-am-stereo-receiver

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run
./build/am_stereo_receiver
```

### RTL-SDR permissions (Linux)
```bash
# Add udev rule to allow non-root USB access
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="0bda", MODE="0666"' | \
    sudo tee /etc/udev/rules.d/99-rtlsdr.rules
sudo udevadm control --reload-rules && sudo udevadm trigger
```

---

## macOS (Homebrew)

```bash
# Install Homebrew if needed: https://brew.sh

brew update
brew install cmake qt@6 soapysdr portaudio

# RTL-SDR support
brew install librtlsdr
# Install SoapyRTLSDR separately (build from source or use osmocom tap):
brew tap pothosware/pothos
brew install soapyrtlsdr

cd cpp-am-stereo-receiver
export Qt6_DIR=$(brew --prefix qt@6)/lib/cmake/Qt6
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.ncpu)
./build/am_stereo_receiver
```

---

## Windows (MSVC / Visual Studio 2022)

### Prerequisites
1. **Visual Studio 2022** with "Desktop development with C++" workload
2. **CMake** ≥ 3.16 (bundled with VS or download from cmake.org)
3. **Qt6** – download the Qt Online Installer from [qt.io](https://www.qt.io/download-open-source)
   - Select `Qt 6.x → MSVC 2019 64-bit`
4. **SoapySDR** – use PothosSDR installer:
   - Download from [GitHub releases](https://github.com/pothosware/PothosSDR/releases)
   - Installs SoapySDR + RTL-SDR support automatically
5. **PortAudio** – download prebuilt binaries from [portaudio.com](http://www.portaudio.com/download.html),
   or install via vcpkg: `vcpkg install portaudio:x64-windows`

### Build
Open a **Developer Command Prompt for VS 2022** and:

```cmd
cd cpp-am-stereo-receiver
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH="C:/Qt/6.x.y/msvc2019_64;C:/PothosSDR" ^
      -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
.\build\Release\am_stereo_receiver.exe
```

### Windows MinGW (alternative)
```bash
# Using MSYS2 (https://www.msys2.org/)
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-qt6-base \
          mingw-w64-x86_64-soapysdr mingw-w64-x86_64-portaudio

cd cpp-am-stereo-receiver
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
./build/am_stereo_receiver.exe
```

---

## Verifying the Build

After building, run with an RTL-SDR connected:

```bash
# List detected SoapySDR devices
SoapySDRUtil --find

# Run the application
./build/am_stereo_receiver
```

The application will:
1. Auto-detect the first available SoapySDR device
2. Apply RTL-SDR direct sampling mode 2 automatically
3. Open the GUI tuned to 7390 kHz (40m amateur band) as default

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "No SDR device found" | Check USB connection; ensure RTL-SDR drivers installed |
| No audio output | Verify PortAudio finds your sound card; check system mixer |
| Build fails: Qt6 not found | Set `Qt6_DIR` environment variable to Qt6 CMake dir |
| Build fails: SoapySDR not found | Add SoapySDR install prefix to `CMAKE_PREFIX_PATH` |
| RTL-SDR permission denied (Linux) | Add udev rule (see above) or run as root (not recommended) |
