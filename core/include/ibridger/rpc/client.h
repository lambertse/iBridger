#pragma once

#include "ibridger/transport/transport_factory.h"
#include "ibridger/protocol/framing.h"
#include "ibridger/protocol/envelope_codec.h"
#include "ibridger/envelope.pb.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <utility>

namespace ibridger {
namespace rpc {

struct ClientConfig {
    std::string endpoint;
    transport::TransportType transport = transport::TransportType::kAuto;
    std::chrono::seconds timeout{30};
};

/// Synchronous RPC client.
///
/// - connect() / disconnect() manage the connection lifecycle.
/// - call() is mutex-serialized: one outstanding request at a time.
/// - Generates monotonically increasing request_ids and validates that
///   the response id matches the sent id (catches protocol bugs early).
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
        const std::string& service,
        const std::string& method,
        const std::string& payload);

private:
    ClientConfig config_;
    std::atomic<uint64_t> next_request_id_{1};

    mutable std::mutex mutex_;
    std::unique_ptr<transport::ITransport> transport_;
    std::shared_ptr<protocol::FramedConnection> framed_;
    std::unique_ptr<protocol::EnvelopeCodec> codec_;
};

} // namespace rpc
} // namespace ibridger
