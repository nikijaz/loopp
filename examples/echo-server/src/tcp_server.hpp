#pragma once

#include <functional>
#include <memory>
#include <unordered_set>

#include "client.hpp"
#include "loopp/event_loop.hpp"
#include "socket.hpp"

/*
 * Callback type for new client connections.
 */
using NewClientCallback = std::function<void(std::shared_ptr<Client> client)>;

/*
 * Simple TCP server that accepts incoming connections and manages connected clients.
 */
class TcpServer {
   private:
    /*
     * The server's listening socket.
     */
    Socket socket_;

    /*
     * The event loop for handling asynchronous events.
     * Passed to clients to handle their events.
     */
    std::shared_ptr<loopp::EventLoop> loop_{loopp::EventLoop::create()};

    /*
     * A set of active clients connected to the server.
     * Used to ensure ownership and manage client lifetimes.
     */
    std::unordered_set<std::shared_ptr<Client>> clients_;

   public:
    explicit TcpServer(int port);
    ~TcpServer() noexcept = default;

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    TcpServer(TcpServer&&) = delete;
    TcpServer& operator=(TcpServer&&) = delete;

    /*
     * Start the TCP server.
     * Throws `std::system_error` on failure.
     */
    void start(const NewClientCallback& callback);

    /*
     * Stop the TCP server if it is running.
     * Closes all client connections and stops the event loop.
     * Returns true on success, false on failure (check errno for details).
     * If the server is not running, it's a no-op and returns true.
     */
    bool close() noexcept;

   private:
    /*
     * Adds a new client to the server, keeping it alive.
     * Returns a shared_ptr to the new client.
     * Throws `std::system_error` on failure.
     */
    std::shared_ptr<Client> connect_client(Socket&& socket);

    /*
     * Called when a client disconnects.
     */
    void on_disconnect_client(const std::shared_ptr<Client>& client) noexcept;
};
