#include "ibridger/sdk/client_stub.h"
#include "ibridger/examples/echo.pb.h"

#include <iostream>

int main(int argc, char* argv[]) {
    const std::string endpoint = (argc > 1) ? argv[1] : "/tmp/ibridger_echo.sock";
    const std::string message  = (argc > 2) ? argv[2] : "hello ibridger";

    ibridger::rpc::ClientConfig cfg;
    cfg.endpoint = endpoint;
    ibridger::sdk::ClientStub stub(cfg);

    if (auto err = stub.connect()) {
        std::cerr << "Failed to connect: " << err.message() << "\n";
        return 1;
    }

    ibridger::examples::EchoRequest req;
    req.set_message(message);

    auto [resp, err] = stub.call<ibridger::examples::EchoRequest,
                                  ibridger::examples::EchoResponse>(
        "EchoService", "Echo", req);

    if (err) {
        std::cerr << "Call failed: " << err.message() << "\n";
        return 1;
    }

    std::cout << "Sent    : " << message << "\n";
    std::cout << "Received: " << resp.message() << "\n";
    std::cout << "Server timestamp (ms): " << resp.timestamp_ms() << "\n";
    return 0;
}
