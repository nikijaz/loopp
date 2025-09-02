#include <fcntl.h>
#include <sys/select.h>

#include <cstdio>
#include <cstring>
#include <loopp/event_loop.hpp>
#include <memory>

namespace loopp {

class EventLoopSelect final : public EventLoop {
   private:
    int wakeup_fd_[2]{-1, -1};

    fd_set read_set_, write_set_;
    int max_fd_{0};

   public:
    EventLoopSelect() {
        // Initialize the read/write sets
        FD_ZERO(&read_set_);
        FD_ZERO(&write_set_);

        // Create a pipe for immediate wakeup
        if (pipe(wakeup_fd_) == -1) throw std::runtime_error("Failed to create wakeup pipe");
        fcntl(wakeup_fd_[0], F_SETFL, O_NONBLOCK);
        fcntl(wakeup_fd_[1], F_SETFL, O_NONBLOCK);

        // Add wakeup FD to read set
        FD_SET(wakeup_fd_[0], &read_set_);
        max_fd_ = wakeup_fd_[0];
    }

    ~EventLoopSelect() override {
        // Close wakeup pipe
        if (wakeup_fd_[0] != -1) close(wakeup_fd_[0]);
        if (wakeup_fd_[1] != -1) close(wakeup_fd_[1]);
    }

    bool add_fd(int fd, EventType type, const EventCallback& callback) override {
        if (event_callbacks_.contains(fd) && event_callbacks_[fd].contains(type)) {
            return true;  // FD and event type already registered
        }

        if (fd >= FD_SETSIZE) return false;  // select() limitation, refer to unix man for more info

        if (type == EventType::READ) FD_SET(fd, &read_set_);
        if (type == EventType::WRITE) FD_SET(fd, &write_set_);

        event_callbacks_[fd][type] = callback;
        max_fd_ = std::max(max_fd_, fd);
        return true;
    }

    void remove_fd(int fd, EventType type) override {
        if (!event_callbacks_.contains(fd) || !event_callbacks_[fd].contains(type)) {
            return;  // FD or event type not registered
        }

        event_callbacks_[fd].erase(type);
        if (event_callbacks_[fd].empty()) {  // If no more callbacks, remove FD
            event_callbacks_.erase(fd);
        }

        if (type == EventType::READ) FD_CLR(fd, &read_set_);
        if (type == EventType::WRITE) FD_CLR(fd, &write_set_);

        if (fd < max_fd_) return;  // Don't recompute max for smaller FDs
        max_fd_ = wakeup_fd_[0];
        for (const auto& [fd, _] : event_callbacks_) {
            max_fd_ = std::max(max_fd_, fd);
        }
    }

    void start() override {
        is_running_.store(true);
        while (is_running_.load()) {
            // Copy the current read/write sets
            fd_set read_set, write_set;
            memcpy(&read_set, &read_set_, sizeof(fd_set));
            memcpy(&write_set, &write_set_, sizeof(fd_set));

            // Add registered FDs to the sets
            for (const auto& [fd, callbacks] : event_callbacks_) {
                for (const auto& [type, callback] : callbacks) {
                    if (type == EventType::READ) FD_SET(fd, &read_set);
                    if (type == EventType::WRITE) FD_SET(fd, &write_set);
                }
            }

            // Wait for events
            int ready_count = select(max_fd_ + 1, &read_set, &write_set, nullptr, nullptr);
            if (ready_count == -1) {
                if (errno == EINTR) continue;  // Signal received, retry
                throw std::runtime_error("Failed to wait for events");
            }
            if (ready_count == 0) continue;  // No events ready

            // Drain the wakeup buffer
            char buffer[64];
            while (read(wakeup_fd_[0], buffer, sizeof(buffer)) > 0) {
            }

            // Collect ready events to avoid issues with callbacks modifying the callback map
            std::vector<std::pair<int, EventType>> ready_events;
            for (const auto& [fd, callbacks] : event_callbacks_) {
                if (FD_ISSET(fd, &read_set) && callbacks.contains(EventType::READ)) {
                    ready_events.emplace_back(fd, EventType::READ);
                }
                if (FD_ISSET(fd, &write_set) && callbacks.contains(EventType::WRITE)) {
                    ready_events.emplace_back(fd, EventType::WRITE);
                }
            }

            // Execute callbacks for ready events
            for (const auto& [fd, type] : ready_events) {
                if (event_callbacks_.contains(fd) && event_callbacks_[fd].contains(type)) {
                    event_callbacks_[fd][type](fd, type);
                }
            }
        }
    };

    void stop() override {
        if (!is_running_.exchange(false)) return;

        write(wakeup_fd_[1], "x", 1);  // Wakeup the loop
    }
};

std::unique_ptr<EventLoop> EventLoop::create() {
    return std::make_unique<EventLoopSelect>();
}

}  // namespace loopp
