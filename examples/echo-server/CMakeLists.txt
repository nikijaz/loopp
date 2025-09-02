cmake_minimum_required(VERSION 3.10.0)
project(echo-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

aux_source_directory(src/ SRC_LIST)
add_executable(echo-server ${SRC_LIST})

target_include_directories(echo-server PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(echo-server PUBLIC loopp)

target_compile_options(echo-server PRIVATE -Wall)
