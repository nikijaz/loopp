#include <unistd.h>

#include <array>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <thread>

#include "loopp/event_loop.hpp"

TEST_CASE("EventLoop handles READ events", "[event_loop]") {
    auto loop = loopp::EventLoop::create();
    REQUIRE(loop != nullptr);

    // Create a pipe
    std::array<int, 2> fds{};
    REQUIRE(pipe(fds.data()) == 0);

    // Set up callback
    std::atomic<bool> is_callback_invoked{false};
    auto callback = [&](int fd, loopp::EventType type) {
        is_callback_invoked = true;
        REQUIRE(type == loopp::EventType::READ);

        char buffer[4];
        [[maybe_unused]] auto bytes_read = read(fd, buffer, sizeof(buffer));

        loop->stop();
    };

    // Add READ callback and start the loop in a separate thread
    REQUIRE(loop->add_fd(fds[0], loopp::EventType::READ, callback));
    std::thread loop_thread([&]() { loop->start(); });

    // Wait for the loop to start
    while (!loop->is_running()) {
        std::this_thread::yield();
    }

    [[maybe_unused]] auto _ = write(fds[1], "test", 4);

    // Wait for the loop to stop
    loop_thread.join();

    REQUIRE(is_callback_invoked);

    close(fds[0]);
    close(fds[1]);
}

TEST_CASE("EventLoop handles WRITE events", "[event_loop]") {
    auto loop = loopp::EventLoop::create();
    REQUIRE(loop != nullptr);

    // Create a pipe
    std::array<int, 2> pipe_fds{};
    REQUIRE(pipe(pipe_fds.data()) == 0);

    // Set up callback
    std::atomic<bool> is_callback_invoked{false};
    auto callback = [&](int /*fd*/, loopp::EventType type) {
        is_callback_invoked = true;
        REQUIRE(type == loopp::EventType::WRITE);

        loop->stop();
    };

    // Add WRITE callback and start the loop in a separate thread
    REQUIRE(loop->add_fd(pipe_fds[1], loopp::EventType::WRITE, callback));
    std::thread loop_thread([&]() { loop->start(); });

    // Wait for the loop to stop (WRITE should be immediately ready)
    loop_thread.join();

    REQUIRE(is_callback_invoked);

    close(pipe_fds[0]);
    close(pipe_fds[1]);
}

TEST_CASE("Adding same fd and event type twice is a no-op", "[event_loop]") {
    auto loop = loopp::EventLoop::create();
    REQUIRE(loop != nullptr);

    // Create a pipe
    std::array<int, 2> pipe_fds{};
    REQUIRE(pipe(pipe_fds.data()) == 0);

    auto callback = [&](int /*fd*/, loopp::EventType /*type*/) {
        loop->stop();
    };

    // Add the same fd and event type twice
    REQUIRE(loop->add_fd(pipe_fds[0], loopp::EventType::READ, callback));
    REQUIRE(loop->add_fd(pipe_fds[0], loopp::EventType::READ, callback));

    // Cleanup
    close(pipe_fds[0]);
    close(pipe_fds[1]);
}

TEST_CASE("Removing non-existent fd is a no-op", "[event_loop]") {
    auto loop = loopp::EventLoop::create();
    REQUIRE(loop != nullptr);

    // Create a pipe
    std::array<int, 2> pipe_fds{};
    REQUIRE(pipe(pipe_fds.data()) == 0);

    // Remove a fd that was never added (should be no-op and return true)
    REQUIRE(loop->remove_fd(pipe_fds[0], loopp::EventType::READ));

    // Cleanup
    close(pipe_fds[0]);
    close(pipe_fds[1]);
}

TEST_CASE("EventLoop can remove file descriptors", "[event_loop]") {
    auto loop = loopp::EventLoop::create();
    REQUIRE(loop != nullptr);

    // Create a pipe
    std::array<int, 2> pipe_fds{};
    REQUIRE(pipe(pipe_fds.data()) == 0);

    // Set up callback
    std::atomic<bool> is_callback_invoked{false};
    auto callback = [&](int /*fd*/, loopp::EventType /*type*/) {
        is_callback_invoked = true;
    };

    // Add READ callback and start the loop in a separate thread
    std::thread loop_thread([&]() { loop->start(); });
    REQUIRE(loop->add_fd(pipe_fds[0], loopp::EventType::READ, callback));

    // Wait for the loop to start
    while (!loop->is_running()) {
        std::this_thread::yield();
    }

    // Remove the fd before writing to it
    REQUIRE(loop->remove_fd(pipe_fds[0], loopp::EventType::READ));

    // Write data to the pipe (should not trigger callback)
    [[maybe_unused]] auto _ = write(pipe_fds[1], "test", 4);

    loop->stop();

    // Wait for the loop to stop
    loop_thread.join();

    REQUIRE_FALSE(is_callback_invoked);

    close(pipe_fds[0]);
    close(pipe_fds[1]);
}

TEST_CASE("EventLoop can be stopped", "[event_loop]") {
    auto loop = loopp::EventLoop::create();
    REQUIRE(loop != nullptr);

    // Start the loop in a separate thread
    std::thread loop_thread([&]() { loop->start(); });

    // Wait for the loop to start
    while (!loop->is_running()) {
        std::this_thread::yield();
    }

    // Stop the loop
    REQUIRE(loop->stop());

    // Wait for the loop to finish
    loop_thread.join();
}
