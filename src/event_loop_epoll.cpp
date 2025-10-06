#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <loopp/event_loop.hpp>
#include <memory>
#include <mutex>
#include <system_error>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace loopp {

/*
 * Epoll-based implementation of the EventLoop.
 * Supported on Linux systems.
 */
class EventLoopEpoll final : public EventLoop {
   private:
    /*
     * Maximum number of events to process at once.
     */
    static constexpr int MAX_EVENTS = 1024;

    /*
     * Indicates if the event loop is running.
     */
    std::atomic<bool> is_running_{false};

    /*
     * Map of file descriptors to their event callbacks.
     */
    std::unordered_map<int, std::unordered_map<EventType, EventCallback>> event_callbacks_;

    /*
     * Mutex to protect access to event callbacks.
     */
    std::mutex mutex_;

    /*
     * File descriptor for the epoll instance.
     */
    int epoll_fd_{-1};

    /*
     * Event file descriptor for immediate wakeup.
     */
    int wakeup_fd_{-1};

   public:
    EventLoopEpoll() {
        // Create epoll instance
        epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd_ == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to create epoll instance");
        }

        // Create eventfd for immediate wakeup
        wakeup_fd_ = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (wakeup_fd_ == -1) {
            close(epoll_fd_);
            epoll_fd_ = -1;
            throw std::system_error(errno, std::system_category(), "Failed to create wakeup eventfd");
        }

        // Add wakeup fd to epoll
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = wakeup_fd_;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, wakeup_fd_, &event) == -1) {
            close(wakeup_fd_);
            close(epoll_fd_);
            wakeup_fd_ = -1;
            epoll_fd_ = -1;
            throw std::system_error(errno, std::system_category(), "Failed to add wakeup fd to epoll");
        }
    }

    ~EventLoopEpoll() noexcept override {
        if (wakeup_fd_ != -1) close(wakeup_fd_);
        if (epoll_fd_ != -1) close(epoll_fd_);
    }

    [[nodiscard]] bool is_running() const noexcept override {
        return is_running_.load();
    }

    bool add_fd(int fd, EventType type, const EventCallback& callback) noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if already registered
        if (event_callbacks_.contains(fd) && event_callbacks_[fd].contains(type)) {
            return true;
        }

        uint32_t events = 0;

        // If FD already registered, get existing events
        if (event_callbacks_.contains(fd)) {
            for (const auto& [existing_type, _] : event_callbacks_[fd]) {
                if (existing_type == EventType::READ) events |= EPOLLIN;
                if (existing_type == EventType::WRITE) events |= EPOLLOUT;
            }
        }

        // Add new event type
        if (type == EventType::READ) events |= EPOLLIN;
        if (type == EventType::WRITE) events |= EPOLLOUT;

        struct epoll_event event;
        event.events = events;
        event.data.fd = fd;

        // Determine whether to add or modify the FD in epoll
        int op = event_callbacks_.contains(fd) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        if (epoll_ctl(epoll_fd_, op, fd, &event) == -1) {
            return false;
        }

        // Register the callback
        event_callbacks_[fd][type] = callback;

        return wakeup();
    }

    bool remove_fd(int fd, EventType type) noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if already unregistered
        if (!event_callbacks_.contains(fd) || !event_callbacks_[fd].contains(type)) {
            return true;
        }

        // Remove the callback
        event_callbacks_[fd].erase(type);

        if (event_callbacks_[fd].empty()) {
            // No more callbacks, remove FD from epoll
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
                return false;
            }
            event_callbacks_.erase(fd);
        } else {
            // Still have callbacks, modify epoll registration
            uint32_t events = 0;
            for (const auto& [remaining_type, _] : event_callbacks_[fd]) {
                if (remaining_type == EventType::READ) events |= EPOLLIN;
                if (remaining_type == EventType::WRITE) events |= EPOLLOUT;
            }

            struct epoll_event event;
            event.events = events;
            event.data.fd = fd;
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) == -1) {
                return false;
            }
        }

        return wakeup();
    }

    void start() override {
        is_running_.store(true);

        while (is_running_.load()) {
            // Wait for events
            struct epoll_event events[MAX_EVENTS];
            int ready_count = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
            if (ready_count == -1) {
                if (errno == EINTR) continue;
                throw std::system_error(errno, std::system_category(), "Failed to wait for events");
            }
            if (ready_count == 0) continue;

            // Drain the wakeup buffer
            for (int i = 0; i < ready_count; ++i) {
                if (events[i].data.fd != wakeup_fd_) continue;

                uint64_t buffer;
                while (read(wakeup_fd_, &buffer, sizeof(buffer)) > 0) {
                }
                break;
            }

            // Collect ready events to avoid issues with callbacks modifying the callback map
            std::vector<std::tuple<int, EventType, EventCallback>> ready_events;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                for (int i = 0; i < ready_count; i++) {
                    int fd = events[i].data.fd;
                    uint32_t epoll_events = events[i].events;

                    if (fd == wakeup_fd_) continue;  // Skip wakeup fd

                    if (!event_callbacks_.contains(fd)) continue;  // FD no longer registered

                    if ((epoll_events & EPOLLIN) && event_callbacks_[fd].contains(EventType::READ)) {
                        ready_events.emplace_back(fd, EventType::READ, event_callbacks_[fd][EventType::READ]);
                    }
                    if ((epoll_events & EPOLLOUT) && event_callbacks_[fd].contains(EventType::WRITE)) {
                        ready_events.emplace_back(fd, EventType::WRITE, event_callbacks_[fd][EventType::WRITE]);
                    }
                }
            }

            // Execute callbacks for ready events
            for (const auto& [fd, type, callback] : ready_events) {
                callback(fd, type);
            }
        }
    }

    bool stop() noexcept override {
        if (!is_running_.exchange(false)) return true;
        return wakeup();
    }

   private:
    /*
     * Wake up the event loop if it's blocked.
     * No-op if already awake.
     */
    bool wakeup() noexcept {
        uint64_t value = 1;
        if (write(wakeup_fd_, &value, sizeof(value)) == -1) {
            return errno == EAGAIN || errno == EWOULDBLOCK;
        }
        return true;
    }
};

std::unique_ptr<EventLoop> EventLoop::create() {
    return std::make_unique<EventLoopEpoll>();
}

}  // namespace loopp
