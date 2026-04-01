#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

#include "ibridger/constants.pb.h"
#include "ibridger/envelope.pb.h"
#include "ibridger/protocol/envelope_codec.h"
#include "ibridger/protocol/framing.h"
#include "ibridger/transport/transport_factory.h"

namespace ibridger {
namespace rpc {

/// Options for automatic reconnection inside call().
///
/// When set on ClientConfig, call() will attempt to reconnect with exponential
/// backoff whenever the transport is lost, instead of returning immediately.
struct ReconnectConfig {
  /// Number of reconnect attempts before giving up. -1 = unlimited.
  int max_attempts = -1;
  /// Initial backoff delay (doubles each attempt).
  std::chrono::milliseconds base_delay{200};
  /// Maximum backoff delay cap.
  std::chrono::milliseconds max_delay{10'000};
  /// Called after a successful reconnect.
  std::function<void()> on_reconnect;
};

struct ClientConfig {
  std::string endpoint;
  transport::TransportType transport = transport::TransportType::kAuto;
  std::chrono::milliseconds timeout{ibridger::DEFAULT_TIMEOUT_MS};
  /// Called when the transport closes unexpectedly (server died).
  std::function<void()> on_disconnect;
  /// When set, call() auto-reconnects instead of returning not_connected.
  std::optional<ReconnectConfig> reconnect;
};

/// Synchronous RPC client.
///
/// - connect() / disconnect() manage the connection lifecycle.
/// - call() is mutex-serialized: one outstanding request at a time.
/// - Generates monotonically increasing request_ids and validates that
///   the response id matches the sent id (catches protocol bugs early).
/// - If ClientConfig::reconnect is set, call() transparently reconnects
///   with exponential backoff when the server is lost.
class Client {
 public:
  explicit Client(ClientConfig config);
  ~Client();

  /// Connect to the server. Returns an error if already connected or if the
  /// server is not reachable.
  std::error_code connect();

  /// Close the connection. Safe to call when already disconnected.
  void disconnect();

  bool is_connected() const;

  /// Send a synchronous RPC call and return the server's response Envelope.
  ///
  /// Returns (Envelope{}, error) on transport or protocol failure.
  /// A NOT_FOUND or INTERNAL status from the server is returned as a valid
  /// Envelope with no error — the caller inspects Envelope::status().
  std::pair<ibridger::Envelope, std::error_code> call(
      const std::string& service, const std::string& method,
      const std::string& payload);

 private:
  ClientConfig config_;
  std::atomic<uint64_t> next_request_id_{1};

  mutable std::mutex mutex_;
  std::unique_ptr<transport::ITransport> transport_;
  std::shared_ptr<protocol::FramedConnection> framed_;
  std::unique_ptr<protocol::EnvelopeCodec> codec_;

  /// Inner connect — caller must already hold mutex_.
  std::error_code connect_locked();

  /// Reset connection state, fire on_disconnect, then re-acquire the lock.
  void handle_disconnect(std::unique_lock<std::mutex>& lock);

  /// Reconnect loop with exponential backoff — caller must hold mutex_.
  /// Temporarily releases the lock during sleep and connect attempts.
  std::error_code attempt_reconnect(std::unique_lock<std::mutex>& lock);
};

}  // namespace rpc
}  // namespace ibridger
