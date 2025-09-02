#pragma once

#include <netinet/in.h>

/*
 * Wrapper class for a socket file descriptor.
 * Implements RAII principles.
 */
class Socket {
   private:
    int fd_;

   public:
    explicit Socket(int fd);
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other);
    Socket& operator=(Socket&& other);

    /*
     * Get the file descriptor associated with the socket.
     */
    int fd() const;

    /*
     * Make the socket non-blocking.
     */
    void set_nonblocking();

    /*
     * Bind the socket to a local address.
     */
    void bind(const sockaddr_in& addr);

    /*
     * Put the socket in a listening state.
     */
    void listen();

    /*
     * Accept a new incoming connection.
     * Returns the file descriptor for the accepted socket, or -1 on error.
     */
    int accept(sockaddr_in& addr);

    /*
     * Read data from the socket.
     * Returns the number of bytes read, or -1 on error.
     */
    ssize_t read(void* buf, size_t size);

    /*
     * Write data to the socket.
     * Returns the number of bytes written, or -1 on error.
     */
    ssize_t write(const void* buf, size_t size);

    /*
     * Create a TCP socket.
     * Returns the file descriptor for the new socket, or -1 on error.
     */
    static int create_tcp_socket();
};
