#pragma once

#include <cstddef>

namespace dsp {

class effect {
public:
  virtual ~effect() = default;
  virtual void process(float *interleaved, size_t frames,
                       int channels) noexcept = 0;
};

} // namespace dsp