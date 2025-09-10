# Add source files
aux_source_directory(src/ SRC_LIST)
add_executable(echo-server ${SRC_LIST})

# Add header files
target_include_directories(echo-server PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Link against loopp library
target_link_libraries(echo-server PRIVATE loopp)
