cmake_minimum_required(VERSION 3.12)
project(lanchat)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific settings
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
endif()

# Option for static linking
option(STATIC_LINKING "Enable static linking" OFF)
if(STATIC_LINKING AND NOT WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

# Include directories
include_directories(src)
include_directories(src/util)

# Source files
set(SOURCES
    src/main.cpp
    src/network/sockets.cpp
    src/network/peer_discovery.cpp
    src/http/http_server.cpp
    src/message/message_handler.cpp
    src/util/utils.cpp
)

# Create executable
add_executable(lanchat ${SOURCES})

# Platform-specific linking
if(WIN32)
    target_link_libraries(lanchat ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(lanchat Threads::Threads)
endif()

# Copy executable to project root for easy access
add_custom_command(TARGET lanchat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:lanchat> ${CMAKE_SOURCE_DIR}/
)