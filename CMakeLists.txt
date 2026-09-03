# MicroKernel OS for ESP32 Build Configuration
# Minimum CMake version required for modern ESP-IDF (v4.4+)
cmake_minimum_required(VERSION 3.16)

# Set component directories
set(EXTRA_COMPONENT_DIRS "os")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(microkernel-esp32)
