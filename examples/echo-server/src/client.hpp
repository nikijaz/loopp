#pragma once

#include <functional>
#include <loopp/event_loop.hpp>
#include <memory>
#include <string>

#include "socket.hpp"

using ClientReadCallback = std::function<void(std::shared_ptr<class Client>, const std::string&)>;
using ClientDisconnectCallback = std::function<void(std::shared_ptr<class Client>)>;

/*
 * Manages read and write operations on the client socket.
 * Must be used with a shared_ptr to ensure proper lifetime management.
 */
class Client : public std::enable_shared_from_this<Client> {
   private:
    Socket socket_;
    std::shared_ptr<loopp::EventLoop> loop_;

    ClientReadCallback read_callback_;
    ClientDisconnectCallback disconnect_callback_;

    std::string write_buffer_{};

    /*
     * Called when socket is readable.
     */
    void handle_read();

    /*
     * Called when socket is writable.
     */
    void handle_write();

    /*
     * Called when the client is disconnected.
     * Closes the client socket and calls the disconnect callback.
     */
    void handle_disconnect();

   public:
    Client(Socket&& socket, std::shared_ptr<loopp::EventLoop> loop);
    ~Client() = default;

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;

    /*
     * Start polling for events on the client socket.
     */
    void start();

    /*
     * Set a callback to be called when data is read from the client socket.
     */
    void on_read(const ClientReadCallback& callback);

    /*
     * Append data to the client's write buffer.
     * Data will be sent to the client socket when the socket is writable.
     */
    void write(const std::string& data);

    /*
     * Set a callback to be called when the client is disconnected.
     */
    void on_disconnect(const ClientDisconnectCallback& callback);

    /*
     * Close the client connection if it is active.
     * Deregisters the client from the event loop and calls the close callback.
     */
    void close();
};
