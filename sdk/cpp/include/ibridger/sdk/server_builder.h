#pragma once

#include "ibridger/rpc/server.h"
#include "ibridger/rpc/service.h"
#include "ibridger/transport/transport_factory.h"

#include <memory>
#include <string>
#include <vector>

namespace ibridger {
namespace sdk {

/// Fluent builder for ibridger::rpc::Server.
///
/// Example:
///   auto server = ServerBuilder()
///       .set_endpoint("/tmp/my.sock")
///       .add_service(std::make_shared<MyService>())
///       .set_max_connections(32)
///       .build();
///   server->start();
class ServerBuilder {
public:
    ServerBuilder& set_endpoint(const std::string& endpoint);
    ServerBuilder& set_transport(transport::TransportType type);
    ServerBuilder& add_service(std::shared_ptr<rpc::IService> service);
    ServerBuilder& set_max_connections(size_t n);
    ServerBuilder& enable_builtins(bool enabled);

    /// Build and return the configured Server.
    /// Throws std::invalid_argument if endpoint is empty.
    std::unique_ptr<rpc::Server> build() const;

private:
    rpc::ServerConfig config_;
    std::vector<std::shared_ptr<rpc::IService>> services_;
};

} // namespace sdk
} // namespace ibridger
