cmake_minimum_required(VERSION 3.12)
project(lanchat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    includes src src/util src/network src/http src/message
)

if(MSVC)
    add_compile_options(/W3)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -O2)
endif()

option(STATIC_LINKING "Enable static linking" OFF)
if(STATIC_LINKING AND NOT WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

set(SOURCES
    src/main.cpp
    src/network/sockets.cpp
    src/network/peer_discovery.cpp
    src/http/http_server.cpp
    src/message/message_handler.cpp
    src/message/message.cpp
    src/util/utils.cpp
)

add_executable(lanchat ${SOURCES})

if(WIN32)
    target_link_libraries(lanchat ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(lanchat Threads::Threads)
endif()

add_custom_command(TARGET lanchat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:lanchat> ${CMAKE_SOURCE_DIR}/
)
