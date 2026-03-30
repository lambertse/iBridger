#include "ibridger/sdk/service_base.h"

namespace ibridger {
namespace sdk {

ServiceBase::ServiceBase(const std::string& name) : name_(name) {}

std::string ServiceBase::name() const {
    return name_;
}

std::vector<std::string> ServiceBase::methods() const {
    return method_names_;
}

rpc::MethodHandler ServiceBase::get_method(const std::string& method) const {
    auto it = handlers_.find(method);
    if (it == handlers_.end()) return nullptr;
    return it->second;
}

void ServiceBase::register_method(const std::string& method, rpc::MethodHandler handler) {
    if (handlers_.find(method) == handlers_.end()) {
        method_names_.push_back(method);
    }
    handlers_[method] = std::move(handler);
}

} // namespace sdk
} // namespace ibridger
