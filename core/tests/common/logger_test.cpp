#include <gtest/gtest.h>
#include "ibridger/common/logger.h"

#include <vector>
#include <string>

using ibridger::common::Logger;
using ibridger::common::LogLevel;

// RAII guard: restores logger to a silent, DEBUG-level state after each test.
struct LoggerGuard {
    ~LoggerGuard() {
        Logger::set_callback(nullptr);
        Logger::set_level(LogLevel::DEBUG);
    }
};

// ─── Callback receives messages ───────────────────────────────────────────────

TEST(Logger, CallbackReceivesMessage) {
    LoggerGuard g;
    std::vector<std::string> received;
    Logger::set_callback([&](LogLevel, const std::string& msg) {
        received.push_back(msg);
    });

    Logger::info("hello logger");

    ASSERT_EQ(received.size(), 1u);
    EXPECT_EQ(received[0], "hello logger");
}

TEST(Logger, CallbackReceivesLevel) {
    LoggerGuard g;
    std::vector<LogLevel> levels;
    Logger::set_callback([&](LogLevel l, const std::string&) {
        levels.push_back(l);
    });

    Logger::debug("d");
    Logger::info("i");
    Logger::warn("w");
    Logger::error("e");

    ASSERT_EQ(levels.size(), 4u);
    EXPECT_EQ(levels[0], LogLevel::DEBUG);
    EXPECT_EQ(levels[1], LogLevel::INFO);
    EXPECT_EQ(levels[2], LogLevel::WARN);
    EXPECT_EQ(levels[3], LogLevel::ERROR);
}

// ─── Level filtering ──────────────────────────────────────────────────────────

TEST(Logger, MessagesBeloMinLevelAreDropped) {
    LoggerGuard g;
    std::vector<LogLevel> received;
    Logger::set_callback([&](LogLevel l, const std::string&) {
        received.push_back(l);
    });

    Logger::set_level(LogLevel::WARN);
    Logger::debug("dropped");
    Logger::info("dropped");
    Logger::warn("kept");
    Logger::error("kept");

    ASSERT_EQ(received.size(), 2u);
    EXPECT_EQ(received[0], LogLevel::WARN);
    EXPECT_EQ(received[1], LogLevel::ERROR);
}

TEST(Logger, ErrorLevelOnlyReceivesErrors) {
    LoggerGuard g;
    int count = 0;
    Logger::set_callback([&](LogLevel, const std::string&) { ++count; });

    Logger::set_level(LogLevel::ERROR);
    Logger::debug("no");
    Logger::info("no");
    Logger::warn("no");
    Logger::error("yes");

    EXPECT_EQ(count, 1);
}

// ─── Null callback is silent ──────────────────────────────────────────────────

TEST(Logger, NullCallbackIsNoOp) {
    LoggerGuard g;
    Logger::set_callback(nullptr);
    // Must not crash.
    EXPECT_NO_THROW(Logger::info("should not crash"));
}

// ─── Replacing callback ───────────────────────────────────────────────────────

TEST(Logger, ReplacingCallbackTakesEffect) {
    LoggerGuard g;
    int first = 0, second = 0;
    Logger::set_callback([&](LogLevel, const std::string&) { ++first; });
    Logger::info("a");

    Logger::set_callback([&](LogLevel, const std::string&) { ++second; });
    Logger::info("b");

    EXPECT_EQ(first,  1);
    EXPECT_EQ(second, 1);
}

// ─── level_name helper ────────────────────────────────────────────────────────

TEST(Logger, LevelNameReturnsCorrectStrings) {
    EXPECT_STREQ(Logger::level_name(LogLevel::DEBUG), "DEBUG");
    EXPECT_STREQ(Logger::level_name(LogLevel::INFO),  "INFO");
    EXPECT_STREQ(Logger::level_name(LogLevel::WARN),  "WARN");
    EXPECT_STREQ(Logger::level_name(LogLevel::ERROR), "ERROR");
}
