# Set CMake version and project configuration
cmake_minimum_required(VERSION 3.10.0)
project(loopp LANGUAGES CXX)

# Set and force C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Enable compile_commands.json for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Check for platform-specific headers
include(CheckIncludeFileCXX)
check_include_file_cxx("sys/epoll.h" HAVE_EPOLL)

# Select event loop implementation
if(HAVE_EPOLL)
    set(EVENT_LOOP_SRC src/event_loop_epoll.cpp)
    message(STATUS "Using epoll event loop backend")
else()
    set(EVENT_LOOP_SRC src/event_loop_select.cpp)
    message(STATUS "Using select event loop backend (fallback)")
endif()

# Add event loop implementation file
add_library(loopp STATIC ${EVENT_LOOP_SRC})

# Add header files
target_include_directories(loopp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Set compiler warnings
target_compile_options(loopp PRIVATE 
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wcast-qual
    -Wformat=2
    -Wundef
)

# Check if we are in the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(BUILD_EXAMPLES OFF)
else()
    set(BUILD_EXAMPLES OFF)
endif()

# Build examples if enabled
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
