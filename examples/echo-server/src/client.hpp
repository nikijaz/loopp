#pragma once

#include <functional>
#include <memory>
#include <string>

#include "loopp/event_loop.hpp"
#include "socket.hpp"

/*
 * Callback type for data read from the client socket.
 */
using ClientReadCallback = std::function<void(std::shared_ptr<class Client> client, const std::string& data)>;

/*
 * Callback type for client disconnection.
 */
using ClientDisconnectCallback = std::function<void(std::shared_ptr<class Client> client)>;

/*
 * Manages read and write operations on the client socket.
 * Must be used with a shared_ptr to ensure proper lifetime management.
 */
class Client : public std::enable_shared_from_this<Client> {
   private:
    /*
     * The client's socket for communication.
     */
    Socket socket_;

    /*
     * The event loop for handling asynchronous events.
     * Owned by the server that created this client.
     */
    std::shared_ptr<loopp::EventLoop> loop_;

    /*
     * The callback to be called when data is read from the client socket.
     */
    ClientReadCallback read_callback_;

    /*
     * The callback to be called when the client is disconnected.
     */
    ClientDisconnectCallback disconnect_callback_;

    /*
     * Buffer for data to be written to the client socket.
     * Data is appended to this buffer and sent when the socket is writable.
     */
    std::string write_buffer_{};

   public:
    Client(Socket&& socket, std::shared_ptr<loopp::EventLoop> loop);
    ~Client() = default;

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;

    /*
     * Start polling for events on the client socket.
     * Returns true on success, false on failure (check errno for details).
     */
    bool start();

    /*
     * Set a callback to be called when data is read from the client socket.
     */
    void on_read(const ClientReadCallback& callback);

    /*
     * Append data to the client's write buffer.
     * Data will be sent to the client socket when the socket is writable.
     * Returns true on success, false on failure (check errno for details).
     */
    bool write(const std::string& data);

    /*
     * Disconnect the client.
     * Returns true on success, false on failure (check errno for details).
     */
    bool disconnect();

    /*
     * Set a callback to be called when the client is disconnected.
     */
    void on_disconnect(const ClientDisconnectCallback& callback);

    /*
     * Close the client connection if it is active.
     * Deregisters the socket from the event loop.
     * Returns true on success, false on failure (check errno for details).
     * If the client is already disconnected, it's a no-op and returns true.
     */
    bool close();

   private:
    /*
     * Called when socket is readable.
     */
    void handle_read();

    /*
     * Called when socket is writable.
     */
    void handle_write();

    /*
     * Called when the client gets disconnected.
     * Calls the user-provided disconnect callback if set.
     */
    void handle_disconnect();
};
