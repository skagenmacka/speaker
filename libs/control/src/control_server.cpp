#include "control/control_server.h"

// cpp-httplib
#include "httplib.h"

#include <atomic>
#include <cctype>
#include <sstream>

namespace {

std::string json_escape(const std::string &in) {
  std::string out;
  out.reserve(in.size() + 8);
  for (char ch : in) {
    switch (ch) {
    case '\\':
      out += "\\\\";
      break;
    case '"':
      out += "\\\"";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if (static_cast<unsigned char>(ch) < 0x20) {
        // skip other control chars
      } else {
        out += ch;
      }
    }
  }
  return out;
}

bool parse_json_name(const std::string &body, std::string &out) {
  const std::string key = "\"name\"";
  const size_t key_pos = body.find(key);
  if (key_pos == std::string::npos) {
    return false;
  }
  size_t pos = body.find(':', key_pos + key.size());
  if (pos == std::string::npos) {
    return false;
  }
  pos++;
  while (pos < body.size() && std::isspace(static_cast<unsigned char>(body[pos]))) {
    pos++;
  }
  if (pos >= body.size() || body[pos] != '"') {
    return false;
  }
  pos++;
  std::string value;
  while (pos < body.size()) {
    char ch = body[pos++];
    if (ch == '\\' && pos < body.size()) {
      char esc = body[pos++];
      switch (esc) {
      case '"':
      case '\\':
      case '/':
        value += esc;
        break;
      case 'n':
        value += '\n';
        break;
      case 'r':
        value += '\r';
        break;
      case 't':
        value += '\t';
        break;
      default:
        value += esc;
        break;
      }
      continue;
    }
    if (ch == '"') {
      out = value;
      return true;
    }
    value += ch;
  }
  return false;
}

} // namespace

