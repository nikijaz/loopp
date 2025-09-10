#pragma once

#include <atomic>
#include <functional>
#include <memory>

namespace loopp {

enum class EventType : uint8_t {
    READ,
    WRITE
};

using EventCallback = std::function<void(int, EventType)>;

/*
 * Manages I/O events for multiple file descriptors.
 */
class EventLoop {
   protected:
    std::atomic<bool> is_running_{false};
    std::unordered_map<int, std::unordered_map<EventType, EventCallback>> event_callbacks_;

   public:
    static std::unique_ptr<EventLoop> create();

    EventLoop() = default;
    virtual ~EventLoop() = default;

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    EventLoop(EventLoop&&) = delete;
    EventLoop& operator=(EventLoop&&) = delete;

    /*
     * Add a file descriptor to the event loop. One callback per file descriptor and event type.
     * Returns true on success, false if it can't be added due to kernel limitations.
     * Safe to call multiple times.
     */
    virtual bool add_fd(int fd, EventType type, const EventCallback& callback) = 0;

    /*
     * Remove a file descriptor from the event loop.
     * If the file descriptor is not found, function does nothing.
     */
    virtual void remove_fd(int fd, EventType type) = 0;

    /*
     * Start the event loop.
     */
    virtual void start() = 0;

    /*
     * Stop the event loop by unblocking waiting for events.
     * Safe to call multiple times, thread-safe.
     */
    virtual void stop() = 0;
};

}  // namespace loopp
