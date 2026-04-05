cmake_minimum_required(VERSION 3.16)
project(AMStereoReceiver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O3 -march=native -ffast-math)
endif()

# ── Dependencies ─────────────────────────────────────────────────────────────

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

# SoapySDR
find_package(SoapySDR REQUIRED)
include_directories(${SoapySDR_INCLUDE_DIRS})

# PortAudio
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
include_directories(${PORTAUDIO_INCLUDE_DIRS})

# ── Sources ───────────────────────────────────────────────────────────────────

set(SOURCES
    src/main.cpp
    src/sdr/SDRDeviceManager.cpp
    src/dsp/DSPPipeline.cpp
    src/dsp/FreqXlator.cpp
    src/dsp/Resampler.cpp
    src/dsp/LowPassFilter.cpp
    src/dsp/CQUAMDecoder.cpp
    src/audio/AudioOutput.cpp
    src/ui/MainWindow.cpp
)

set(HEADERS
    src/sdr/SDRDeviceManager.h
    src/dsp/DSPPipeline.h
    src/dsp/FreqXlator.h
    src/dsp/Resampler.h
    src/dsp/LowPassFilter.h
    src/dsp/CQUAMDecoder.h
    src/audio/AudioOutput.h
    src/ui/MainWindow.h
    src/RingBuffer.h
)

# ── Executable ───────────────────────────────────────────────────────────────

qt_add_executable(am_stereo_receiver ${SOURCES} ${HEADERS})

target_include_directories(am_stereo_receiver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SoapySDR_INCLUDE_DIRS}
    ${PORTAUDIO_INCLUDE_DIRS}
)

target_link_libraries(am_stereo_receiver PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${SoapySDR_LIBRARIES}
    ${PORTAUDIO_LIBRARIES}
)

# ── Install ───────────────────────────────────────────────────────────────────

install(TARGETS am_stereo_receiver
    RUNTIME DESTINATION bin
)

# ── Summary ───────────────────────────────────────────────────────────────────

message(STATUS "")
message(STATUS "AMStereoReceiver ${PROJECT_VERSION}")
message(STATUS "  Build type  : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Qt6         : ${Qt6_VERSION}")
message(STATUS "  SoapySDR    : ${SoapySDR_VERSION}")
message(STATUS "  Install to  : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
