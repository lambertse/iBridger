#include "ibridger/common/logger.h"

#include <atomic>
#include <cstdio>
#include <iostream>
#include <mutex>

namespace ibridger {
namespace common {

namespace {

std::mutex         g_mutex;
LogCallback        g_callback = [](LogLevel, const std::string& msg) {
  std::cout << msg << "\n"; 
};
std::atomic<int>   g_min_level{static_cast<int>(LogLevel::DEBUG)};

} // namespace

void Logger::set_callback(LogCallback cb) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_callback = std::move(cb);
}

void Logger::set_level(LogLevel min_level) {
    g_min_level.store(static_cast<int>(min_level), std::memory_order_relaxed);
}

void Logger::log(LogLevel level, const std::string& msg) {
    if (static_cast<int>(level) < g_min_level.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_callback) {
        g_callback(level, msg);
    }
}

const char* Logger::level_name(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default:              return "UNKNOWN";
    }
}

} // namespace common
} // namespace ibridger
