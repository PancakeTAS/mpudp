cmake_minimum_required(VERSION 4.0)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_CLANG_TIDY clang-tidy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(mpudp
    DESCRIPTION "Multiport UDP"
    LANGUAGES CXX)

set(SOURCES
    "src/epoll/epoll.cpp"
    "src/server/endpoint.cpp"
    "src/sock/sock.cpp"
    "src/sock/udp.cpp"
    "src/client.cpp"
    "src/main.cpp"
    "src/server.cpp"
    "src/tun.cpp")

add_executable(mpudp ${SOURCES})

set_target_properties(mpudp PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON)
set_target_properties(mpudp PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    SKIP_RPATH ON)
target_compile_options(mpudp PRIVATE
    -Weverything
    -Wno-pre-c++20-compat-pedantic
    -Wno-pre-c++17-compat
    -Wno-c++98-compat-pedantic
    -Wno-c++98-compat
    -Wno-missing-designated-field-initializers
    -Wno-shadow
    -Wno-switch-enum
    -Wno-switch-default
    -Wno-padded
    -Wno-unsafe-buffer-usage-in-container)
