#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace loopp {

/*
 * Types of events that can be monitored.
 * If updating, also update EventCallbacks struct.
 */
enum class EventType : uint8_t {
    READ,
    WRITE
};

/*
 * Function signature for event callbacks.
 */
using EventCallback = std::function<void(int fd, EventType type)>;

/*
 * Manages I/O events for multiple file descriptors, thread-safe.
 * Uses the best available mechanism based on the platform.
 */
class EventLoop {
   public:
    /*
     * Create an instance of the EventLoop.
     * Best available implementation will be chosen.
     * Returns a unique_ptr to the created EventLoop instance.
     * Throws `std::system_error` on initialization failure.
     */
    static std::unique_ptr<EventLoop> create();

    EventLoop() = default;
    virtual ~EventLoop() noexcept = default;

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    EventLoop(EventLoop&&) = delete;
    EventLoop& operator=(EventLoop&&) = delete;

    /*
     * Check if the event loop is currently running.
     */
    [[nodiscard]] virtual bool is_running() const noexcept = 0;

    /*
     * Add a file descriptor to the event loop with the specified event type and callback.
     * Returns true on success, false on failure (check errno for details).
     * If the file descriptor and event type are already registered, it's a no-op and returns true.
     */
    virtual bool add_fd(int fd, EventType type, const EventCallback& callback) noexcept = 0;

    /*
     * Remove a file descriptor and event type from the event loop.
     * Returns true on success, false on failure (check errno for details).
     * If the file descriptor or event type is not registered, it's a no-op and returns true.
     */
    virtual bool remove_fd(int fd, EventType type) noexcept = 0;

    /*
     * Start the event loop.
     * This call blocks until stop() is called from another thread.
     */
    virtual void start() = 0;

    /*
     * Stop the event loop.
     * Returns true on success, false if it can't be woken up (check errno for details).
     * If the loop is not running, it's a no-op and returns true.
     */
    virtual bool stop() noexcept = 0;
};

}  // namespace loopp
