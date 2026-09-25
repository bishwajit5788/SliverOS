# SliverOS for ESP32-S3 Build Configuration
# Minimum CMake version required for modern ESP-IDF (v5.0+)
cmake_minimum_required(VERSION 3.16)

# Set component directories
set(EXTRA_COMPONENT_DIRS "os" "os/main")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(microkernel-esp32)