namespace control {

void control_server::start(const std::string &host, int port) {
  if (running.exchange(true))
    return;

  thread = std::thread([this, host, port]() {
    httplib::Server svr;

    // CORS (dev): allow controller UI on localhost:5173
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "http://localhost:5173"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS, PATCH"},
        {"Access-Control-Allow-Headers", "Content-Type"},
    });

    // Preflight
    svr.Options(R"(.*)", [](const httplib::Request &, httplib::Response &res) {
      res.status = 204;
    });

    // GET /health
    svr.Get("/health", [](const httplib::Request &, httplib::Response &res) {
      res.set_content("ok\n", "text/plain");
    });

    // GET /state
    svr.Get("/state", [this](const httplib::Request &, httplib::Response &res) {
      float gain_db = 0.0f;
      if (state.gain_db) {
        gain_db = state.gain_db->load(std::memory_order_relaxed);
      }

      float reverb_delay_ms = 0.0f, reverb_feedback = 0.0f, reverb_wet = 0.0f,
            reverb_dry = 0.0f;
      if (state.reverb_delay_ms) {
        reverb_delay_ms =
            state.reverb_delay_ms->load(std::memory_order_relaxed);
      }
      if (state.reverb_feedback) {
        reverb_feedback =
            state.reverb_feedback->load(std::memory_order_relaxed);
      }
      if (state.reverb_wet) {
        reverb_wet = state.reverb_wet->load(std::memory_order_relaxed);
      }
      if (state.reverb_dry) {
        reverb_dry = state.reverb_dry->load(std::memory_order_relaxed);
      }

      float dc_blocker_cutoff_hz = 0.0f;
      if (state.dc_blocker_cutoff_hz) {
        dc_blocker_cutoff_hz =
            state.dc_blocker_cutoff_hz->load(std::memory_order_relaxed);
      }

      float eq_low_db = 0.0f, eq_mid_db = 0.0f, eq_high_db = 0.0f;
      if (state.eq_low_db) {
        eq_low_db = state.eq_low_db->load(std::memory_order_relaxed);
      }
      if (state.eq_mid_db) {
        eq_mid_db = state.eq_mid_db->load(std::memory_order_relaxed);
      }
      if (state.eq_high_db) {
        eq_high_db = state.eq_high_db->load(std::memory_order_relaxed);
      }

      std::string now_playing;
      if (state.now_playing && state.now_playing_mutex) {
        std::lock_guard<std::mutex> lock(*state.now_playing_mutex);
        now_playing = *state.now_playing;
      }

      std::ostringstream os;
      os << "{";

      os << "\"gain_db\":" << gain_db << ",";

      os << "\"reverb_delay_ms\":" << reverb_delay_ms << ",";
      os << "\"reverb_feedback\":" << reverb_feedback << ",";
      os << "\"reverb_wet\":" << reverb_wet << ",";
      os << "\"reverb_dry\":" << reverb_dry << ",";
      os << "\"dc_blocker_cutoff_hz\":" << dc_blocker_cutoff_hz << ",";

      os << "\"eq_low_db\":" << eq_low_db << ",";
      os << "\"eq_mid_db\":" << eq_mid_db << ",";
      os << "\"eq_high_db\":" << eq_high_db << ",";

      os << "\"now_playing\":\"" << json_escape(now_playing) << "\"";

      os << "}";

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

    // PATCH /state?gain_db=-6&reverb_wet=0.2
    svr.Patch("/state", [this](const httplib::Request &req,
                               httplib::Response &res) {
      int updated = 0;
      auto apply = [&](const char *name, std::atomic<float> *target,
                       float min_v, float max_v, bool clamp) -> bool {
        if (!req.has_param(name))
          return true;
        if (!target) {
          res.status = 500;
          res.set_content("param not configured\n", "text/plain");
          return false;
        }
        try {
          float v = std::stof(req.get_param_value(name));
          if (clamp) {
            if (v < min_v)
              v = min_v;
            if (v > max_v)
              v = max_v;
          }
          target->store(v, std::memory_order_relaxed);
          updated++;
          return true;
        } catch (...) {
          res.status = 400;
          res.set_content("invalid param\n", "text/plain");
          return false;
        }
      };

      if (!apply("gain_db", state.gain_db, -60.0f, 12.0f, true))
        return;
      if (!apply("reverb_delay_ms", state.reverb_delay_ms, 0.0f, 2000.0f, true))
        return;
      if (!apply("reverb_feedback", state.reverb_feedback, 0.0f, 1.0f, true))
        return;
      if (!apply("reverb_wet", state.reverb_wet, 0.0f, 1.0f, true))
        return;
      if (!apply("reverb_dry", state.reverb_dry, 0.0f, 1.0f, true))
        return;
      if (!apply("dc_blocker_cutoff_hz", state.dc_blocker_cutoff_hz, 1.0f,
                 2000.0f, true))
        return;
      if (!apply("eq_low_db", state.eq_low_db, -12.0f, 12.0f, true))
        return;
      if (!apply("eq_mid_db", state.eq_mid_db, -12.0f, 12.0f, true))
        return;
      if (!apply("eq_high_db", state.eq_high_db, -12.0f, 12.0f, true))
        return;

      if (updated == 0) {
        res.status = 400;
        res.set_content("no params\n", "text/plain");
        return;
      }

      res.set_content("ok\n", "text/plain");
    });

    // POST /now_playing?name=...
    svr.Post("/now_playing",
             [this](const httplib::Request &req, httplib::Response &res) {
               if (!state.now_playing || !state.now_playing_mutex) {
                 res.status = 500;
                 res.set_content("now_playing not configured\n", "text/plain");
                 return;
               }

               std::string name;
               bool got = false;
               if (req.has_param("name")) {
                 name = req.get_param_value("name");
                 got = true;
               } else if (!req.body.empty()) {
                 got = parse_json_name(req.body, name);
               }

               if (!got) {
                 res.status = 400;
                 res.set_content("missing name\n", "text/plain");
                 return;
               }

               {
                 std::lock_guard<std::mutex> lock(*state.now_playing_mutex);
                 *state.now_playing = name;
               }
               res.set_content("ok\n", "text/plain");
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
