#include "control/control_server.h"

// cpp-httplib
#include "httplib.h"

#include <sstream>

namespace control {

void control_server::start(const std::string &host, int port) {
  if (running.exchange(true))
    return;

  thread = std::thread([this, host, port]() {
    httplib::Server svr;

    // GET /health
    svr.Get("/health", [](const httplib::Request &, httplib::Response &res) {
      res.set_content("ok\n", "text/plain");
    });

    // GET /gain
    svr.Get("/gain", [this](const httplib::Request &, httplib::Response &res) {
      float db = 0.0f;
      if (state.gain_db)
        db = state.gain_db->load(std::memory_order_relaxed);

      std::ostringstream os;
      os << "{ \"gain_db\": " << db << " }\n";
      res.set_content(os.str(), "application/json");
    });

    // POST /gain?db=-6.0
    svr.Post("/gain",
             [this](const httplib::Request &req, httplib::Response &res) {
               if (!state.gain_db) {
                 res.status = 500;
                 res.set_content("gain not configured\n", "text/plain");
                 return;
               }
               if (!req.has_param("db")) {
                 res.status = 400;
                 res.set_content("missing db param\n", "text/plain");
                 return;
               }
               const auto dbStr = req.get_param_value("db");
               try {
                 float db = std::stof(dbStr);
                 // clampa rimligt
                 if (db < -60.0f)
                   db = -60.0f;
                 if (db > 12.0f)
                   db = 12.0f;

                 state.gain_db->store(db, std::memory_order_relaxed);
                 res.set_content("ok\n", "text/plain");
               } catch (...) {
                 res.status = 400;
                 res.set_content("invalid db\n", "text/plain");
               }
             });

    // Blockande lyssning (kör i separat tråd)
    svr.listen(host, port);
    running.store(false);
  });
}

void control_server::stop() {
  // Minimal variant: du kan utöka med server.stop() om du håller serverobjektet
  // som member.
  running.store(false);
  if (thread.joinable())
    thread.join();
}

} // namespace control
