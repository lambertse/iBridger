#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ibridger/rpc/service.h"

namespace ibridger {
namespace sdk {

/// Convenience base class for implementing IService.
///
/// Subclasses register handlers in their constructor via register_method().
///
/// Untyped variant (raw payload strings):
///   register_method("DoThing", [](const std::string& p) { ... });
///
/// Typed variant (auto proto serialization):
///   register_method<EchoRequest, EchoResponse>("Echo",
///       [](const EchoRequest& req) { EchoResponse r; ...; return r; });
class ServiceBase : public rpc::IService {
 public:
  explicit ServiceBase(const std::string& name);

  std::string name() const override;
  std::vector<std::string> methods() const override;
  rpc::MethodHandler get_method(const std::string& method) const override;

 protected:
  /// Register a raw (string→string) handler.
  void register_method(const std::string& method, rpc::MethodHandler handler);

  /// Register a typed handler. TReq and TResp must be protobuf messages.
  template <typename TReq, typename TResp>
  void register_method(const std::string& method,
                       std::function<TResp(const TReq&)> handler) {
    register_method(method,
                    [handler = std::move(handler)](const std::string& payload)
                        -> std::pair<std::string, std::error_code> {
                      TReq req;
                      if (!req.ParseFromString(payload)) {
                        return {{},
                                std::make_error_code(std::errc::bad_message)};
                      }
                      TResp resp = handler(req);
                      std::string out;
                      if (!resp.SerializeToString(&out)) {
                        return {{}, std::make_error_code(std::errc::io_error)};
                      }
                      return {std::move(out), {}};
                    });
  }

 private:
  std::string name_;
  std::unordered_map<std::string, rpc::MethodHandler> handlers_;
  std::vector<std::string> method_names_;
};

}  // namespace sdk
}  // namespace ibridger
