cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_CLANG_TIDY clang-tidy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(mpudp
    DESCRIPTION "Multiport UDP"
    LANGUAGES CXX)

set(SOURCES
    "src/server/tun.cpp"
    "src/epoll.cpp"
    "src/sock.cpp")

add_library(mpudp_lib STATIC ${SOURCES})
add_executable(mpudp_client "src/client/main.cpp")
add_executable(mpudp_server "src/server/main.cpp")

foreach(target mpudp_lib mpudp_client mpudp_server)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON)
    set_target_properties(${target} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        SKIP_RPATH ON)
    target_include_directories(${target}
        PUBLIC include)
    target_compile_options(${target} PRIVATE
        -Weverything
        -Wno-pre-c++20-compat-pedantic
        -Wno-pre-c++17-compat
        -Wno-c++98-compat-pedantic
        -Wno-c++98-compat
        -Wno-missing-designated-field-initializers
        -Wno-shadow
        -Wno-switch-enum
        -Wno-switch-default
        -Wno-padded)
endforeach()

foreach(exe mpudp_client mpudp_server)
    target_link_libraries(${exe} PRIVATE mpudp_lib)
endforeach()
