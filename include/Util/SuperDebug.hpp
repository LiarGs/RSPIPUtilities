#pragma once
#include <chrono>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace SuperDebug {

namespace Color {
constexpr const char *RESET = "\033[0m";
constexpr const char *RED = "\033[31m";
constexpr const char *YELLOW = "\033[33m";
constexpr const char *GREEN = "\033[32m";
constexpr const char *CYAN = "\033[36m";
constexpr const char *WHITE = "\033[37m";
} // namespace Color

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
    const char *color = Color::WHITE;

    switch (level) {
    case Level::Info:
        level_str = "info";
        color = Color::GREEN;
        break;
    case Level::Warn:
        level_str = "warning";
        color = Color::YELLOW;
        break;
    case Level::Error:
        level_str = "error";
        color = Color::RED;
        break;
    }

    std::cout << color << "[" << current_time_string() << "] "
              << "[" << level_str << "] " << formatted << Color::RESET
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

} // namespace SuperDebug

using namespace SuperDebug;