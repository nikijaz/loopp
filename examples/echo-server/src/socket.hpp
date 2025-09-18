#pragma once

#include <netinet/in.h>
#include <sys/types.h>

#include <cstddef>

/*
 * Wrapper class for a socket file descriptor.
 * Implements RAII principles.
 */
class Socket {
   private:
    int fd_;

   public:
    explicit Socket(int fd);
    ~Socket() noexcept;

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    /*
     * Get the file descriptor associated with the socket.
     */
    [[nodiscard]] int fd() const noexcept;

    /*
     * Make the socket non-blocking.
     * Returns true on success, false on failure (check errno for details).
     */
    bool set_nonblocking() noexcept;

    /*
     * Enable SO_REUSEADDR on the socket.
     * Returns true on success, false on failure (check errno for details).
     */
    bool set_reuse_addr() noexcept;

    /*
     * Bind the socket to a local address.
     * Returns true on success, false on failure (check errno for details).
     */
    bool bind(const sockaddr_in& addr) noexcept;

    /*
     * Put the socket in a listening state.
     * Returns true on success, false on failure (check errno for details).
     */
    bool listen() noexcept;

    /*
     * Accept a new incoming connection.
     * Returns the file descriptor for the accepted socket, or -1 on error (check errno for details).
     */
    int accept(sockaddr_in& addr) noexcept;

    /*
     * Read data from the socket.
     * Returns the number of bytes read, or -1 on error (check errno for details).
     */
    ssize_t read(void* buf, size_t size) noexcept;

    /*
     * Write data to the socket.
     * Returns the number of bytes written, or -1 on error (check errno for details).
     */
    ssize_t write(const void* buf, size_t size) noexcept;

    /*
     * Create a TCP socket.
     * Returns a Socket object for the new socket.
     * Throws `std::runtime_error` on failure.
     */
    static Socket create_tcp_socket();
};
