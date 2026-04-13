cmake_minimum_required(VERSION 3.16)
project(CQUAM_AM_Stereo_Tuner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

# PortAudio via pkg-config (works reliably on Debian/RPi)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# Collect sources
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(CQUAM_AM_Stereo_Tuner ${SOURCES})

target_include_directories(CQUAM_AM_Stereo_Tuner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PORTAUDIO_INCLUDE_DIRS}
)

target_link_libraries(CQUAM_AM_Stereo_Tuner PRIVATE
    Qt6::Core
    Qt6::Widgets
    ${PORTAUDIO_LIBRARIES}
    SoapySDR
    Threads::Threads
)

message(STATUS "✅ Project configured successfully!")
message(STATUS "Executable: CQUAM_AM_Stereo_Tuner")
message(STATUS "Qt6: ${Qt6_FOUND}")
message(STATUS "PortAudio: ${PORTAUDIO_LIBRARIES}")
