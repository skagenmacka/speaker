find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

add_library(speaker_audio
  src/port_audio_output.cpp
)

target_include_directories(speaker_audio PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${PORTAUDIO_INCLUDE_DIRS}
)

target_link_directories(speaker_audio PUBLIC
  ${PORTAUDIO_LIBRARY_DIRS}
)

target_link_libraries(speaker_audio PUBLIC
  ${PORTAUDIO_LIBRARIES}
)

target_compile_options(speaker_audio PUBLIC
  ${PORTAUDIO_CFLAGS_OTHER}
)

target_enable_warnings(speaker_audio)
