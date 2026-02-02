#include "audio/port_audio_output.h"
#include "audio/ring_buffer.h"

#include "control/control_server.h"

#include "dsp/dc_blocker.h"
#include "dsp/distortion.h"
#include "dsp/effect_chain.h"
#include "dsp/eq3band.h"
#include "dsp/gain.h"
#include "dsp/reverb.h"

#include <atomic>
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
  std::atomic<float> reverb_delay_ms{120.0f};
  std::atomic<float> reverb_feedback{0.25f};
  std::atomic<float> reverb_wet{0.55f};
  std::atomic<float> reverb_dry{0.8f};
  std::atomic<float> dc_blocker_cutoff_hz{10.0f};
  std::atomic<float> eq_low_db{10.0f};
  std::atomic<float> eq_mid_db{0.0f};
  std::atomic<float> eq_high_db{0.0f};

  constexpr int sample_rate = 44100;
  constexpr int channels = 2;
  constexpr float buffer_seconds = .2f;

  // dsp
  dsp::gain gain;
  dsp::EffectChain effect_chain;

  auto eq = std::make_unique<dsp::eq3band>(sample_rate);
  auto *eq_ptr = eq.get();
  effect_chain.add(std::move(eq));

  auto reverb = std::make_unique<dsp::reverb>(sample_rate, 2000.0f, channels);
  auto *reverb_ptr = reverb.get();
  effect_chain.add(std::move(reverb));

  effect_chain.add(std::make_unique<dsp::distortion>());

  auto dc_blocker = std::make_unique<dsp::dc_blocker>(
      dc_blocker_cutoff_hz.load(std::memory_order_relaxed));
  auto *dc_blocker_ptr = dc_blocker.get();
  effect_chain.add(std::move(dc_blocker));

  // control server
  control::control_state state;
  state.gain_db = &gain_db;
  state.reverb_delay_ms = &reverb_delay_ms;
  state.reverb_feedback = &reverb_feedback;
  state.reverb_wet = &reverb_wet;
  state.reverb_dry = &reverb_dry;
  state.dc_blocker_cutoff_hz = &dc_blocker_cutoff_hz;
  state.eq_low_db = &eq_low_db;
  state.eq_mid_db = &eq_mid_db;
  state.eq_high_db = &eq_high_db;

  control::control_server server(state);
  server.start("0.0.0.0", 8080);

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

    gain.set_db(gain_db.load(std::memory_order_relaxed));
    for (size_t i = 0; i < samples; i++) {
      buf[i] = gain.process(buf[i]);
    }

    reverb_ptr->setDelayMs(reverb_delay_ms.load(std::memory_order_relaxed));
    reverb_ptr->setFeedback(reverb_feedback.load(std::memory_order_relaxed));
    reverb_ptr->setWet(reverb_wet.load(std::memory_order_relaxed));
    reverb_ptr->setDry(reverb_dry.load(std::memory_order_relaxed));
    dc_blocker_ptr->set_cutoff(
        dc_blocker_cutoff_hz.load(std::memory_order_relaxed));

    eq_ptr->set_low_db(eq_low_db.load(std::memory_order_relaxed));
    eq_ptr->set_mid_db(eq_mid_db.load(std::memory_order_relaxed));
    eq_ptr->set_high_db(eq_high_db.load(std::memory_order_relaxed));

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
