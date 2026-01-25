#include "audio/port_audio_output.h"
#include "audio/ring_buffer.h"

#include <portaudio.h>
#include <stdexcept>

namespace audio {

struct port_audio_output::impl {
  ring_buffer *rb = nullptr;
  config cfg{};
  PaStream *stream = nullptr;
};

static int callback(const void *, void *output, unsigned long frameCount,
                    const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags,
                    void *userData) {
  auto *impl = static_cast<port_audio_output::impl *>(userData);
  float *out = static_cast<float *>(output);

  const unsigned long total =
      frameCount * static_cast<unsigned long>(impl->cfg.channels);
  for (unsigned long i = 0; i < total; ++i) {
    float s;
    if (impl->rb->pop(s))
      out[i] = s;
    else
      out[i] = 0.0f; // underrun => silence
  }
  return paContinue;
}

port_audio_output::port_audio_output() : impl_(new impl()) {}

port_audio_output::~port_audio_output() {
  try {
    stop();
  } catch (...) {
  }
  delete impl_;
}

void port_audio_output::start(ring_buffer &rb, const config &cfg) {
  impl_->rb = &rb;
  impl_->cfg = cfg;

  PaError e = Pa_Initialize();
  if (e != paNoError)
    throw std::runtime_error("Pa_Initialize failed");

  e = Pa_OpenDefaultStream(&impl_->stream, 0, cfg.channels, paFloat32,
                           cfg.sampleRate, cfg.framesPerBuffer, callback,
                           impl_);
  if (e != paNoError)
    throw std::runtime_error("Pa_OpenDefaultStream failed");

  e = Pa_StartStream(impl_->stream);
  if (e != paNoError)
    throw std::runtime_error("Pa_StartStream failed");
}

void port_audio_output::stop() {
  if (impl_->stream) {
    Pa_StopStream(impl_->stream);
    Pa_CloseStream(impl_->stream);
    impl_->stream = nullptr;
    Pa_Terminate();
  }
}

} // namespace audio
