#pragma once

#include <memory>
#include <string>
#include <system_error>
#include <utility>

#include "ibridger/constants.pb.h"
#include "ibridger/transport/connection.h"

namespace ibridger {
namespace protocol {

/// Maximum allowed payload size per frame — sourced from constants.proto so
/// all SDK implementations share a single canonical value.
constexpr size_t kMaxFrameSize = static_cast<size_t>(ibridger::MAX_FRAME_SIZE);

/// Wraps an `IConnection` with length-prefixed framing.
///
/// Wire format: [4-byte big-endian length][payload bytes]
///
/// - Zero-length frames are valid (heartbeat / keepalive).
/// - Frames larger than kMaxFrameSize are rejected on both send and recv.
/// - FramedConnection owns the underlying IConnection.
class FramedConnection {
 public:
  explicit FramedConnection(std::unique_ptr<transport::IConnection> conn);

  /// Send `data` as a single frame.
  /// Returns an error if the payload exceeds kMaxFrameSize or the connection
  /// fails.
  std::error_code send_frame(const std::string& data);

  /// Receive the next frame.
  /// Returns ("", error) on failure. Returns ("", {}) for a zero-length frame.
  std::pair<std::string, std::error_code> recv_frame();

  /// Close the underlying connection. Safe to call from another thread to
  /// unblock a concurrent recv_frame() — it will return with an error.
  void close();

  bool is_connected() const;

 private:
  /// Read exactly `n` bytes into `buf`, looping over partial reads.
  /// Returns a connection-closed error if the peer disconnects mid-read.
  std::error_code read_exact(uint8_t* buf, size_t n);

  std::unique_ptr<transport::IConnection> conn_;
};

}  // namespace protocol
}  // namespace ibridger
