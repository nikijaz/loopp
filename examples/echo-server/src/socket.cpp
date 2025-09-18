#include "socket.hpp"

#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstddef>
#include <stdexcept>

Socket::Socket(int fd) : fd_(fd) {
    if (fd_ < 0) {
        throw std::runtime_error("Invalid file descriptor");
    }
}

Socket::~Socket() noexcept {
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

int Socket::fd() const noexcept {
    return fd_;
}

bool Socket::set_nonblocking() noexcept {
    int flags = fcntl(fd_, F_GETFL, 0);
    return flags != -1 && fcntl(fd_, F_SETFL, flags | O_NONBLOCK) != -1;
}

bool Socket::set_reuse_addr() noexcept {
    int value = 1;
    return setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) != -1;
}

bool Socket::bind(const sockaddr_in& addr) noexcept {
    return ::bind(fd_, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != -1;
}

bool Socket::listen() noexcept {
    return ::listen(fd_, SOMAXCONN) != -1;
}

int Socket::accept(sockaddr_in& addr) noexcept {
    socklen_t len = sizeof(addr);
    int cfd = ::accept(fd_, reinterpret_cast<sockaddr*>(&addr), &len);
    return cfd;
}

ssize_t Socket::read(void* buf, size_t size) noexcept {
    return ::read(fd_, buf, size);
}

ssize_t Socket::write(const void* buf, size_t size) noexcept {
    return ::write(fd_, buf, size);
}

Socket Socket::create_tcp_socket() {
    int new_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    return Socket(new_fd);
}
