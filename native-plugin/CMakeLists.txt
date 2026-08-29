cmake_minimum_required(VERSION 3.10)
project(everything_mcp_plugin C)

set(CMAKE_C_STANDARD 99)

add_library(mcp_server64 SHARED
    plugin_main.c
    mcp_server.c
    everything_plugin.h
    mcp_server.h
)

target_link_libraries(mcp_server64 PRIVATE kernel32 user32)

set_target_properties(mcp_server64 PROPERTIES
    PREFIX ""
    OUTPUT_NAME "mcp_server64"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../dist-plugin"
)
