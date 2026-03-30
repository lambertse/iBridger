#include "ibridger/sdk/server_builder.h"

#include <stdexcept>

namespace ibridger {
namespace sdk {

ServerBuilder& ServerBuilder::set_endpoint(const std::string& endpoint) {
    config_.endpoint = endpoint;
    return *this;
}

ServerBuilder& ServerBuilder::set_transport(transport::TransportType type) {
    config_.transport = type;
    return *this;
}

ServerBuilder& ServerBuilder::add_service(std::shared_ptr<rpc::IService> service) {
    services_.push_back(std::move(service));
    return *this;
}

ServerBuilder& ServerBuilder::set_max_connections(size_t n) {
    config_.max_connections = n;
    return *this;
}

ServerBuilder& ServerBuilder::enable_builtins(bool enabled) {
    config_.register_builtins = enabled;
    return *this;
}

std::unique_ptr<rpc::Server> ServerBuilder::build() const {
    if (config_.endpoint.empty()) {
        throw std::invalid_argument("ServerBuilder: endpoint must not be empty");
    }
    auto server = std::make_unique<rpc::Server>(config_);
    for (const auto& svc : services_) {
        server->register_service(svc);
    }
    return server;
}

} // namespace sdk
} // namespace ibridger
