#pragma once
#include <chrono>
#include <format>
#include <functional> // [新增]
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

// [新增] 定义日志回调函数类型：接收日志等级和格式化后的消息
using LogCallback = std::function<void(Level, const std::string &)>;

// [新增] 全局回调对象 (使用 inline 允许在头文件中定义全局变量)
inline LogCallback _GlobalLogCallback = nullptr;

// [新增] 设置回调函数的接口
inline void SetLoggerCallback(LogCallback callback) {
    _GlobalLogCallback = callback;
}

template <typename... Args>
inline void log(Level level, std::string_view fmt, Args &&...args) {
    std::string content = std::vformat(fmt, std::make_format_args(args...));

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

    // 1. 原始逻辑：输出到控制台
    std::cout << color << "[" << current_time_string() << "] "
              << "[" << level_str << "] " << content << DebugColor::RESET
              << std::endl;

    // 2. [新增] 逻辑：如果设置了回调，通知外部系统 (如 Qt)
    if (_GlobalLogCallback) {
        // 构造一个不带 ANSI 颜色码的字符串给 GUI
        std::string guiMsg = std::format("[{}] [{}] {}", current_time_string(), level_str, content);
        _GlobalLogCallback(level, guiMsg);
    }
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
        : _Name(name), _Start(std::chrono::high_resolution_clock::now()) {
        Info("Timer [{}] Started...", _Name);
    }

    void Stop() {
        auto end = std::chrono::high_resolution_clock::now();
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - _Start).count();
        double durationSec = durationMs / 1000.0;

        Info("Timer [{}] Finished. Duration: {} ms ({:.3f} s)", _Name, durationMs, durationSec);
        _IsStoped = true;
    }

    ~ScopeTimer() {
        if (!_IsStoped) {
            Stop();
        }
    }

    ScopeTimer(const ScopeTimer &) = delete;
    ScopeTimer &operator=(const ScopeTimer &) = delete;

  private:
    bool _IsStoped = false;
    std::string _Name;
    std::chrono::high_resolution_clock::time_point _Start;
};

} // namespace SuperDebug

using namespace SuperDebug;