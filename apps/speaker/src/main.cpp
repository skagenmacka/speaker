#include "audio/port_audio_output.h"
#include "audio/ring_buffer.h"

#include "control/control_server.h"

#include "dsp/dc_blocker.h"
#include "dsp/distortion.h"
#include "dsp/effect_chain.h"
#include "dsp/gain.h"
#include "dsp/reverb.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <thread>

static float s16_to_float(int16_t v) {
  return static_cast<float>(v) / 32768.0f;
}

int main(/* int argc, char **argv */) {
  std::cout << "Startar högtalarsystem...\n";

  // shared state
  std::atomic<float> gain_db{0.0f};

  // dsp
  // dsp::Gain gain;
  // gain.set_db(gainDb);
  dsp::EffectChain effect_chain;
  effect_chain.add(std::make_unique<dsp::reverb>());
  effect_chain.add(std::make_unique<dsp::distortion>());
  effect_chain.add(std::make_unique<dsp::dc_blocker>());

  // control server
  control::control_state state;
  state.gain_db = &gain_db;

  control::control_server server(state);
  server.start("0.0.0.0", 8080);

  constexpr int sample_rate = 44100;
  constexpr int channels = 2;
  constexpr float buffer_seconds = .2f;
  audio::ring_buffer rb(static_cast<size_t>(sample_rate) * channels *
                        buffer_seconds);

  audio::port_audio_output out;
  out.start(rb, audio::port_audio_output::config{.sampleRate = sample_rate,
                                                 .channels = channels,
                                                 .framesPerBuffer = 512});

  constexpr size_t IN_FRAMES = 1024;
  int16_t in[IN_FRAMES * channels];
  std::vector<float> buf(IN_FRAMES * channels);

  while (true) {
    const size_t samples =
        std::fread(in, sizeof(int16_t), IN_FRAMES * channels, stdin);

    if (samples == 0) {
      if (std::ferror(stdin)) {
        std::perror("std fread");
      }

      break;
    }

    const size_t frames = samples / channels;

    // convert to float
    for (size_t i = 0; i < samples; i++) {
      buf[i] = s16_to_float(in[i]);
    }

    effect_chain.process(buf.data(), frames, channels);

    for (size_t i = 0; i < samples; ++i) {
      // Backpressure: wait if buffer full
      while (!rb.push(buf[i])) {
        std::this_thread::sleep_for(std::chrono::microseconds(200));
      }
    }
  }

  out.stop();
  return 0;
}
