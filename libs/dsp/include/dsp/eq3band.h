#pragma once

#include "dsp/effect.h"
#include <algorithm>
#include <cmath>

namespace dsp {

class eq3band final : public effect {
public:
  explicit eq3band(float sample_rate_hz /*, int channels = 2*/)
      : sample_rate(sample_rate_hz) /*, channels(channels)*/ {
    update_all();
  }

  void set_low_db(float db) {
    low_db = std::clamp(db, -12.0f, 12.0f);
    update_low();
  }

  void set_mid_db(float db) {
    mid_db = std::clamp(db, -12.0f, 12.0f);
    update_mid();
  }

  void set_high_db(float db) {
    high_db = std::clamp(db, -12.0f, 12.0f);
    update_high();
  }

  void process(float *interleaved, size_t frames, int ch) noexcept override {
    if (ch <= 0) {
      return;
    }

    const int chn = std::min(ch, 2);
    for (size_t f = 0; f < frames; f++) {
      for (int c = 0; c < chn; c++) {
        const size_t i = f * static_cast<size_t>(ch) + c;
        float x = interleaved[i];
        x = low.process(x, c);
        x = mid.process(x, c);
        x = high.process(x, c);
        interleaved[i] = x;
      }
    }
  }

private:
  struct biquad {
    float b0{1.0f}, b1{0.0f}, b2{0.0f}, a1{0.0f}, a2{0.0f};
    float z1[2]{0.0f, 0.0f};
    float z2[2]{0.0f, 0.0f};

    void reset() {
      z1[0] = z1[1] = 0.0f;
      z2[0] = z2[1] = 0.0f;
    }

    inline float process(float x, int ch) {
      float y = b0 * x + z1[ch];
      z1[ch] = b1 * x - a1 * y + z2[ch];
      z2[ch] = b2 * x - a2 * y;
      return y;
    }

    void set_peaking(float sample_rate, float freq_hz, float q, float db) {
      const float a = std::pow(10.0f, db / 40.0f);
      const float w0 = 2.0f * static_cast<float>(M_PI) * freq_hz / sample_rate;
      const float cw = std::cos(w0);
      const float sw = std::sin(w0);
      const float alpha = sw / (2.0f * q);

      const float b0n = 1.0f + alpha * a;
      const float b1n = -2.0f * cw;
      const float b2n = 1.0f - alpha * a;
      const float a0n = 1.0f + alpha / a;
      const float a1n = -2.0f * cw;
      const float a2n = 1.0f - alpha / a;
      set_normalized(b0n, b1n, b2n, a0n, a1n, a2n);
    }

    void set_low_shelf(float sample_rate, float freq_hz, float db) {
      const float a = std::pow(10.0f, db / 40.0f);
      const float w0 = 2.0f * static_cast<float>(M_PI) * freq_hz / sample_rate;
      const float cw = std::cos(w0);
      const float sw = std::sin(w0);
      const float alpha = sw * std::sqrt(2.0f) * 0.5f;
      const float sa = std::sqrt(a);

      const float b0n = a * ((a + 1.0f) - (a - 1.0f) * cw + 2.0f * sa * alpha);
      const float b1n = 2.0f * a * ((a - 1.0f) - (a + 1.0f) * cw);
      const float b2n = a * ((a + 1.0f) - (a - 1.0f) * cw - 2.0f * sa * alpha);
      const float a0n = (a + 1.0f) + (a - 1.0f) * cw + 2.0f * sa * alpha;
      const float a1n = -2.0f * ((a - 1.0f) + (a + 1.0f) * cw);
      const float a2n = (a + 1.0f) + (a - 1.0f) * cw - 2.0f * sa * alpha;
      set_normalized(b0n, b1n, b2n, a0n, a1n, a2n);
    }

    void set_high_shelf(float sample_rate, float freq_hz, float db) {
      const float a = std::pow(10.0f, db / 40.0f);
      const float w0 = 2.0f * static_cast<float>(M_PI) * freq_hz / sample_rate;
      const float cw = std::cos(w0);
      const float sw = std::sin(w0);
      const float alpha = sw * std::sqrt(2.0f) * 0.5f;
      const float sa = std::sqrt(a);

      const float b0n = a * ((a + 1.0f) + (a - 1.0f) * cw + 2.0f * sa * alpha);
      const float b1n = -2.0f * a * ((a - 1.0f) + (a + 1.0f) * cw);
      const float b2n = a * ((a + 1.0f) + (a - 1.0f) * cw - 2.0f * sa * alpha);
      const float a0n = (a + 1.0f) - (a - 1.0f) * cw + 2.0f * sa * alpha;
      const float a1n = 2.0f * ((a - 1.0f) - (a + 1.0f) * cw);
      const float a2n = (a + 1.0f) - (a - 1.0f) * cw - 2.0f * sa * alpha;
      set_normalized(b0n, b1n, b2n, a0n, a1n, a2n);
    }

    void set_normalized(float b0n, float b1n, float b2n, float a0n, float a1n,
                        float a2n) {
      b0 = b0n / a0n;
      b1 = b1n / a0n;
      b2 = b2n / a0n;
      a1 = a1n / a0n;
      a2 = a2n / a0n;
    }
  };

  void update_all() {
    update_low();
    update_mid();
    update_high();
  }

  void update_low() { low.set_low_shelf(sample_rate, low_freq_hz, low_db); }
  void update_mid() {
    mid.set_peaking(sample_rate, mid_freq_hz, mid_q, mid_db);
  }
  void update_high() {
    high.set_high_shelf(sample_rate, high_freq_hz, high_db);
  }

  float sample_rate;
  // int channels;

  float low_db{0.0f};
  float mid_db{0.0f};
  float high_db{0.0f};

  float low_freq_hz{120.0f};
  float mid_freq_hz{1000.0f};
  float high_freq_hz{8000.0f};
  float mid_q{0.9f};

  biquad low;
  biquad mid;
  biquad high;
};

} // namespace dsp
