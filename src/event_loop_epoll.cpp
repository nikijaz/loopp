#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <loopp/event_loop.hpp>
#include <memory>
#include <stdexcept>
#include <vector>

namespace loopp {

class EventLoopEpoll final : public EventLoop {
   private:
    int epoll_fd_{-1};
    int wakeup_fd_{-1};

    static constexpr int MAX_EVENTS = 1024;

   public:
    EventLoopEpoll() {
        // Create epoll instance
        epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd_ == -1) throw std::runtime_error("Failed to create epoll instance");

        // Create eventfd for immediate wakeup
        wakeup_fd_ = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (wakeup_fd_ == -1) {
            close(epoll_fd_);
            throw std::runtime_error("Failed to create wakeup eventfd");
        }

        // Add wakeup fd to epoll
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = wakeup_fd_;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, wakeup_fd_, &event) == -1) {
            close(wakeup_fd_);
            close(epoll_fd_);
            throw std::runtime_error("Failed to add wakeup fd to epoll");
        }
    }

    ~EventLoopEpoll() override {
        if (wakeup_fd_ != -1) close(wakeup_fd_);
        if (epoll_fd_ != -1) close(epoll_fd_);
    }

    bool add_fd(int fd, EventType type, const EventCallback& callback) override {
        if (event_callbacks_.contains(fd) && event_callbacks_[fd].contains(type)) {
            return true;  // FD and event type already registered
        }

        uint32_t events = 0;
        if (event_callbacks_.contains(fd)) {
            // FD already exists, modify existing registration
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

        int op = event_callbacks_.contains(fd) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        if (epoll_ctl(epoll_fd_, op, fd, &event) == -1) {
            return false;  // Failed to add/modify fd
        }

        event_callbacks_[fd][type] = callback;
        return true;
    }

    void remove_fd(int fd, EventType type) override {
        if (!event_callbacks_.contains(fd) || !event_callbacks_[fd].contains(type)) {
            return;  // FD or event type not registered
        }

        event_callbacks_[fd].erase(type);

        if (event_callbacks_[fd].empty()) {
            // No more callbacks, remove FD from epoll
            epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
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
            epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
        }
    }

    void start() override {
        is_running_.store(true);
        struct epoll_event events[MAX_EVENTS];

        while (is_running_.load()) {
            // Wait for events
            int ready_count = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
            if (ready_count == -1) {
                if (errno == EINTR) continue;  // Signal received, retry
                throw std::runtime_error("Failed to wait for events");
            }
            if (ready_count == 0) continue;  // No events ready

            // Drain the wakeup eventfd
            for (int i = 0; i < ready_count; ++i) {
                if (events[i].data.fd == wakeup_fd_) {
                    uint64_t value;
                    while (read(wakeup_fd_, &value, sizeof(value)) > 0) {
                    }
                    break;
                }
            }

            // Collect ready events to avoid issues with callbacks modifying the callback map
            std::vector<std::pair<int, EventType>> ready_events;
            for (int i = 0; i < ready_count; ++i) {
                int fd = events[i].data.fd;
                uint32_t epoll_events = events[i].events;

                if (fd == wakeup_fd_) continue;  // Skip wakeup fd

                if (!event_callbacks_.contains(fd)) continue;  // FD no longer registered

                if ((epoll_events & EPOLLIN) && event_callbacks_[fd].contains(EventType::READ)) {
                    ready_events.emplace_back(fd, EventType::READ);
                }
                if ((epoll_events & EPOLLOUT) && event_callbacks_[fd].contains(EventType::WRITE)) {
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
    }

    void stop() override {
        if (!is_running_.exchange(false)) return;

        // Write to eventfd to wakeup epoll_wait
        uint64_t value = 1;
        write(wakeup_fd_, &value, sizeof(value));
    }
};

std::unique_ptr<EventLoop> EventLoop::create() {
    return std::make_unique<EventLoopEpoll>();
}

}  // namespace loopp
