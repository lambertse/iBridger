#include "ibridger/common/error.h"

namespace ibridger {
namespace common {

namespace {

struct IbridgerErrorCategory : std::error_category {
  const char* name() const noexcept override { return "ibridger"; }

  std::string message(int ev) const override {
    switch (static_cast<Error>(ev)) {
      case Error::ok:
        return "success";
      case Error::not_connected:
        return "not connected";
      case Error::already_connected:
        return "already connected";
      case Error::already_registered:
        return "service already registered";
      case Error::service_not_found:
        return "service not found";
      case Error::method_not_found:
        return "method not found";
      case Error::protocol_error:
        return "protocol error";
      case Error::frame_too_large:
        return "frame too large";
      case Error::serialization_error:
        return "serialization error";
      case Error::timeout:
        return "operation timed out";
      case Error::internal:
        return "internal error";
      default:
        return "unknown ibridger error";
    }
  }
};

}  // namespace

const std::error_category& ibridger_category() {
  static IbridgerErrorCategory instance;
  return instance;
}

}  // namespace common
}  // namespace ibridger
