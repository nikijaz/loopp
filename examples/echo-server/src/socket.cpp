#include "socket.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <stdexcept>

Socket::Socket(int fd) : fd_(fd) {
    if (fd_ < 0) {
        throw std::runtime_error("Invalid file descriptor");
    }
}

Socket::~Socket() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

int Socket::fd() const {
    return fd_;
}

void Socket::set_nonblocking() {
    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("Failed to get socket flags");
    }
    if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("Failed to set non-blocking");
    }
}

void Socket::bind(const sockaddr_in& addr) {
    if (::bind(fd_, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == -1) {
        throw std::runtime_error("Failed to bind socket");
    }
}

void Socket::listen() {
    if (::listen(fd_, SOMAXCONN) == -1) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

int Socket::accept(sockaddr_in& addr) {
    socklen_t len = sizeof(addr);
    int cfd = ::accept(fd_, reinterpret_cast<sockaddr*>(&addr), &len);
    return cfd;
}

ssize_t Socket::read(void* buf, size_t size) {
    return ::read(fd_, buf, size);
}

ssize_t Socket::write(const void* buf, size_t size) {
    return ::write(fd_, buf, size);
}

int Socket::create_tcp_socket() {
    int new_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (new_fd == -1) {
        throw std::runtime_error("Failed to create socket");
    }
    return new_fd;
}
