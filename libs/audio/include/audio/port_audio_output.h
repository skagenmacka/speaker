#pragma once
#include <cstdint>

#include "audio/ring_buffer.h"

namespace audio {

class port_audio_output {
public:
  struct config {
    int sampleRate = 44100;
    int channels = 2;
    int framesPerBuffer = 512;
  };

  port_audio_output();
  ~port_audio_output();

  void start(ring_buffer &rb, const config &cfg);
  void stop();

  struct impl;

private:
  impl *impl_ = nullptr;
};

} // namespace audio
