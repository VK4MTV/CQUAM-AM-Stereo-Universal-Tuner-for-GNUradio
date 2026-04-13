cmake_minimum_required(VERSION 3.16)
project(CQUAM_AM_Stereo_Tuner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Useful for development
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(PortAudio REQUIRED)
find_package(SoapySDR REQUIRED)

# Collect all source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.c"
)

# Create the executable
add_executable(CQUAM_AM_Stereo_Tuner ${SOURCES})

# Include directories
target_include_directories(CQUAM_AM_Stereo_Tuner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sdr
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp
    # Add any other subfolders you have under src/
)

# Link libraries
target_link_libraries(CQUAM_AM_Stereo_Tuner PRIVATE
    PortAudio::PortAudio
    SoapySDR
    Threads::Threads
    # Add any other libraries you use (e.g. FFTW, liquid-dsp, etc.) here
)

# Optional: Install target (makes "make install" useful)
install(TARGETS CQUAM_AM_Stereo_Tuner
    RUNTIME DESTINATION bin
)

message(STATUS "Project configured successfully!")
message(STATUS "Executable name: CQUAM_AM_Stereo_Tuner")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
