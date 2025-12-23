#pragma once
#include <chrono>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace SuperDebug {

namespace DebugColor {
constexpr const char *RESET = "\033[0m";
constexpr const char *RED = "\033[31m";
constexpr const char *YELLOW = "\033[33m";
constexpr const char *GREEN = "\033[32m";
constexpr const char *CYAN = "\033[36m";
constexpr const char *WHITE = "\033[37m";
} // namespace DebugColor

inline std::string current_time_string() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto time_t_now = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time_t_now);
#else
    localtime_r(&time_t_now, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0')
        << std::setw(3) << ms.count();
    return oss.str();
}

enum class Level { Info,
                   Warn,
                   Error };

template <typename... Args>
inline void log(Level level, std::string_view fmt, Args &&...args) {
    std::string formatted = std::vformat(fmt, std::make_format_args(args...));

    std::string level_str;
    const char *color = DebugColor::WHITE;

    switch (level) {
    case Level::Info:
        level_str = "info";
        color = DebugColor::GREEN;
        break;
    case Level::Warn:
        level_str = "warning";
        color = DebugColor::YELLOW;
        break;
    case Level::Error:
        level_str = "error";
        color = DebugColor::RED;
        break;
    }

    std::cout << color << "[" << current_time_string() << "] "
              << "[" << level_str << "] " << formatted << DebugColor::RESET
              << std::endl;
}

template <typename... Args>
inline void Info(std::string_view fmt, Args &&...args) {
    log(Level::Info, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void Warn(std::string_view fmt, Args &&...args) {
    log(Level::Warn, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void Error(std::string_view fmt, Args &&...args) {
    log(Level::Error, fmt, std::forward<Args>(args)...);
}

class ScopeTimer {
  public:
    explicit ScopeTimer(std::string_view name)
        : _name(name), _start(std::chrono::high_resolution_clock::now()) {
        Info("Timer [{}] Started...", _name);
    }

    void Stop() {
        auto end = std::chrono::high_resolution_clock::now();
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - _start).count();
        double durationSec = durationMs / 1000.0;

        Info("Timer [{}] Finished. Duration: {} ms ({:.3f} s)", _name, durationMs, durationSec);
    }

    ~ScopeTimer() {
        Stop();
    }

    ScopeTimer(const ScopeTimer &) = delete;
    ScopeTimer &operator=(const ScopeTimer &) = delete;

  private:
    std::string _name;
    std::chrono::high_resolution_clock::time_point _start;
};

} // namespace SuperDebug

using namespace SuperDebug;