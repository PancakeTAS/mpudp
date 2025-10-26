cmake_minimum_required(VERSION 4.0)

project(mpudp
    DESCRIPTION "Multiport UDP"
    LANGUAGES CXX)

set(MPUDP_RECVBUF 65535 CACHE STRING "UDP receive buffer size")

set(SOURCES
    "src/epoll/epoll.cpp"
    "src/selection/wrr.cpp"
    "src/tun/endpoint.cpp"
    "src/tun/tun.cpp"
    "src/sock/sock.cpp"
    "src/sock/udp.cpp"
    "src/side/client.cpp"
    "src/side/server.cpp"
    "src/config.cpp"
    "src/main.cpp")
add_executable(mpudp ${SOURCES})

set_target_properties(mpudp PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_VISIBILITY_PRESET hidden
    SKIP_RPATH ON)
target_include_directories(mpudp SYSTEM PRIVATE
    thirdparty)
target_compile_definitions(mpudp PRIVATE
    MPUDP_RECVBUF=${MPUDP_RECVBUF})
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(mpudp PROPERTIES
        EXPORT_COMPILE_COMMANDS ON)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
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
    endif()
endif()
