#include "ibridger/rpc/client.h"

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

    if (codec_) return std::make_error_code(std::errc::already_connected);

    transport_ = transport::TransportFactory::create(config_.transport);
    if (!transport_) return std::make_error_code(std::errc::not_supported);

    auto [conn, err] = transport_->connect(config_.endpoint);
    if (err) return err;

    framed_ = std::make_shared<protocol::FramedConnection>(std::move(conn));
    codec_  = std::make_unique<protocol::EnvelopeCodec>(framed_);
    return {};
}

// ─── disconnect ───────────────────────────────────────────────────────────────

void Client::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (framed_) {
        framed_->close();
        framed_.reset();
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
        return {ibridger::Envelope{}, std::make_error_code(std::errc::not_connected)};
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
        return {ibridger::Envelope{},
                std::make_error_code(std::errc::protocol_error)};
    }

    return {std::move(response), {}};
}

} // namespace rpc
} // namespace ibridger
