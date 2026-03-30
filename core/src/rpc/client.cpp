#include "ibridger/rpc/client.h"
#include "ibridger/common/logger.h"
#include "ibridger/common/error.h"

namespace ibridger {
namespace rpc {

Client::Client(ClientConfig config)
    : config_(std::move(config)) {}

Client::~Client() {
    disconnect();
}

// ─── connect ──────────────────────────────────────────────────────────────────

std::error_code Client::connect() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (codec_) return common::make_error_code(common::Error::already_connected);

    transport_ = transport::TransportFactory::create(config_.transport);
    if (!transport_) return common::make_error_code(common::Error::internal);

    auto [conn, err] = transport_->connect(config_.endpoint);
    if (err) {
        common::Logger::error("Client: connect failed: " + err.message());
        return err;
    }

    framed_ = std::make_shared<protocol::FramedConnection>(std::move(conn));
    codec_  = std::make_unique<protocol::EnvelopeCodec>(framed_);
    common::Logger::info("Client: connected to " + config_.endpoint);
    return {};
}

// ─── disconnect ───────────────────────────────────────────────────────────────

void Client::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (framed_) {
        framed_->close();
        framed_.reset();
        common::Logger::info("Client: disconnected");
    }
    codec_.reset();
    transport_.reset();
}

// ─── is_connected ─────────────────────────────────────────────────────────────

bool Client::is_connected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return framed_ && framed_->is_connected();
}

// ─── call ─────────────────────────────────────────────────────────────────────

std::pair<ibridger::Envelope, std::error_code> Client::call(
    const std::string& service,
    const std::string& method,
    const std::string& payload) {

    std::lock_guard<std::mutex> lock(mutex_);

    if (!codec_) {
        return {ibridger::Envelope{}, common::make_error_code(common::Error::not_connected)};
    }

    const uint64_t id = next_request_id_++;

    ibridger::Envelope request;
    request.set_type(ibridger::REQUEST);
    request.set_request_id(id);
    request.set_service_name(service);
    request.set_method_name(method);
    request.set_payload(payload);

    if (auto err = codec_->send(request); err) {
        return {ibridger::Envelope{}, err};
    }

    auto [response, recv_err] = codec_->recv();
    if (recv_err) {
        return {ibridger::Envelope{}, recv_err};
    }

    // Validate correlation — a mismatch indicates a protocol bug.
    if (response.request_id() != id) {
        common::Logger::error("Client: request_id mismatch (sent " +
            std::to_string(id) + ", got " +
            std::to_string(response.request_id()) + ")");
        return {ibridger::Envelope{},
                common::make_error_code(common::Error::protocol_error)};
    }

    return {std::move(response), {}};
}

} // namespace rpc
} // namespace ibridger
