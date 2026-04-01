#pragma once

#include <functional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace ibridger {
namespace rpc {

/// Handler for a single RPC method.
///
/// Receives the raw protobuf-serialized request payload and returns the
/// serialized response payload (or an error). The caller (Dispatcher) is
/// responsible for wrapping the result in an Envelope.
using MethodHandler = std::function<std::pair<std::string, std::error_code>(
    const std::string& payload)>;

/// Abstract interface for an RPC service.
///
/// Each service has a unique name, a fixed set of methods, and a handler
/// for each method. The handler receives and returns raw serialized bytes
/// so the service layer stays independent of the transport and protocol.
class IService {
 public:
  virtual ~IService() = default;

  /// Fully-qualified service name, e.g. "com.example.EchoService".
  virtual std::string name() const = 0;

  /// Names of all methods exposed by this service.
  virtual std::vector<std::string> methods() const = 0;

  /// Return the handler for `method_name`, or nullptr if not found.
  virtual MethodHandler get_method(const std::string& method_name) const = 0;
};

}  // namespace rpc
}  // namespace ibridger
