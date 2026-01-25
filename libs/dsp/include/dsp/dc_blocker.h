#pragma once

#include "dsp/effect.h"
#include <cmath>
#include <vector>

namespace dsp {
class dc_blocker final : public effect {
public:
  dc_blocker(double hz = 10.0) { set_cutoff(hz); }

  void set_cutoff(double hz) {
    // R = e^(-2*pi*fc/fs)
    r = static_cast<float>(std::exp(-2.0 * M_PI * hz / sample_rate));
  }

  void process(float *buf, size_t frames, int ch) noexcept override {
    if (ch <= 0) {
      return;
    }

    for (size_t f = 0; f < frames; f++) {
      for (int c = 0; c < ch; c++) {
        const size_t i = f * static_cast<size_t>(ch) + c;

        const float x = buf[i];
        const float y = x - x_prev[c] + r * y_prev[c];
        x_prev[c] = x;
        y_prev[c] = y;

        buf[i] = y;
      }
    }
  }

private:
  // std::vector<float> x_prev, y_prev;
  float x_prev[2]{0.0, 0.0}, y_prev[2]{0.0, 0.0};

  const double sample_rate = 44100.0;
  float r{0.995f};
};
} // namespace dsp