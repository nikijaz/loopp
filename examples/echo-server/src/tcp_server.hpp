#pragma once

#include <functional>
#include <loopp/event_loop.hpp>
#include <unordered_set>

#include "client.hpp"
#include "socket.hpp"

using NewClientCallback = std::function<void(std::shared_ptr<Client>)>;

/*
 * Manages the listening socket and the event loop.
 * Accepts incoming client connections and calls the provided callback.
 */
class TcpServer {
   private:
    Socket socket_;
    std::shared_ptr<loopp::EventLoop> loop_{loopp::EventLoop::create()};
    std::unordered_set<std::shared_ptr<Client>> clients_;

    /*
     * Adds a new client to the server, keeping it alive.
     * Returns a shared_ptr to the new client.
     */
    std::shared_ptr<Client> connect_client(Socket&& socket);

    /*
     * Disconnects a client from the server.
     * Closes the client's connection and removes it from the active clients list.
     */
    void disconnect_client(const std::shared_ptr<Client>& client);

   public:
    explicit TcpServer(int port);
    ~TcpServer() = default;

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    TcpServer(TcpServer&&) = delete;
    TcpServer& operator=(TcpServer&&) = delete;

    /*
     * Start the TCP server.
     */
    void start(const NewClientCallback& callback);

    /*
     * Stop the TCP server if it is running.
     * Closes all client connections and stops the event loop.
     */
    void close();
};
