#include "client.hpp"

constexpr size_t BUFFER_SIZE = 1024;

Client::Client(Socket&& socket, std::shared_ptr<loopp::EventLoop> loop) : socket_(std::move(socket)), loop_(loop) {
    socket_.set_nonblocking();
}

void Client::handle_read() {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = socket_.read(buffer, sizeof(buffer));

    if (bytes_read > 0 && read_callback_) {  // We received data
        read_callback_(shared_from_this(), std::string(buffer, bytes_read));
    }

    if (bytes_read == 0) {  // No data, client disconnected
        handle_disconnect();
    }

    if (errno != EAGAIN && errno != EWOULDBLOCK) {  // An error occurred
        handle_disconnect();
    }
}

void Client::handle_write() {
    ssize_t bytes_written = socket_.write(write_buffer_.data(), write_buffer_.size());

    if (bytes_written > 0) {  // We sent data
        write_buffer_.erase(0, bytes_written);
    }

    if (bytes_written < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {  // An error occurred
            handle_disconnect();
        }
    }

    if (write_buffer_.empty()) {  // We finished writing all data
        loop_->remove_fd(socket_.fd(), loopp::EventType::WRITE);
    }
}

void Client::handle_disconnect() {
    close();  // Close the socket first to avoid dangling file descriptors in the event loop
    if (disconnect_callback_) {
        disconnect_callback_(shared_from_this());
    }
}

void Client::start() {
    loop_->add_fd(socket_.fd(), loopp::EventType::READ, [this](int, loopp::EventType) { handle_read(); });
}

void Client::on_read(const ClientReadCallback& callback) {
    read_callback_ = callback;
}

void Client::write(const std::string& data) {
    write_buffer_.append(data);
    loop_->add_fd(socket_.fd(), loopp::EventType::WRITE, [this](int, loopp::EventType) { handle_write(); });
}

void Client::on_disconnect(const ClientDisconnectCallback& callback) {
    disconnect_callback_ = callback;
}

void Client::close() {
    loop_->remove_fd(socket_.fd(), loopp::EventType::READ);
    loop_->remove_fd(socket_.fd(), loopp::EventType::WRITE);
}
