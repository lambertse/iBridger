#include "ibridger/rpc/dispatcher.h"

#include <gtest/gtest.h>

#include "ibridger/rpc/service_registry.h"

using ibridger::rpc::Dispatcher;
using ibridger::rpc::IService;
using ibridger::rpc::MethodHandler;
using ibridger::rpc::ServiceRegistry;

namespace {

class FakeService : public IService {
 public:
  FakeService(std::string name,
              std::unordered_map<std::string, MethodHandler> handlers)
      : name_(std::move(name)), handlers_(std::move(handlers)) {}

  std::string name() const override { return name_; }

  std::vector<std::string> methods() const override {
    std::vector<std::string> m;
    for (const auto& [k, _] : handlers_) m.push_back(k);
    return m;
  }

  MethodHandler get_method(const std::string& method) const override {
    auto it = handlers_.find(method);
    return it != handlers_.end() ? it->second : nullptr;
  }

 private:
  std::string name_;
  std::unordered_map<std::string, MethodHandler> handlers_;
};

/// Builds a registry with one "EchoService" that has:
///   - "Echo"      → returns payload unchanged
///   - "EchoError" → returns an error_code
///   - "EchoThrow" → throws a std::runtime_error
std::shared_ptr<ServiceRegistry> make_registry() {
  auto registry = std::make_shared<ServiceRegistry>();

  std::unordered_map<std::string, MethodHandler> handlers;

  handlers["Echo"] = [](const std::string& payload)
      -> std::pair<std::string, std::error_code> { return {payload, {}}; };

  handlers["EchoError"] =
      [](const std::string&) -> std::pair<std::string, std::error_code> {
    return {"", std::make_error_code(std::errc::io_error)};
  };

  handlers["EchoThrow"] =
      [](const std::string&) -> std::pair<std::string, std::error_code> {
    throw std::runtime_error("handler exploded");
    return {"", {}};
  };

  registry->register_service(
      std::make_shared<FakeService>("EchoService", handlers));
  return registry;
}

/// Helper: build a minimal REQUEST envelope.
ibridger::Envelope make_request(uint64_t id, const std::string& service,
                                const std::string& method,
                                const std::string& payload = "") {
  ibridger::Envelope env;
  env.set_type(ibridger::REQUEST);
  env.set_request_id(id);
  env.set_service_name(service);
  env.set_method_name(method);
  env.set_payload(payload);
  return env;
}

}  // namespace

// ─── Successful dispatch
// ──────────────────────────────────────────────────────

TEST(Dispatcher, SuccessfulDispatchReturnsHandlerOutput) {
  Dispatcher dispatcher(make_registry());

  auto response =
      dispatcher.dispatch(make_request(1, "EchoService", "Echo", "hello"));

  EXPECT_EQ(response.type(), ibridger::RESPONSE);
  EXPECT_EQ(response.status(), ibridger::OK);
  EXPECT_EQ(response.payload(), "hello");
  EXPECT_EQ(response.request_id(), 1ULL);
  EXPECT_EQ(response.error_message(), "");
}

// ─── Unknown service → NOT_FOUND ─────────────────────────────────────────────

TEST(Dispatcher, UnknownServiceReturnsNotFound) {
  Dispatcher dispatcher(make_registry());

  auto response =
      dispatcher.dispatch(make_request(42, "ghost.Service", "DoThing"));

  EXPECT_EQ(response.type(), ibridger::ERROR);
  EXPECT_EQ(response.status(), ibridger::NOT_FOUND);
  EXPECT_EQ(response.request_id(), 42ULL);
  EXPECT_NE(response.error_message().find("ghost.Service"), std::string::npos);
}

// ─── Unknown method → NOT_FOUND
// ───────────────────────────────────────────────

TEST(Dispatcher, UnknownMethodReturnsNotFound) {
  Dispatcher dispatcher(make_registry());

  auto response =
      dispatcher.dispatch(make_request(7, "EchoService", "NonExistentMethod"));

  EXPECT_EQ(response.type(), ibridger::ERROR);
  EXPECT_EQ(response.status(), ibridger::NOT_FOUND);
  EXPECT_EQ(response.request_id(), 7ULL);
  EXPECT_NE(response.error_message().find("NonExistentMethod"),
            std::string::npos);
  EXPECT_NE(response.error_message().find("EchoService"), std::string::npos);
}

// ─── Handler returns error → INTERNAL ────────────────────────────────────────

TEST(Dispatcher, HandlerErrorReturnsInternal) {
  Dispatcher dispatcher(make_registry());

  auto response =
      dispatcher.dispatch(make_request(99, "EchoService", "EchoError"));

  EXPECT_EQ(response.type(), ibridger::ERROR);
  EXPECT_EQ(response.status(), ibridger::INTERNAL);
  EXPECT_EQ(response.request_id(), 99ULL);
  EXPECT_FALSE(response.error_message().empty());
}

// ─── Handler throws → INTERNAL, never crashes ────────────────────────────────

TEST(Dispatcher, HandlerExceptionReturnsInternalNeverCrashes) {
  Dispatcher dispatcher(make_registry());

  auto response =
      dispatcher.dispatch(make_request(55, "EchoService", "EchoThrow"));

  EXPECT_EQ(response.type(), ibridger::ERROR);
  EXPECT_EQ(response.status(), ibridger::INTERNAL);
  EXPECT_EQ(response.request_id(), 55ULL);
  EXPECT_NE(response.error_message().find("handler exploded"),
            std::string::npos);
}

// ─── request_id preserved in all outcomes ────────────────────────────────────

TEST(Dispatcher, RequestIdAlwaysPreserved) {
  Dispatcher dispatcher(make_registry());

  const uint64_t kId = 0xDEADBEEFCAFEBABEULL;

  // Success path
  EXPECT_EQ(dispatcher.dispatch(make_request(kId, "EchoService", "Echo", "x"))
                .request_id(),
            kId);

  // Unknown service
  EXPECT_EQ(
      dispatcher.dispatch(make_request(kId, "no.Service", "M")).request_id(),
      kId);

  // Unknown method
  EXPECT_EQ(dispatcher.dispatch(make_request(kId, "EchoService", "NoMethod"))
                .request_id(),
            kId);

  // Handler error
  EXPECT_EQ(dispatcher.dispatch(make_request(kId, "EchoService", "EchoError"))
                .request_id(),
            kId);

  // Handler throws
  EXPECT_EQ(dispatcher.dispatch(make_request(kId, "EchoService", "EchoThrow"))
                .request_id(),
            kId);
}
