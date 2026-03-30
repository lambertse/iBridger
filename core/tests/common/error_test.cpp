#include <gtest/gtest.h>
#include "ibridger/common/error.h"

using ibridger::common::Error;
using ibridger::common::ibridger_category;
using ibridger::common::make_error_code;

// ─── Category name ────────────────────────────────────────────────────────────

TEST(IbridgerError, CategoryNameIsIbridger) {
    EXPECT_STREQ(ibridger_category().name(), "ibridger");
}

// ─── Error messages ───────────────────────────────────────────────────────────

TEST(IbridgerError, OkMessageIsSuccess) {
    EXPECT_EQ(make_error_code(Error::ok).message(), "success");
}

TEST(IbridgerError, AllCodesHaveNonEmptyMessages) {
    const Error codes[] = {
        Error::ok,
        Error::not_connected,
        Error::already_connected,
        Error::already_registered,
        Error::service_not_found,
        Error::method_not_found,
        Error::protocol_error,
        Error::frame_too_large,
        Error::serialization_error,
        Error::timeout,
        Error::internal,
    };
    for (auto code : codes) {
        EXPECT_FALSE(make_error_code(code).message().empty())
            << "Empty message for code " << static_cast<int>(code);
    }
}

// ─── Distinct codes don't collide ────────────────────────────────────────────

TEST(IbridgerError, CodesAreDistinct) {
    EXPECT_NE(make_error_code(Error::not_connected),
              make_error_code(Error::already_connected));
    EXPECT_NE(make_error_code(Error::service_not_found),
              make_error_code(Error::method_not_found));
}

// ─── Category separation from std ────────────────────────────────────────────

TEST(IbridgerError, CategoryDiffersFromSystemCategory) {
    auto ibridger_ec = make_error_code(Error::not_connected);
    auto system_ec   = std::make_error_code(std::errc::not_connected);
    // Same integer value but different categories — must not be equal.
    EXPECT_NE(ibridger_ec, system_ec);
}

// ─── is_error_code_enum specialisation ───────────────────────────────────────

TEST(IbridgerError, ImplicitConversionToErrorCode) {
    // If the is_error_code_enum specialisation is correct this compiles
    // and produces a valid error_code.
    std::error_code ec = Error::timeout;
    EXPECT_EQ(ec.category(), ibridger_category());
    EXPECT_EQ(ec.value(), static_cast<int>(Error::timeout));
    EXPECT_FALSE(ec.message().empty());
}

// ─── ok code evaluates to false ──────────────────────────────────────────────

TEST(IbridgerError, OkCodeIsFalsy) {
    std::error_code ec = Error::ok;
    EXPECT_FALSE(ec);
}

TEST(IbridgerError, NonOkCodeIsTruthy) {
    std::error_code ec = Error::internal;
    EXPECT_TRUE(ec);
}
