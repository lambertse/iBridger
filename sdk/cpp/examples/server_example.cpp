#include "ibridger/sdk/server_builder.h"
#include "echo_service.h"

#include <csignal>
#include <cstdlib>
#include <iostream>

static volatile bool g_running = true;

int main(int argc, char* argv[]) {
    const std::string endpoint = (argc > 1) ? argv[1] : "/tmp/ibridger_echo.sock";

    auto server = ibridger::sdk::ServerBuilder()
        .set_endpoint(endpoint)
        .add_service(std::make_shared<ibridger::examples::EchoService>())
        .build();

    auto err = server->start();
    if (err) {
        std::cerr << "Failed to start server: " << err.message() << "\n";
        return 1;
    }

    std::cout << "Echo server listening on " << endpoint << "\n";
    std::cout << "Press Ctrl-C to stop.\n";

    ::signal(SIGINT,  [](int) { g_running = false; });
    ::signal(SIGTERM, [](int) { g_running = false; });

    while (g_running) ::pause();

    server->stop();
    std::cout << "Server stopped.\n";
    return 0;
}
