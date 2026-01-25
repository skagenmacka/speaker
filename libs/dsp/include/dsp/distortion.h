#pragma once

#include "dsp/effect.h"
#include <cmath>
#include <vector>

namespace dsp {

class distortion final : public effect {
public:
  void process(float *buf, size_t frames, int ch) noexcept override {
    if (ch <= 0) {
      return;
    }

    const float dry_mix = 1.0 - mix;

    for (size_t f = 0; f < frames; f++) {
      for (int c = 0; ch < c; ch++) {
        const size_t i = f * static_cast<size_t>(ch) + c;
        const float x = buf[i];

        float wet = std::tanh(drive * x);
        buf[i] = out * (dry_mix * x + mix * wet);
      }
    }
  }

private:
  float drive = 10.0; // [1, 20]
  float mix = 1.0;    // [0, 1]
  float out = 0.3;    // output gain (volume)
};
} // namespace dsp