// Compile test: verify that #include <ibridger/ibridger.h> exposes the full
// core API without requiring any additional includes.
#include <gtest/gtest.h>
#include "ibridger/ibridger.h"

TEST(ConvenienceHeader, TransportTypesAccessible) {
    // TransportFactory and TransportType are reachable.
    auto t = ibridger::transport::TransportFactory::create(
        ibridger::transport::TransportType::kAuto);
    (void)t;
}

TEST(ConvenienceHeader, ServerAndClientConfigAccessible) {
    ibridger::rpc::ServerConfig sc;
    sc.endpoint = "/tmp/test";

    ibridger::rpc::ClientConfig cc;
    cc.endpoint = "/tmp/test";

    // Merely constructing the configs exercises the types.
    (void)sc;
    (void)cc;
}

TEST(ConvenienceHeader, PingServiceAccessible) {
    auto svc = std::make_shared<ibridger::rpc::builtin::PingService>();
    EXPECT_EQ(svc->name(), "ibridger.Ping");
}

TEST(ConvenienceHeader, EnvelopeProtoAccessible) {
    ibridger::Envelope env;
    env.set_type(ibridger::REQUEST);
    EXPECT_EQ(env.type(), ibridger::REQUEST);
}
