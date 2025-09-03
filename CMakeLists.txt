cmake_minimum_required(VERSION 3.10.0)
project(loopp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check for epoll availability
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

add_library(loopp STATIC ${EVENT_LOOP_SRC})

target_include_directories(loopp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(examples)

target_compile_options(loopp PRIVATE -Wall)
