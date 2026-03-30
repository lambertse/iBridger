#pragma once

#include "ibridger/rpc/client.h"
#include "ibridger/rpc.pb.h"

#include <string>
#include <system_error>
#include <utility>

namespace ibridger {
namespace sdk {

/// High-level RPC client with typed call support.
///
/// Example:
///   ClientStub stub(ClientConfig{"/tmp/my.sock"});
///   stub.connect();
///   auto [resp, err] = stub.call<EchoRequest, EchoResponse>(
///       "EchoService", "Echo", request);
class ClientStub {
public:
    explicit ClientStub(rpc::ClientConfig config);

    std::error_code connect();
    void disconnect();
    bool is_connected() const;

    /// Typed call: serializes TReq, sends, deserializes TResp.
    /// Returns (TResp{}, error) on transport/protocol/parse failure.
    /// Server-side NOT_FOUND/INTERNAL are returned as std::errc::no_such_file_or_directory
    /// / std::errc::io_error respectively.
    template <typename TReq, typename TResp>
    std::pair<TResp, std::error_code> call(
        const std::string& service,
        const std::string& method,
        const TReq& request) {

        std::string payload;
        if (!request.SerializeToString(&payload)) {
            return {TResp{}, std::make_error_code(std::errc::invalid_argument)};
        }

        auto [env, err] = client_.call(service, method, payload);
        if (err) return {TResp{}, err};

        if (env.status() != ibridger::OK) {
            return {TResp{}, status_to_errc(env.status())};
        }

        TResp resp;
        if (!resp.ParseFromString(env.payload())) {
            return {TResp{}, std::make_error_code(std::errc::bad_message)};
        }
        return {std::move(resp), {}};
    }

    /// Convenience: send a Ping and return the Pong.
    std::pair<ibridger::Pong, std::error_code> ping();

private:
    rpc::Client client_;

    static std::error_code status_to_errc(ibridger::StatusCode status);
};

} // namespace sdk
} // namespace ibridger
