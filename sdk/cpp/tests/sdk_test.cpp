#include <gtest/gtest.h>
#include "ibridger/sdk/server_builder.h"
#include "ibridger/sdk/client_stub.h"
#include "ibridger/sdk/service_base.h"
#include "ibridger/rpc.pb.h"

#include <unistd.h>

using ibridger::sdk::ClientStub;
using ibridger::sdk::ServerBuilder;
using ibridger::sdk::ServiceBase;
using ibridger::rpc::ClientConfig;

namespace {

std::string make_socket_path() {
    char tpl[] = "/tmp/ibridger_sdk_test_XXXXXX";
    int fd = ::mkstemp(tpl);
    ::close(fd);
    ::unlink(tpl);
    return tpl;
}

// ─── A simple typed service via ServiceBase ───────────────────────────────────

class EchoService : public ServiceBase {
public:
    EchoService() : ServiceBase("EchoService") {
        register_method<ibridger::Ping, ibridger::Pong>(
            "Echo",
            [](const ibridger::Ping& req) {
                ibridger::Pong resp;
                resp.set_server_id(req.client_id());  // echo the client_id back
                resp.set_timestamp_ms(42);
                return resp;
            });
    }
};

struct TestServer {
    std::string path;
    std::unique_ptr<ibridger::rpc::Server> server;

    explicit TestServer(const std::string& p,
                        bool add_echo = false,
                        bool builtins = true)
        : path(p) {
        ServerBuilder builder;
        builder.set_endpoint(p).enable_builtins(builtins);
        if (add_echo) builder.add_service(std::make_shared<EchoService>());
        server = builder.build();
        auto err = server->start();
        if (err) throw std::runtime_error("server start failed: " + err.message());
    }
    ~TestServer() {
        server->stop();
        ::unlink(path.c_str());
    }
};

} // namespace

// ─── ServerBuilder ────────────────────────────────────────────────────────────

TEST(ServerBuilder, ThrowsOnEmptyEndpoint) {
    EXPECT_THROW(ServerBuilder().build(), std::invalid_argument);
}

TEST(ServerBuilder, ProducesWorkingServer) {
    auto path = make_socket_path();
    TestServer srv(path);
    EXPECT_TRUE(srv.server->is_running());
}

TEST(ServerBuilder, MaxConnectionsPropagated) {
    auto path = make_socket_path();
    // Just verify it builds without error.
    auto server = ServerBuilder()
        .set_endpoint(path)
        .set_max_connections(4)
        .build();
    ASSERT_NE(server, nullptr);
}

// ─── ServiceBase ─────────────────────────────────────────────────────────────

TEST(ServiceBase, MethodRegistrationAndMetadata) {
    EchoService svc;
    EXPECT_EQ(svc.name(), "EchoService");
    ASSERT_EQ(svc.methods().size(), 1u);
    EXPECT_EQ(svc.methods()[0], "Echo");
}

TEST(ServiceBase, UnknownMethodReturnsNull) {
    EchoService svc;
    EXPECT_EQ(svc.get_method("NoSuchMethod"), nullptr);
}

TEST(ServiceBase, TypedHandlerRoundtrip) {
    EchoService svc;
    auto handler = svc.get_method("Echo");
    ASSERT_NE(handler, nullptr);

    ibridger::Ping req;
    req.set_client_id("hello");
    std::string payload;
    ASSERT_TRUE(req.SerializeToString(&payload));

    auto [resp_payload, err] = handler(payload);
    ASSERT_FALSE(err) << err.message();

    ibridger::Pong pong;
    ASSERT_TRUE(pong.ParseFromString(resp_payload));
    EXPECT_EQ(pong.server_id(), "hello");
    EXPECT_EQ(pong.timestamp_ms(), 42);
}

TEST(ServiceBase, TypedHandlerRejectsMalformedPayload) {
    EchoService svc;
    auto handler = svc.get_method("Echo");
    ASSERT_NE(handler, nullptr);

    auto [out, err] = handler("\x01\x02\x03 not proto");
    EXPECT_TRUE(err);
}

// ─── ClientStub ──────────────────────────────────────────────────────────────

TEST(ClientStub, ConnectDisconnect) {
    auto path = make_socket_path();
    TestServer srv(path);

    ClientConfig cfg;
    cfg.endpoint = path;
    ClientStub stub(cfg);

    EXPECT_FALSE(stub.is_connected());
    ASSERT_FALSE(stub.connect());
    EXPECT_TRUE(stub.is_connected());
    stub.disconnect();
    EXPECT_FALSE(stub.is_connected());
}

TEST(ClientStub, CallWhenDisconnectedReturnsError) {
    ClientConfig cfg;
    cfg.endpoint = "/tmp/irrelevant";
    ClientStub stub(cfg);

    ibridger::Ping req;
    auto [resp, err] = stub.call<ibridger::Ping, ibridger::Pong>(
        "ibridger.Ping", "Ping", req);
    EXPECT_TRUE(err);
}

TEST(ClientStub, TypedCallRoundtrip) {
    auto path = make_socket_path();
    TestServer srv(path, /*add_echo=*/true);

    ClientConfig cfg;
    cfg.endpoint = path;
    ClientStub stub(cfg);
    ASSERT_FALSE(stub.connect());

    ibridger::Ping req;
    req.set_client_id("sdk-test");
    auto [pong, err] = stub.call<ibridger::Ping, ibridger::Pong>(
        "EchoService", "Echo", req);

    ASSERT_FALSE(err) << err.message();
    EXPECT_EQ(pong.server_id(), "sdk-test");
    EXPECT_EQ(pong.timestamp_ms(), 42);
}

TEST(ClientStub, PingConvenience) {
    auto path = make_socket_path();
    TestServer srv(path, /*add_echo=*/false, /*builtins=*/true);

    ClientConfig cfg;
    cfg.endpoint = path;
    ClientStub stub(cfg);
    ASSERT_FALSE(stub.connect());

    auto [pong, err] = stub.ping();
    ASSERT_FALSE(err) << err.message();
    EXPECT_EQ(pong.server_id(), "ibridger-server");
    EXPECT_GT(pong.timestamp_ms(), 0);
}

TEST(ClientStub, NotFoundMapsToErrc) {
    auto path = make_socket_path();
    TestServer srv(path);

    ClientConfig cfg;
    cfg.endpoint = path;
    ClientStub stub(cfg);
    ASSERT_FALSE(stub.connect());

    ibridger::Ping req;
    auto [resp, err] = stub.call<ibridger::Ping, ibridger::Pong>(
        "ghost.Service", "Nope", req);
    EXPECT_EQ(err, std::make_error_code(std::errc::no_such_file_or_directory));
}
