#pragma once

#include <functional>
#include <string>

namespace ibridger {
namespace common {

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

using LogCallback = std::function<void(LogLevel, const std::string&)>;

/// Lightweight, pluggable logger.
///
/// By default all messages are silently discarded. Call set_callback() to
/// route output wherever you like (stderr, syslog, a test spy, etc.).
/// Call set_level() to suppress messages below a minimum severity.
///
/// Thread-safe: callback and level are stored in atomics / a mutex-guarded
/// pointer so they can be changed at runtime from any thread.
///
/// Usage:
///   Logger::set_callback([](LogLevel l, const std::string& msg) {
///       std::cerr << msg << "\n";
///   });
///   Logger::set_level(LogLevel::INFO);
///   Logger::log(LogLevel::INFO, "server started");
class Logger {
public:
    /// Replace the active log callback. Pass nullptr to silence output.
    static void set_callback(LogCallback cb);

    /// Set the minimum level; messages below this level are dropped.
    static void set_level(LogLevel min_level);

    /// Emit a log message (no-op if below min_level or no callback set).
    static void log(LogLevel level, const std::string& msg);

    /// Convenience helpers.
    static void debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
    static void info (const std::string& msg) { log(LogLevel::INFO,  msg); }
    static void warn (const std::string& msg) { log(LogLevel::WARN,  msg); }
    static void error(const std::string& msg) { log(LogLevel::ERROR, msg); }

    /// Convert a LogLevel to a short string ("DEBUG", "INFO", …).
    static const char* level_name(LogLevel level);
};

} // namespace common
} // namespace ibridger
