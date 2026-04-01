#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "ibridger/rpc.pb.h"
#include "ibridger/rpc/service.h"

namespace ibridger {
namespace rpc {

/// Server-side registry mapping service names to IService implementations.
///
/// Thread-safe: all public methods may be called from multiple threads.
/// Typical usage: register all services at startup, then read-only during
/// request handling.
class ServiceRegistry {
 public:
  ServiceRegistry() = default;

  /// Register a service. Returns std::errc::file_exists if a service with
  /// the same name is already registered.
  std::error_code register_service(std::shared_ptr<IService> service);

  /// Look up a service by name. Returns nullptr if not found.
  std::shared_ptr<IService> find_service(const std::string& name) const;

  /// Return descriptors for all registered services.
  std::vector<ibridger::ServiceDescriptor> list_services() const;

  /// Number of registered services.
  size_t size() const;

 private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, std::shared_ptr<IService>> services_;
};

}  // namespace rpc
}  // namespace ibridger
