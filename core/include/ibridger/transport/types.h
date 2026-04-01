#pragma once

#include <cstdint>
#include <string>
#include <system_error>

namespace ibridger {
namespace transport {

using ConnectionId = uint64_t;

enum class TransportError {
  kOk = 0,
  kConnectionRefused,
  kConnectionReset,
  kConnectionClosed,
  kAddressInUse,
  kNotSupported,
  kTimeout,
  kInvalidArgument,
  kInternalError,
};

class TransportErrorCategory : public std::error_category {
 public:
  const char* name() const noexcept override { return "ibridger.transport"; }

  std::string message(int ev) const override {
    switch (static_cast<TransportError>(ev)) {
      case TransportError::kOk:
        return "success";
      case TransportError::kConnectionRefused:
        return "connection refused";
      case TransportError::kConnectionReset:
        return "connection reset";
      case TransportError::kConnectionClosed:
        return "connection closed";
      case TransportError::kAddressInUse:
        return "address already in use";
      case TransportError::kNotSupported:
        return "transport not supported on this platform";
      case TransportError::kTimeout:
        return "operation timed out";
      case TransportError::kInvalidArgument:
        return "invalid argument";
      case TransportError::kInternalError:
        return "internal transport error";
      default:
        return "unknown transport error";
    }
  }
};

inline const std::error_category& transport_error_category() {
  static TransportErrorCategory instance;
  return instance;
}

inline std::error_code make_transport_error(TransportError e) {
  return {static_cast<int>(e), transport_error_category()};
}

}  // namespace transport
}  // namespace ibridger
