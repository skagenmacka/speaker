#pragma once
#include <atomic>
#include <cstddef>
#include <vector>

namespace audio {

class ring_buffer {
public:
  explicit ring_buffer(size_t capacitySamples) : buf_(capacitySamples) {}

  size_t capacity() const { return buf_.size(); }

  bool push(float s) {
    // Returns false if full
    if (count_.load(std::memory_order_acquire) >= capacity())
      return false;
    const size_t w = w_.load(std::memory_order_relaxed);
    buf_[w] = s;
    w_.store((w + 1) % capacity(), std::memory_order_release);
    count_.fetch_add(1, std::memory_order_release);
    return true;
  }

  bool pop(float &out) {
    // Returns false if empty
    if (count_.load(std::memory_order_acquire) == 0)
      return false;
    const size_t r = r_.load(std::memory_order_relaxed);
    out = buf_[r];
    r_.store((r + 1) % capacity(), std::memory_order_release);
    count_.fetch_sub(1, std::memory_order_release);
    return true;
  }

  size_t count() const { return count_.load(std::memory_order_acquire); }

private:
  std::vector<float> buf_;
  std::atomic<size_t> r_{0};
  std::atomic<size_t> w_{0};
  std::atomic<size_t> count_{0};
};

} // namespace audio
