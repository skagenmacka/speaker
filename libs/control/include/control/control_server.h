#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace control {

struct control_state {
  std::atomic<float> *gain_db = nullptr; // pekar på central atomic
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
