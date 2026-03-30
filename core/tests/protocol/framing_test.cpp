#include <gtest/gtest.h>
#include "ibridger/protocol/framing.h"
#include "ibridger/transport/unix_socket_transport.h"
#include "ibridger/common/error.h"

#include <sys/socket.h>
#include <unistd.h>
#include <thread>

using ibridger::protocol::FramedConnection;
using ibridger::protocol::kMaxFrameSize;
using ibridger::transport::UnixSocketConnection;

namespace {

/// Creates a connected socket pair and returns two FramedConnections.
/// fds[0] → first, fds[1] → second.
std::pair<FramedConnection, FramedConnection> make_framed_pair() {
    int fds[2];
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
        throw std::runtime_error("socketpair failed");
    }
    return {
        FramedConnection(std::make_unique<UnixSocketConnection>(fds[0], 1)),
        FramedConnection(std::make_unique<UnixSocketConnection>(fds[1], 2)),
    };
}

} // namespace

// ─── Single frame roundtrip ───────────────────────────────────────────────────

TEST(FramedConnection, SingleFrameRoundtrip) {
    auto [sender, receiver] = make_framed_pair();

    const std::string msg = "hello framed world";
    ASSERT_FALSE(sender.send_frame(msg));

    auto [frame, err] = receiver.recv_frame();
    EXPECT_FALSE(err) << err.message();
    EXPECT_EQ(frame, msg);
}

// ─── Empty frame roundtrip ────────────────────────────────────────────────────

TEST(FramedConnection, EmptyFrameRoundtrip) {
    auto [sender, receiver] = make_framed_pair();

    ASSERT_FALSE(sender.send_frame(""));

    auto [frame, err] = receiver.recv_frame();
    EXPECT_FALSE(err) << err.message();
    EXPECT_EQ(frame, "");
    EXPECT_EQ(frame.size(), 0u);
}

// ─── Multiple sequential frames ───────────────────────────────────────────────

TEST(FramedConnection, MultipleSequentialFrames) {
    auto [sender, receiver] = make_framed_pair();

    const std::vector<std::string> messages = {
        "frame one",
        "",               // empty heartbeat
        "frame three",
        std::string(1024, 'A'),  // 1 KB
    };

    for (const auto& msg : messages) {
        ASSERT_FALSE(sender.send_frame(msg)) << "send failed for: " << msg;
    }

    for (const auto& expected : messages) {
        auto [frame, err] = receiver.recv_frame();
        EXPECT_FALSE(err) << err.message();
        EXPECT_EQ(frame, expected);
    }
}

// ─── send_frame rejects oversized payload ─────────────────────────────────────

TEST(FramedConnection, SendRejectsOversizedPayload) {
    auto [sender, receiver] = make_framed_pair();

    // Construct a string one byte over the limit without initialising content
    // (avoids a 16 MB zero-fill — the size check happens before any I/O).
    std::string oversized;
    oversized.resize(kMaxFrameSize + 1);

    auto err = sender.send_frame(oversized);
    EXPECT_EQ(err, ibridger::common::make_error_code(ibridger::common::Error::frame_too_large));
}

// ─── recv_frame rejects oversized length header ───────────────────────────────

TEST(FramedConnection, RecvRejectsOversizedLengthHeader) {
    int fds[2];
    ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    FramedConnection receiver(std::make_unique<UnixSocketConnection>(fds[1], 2));

    // Manually write a header claiming 32 MB (no payload follows).
    constexpr uint32_t kBadLen = 32u * 1024u * 1024u;
    uint8_t header[4] = {
        static_cast<uint8_t>((kBadLen >> 24) & 0xFF),
        static_cast<uint8_t>((kBadLen >> 16) & 0xFF),
        static_cast<uint8_t>((kBadLen >>  8) & 0xFF),
        static_cast<uint8_t>((kBadLen >>  0) & 0xFF),
    };
    ::write(fds[0], header, 4);
    ::close(fds[0]);

    auto [frame, err] = receiver.recv_frame();
    EXPECT_EQ(err, ibridger::common::make_error_code(ibridger::common::Error::frame_too_large));
}

// ─── recv_frame returns error on mid-frame disconnect ─────────────────────────

TEST(FramedConnection, RecvErrorOnPeerDisconnectMidFrame) {
    int fds[2];
    ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    FramedConnection receiver(std::make_unique<UnixSocketConnection>(fds[1], 2));

    // Write a header saying 100 bytes, then close without sending payload.
    constexpr uint32_t kLen = 100;
    uint8_t header[4] = {0, 0, 0, static_cast<uint8_t>(kLen)};
    ::write(fds[0], header, 4);
    ::close(fds[0]);

    auto [frame, err] = receiver.recv_frame();
    EXPECT_TRUE(err) << "Expected an error on mid-frame disconnect";
    EXPECT_EQ(err, std::make_error_code(std::errc::connection_reset));
}

// ─── Bidirectional exchange ───────────────────────────────────────────────────

TEST(FramedConnection, BidirectionalExchange) {
    auto pair = make_framed_pair();
    FramedConnection& a = pair.first;
    FramedConnection& b = pair.second;

    const std::string ping = "ping";
    const std::string pong = "pong";

    std::thread t([&b, &ping, &pong]() {
        auto [frame, err] = b.recv_frame();
        ASSERT_FALSE(err);
        EXPECT_EQ(frame, ping);
        ASSERT_FALSE(b.send_frame(pong));
    });

    ASSERT_FALSE(a.send_frame(ping));
    auto [frame, err] = a.recv_frame();
    EXPECT_FALSE(err) << err.message();
    EXPECT_EQ(frame, pong);

    t.join();
}
