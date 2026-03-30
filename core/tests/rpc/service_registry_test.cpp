#include <gtest/gtest.h>
#include "ibridger/rpc/service_registry.h"
#include "ibridger/common/error.h"

#include <algorithm>

using ibridger::rpc::IService;
using ibridger::rpc::MethodHandler;
using ibridger::rpc::ServiceRegistry;

namespace {

/// Minimal concrete IService for testing.
class FakeService : public IService {
public:
    FakeService(std::string name,
                std::vector<std::string> method_names,
                std::unordered_map<std::string, MethodHandler> handlers = {})
        : name_(std::move(name))
        , methods_(std::move(method_names))
        , handlers_(std::move(handlers)) {}

    std::string name() const override { return name_; }
    std::vector<std::string> methods() const override { return methods_; }

    MethodHandler get_method(const std::string& method_name) const override {
        auto it = handlers_.find(method_name);
        return it != handlers_.end() ? it->second : nullptr;
    }

private:
    std::string name_;
    std::vector<std::string> methods_;
    std::unordered_map<std::string, MethodHandler> handlers_;
};

std::shared_ptr<FakeService> make_echo_service() {
    MethodHandler echo_handler = [](const std::string& payload)
        -> std::pair<std::string, std::error_code> {
        return {payload, {}};  // echo: return payload unchanged
    };
    return std::make_shared<FakeService>(
        "com.example.EchoService",
        std::vector<std::string>{"Echo", "EchoError"},
        std::unordered_map<std::string, MethodHandler>{{"Echo", echo_handler}});
}

} // namespace

// ─── Register and find ────────────────────────────────────────────────────────

TEST(ServiceRegistry, RegisterAndFind) {
    ServiceRegistry registry;
    ASSERT_FALSE(registry.register_service(make_echo_service()));

    auto svc = registry.find_service("com.example.EchoService");
    ASSERT_NE(svc, nullptr);
    EXPECT_EQ(svc->name(), "com.example.EchoService");
}

// ─── Find method and invoke handler ──────────────────────────────────────────

TEST(ServiceRegistry, FindMethodAndInvokeHandler) {
    ServiceRegistry registry;
    ASSERT_FALSE(registry.register_service(make_echo_service()));

    auto svc = registry.find_service("com.example.EchoService");
    ASSERT_NE(svc, nullptr);

    auto handler = svc->get_method("Echo");
    ASSERT_NE(handler, nullptr);

    auto [response, err] = handler("hello");
    EXPECT_FALSE(err) << err.message();
    EXPECT_EQ(response, "hello");
}

// ─── Null handler for unknown method ─────────────────────────────────────────

TEST(ServiceRegistry, GetMethodReturnsNullForUnknownMethod) {
    ServiceRegistry registry;
    ASSERT_FALSE(registry.register_service(make_echo_service()));

    auto svc = registry.find_service("com.example.EchoService");
    ASSERT_NE(svc, nullptr);

    auto handler = svc->get_method("NonExistent");
    EXPECT_EQ(handler, nullptr);
}

// ─── Service not found ────────────────────────────────────────────────────────

TEST(ServiceRegistry, FindServiceReturnsNullIfNotRegistered) {
    ServiceRegistry registry;
    EXPECT_EQ(registry.find_service("ghost.Service"), nullptr);
}

// ─── Duplicate registration ───────────────────────────────────────────────────

TEST(ServiceRegistry, DuplicateRegistrationReturnsError) {
    ServiceRegistry registry;
    ASSERT_FALSE(registry.register_service(make_echo_service()));

    auto err = registry.register_service(make_echo_service());
    EXPECT_EQ(err, ibridger::common::make_error_code(ibridger::common::Error::already_registered))
        << "Expected already_registered for duplicate, got: " << err.message();

    // Registry should still contain exactly one entry.
    EXPECT_EQ(registry.size(), 1u);
}

// ─── List services ────────────────────────────────────────────────────────────

TEST(ServiceRegistry, ListServicesReturnsCorrectDescriptors) {
    ServiceRegistry registry;

    ASSERT_FALSE(registry.register_service(make_echo_service()));
    ASSERT_FALSE(registry.register_service(std::make_shared<FakeService>(
        "com.example.PingService",
        std::vector<std::string>{"Ping"})));

    auto descriptors = registry.list_services();
    ASSERT_EQ(descriptors.size(), 2u);

    // Order is unspecified (unordered_map) — find each by name.
    auto find_desc = [&](const std::string& name) {
        auto it = std::find_if(descriptors.begin(), descriptors.end(),
            [&](const ibridger::ServiceDescriptor& d) { return d.name() == name; });
        return it != descriptors.end() ? &(*it) : nullptr;
    };

    const auto* echo_desc = find_desc("com.example.EchoService");
    ASSERT_NE(echo_desc, nullptr);
    EXPECT_EQ(echo_desc->methods_size(), 2);

    const auto* ping_desc = find_desc("com.example.PingService");
    ASSERT_NE(ping_desc, nullptr);
    EXPECT_EQ(ping_desc->methods_size(), 1);
    EXPECT_EQ(ping_desc->methods(0), "Ping");
}

// ─── Multiple services coexist ────────────────────────────────────────────────

TEST(ServiceRegistry, MultipleServicesCoexist) {
    ServiceRegistry registry;

    for (int i = 0; i < 5; i++) {
        auto svc = std::make_shared<FakeService>(
            "Service" + std::to_string(i),
            std::vector<std::string>{"Method"});
        ASSERT_FALSE(registry.register_service(svc));
    }

    EXPECT_EQ(registry.size(), 5u);
    for (int i = 0; i < 5; i++) {
        EXPECT_NE(registry.find_service("Service" + std::to_string(i)), nullptr);
    }
}
