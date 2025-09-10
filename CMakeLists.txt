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

# Add event loop imlpementation file
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

# Static analysis with clang-tidy
find_program(CLANG_TIDY clang-tidy)
if(CLANG_TIDY)
    file(GLOB_RECURSE CLANG_TIDY_TARGETS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.[ch]pp
    )

    add_custom_target(clang-tidy
        COMMAND ${CLANG_TIDY}
        -p ${CMAKE_BINARY_DIR}
        ${CLANG_TIDY_TARGETS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# Static analysis with cppcheck
find_program(CPPCHECK cppcheck)
if(CPPCHECK)
    add_custom_target(cppcheck
        COMMAND ${CPPCHECK}
        --enable=all
        --std=c++23
        --force
        --quiet
        --suppress=missingIncludeSystem
        --suppress=unusedFunction
        --error-exitcode=1
        --project=${CMAKE_BINARY_DIR}/compile_commands.json
        ${CPPCHECK_TARGETS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# Code formatting
find_program(CLANG_FORMAT clang-format) 
if(CLANG_FORMAT)
    file(GLOB_RECURSE CLANG_FORMAT_TARGETS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.[ch]pp
    )

    add_custom_target(clang-format
        COMMAND ${CLANG_FORMAT}
        --dry-run
        --Werror
        ${CLANG_FORMAT_TARGETS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# Build examples if enabled
option(BUILD_EXAMPLES OFF)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
