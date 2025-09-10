#include "tcp_server.hpp"

#include <netinet/in.h>

TcpServer::TcpServer(int port) : socket_(Socket::create_tcp_socket()) {
    socket_.set_nonblocking();

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    socket_.bind(addr);
}

std::shared_ptr<Client> TcpServer::connect_client(Socket&& socket) {
    auto client = std::make_shared<Client>(std::move(socket), loop_);
    clients_.insert(client);
    client->on_disconnect([this](const auto& client) {
        disconnect_client(client);
    });
    client->start();
    return client;
}

void TcpServer::disconnect_client(const std::shared_ptr<Client>& client) {
    clients_.erase(client);
}

void TcpServer::start(const NewClientCallback& new_client_callback) {
    socket_.listen();

    // Register main socket for reading
    loop_->add_fd(socket_.fd(), loopp::EventType::READ, [this, new_client_callback](int, loopp::EventType) {
        sockaddr_in addr{};
        int cfd = socket_.accept(addr);
        if (cfd == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                throw std::runtime_error("Failed to accept connection");
            }
            return;  // No pending connections
        }

        auto client = connect_client(Socket(cfd));
        new_client_callback(client);
    });

    loop_->start();
}

void TcpServer::close() {
    // Close all active client connections
    for (auto& client : clients_) {
        client->close();
    }

    loop_->stop();
}
