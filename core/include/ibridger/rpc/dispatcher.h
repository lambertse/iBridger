#pragma once

#include <memory>

#include "ibridger/envelope.pb.h"
#include "ibridger/rpc/service_registry.h"

namespace ibridger {
namespace rpc {

/// Routes an incoming request Envelope to the correct service and method,
/// then wraps the handler result in a response Envelope.
///
/// Response rules:
///   - request_id is always copied from the request.
///   - type = RESPONSE + status = OK   on success.
///   - type = ERROR + status = NOT_FOUND  if the service or method is unknown.
///   - type = ERROR + status = INTERNAL   if the handler returns an error or
///   throws.
class Dispatcher {
 public:
  explicit Dispatcher(std::shared_ptr<ServiceRegistry> registry);

  /// Dispatch `request` and return the corresponding response Envelope.
  /// Never throws — exceptions from handlers are caught and turned into
  /// INTERNAL error responses.
  ibridger::Envelope dispatch(const ibridger::Envelope& request) const;

 private:
  std::shared_ptr<ServiceRegistry> registry_;

  static ibridger::Envelope make_error(uint64_t request_id,
                                       ibridger::StatusCode status,
                                       const std::string& message);
};

}  // namespace rpc
}  // namespace ibridger
