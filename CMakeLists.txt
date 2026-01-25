cmake_minimum_required(VERSION 3.20)
project(speaker LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(Warnings)

# header only
add_library(httplib_vendor INTERFACE)

target_include_directories(httplib_vendor INTERFACE
  ${CMAKE_SOURCE_DIR}/third_party/httplib
)

add_subdirectory(libs/dsp)
add_subdirectory(libs/audio)
add_subdirectory(libs/control)

add_subdirectory(apps/speaker)
