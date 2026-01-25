add_library(speaker_dsp
  src/gain.cpp
)

target_include_directories(speaker_dsp PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_enable_warnings(speaker_dsp)