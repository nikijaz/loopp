#include "tcp_server.hpp"

#include <netinet/in.h>
#include <sys/socket.h>

#include <cerrno>
#include <cstdint>
#include <memory>
#include <system_error>
#include <utility>

#include "client.hpp"
#include "loopp/event_loop.hpp"
#include "socket.hpp"

TcpServer::TcpServer(int port) : socket_(Socket::create_tcp_socket()) {
    if (!socket_.set_reuse_addr()) {
        throw std::system_error(errno, std::system_category(), "Failed to set SO_REUSEADDR");
    }

    if (!socket_.set_nonblocking()) {
        throw std::system_error(errno, std::system_category(), "Failed to set non-blocking");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (!socket_.bind(addr)) {
        throw std::system_error(errno, std::system_category(), "Failed to bind socket");
    }
}

void TcpServer::start(const NewClientCallback& new_client_callback) {
    if (!socket_.listen()) {
        throw std::system_error(errno, std::system_category(), "Failed to listen on socket");
    }

    auto callback = [this, new_client_callback](int, loopp::EventType) {
        sockaddr_in addr{};
        int cfd = socket_.accept(addr);
        if (cfd == -1) {
            return;  // Either no pending connections or an error occurred
        }

        auto client = connect_client(Socket(cfd));
        new_client_callback(client);
    };

    // Register main socket for reading
    if (!loop_->add_fd(socket_.fd(), loopp::EventType::READ, callback)) {
        throw std::system_error(errno, std::system_category(), "Failed to add server socket to event loop");
    }

    loop_->start();
}

bool TcpServer::close() noexcept {
    bool success = true;

    // Close all active client connections
    for (auto& client : clients_) {
        success &= client->close();
    }

    success &= loop_->stop();
    return success;
}

std::shared_ptr<Client> TcpServer::connect_client(Socket&& socket) {
    auto client = std::make_shared<Client>(std::move(socket), loop_);
    clients_.insert(client);

    client->on_disconnect([this](const auto& client) {
        on_disconnect_client(client);
    });

    if (!client->start()) {
        throw std::system_error(errno, std::system_category(), "Failed to start client");
    }
    return client;
}

void TcpServer::on_disconnect_client(const std::shared_ptr<Client>& client) noexcept {
    clients_.erase(client);
}
