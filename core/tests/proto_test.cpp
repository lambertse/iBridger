#include <gtest/gtest.h>

#include "ibridger/envelope.pb.h"
#include "ibridger/rpc.pb.h"

TEST(ProtoEnvelope, RoundtripAllFields) {
  ibridger::Envelope env;
  env.set_type(ibridger::REQUEST);
  env.set_request_id(42);
  env.set_service_name("my.Service");
  env.set_method_name("DoWork");
  env.set_payload("binary-payload");
  env.set_status(ibridger::OK);
  env.set_error_message("");
  (*env.mutable_metadata())["key1"] = "value1";
  (*env.mutable_metadata())["key2"] = "value2";

  std::string serialized;
  ASSERT_TRUE(env.SerializeToString(&serialized));

  ibridger::Envelope parsed;
  ASSERT_TRUE(parsed.ParseFromString(serialized));

  EXPECT_EQ(parsed.type(), ibridger::REQUEST);
  EXPECT_EQ(parsed.request_id(), 42u);
  EXPECT_EQ(parsed.service_name(), "my.Service");
  EXPECT_EQ(parsed.method_name(), "DoWork");
  EXPECT_EQ(parsed.payload(), "binary-payload");
  EXPECT_EQ(parsed.status(), ibridger::OK);
  EXPECT_EQ(parsed.metadata().at("key1"), "value1");
  EXPECT_EQ(parsed.metadata().at("key2"), "value2");
}

TEST(ProtoEnvelope, ErrorEnvelope) {
  ibridger::Envelope env;
  env.set_type(ibridger::ERROR);
  env.set_request_id(99);
  env.set_status(ibridger::NOT_FOUND);
  env.set_error_message("service not found");

  std::string serialized;
  ASSERT_TRUE(env.SerializeToString(&serialized));

  ibridger::Envelope parsed;
  ASSERT_TRUE(parsed.ParseFromString(serialized));

  EXPECT_EQ(parsed.type(), ibridger::ERROR);
  EXPECT_EQ(parsed.status(), ibridger::NOT_FOUND);
  EXPECT_EQ(parsed.error_message(), "service not found");
}

TEST(ProtoPingPong, RoundtripPing) {
  ibridger::Ping ping;
  ping.set_client_id("client-123");

  std::string serialized;
  ASSERT_TRUE(ping.SerializeToString(&serialized));

  ibridger::Ping parsed;
  ASSERT_TRUE(parsed.ParseFromString(serialized));

  EXPECT_EQ(parsed.client_id(), "client-123");
}

TEST(ProtoPingPong, RoundtripPong) {
  ibridger::Pong pong;
  pong.set_server_id("server-abc");
  pong.set_timestamp_ms(1700000000000LL);

  std::string serialized;
  ASSERT_TRUE(pong.SerializeToString(&serialized));

  ibridger::Pong parsed;
  ASSERT_TRUE(parsed.ParseFromString(serialized));

  EXPECT_EQ(parsed.server_id(), "server-abc");
  EXPECT_EQ(parsed.timestamp_ms(), 1700000000000LL);
}

TEST(ProtoServiceDescriptor, RoundtripDescriptor) {
  ibridger::ServiceDescriptor desc;
  desc.set_name("my.Service");
  desc.add_methods("MethodA");
  desc.add_methods("MethodB");

  std::string serialized;
  ASSERT_TRUE(desc.SerializeToString(&serialized));

  ibridger::ServiceDescriptor parsed;
  ASSERT_TRUE(parsed.ParseFromString(serialized));

  EXPECT_EQ(parsed.name(), "my.Service");
  ASSERT_EQ(parsed.methods_size(), 2);
  EXPECT_EQ(parsed.methods(0), "MethodA");
  EXPECT_EQ(parsed.methods(1), "MethodB");
}
