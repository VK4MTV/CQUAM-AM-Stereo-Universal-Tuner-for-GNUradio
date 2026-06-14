cmake_minimum_required(VERSION 3.16)
project(am_stereo_receiver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Required packages --------------------------------------------------------
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
find_package(SoapySDR REQUIRED)
find_package(Threads REQUIRED)

# PortAudio (try CMake config first, fall back to pkg-config)
find_package(PortAudio QUIET)
if(NOT PortAudio_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
endif()

# --- Sources ------------------------------------------------------------------
file(GLOB_RECURSE SOURCES "src/*.cpp")
# Add NoiseBlanker explicitly
list(APPEND SOURCES
    src/dsp/NoiseBlanker.cpp
)

add_executable(am_stereo_receiver ${SOURCES})

target_include_directories(am_stereo_receiver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(PortAudio_FOUND AND TARGET PortAudio::PortAudio)
    set(PORTAUDIO_TARGET PortAudio::PortAudio)
else()
    set(PORTAUDIO_TARGET ${PORTAUDIO_LIBRARIES})
    target_include_directories(am_stereo_receiver PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
endif()

target_link_libraries(am_stereo_receiver PRIVATE
    Qt6::Core
    Qt6::Widgets
    SoapySDR
    Threads::Threads
    ${PORTAUDIO_TARGET}
    soxr
)

# --- Status messages ----------------------------------------------------------
message(STATUS "✅ Configured successfully!")
message(STATUS "Executable   : am_stereo_receiver")
message(STATUS "Qt6          : ${Qt6_FOUND}")
message(STATUS "SoapySDR     : ${SoapySDR_FOUND}")
message(STATUS "PortAudio    : ${PORTAUDIO_TARGET}")
