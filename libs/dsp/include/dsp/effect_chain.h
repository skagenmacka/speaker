#pragma once

#include "dsp/effect.h"

#include <memory>
#include <vector>

namespace dsp {

class EffectChain {
public:
  void add(std::unique_ptr<effect> fx) { fx_list.push_back(std::move(fx)); }

  void process(float *buf, size_t frames, int ch) noexcept {
    for (auto &e : fx_list) {
      e->process(buf, frames, ch);
    }
  }

private:
  std::vector<std::unique_ptr<effect>> fx_list;
};

} // namespace dsp