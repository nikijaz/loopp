# loopp

A high-performance C++ event loop library for asynchronous I/O operations.
Provides a clean, modern interface for building event-driven applications with
thread-safe design.

## Usage

Create an event loop, register file descriptors with callbacks for specific
events, then start the loop. When events occur, your callbacks are invoked with
the file descriptor and event type.

```cpp
auto loop = loopp::EventLoop::create();
loop->add_fd(fd, loopp::EventType::READ, callback);
loop->start(); // Blocks until loop->stop() called
```

See [examples/echo-server](examples/echo-server) for a complete TCP server
implementation.

## Installation

**CMake FetchContent:**

```cmake
include(FetchContent)  
FetchContent_Declare(loopp GIT_REPOSITORY https://github.com/nikijaz/loopp.git)  
FetchContent_MakeAvailable(loopp)  
target_link_libraries(your_target PRIVATE loopp)
```

**CMake Submodule:**

```cmake
add_subdirectory(path/to/loopp)  
target_link_libraries(your_target PRIVATE loopp)
```

## Backend I/O

The best available backend is chosen at build time, top to bottom:

- **[epoll](https://en.wikipedia.org/wiki/Epoll)** - Linux-specific, `O(1)` scaling,
  efficient for large numbers of file descriptors.
- **[select](https://en.wikipedia.org/wiki/Select_(Unix))** - POSIX fallback
  with `O(n)` scaling, limited by `FD_SETSIZE` (typically 1024 file descriptors).

## Development

0. Ensure **CMake 3.10+** and **C++23 compiler** are installed.
1. Clone and navigate to the repository:

    ```bash
    git clone https://github.com/nikijaz/loopp.git
    cd loopp/
    ```

2. Create a build directory and set up the project:

    ```bash
    mkdir build && cd build
    cmake .. # Add -DBUILD_EXAMPLES=ON to build all examples
    ```

3. Make changes to the code.

4. Verify your changes by building:

    ```bash
    make
    ```

5. Check for any code quality issues:

    ```bash
    ../scripts/code-quality.sh
    ```
