#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace control {

// standard state för att kontrollera värden
// varje värde pekar på central atomic
struct control_state {
  // gain
  std::atomic<float> *gain_db = nullptr;

  // reverb
  std::atomic<float> *reverb_delay_ms = nullptr;
  std::atomic<float> *reverb_feedback = nullptr;
  std::atomic<float> *reverb_wet = nullptr;
  std::atomic<float> *reverb_dry = nullptr;

  // dc blocker
  std::atomic<float> *dc_blocker_cutoff_hz = nullptr;
};

class control_server {
public:
  control_server(control_state state) : state(state) {}

  // t.ex. "0.0.0.0" och port 8080
  void start(const std::string &host, int port);
  void stop();

private:
  control_state state;
  std::thread thread;
  std::atomic<bool> running{false};
};

} // namespace control
