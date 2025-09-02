cmake_minimum_required(VERSION 3.10.0)
project(loopp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EVENT_LOOP_SRC src/event_loop_select.cpp)
add_library(loopp STATIC ${EVENT_LOOP_SRC})

target_include_directories(loopp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(examples)

target_compile_options(loopp PRIVATE -Wall)
