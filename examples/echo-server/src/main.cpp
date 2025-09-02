#include <csignal>
#include <iostream>

#include "tcp_server.hpp"

constexpr int SERVER_PORT = 8080;

static TcpServer server(SERVER_PORT);

void shutdown_handler(int) {
    server.close();
}

int main() {
    // Needed for graceful shutdown
    std::signal(SIGINT, shutdown_handler);
    std::signal(SIGTERM, shutdown_handler);

    std::cout << "Server starting on port " << SERVER_PORT << std::endl;
    server.start([](auto client) {
        client->write("Hello, World!\n");

        client->on_read([](auto client, const std::string& data) {
            client->write("Echo: " + data);
        });
    });

    std::cout << "Server shut down" << std::endl;
    return EXIT_SUCCESS;
}
