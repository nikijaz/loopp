#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <set>
#include <system_error>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "loopp/event_loop.hpp"

namespace loopp {

/*
 * Select-based implementation of the EventLoop.
 * Supported on all POSIX systems.
 * Has limitation on maximum number of file descriptors (`FD_SETSIZE`).
 */
class EventLoopSelect final : public EventLoop {
   private:
    /*
     * Indicates if the event loop is running.
     */
    std::atomic<bool> is_running_{false};

    /*
     * Map of file descriptors to their event callbacks.
     */
    std::unordered_map<int, std::unordered_map<EventType, EventCallback>> event_callbacks_;

    /*
     * File descriptor set.
     * Should not be passed directly to select(), copy it first.
     */
    fd_set read_set_, write_set_;

    /*
     * Mutex to protect access to event callbacks and FD sets.
     */
    std::mutex mutex_;

    /*
     * Pipe for immediate wakeup.
     * First element is read end, second is write end.
     */
    int wakeup_fd_[2]{-1, -1};

    /*
     * Used to efficiently track the maximum file descriptor.
     */
    std::multiset<int> fds_;

   public:
    EventLoopSelect() {
        FD_ZERO(&read_set_);
        FD_ZERO(&write_set_);

        // Create pipe for immediate wakeup
        if (pipe(wakeup_fd_) == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to create wakeup pipe");
        }

        // Set both ends of the pipe to non-blocking
        if (fcntl(wakeup_fd_[0], F_SETFL, O_NONBLOCK) == -1 || fcntl(wakeup_fd_[1], F_SETFL, O_NONBLOCK) == -1) {
            close(wakeup_fd_[0]);
            close(wakeup_fd_[1]);
            wakeup_fd_[0] = -1;
            wakeup_fd_[1] = -1;
            throw std::system_error(errno, std::system_category(), "Failed to set non-blocking on wakeup pipe");
        }

        FD_SET(wakeup_fd_[0], &read_set_);
        fds_.insert(wakeup_fd_[0]);
    }

    ~EventLoopSelect() noexcept override {
        // Close the wakeup pipe
        if (wakeup_fd_[0] != -1) close(wakeup_fd_[0]);
        if (wakeup_fd_[1] != -1) close(wakeup_fd_[1]);
    }

    bool add_fd(int fd, EventType type, const EventCallback& callback) noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if already registered
        if (event_callbacks_.contains(fd) && event_callbacks_[fd].contains(type)) {
            return true;
        }

        // Check FD_SETSIZE limitation, refer to unix man for more info
        if (fd >= FD_SETSIZE) {
            errno = EMFILE;
            return false;
        }

        // Add to appropriate FD set
        switch (type) {
            case EventType::READ:
                FD_SET(fd, &read_set_);
                break;
            case EventType::WRITE:
                FD_SET(fd, &write_set_);
                break;
            default:
                errno = EINVAL;
                return false;
        }

        // Register the callback
        event_callbacks_[fd][type] = callback;

        fds_.insert(fd);
        return wakeup();
    }

    bool remove_fd(int fd, EventType type) noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if already unregistered
        if (!event_callbacks_.contains(fd) || !event_callbacks_[fd].contains(type)) {
            return true;
        }

        // Remove from appropriate FD set
        switch (type) {
            case EventType::READ:
                FD_CLR(fd, &read_set_);
                break;
            case EventType::WRITE:
                FD_CLR(fd, &write_set_);
                break;
            default:
                errno = EINVAL;
                return false;
        }

        // Remove the callback
        event_callbacks_[fd].erase(type);
        if (event_callbacks_[fd].empty()) {
            event_callbacks_.erase(fd);
        }

        fds_.erase(fds_.find(fd));
        return wakeup();
    }

    void start() override {
        is_running_.store(true);

        while (is_running_.load()) {
            // Copy current max FD and FD sets under lock
            int max_fd;
            fd_set read_set, write_set;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                max_fd = fds_.empty() ? -1 : *fds_.rbegin();
                read_set = read_set_;
                write_set = write_set_;
            }

            // Wait for events
            int ready_count = select(max_fd + 1, &read_set, &write_set, nullptr, nullptr);
            if (ready_count == -1) {
                if (errno == EINTR) continue;
                throw std::system_error(errno, std::system_category(), "Failed to wait for events");
            }
            if (ready_count == 0) continue;

            // Drain the wakeup buffer
            uint64_t buffer;
            while (read(wakeup_fd_[0], &buffer, sizeof(buffer)) > 0) {
            }

            // Collect ready events to avoid issues with callbacks modifying the callback map
            std::vector<std::tuple<int, EventType, EventCallback>> ready_events;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                for (const auto& [fd, callbacks] : event_callbacks_) {
                    if (FD_ISSET(fd, &read_set) && callbacks.contains(EventType::READ)) {
                        ready_events.emplace_back(fd, EventType::READ, event_callbacks_[fd][EventType::READ]);
                    }
                    if (FD_ISSET(fd, &write_set) && callbacks.contains(EventType::WRITE)) {
                        ready_events.emplace_back(fd, EventType::WRITE, event_callbacks_[fd][EventType::WRITE]);
                    }
                }
            }

            // Execute callbacks for ready events
            for (const auto& [fd, type, callback] : ready_events) {
                callback(fd, type);
            }
        }
    };

    bool stop() noexcept override {
        if (!is_running_.exchange(false)) return true;
        return write(wakeup_fd_[1], "x", 1) != -1;
    }

   private:
    /*
     * Wake up the event loop if it's blocked.
     * No-op if already awake.
     */
    bool wakeup() noexcept {
        if (write(wakeup_fd_[1], "x", 1) == -1) {
            return errno == EAGAIN || errno == EWOULDBLOCK;
        }
        return true;
    }
};

std::unique_ptr<EventLoop> EventLoop::create() {
    return std::make_unique<EventLoopSelect>();
}

}  // namespace loopp
