add_library(speaker_control
  src/control_server.cpp
)

target_include_directories(speaker_control PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(speaker_control PUBLIC httplib_vendor)

target_enable_warnings(speaker_control)