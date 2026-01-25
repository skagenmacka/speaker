add_executable(speaker
  src/main.cpp
)

target_link_libraries(speaker PRIVATE
  speaker_dsp
  speaker_audio
  speaker_control
)

target_enable_warnings(speaker)