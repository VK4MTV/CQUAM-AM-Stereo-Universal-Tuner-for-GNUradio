# Architecture – AM Stereo Receiver

## Overview

The application is a standalone C++17 real-time DSP pipeline that:
- Auto-detects any SoapySDR-compatible SDR receiver
- Decodes C-QUAM AM stereo broadcasts
- Outputs 48 kHz stereo PCM to the system audio device

---

## Signal Chain

```
┌─────────────────────────────────────────────────────────────────────┐
│ SDR Hardware (RTL-SDR / HackRF / USRP / …)                          │
│   Sample rate : 1 MHz   Format: complex<float> IQ                   │
└────────────────────────┬────────────────────────────────────────────┘
                         │  SoapySDR readStream()  [SDR I/O thread]
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│ FreqXlator  (src/dsp/FreqXlator.cpp)                                │
│   Multiply by exp(-j·2π·100kHz·n/1MHz)                              │
│   Shifts signal down by 100 kHz (NCO / phase accumulator)           │
└────────────────────────┬────────────────────────────────────────────┘
                         │  1 MHz complex<float>
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│ ComplexResampler  (src/dsp/Resampler.cpp)                            │
│   Polyphase FIR   interpolate=12, decimate=125                       │
│   1 000 000 Hz → 96 000 Hz                                           │
└────────────────────────┬────────────────────────────────────────────┘
                         │  96 kHz complex<float>
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│ LowPassFilter  (src/dsp/LowPassFilter.cpp)                           │
│   Windowed-sinc FIR, Hamming window                                  │
│   Cutoff: variable (2.5 kHz – 15 kHz, default 10 kHz)               │
│   Transition band: 2 kHz                                             │
└────────────────────────┬────────────────────────────────────────────┘
                         │  96 kHz complex<float>
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│ CQUAMDecoder  (src/dsp/CQUAMDecoder.cpp)                             │
│   PLL  (zeta=0.707, ωN=100)  – Costas-loop-style carrier lock       │
│   VCO rotation → complex demodulation                                │
│   Envelope detector (fast max-abs approximation)                     │
│   Stereo extraction: L+R = env·cos(γ)−1,  L−R = Q/cos(γ)           │
│   Goertzel filter → 25 Hz pilot tone detection                       │
│   Biquad IIR notch (selectable: 5/9/10 kHz or variable)             │
│   Output: two float streams (L, R) at 96 kHz                         │
└─────────┬──────────────────────────────────────────┬────────────────┘
          │ L channel                                 │ R channel
          ▼                                           ▼
┌──────────────────────────┐           ┌──────────────────────────┐
│ FloatResampler (Left)    │           │ FloatResampler (Right)   │
│ 96 kHz → 48 kHz          │           │ 96 kHz → 48 kHz          │
│ interpolate=1, decimate=2│           │ interpolate=1, decimate=2│
└──────────┬───────────────┘           └────────────┬─────────────┘
           │                                         │
           └─────────────────┬───────────────────────┘
                             │  Interleaved stereo float32 @ 48 kHz
                             ▼
┌─────────────────────────────────────────────────────────────────────┐
│ AudioOutput  (src/audio/AudioOutput.cpp)                             │
│   Lock-free ring buffer (SPSC, 65536 frames)                         │
│   PortAudio callback → system sound card                             │
│   Sample rate: 48 000 Hz  Channels: 2  Format: float32              │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Thread Architecture

| Thread          | Responsibility                            | Synchronisation |
|-----------------|-------------------------------------------|-----------------|
| **GUI thread**  | Qt event loop; user interaction           | Qt signals/slots |
| **SDR I/O**     | `SoapySDR::readStream()` → DSP callback  | detached std::thread |
| **PortAudio**   | Pulls audio from ring buffer              | lock-free RingBuffer |
| **GUI timer**   | Polls `DSPPipeline` status @ 10 Hz        | atomic float reads |

DSP runs **synchronously inside the SDR I/O callback** – no extra thread needed.
This keeps latency minimal and avoids an additional queue.

---

## Key Design Decisions

### Polyphase Resampler
A windowed-sinc prototype FIR is decomposed into L polyphase branches.
For the 1 MHz → 96 kHz stage (interp=12, decim=125) this means:
- 12 polyphase branches of 16 taps each
- For each output sample: read new inputs as needed, apply one branch

### Biquad Notch Filter
Direct Form II transposed to minimise intermediate value magnitude.
Coefficients use the same formula as `scipy.signal.iirnotch`.

### Lock-Free Audio Buffer
`RingBuffer<float, 131072>` (SPSC) holds ~1.4 s of stereo audio at 48 kHz.
The SDR callback pushes samples; the PortAudio callback pops them.
No mutex required between these two threads.

---

## Tayloe Detector Integration (Phase 2)

The architecture is explicitly designed for a future hardware-native path:

```
Current (Phase 1):
  RTL-SDR @ 1 MHz  →  [FreqXlator + ComplexResampler]  →  96 kHz IF

Future (Phase 2 – RISC native):
  Tayloe Detector + Stereo ADC @ 96 kHz  →  96 kHz IF  (drop-in)
```

**To implement Phase 2:**
1. Replace `SDRDeviceManager` + `FreqXlator` + `ComplexResampler` with a
   direct audio-device capture (ALSA/WASAPI/CoreAudio) reading from the
   stereo ADC connected to the Tayloe detector.
2. Feed the resulting `complex<float>` stream at 120 kHz directly into
   `LowPassFilter` → `CQUAMDecoder`.
3. The rest of the pipeline (notch, resampler, audio out) is unchanged.

The Tayloe detector produces quadrature (I/Q) outputs natively, so:
- Left ADC channel → real (I) component
- Right ADC channel → imaginary (Q) component

This maps directly to `std::complex<float>` that `CQUAMDecoder` expects.

### Recommended RISC ADC
Any 16-bit I²S or SPI stereo ADC running at 96–120 kHz works, for example:
- PCM1802 (stereo, 96 kHz, SPI)
- WM8782 (stereo, 192 kHz max, I²S)
- CS5340 (stereo, 96 kHz, SPI/I²S)

---

## File Map

```
cpp-am-stereo-receiver/
├── CMakeLists.txt
├── docs/
│   ├── BUILD.md          ← compilation guide
│   ├── ARCHITECTURE.md   ← this file
│   └── PORTING_NOTES.md  ← Python→C++ translation notes
└── src/
    ├── main.cpp
    ├── RingBuffer.h       ← SPSC lock-free ring buffer
    ├── sdr/
    │   ├── SDRDeviceManager.h/.cpp   ← auto-detect + SoapySDR wrapper
    ├── dsp/
    │   ├── DSPPipeline.h/.cpp        ← signal chain orchestrator
    │   ├── FreqXlator.h/.cpp         ← -100 kHz frequency translation
    │   ├── Resampler.h/.cpp          ← polyphase rational resampler
    │   ├── LowPassFilter.h/.cpp      ← Hamming FIR, variable bandwidth
    │   └── CQUAMDecoder.h/.cpp       ← C-QUAM PLL + stereo decoder
    ├── audio/
    │   └── AudioOutput.h/.cpp        ← PortAudio 48 kHz stereo sink
    └── ui/
        └── MainWindow.h/.cpp         ← Qt6 GUI
```
