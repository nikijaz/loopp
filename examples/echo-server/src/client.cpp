#include "client.hpp"

#include <sys/types.h>

#include <cerrno>
#include <cstddef>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

#include "loopp/event_loop.hpp"
#include "socket.hpp"

constexpr size_t BUFFER_SIZE = 1024;

Client::Client(Socket&& socket, std::shared_ptr<loopp::EventLoop> loop) : socket_(std::move(socket)), loop_(std::move(loop)) {
    if (!socket_.set_nonblocking()) {
        throw std::system_error(errno, std::system_category(), "Failed to set client socket to non-blocking");
    }
}

bool Client::start() {
    return loop_->add_fd(socket_.fd(), loopp::EventType::READ,
                         [this](int, loopp::EventType) { handle_read(); });
}

void Client::on_read(const ClientReadCallback& callback) {
    read_callback_ = callback;
}

bool Client::write(const std::string& data) {
    write_buffer_.append(data);
    return loop_->add_fd(socket_.fd(), loopp::EventType::WRITE,
                         [this](int, loopp::EventType) { handle_write(); });
}

bool Client::disconnect() {
    bool success = close();
    handle_disconnect();
    return success;
}

void Client::on_disconnect(const ClientDisconnectCallback& callback) {
    disconnect_callback_ = callback;
}

bool Client::close() {
    if (!loop_->remove_fd(socket_.fd(), loopp::EventType::READ)) {
        return false;
    }
    if (!loop_->remove_fd(socket_.fd(), loopp::EventType::WRITE)) {
        return false;
    }
    return true;
}

void Client::handle_read() {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = socket_.read(buffer, sizeof(buffer));

    // We received data
    if (bytes_read > 0 && read_callback_) {
        read_callback_(shared_from_this(), std::string(buffer, static_cast<size_t>(bytes_read)));
    }

    // No data, client disconnected
    if (bytes_read == 0) {
        disconnect();
    }

    // An error occurred
    if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        disconnect();
    }
}

void Client::handle_write() {
    ssize_t bytes_written = socket_.write(write_buffer_.data(), write_buffer_.size());

    // We sent data
    if (bytes_written > 0) {
        write_buffer_.erase(0, static_cast<size_t>(bytes_written));
    }

    // An error occurred
    if (bytes_written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        disconnect();
    }

    // We finished writing all data
    if (write_buffer_.empty()) {
        if (!loop_->remove_fd(socket_.fd(), loopp::EventType::WRITE)) {
            // If remove fails, disconnect
            disconnect();
        }
    }
}

void Client::handle_disconnect() {
    if (disconnect_callback_) {
        disconnect_callback_(shared_from_this());
    }
}
