#pragma once

#include <memory>
#include <string>
#include <system_error>
#include <utility>

#include "ibridger/transport/connection.h"

namespace ibridger {
namespace transport {

class ITransport {
 public:
  virtual ~ITransport() = default;

  /// Bind and start listening on `endpoint` (e.g. a socket path).
  virtual std::error_code listen(const std::string& endpoint) = 0;

  /// Block until a client connects. Returns (connection, error).
  /// Returns a non-null connection on success.
  virtual std::pair<std::unique_ptr<IConnection>, std::error_code> accept() = 0;

  /// Connect to a listening server at `endpoint`. Returns (connection, error).
  virtual std::pair<std::unique_ptr<IConnection>, std::error_code> connect(
      const std::string& endpoint) = 0;

  /// Stop accepting new connections and release the bound endpoint.
  virtual void close() = 0;
};

}  // namespace transport
}  // namespace ibridger
