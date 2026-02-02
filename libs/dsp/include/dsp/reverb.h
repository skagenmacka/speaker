#pragma once
#include "effect.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dsp {

class reverb final : public effect {
public:
  explicit reverb(int sampleRate = 44100, float maxDelayMs = 2000.0f,
                  int maxChannels = 2)
      : sampleRate_(sampleRate), maxChannels_(std::max(1, maxChannels)) {

    const size_t maxDelaySamples = static_cast<size_t>(
        (maxDelayMs * 0.001f) * static_cast<float>(sampleRate_));
    // +1 för att undvika edge-cases vid wrap
    delayLineLen_ = std::max<size_t>(1, maxDelaySamples + 1);

    delayLines_.resize(static_cast<size_t>(maxChannels_) * delayLineLen_, 0.0f);
    writeIdx_.assign(static_cast<size_t>(maxChannels_), 0);

    setDelayMs(120.0f);
    setFeedback(0.25f);
    setWet(0.55f);
    setDry(0.8f);
  }

  void setDelayMs(float ms) noexcept {
    delayMs_.store(ms, std::memory_order_relaxed);
  }
  void setFeedback(float fb) noexcept {
    feedback_.store(fb, std::memory_order_relaxed);
  }
  void setWet(float wet) noexcept {
    wet_.store(wet, std::memory_order_relaxed);
  }
  void setDry(float dry) noexcept {
    dry_.store(dry, std::memory_order_relaxed);
  }

  void process(float *interleaved, size_t frames,
               int channels) noexcept override {
    if (!interleaved || frames == 0 || channels <= 0)
      return;

    const int ch = std::min(channels, maxChannels_);

    // Läs parametrar en gång per block (billigt, stabilt)
    float delayMs = delayMs_.load(std::memory_order_relaxed);
    float fb = feedback_.load(std::memory_order_relaxed);
    float wet = wet_.load(std::memory_order_relaxed);
    float dry = dry_.load(std::memory_order_relaxed);

    // Clamp för stabilitet
    delayMs = std::clamp(delayMs, 0.0f, maxDelayMs());
    fb = std::clamp(fb, 0.0f, 0.98f); // >~1 ger runaway
    wet = std::clamp(wet, 0.0f, 1.0f);
    dry = std::clamp(dry, 0.0f, 2.0f);

    const size_t delaySamples = static_cast<size_t>(
        (delayMs * 0.001f) * static_cast<float>(sampleRate_));

    for (size_t f = 0; f < frames; ++f) {
      const size_t base = f * static_cast<size_t>(channels);

      for (int c = 0; c < ch; ++c) {
        float x = interleaved[base + static_cast<size_t>(c)];

        size_t &w = writeIdx_[static_cast<size_t>(c)];
        // Läsposition = w - delaySamples (wrap)
        size_t r = (w + delayLineLen_ - (delaySamples % delayLineLen_)) %
                   delayLineLen_;

        float *line = &delayLines_[static_cast<size_t>(c) * delayLineLen_];

        float delayed = line[r];

        // feedback: skriv tillbaka input + delayed*fb
        line[w] = x + delayed * fb;

        // mix
        interleaved[base + static_cast<size_t>(c)] = dry * x + wet * delayed;

        // advance
        w = (w + 1) % delayLineLen_;
      }
    }
  }

  float maxDelayMs() const noexcept {
    return (static_cast<float>(delayLineLen_ - 1) /
            static_cast<float>(sampleRate_)) *
           1000.0f;
  }

private:
  int sampleRate_{44100};
  int maxChannels_{2};

  size_t delayLineLen_{1};
  std::vector<float> delayLines_;
  std::vector<size_t> writeIdx_;

  std::atomic<float> delayMs_{350.0f};
  std::atomic<float> feedback_{0.35f};
  std::atomic<float> wet_{0.25f};
  std::atomic<float> dry_{1.0f};
};

} // namespace dsp
