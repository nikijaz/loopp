# Echo Server

A complete TCP echo server implementation using the loopp event loop library.
Demonstrates connection handling, asynchronous I/O operations and graceful
shutdown patterns.

## Installation

0. Ensure **CMake 3.10+** and **C++23 compiler** are installed.

1. Clone and navigate to the repository:

   ```bash
   git clone https://github.com/nikijaz/loopp.git
   cd loopp/
   ```

2. Create a build directory and set up the project:

   ```bash
   mkdir build && cd build
   cmake -DBUILD_EXAMPLES=ON ..
   ```

3. Build the echo server:

   ```bash
   make
   ```

4. Run the echo server:

   ```bash
   ./examples/echo-server/echo-server &
   ```

5. Connect using `telnet` or `netcat`:

   ```bash
   telnet localhost 8080
   # or
   nc localhost 8080
   ```

## Code Structure

```cpp
echo-server/
├── src/
│   ├── main.cpp        // Entry point with signal handling and server configuration
│   ├── client.cpp      // Individual client connection handler with buffered I/O
│   ├── client.hpp
│   ├── socket.cpp      // Low-level socket wrapper for network operations
│   ├── socket.hpp
│   ├── tcp_server.cpp  // High-level TCP server abstraction managing connections
│   └── tcp_server.hpp
└── CMakeLists.txt
```
