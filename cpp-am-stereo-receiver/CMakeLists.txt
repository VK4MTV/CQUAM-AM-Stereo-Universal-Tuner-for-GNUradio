cmake_minimum_required(VERSION 3.16)
project(CQUAM_AM_Stereo_Tuner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find dependencies
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
find_package(Threads REQUIRED)

# Use pkg-config for PortAudio (best on Debian/RPi)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# Collect all source files recursively
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create executable
add_executable(CQUAM_AM_Stereo_Tuner ${SOURCES})

# Include directories
target_include_directories(CQUAM_AM_Stereo_Tuner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PORTAUDIO_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(CQUAM_AM_Stereo_Tuner PRIVATE
    Qt6::Core
    Qt6::Widgets
    ${PORTAUDIO_LIBRARIES}
    SoapySDR
    Threads::Threads
)

# Qt-specific settings
set_target_properties(CQUAM_AM_Stereo_Tuner PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

message(STATUS "Project configured successfully!")
message(STATUS "Executable: CQUAM_AM_Stereo_Tuner")
message(STATUS "Qt6 found: ${Qt6_FOUND}")
message(STATUS "PortAudio: ${PORTAUDIO_LIBRARIES}")
