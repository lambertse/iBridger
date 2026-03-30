#include <gtest/gtest.h>
#include "ibridger/rpc/client.h"
#include "ibridger/rpc/server.h"
#include "ibridger/rpc/service.h"

#include <unistd.h>

using ibridger::rpc::Client;
using ibridger::rpc::ClientConfig;
using ibridger::rpc::IService;
using ibridger::rpc::MethodHandler;
using ibridger::rpc::Server;
using ibridger::rpc::ServerConfig;

namespace {

std::string make_socket_path() {
    char tpl[] = "/tmp/ibridger_client_test_XXXXXX";
    int fd = ::mkstemp(tpl);
    ::close(fd);
    ::unlink(tpl);
    return tpl;
}

class EchoService : public IService {
public:
    std::string name() const override { return "EchoService"; }
    std::vector<std::string> methods() const override { return {"Echo"}; }
    MethodHandler get_method(const std::string& m) const override {
        if (m == "Echo") {
            return [](const std::string& p) -> std::pair<std::string, std::error_code> {
                return {p, {}};
            };
        }
        return nullptr;
    }
};

/// RAII helper: starts a server on construction, stops on destruction.
struct TestServer {
    std::string path;
    Server server;

    explicit TestServer(const std::string& p)
        : path(p), server([&] {
            ServerConfig cfg;
            cfg.endpoint = p;
            return cfg;
        }()) {
        server.register_service(std::make_shared<EchoService>());
        auto err = server.start();
        if (err) throw std::runtime_error("server start failed: " + err.message());
    }
    ~TestServer() {
        server.stop();
        ::unlink(path.c_str());
    }
};

} // namespace

// ─── Connect / disconnect lifecycle ──────────────────────────────────────────

TEST(Client, ConnectDisconnectLifecycle) {
    auto path = make_socket_path();
    TestServer srv(path);

    ClientConfig cfg;
    cfg.endpoint = path;
    Client client(cfg);

    EXPECT_FALSE(client.is_connected());

    ASSERT_FALSE(client.connect());
    EXPECT_TRUE(client.is_connected());

    client.disconnect();
    EXPECT_FALSE(client.is_connected());
}

// ─── Successful call roundtrip ────────────────────────────────────────────────

TEST(Client, SuccessfulCallRoundtrip) {
    auto path = make_socket_path();
    TestServer srv(path);

    ClientConfig cfg;
    cfg.endpoint = path;
    Client client(cfg);
    ASSERT_FALSE(client.connect());

    auto [resp, err] = client.call("EchoService", "Echo", "hello client");

    ASSERT_FALSE(err) << err.message();
    EXPECT_EQ(resp.type(),    ibridger::RESPONSE);
    EXPECT_EQ(resp.status(),  ibridger::OK);
    EXPECT_EQ(resp.payload(), "hello client");
}

// ─── NOT_FOUND propagation ────────────────────────────────────────────────────

TEST(Client, NotFoundErrorPropagation) {
    auto path = make_socket_path();
    TestServer srv(path);

    ClientConfig cfg;
    cfg.endpoint = path;
    Client client(cfg);
    ASSERT_FALSE(client.connect());

    auto [resp, err] = client.call("ghost.Service", "DoThing", "");

    // Transport succeeded — the server replied with an error Envelope.
    ASSERT_FALSE(err) << err.message();
    EXPECT_EQ(resp.type(),   ibridger::ERROR);
    EXPECT_EQ(resp.status(), ibridger::NOT_FOUND);
    EXPECT_FALSE(resp.error_message().empty());
}

// ─── Connection refused when no server running ────────────────────────────────

TEST(Client, ConnectionRefusedWhenNoServer) {
    ClientConfig cfg;
    cfg.endpoint = "/tmp/ibridger_no_server_" + std::to_string(::getpid());
    Client client(cfg);

    auto err = client.connect();
    EXPECT_TRUE(err) << "Expected connection error, got success";
    EXPECT_FALSE(client.is_connected());
}

// ─── Multiple sequential calls on same connection ─────────────────────────────

TEST(Client, MultipleSequentialCalls) {
    auto path = make_socket_path();
    TestServer srv(path);

    ClientConfig cfg;
    cfg.endpoint = path;
    Client client(cfg);
    ASSERT_FALSE(client.connect());

    for (int i = 0; i < 10; i++) {
        std::string payload = "msg-" + std::to_string(i);
        auto [resp, err] = client.call("EchoService", "Echo", payload);

        ASSERT_FALSE(err) << "call " << i << " failed: " << err.message();
        EXPECT_EQ(resp.type(),    ibridger::RESPONSE);
        EXPECT_EQ(resp.status(),  ibridger::OK);
        EXPECT_EQ(resp.payload(), payload);
        // request_id increments with each call
        EXPECT_EQ(resp.request_id(), static_cast<uint64_t>(i + 1));
    }
}

// ─── call() before connect returns not_connected ──────────────────────────────

TEST(Client, CallBeforeConnectReturnsError) {
    ClientConfig cfg;
    cfg.endpoint = "/tmp/irrelevant";
    Client client(cfg);

    auto [resp, err] = client.call("EchoService", "Echo", "hi");
    EXPECT_EQ(err, std::make_error_code(std::errc::not_connected));
}

// ─── Reconnect after disconnect ───────────────────────────────────────────────

TEST(Client, ReconnectAfterDisconnect) {
    auto path = make_socket_path();
    TestServer srv(path);

    ClientConfig cfg;
    cfg.endpoint = path;
    Client client(cfg);

    ASSERT_FALSE(client.connect());
    auto [r1, e1] = client.call("EchoService", "Echo", "first");
    ASSERT_FALSE(e1);
    EXPECT_EQ(r1.payload(), "first");

    client.disconnect();
    EXPECT_FALSE(client.is_connected());

    ASSERT_FALSE(client.connect());
    auto [r2, e2] = client.call("EchoService", "Echo", "second");
    ASSERT_FALSE(e2);
    EXPECT_EQ(r2.payload(), "second");
}
