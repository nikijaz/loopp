#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

#include "tcp_server.hpp"

static constexpr int SERVER_PORT = 8080;

static TcpServer server(SERVER_PORT);

void shutdown_handler(int) {
    server.close();
}

int main() {
    // Needed for graceful shutdown
    std::signal(SIGINT, shutdown_handler);
    std::signal(SIGTERM, shutdown_handler);

    std::cout << "Server starting on port " << SERVER_PORT << '\n';
    server.start([](const auto& client) {
        client->write("Hello, World!\n");

        client->on_read([](const auto& client, const std::string& data) {
            client->write("Echo: " + data);
        });
    });

    std::cout << "Server shut down" << '\n';
    return EXIT_SUCCESS;
}
