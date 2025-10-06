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

# Backend selection
set(BACKEND "auto" CACHE STRING "")
set_property(CACHE BACKEND PROPERTY STRINGS auto epoll select)

# Determine backend to use
if(BACKEND STREQUAL "auto")
    include(CheckIncludeFileCXX)
    check_include_file_cxx("sys/epoll.h" HAVE_EPOLL)
    if(HAVE_EPOLL)
        set(SELECTED_BACKEND "epoll")
    else()
        set(SELECTED_BACKEND "select")
    endif()
elseif(BACKEND STREQUAL "epoll" OR BACKEND STREQUAL "select")
    set(SELECTED_BACKEND ${BACKEND})
else()
    message(FATAL_ERROR "Invalid BACKEND: ${BACKEND}. Must be auto, epoll, or select")
endif()

# Validate availability if explicitly requested
if(SELECTED_BACKEND STREQUAL "epoll")
    include(CheckIncludeFileCXX)
    check_include_file_cxx("sys/epoll.h" HAVE_EPOLL)
    if(NOT HAVE_EPOLL)
        message(FATAL_ERROR "epoll backend requested but sys/epoll.h not found")
    endif()
endif()

# Set source file and display message
set(EVENT_LOOP_SRC src/event_loop_${SELECTED_BACKEND}.cpp)
message(STATUS "Using ${SELECTED_BACKEND} event loop backend")

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

# Include tests only if this is the main project
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Add Catch2 subdirectory
    add_subdirectory(lib/Catch2 SYSTEM)

    # Add test source files
    file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
    add_executable(loopp_test ${TEST_SOURCES})

    # Enable testing
    enable_testing()
    target_link_libraries(loopp_test PRIVATE loopp Catch2::Catch2WithMain)

    # Discover and register individual tests with CTest
    include(Catch)
    catch_discover_tests(loopp_test)
endif()
