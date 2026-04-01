#include "ibridger/rpc/dispatcher.h"

#include <exception>

namespace ibridger {
namespace rpc {

Dispatcher::Dispatcher(std::shared_ptr<ServiceRegistry> registry)
    : registry_(std::move(registry)) {}

ibridger::Envelope Dispatcher::dispatch(
    const ibridger::Envelope& request) const {
  const uint64_t id = request.request_id();

  // ── Locate service ────────────────────────────────────────────────────────
  auto service = registry_->find_service(request.service_name());
  if (!service) {
    return make_error(id, ibridger::NOT_FOUND,
                      "service '" + request.service_name() + "' not found");
  }

  // ── Locate method ─────────────────────────────────────────────────────────
  auto handler = service->get_method(request.method_name());
  if (!handler) {
    return make_error(id, ibridger::NOT_FOUND,
                      "method '" + request.method_name() +
                          "' not found on service '" + request.service_name() +
                          "'");
  }

  // ── Invoke handler ────────────────────────────────────────────────────────
  try {
    auto [payload, err] = handler(request.payload());
    if (err) {
      return make_error(id, ibridger::INTERNAL, err.message());
    }

    ibridger::Envelope response;
    response.set_type(ibridger::RESPONSE);
    response.set_request_id(id);
    response.set_status(ibridger::OK);
    response.set_payload(std::move(payload));
    return response;

  } catch (const std::exception& e) {
    return make_error(id, ibridger::INTERNAL,
                      std::string("handler threw: ") + e.what());
  } catch (...) {
    return make_error(id, ibridger::INTERNAL,
                      "handler threw unknown exception");
  }
}

ibridger::Envelope Dispatcher::make_error(uint64_t request_id,
                                          ibridger::StatusCode status,
                                          const std::string& message) {
  ibridger::Envelope response;
  response.set_type(ibridger::ERROR);
  response.set_request_id(request_id);
  response.set_status(status);
  response.set_error_message(message);
  return response;
}

}  // namespace rpc
}  // namespace ibridger
