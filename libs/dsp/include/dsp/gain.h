#pragma once
#include <algorithm>
#include <cmath>

namespace dsp {

class gain {
public:
  void set_db(float db) {
    db_ = std::clamp(db, -80.0f, 12.0f);
    linear_ = std::pow(10.0f, db_ / 20.0f);
  }

  float db() const { return db_; }
  float linear() const { return linear_; }

  inline float process(float x) const { return x * linear_; }

private:
  float db_ = 0.0f;
  float linear_ = 1.0f;
};

} // namespace dsp