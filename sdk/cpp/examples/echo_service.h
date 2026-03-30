#pragma once

#include "ibridger/sdk/service_base.h"
#include "ibridger/examples/echo.pb.h"

#include <chrono>

namespace ibridger {
namespace examples {

/// Example service: uppercases the incoming message and stamps a timestamp.
class EchoService : public sdk::ServiceBase {
public:
    EchoService() : ServiceBase("EchoService") {
        register_method<ibridger::examples::EchoRequest,
                        ibridger::examples::EchoResponse>(
            "Echo",
            [](const ibridger::examples::EchoRequest& req) {
                ibridger::examples::EchoResponse resp;

                std::string upper = req.message();
                for (auto& c : upper) c = static_cast<char>(::toupper(c));
                resp.set_message(upper);

                const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                resp.set_timestamp_ms(static_cast<int64_t>(now_ms));

                return resp;
            });
    }
};

} // namespace examples
} // namespace ibridger
