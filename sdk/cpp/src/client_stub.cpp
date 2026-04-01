#include "ibridger/sdk/client_stub.h"

namespace ibridger {
namespace sdk {

ClientStub::ClientStub(rpc::ClientConfig config) : client_(std::move(config)) {}

std::error_code ClientStub::connect() { return client_.connect(); }

void ClientStub::disconnect() { client_.disconnect(); }

bool ClientStub::is_connected() const { return client_.is_connected(); }

std::pair<ibridger::Pong, std::error_code> ClientStub::ping() {
  ibridger::Ping req;
  return call<ibridger::Ping, ibridger::Pong>("ibridger.Ping", "Ping", req);
}

std::error_code ClientStub::status_to_errc(ibridger::StatusCode status) {
  switch (status) {
    case ibridger::NOT_FOUND:
      return std::make_error_code(std::errc::no_such_file_or_directory);
    case ibridger::INVALID_ARGUMENT:
      return std::make_error_code(std::errc::invalid_argument);
    case ibridger::TIMEOUT:
      return std::make_error_code(std::errc::timed_out);
    case ibridger::INTERNAL:
    default:
      return std::make_error_code(std::errc::io_error);
  }
}

}  // namespace sdk
}  // namespace ibridger
