#include "constants.hpp"
#include "epoll.hpp"
#include "server/tun.hpp"
#include "sock.hpp"

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>

namespace {
    void handle_data(sock::buf<tun::RECV_BUF>& data, size_t len) {
        std::cerr << "recv " << len << " bytes\n";
    }
    [[noreturn]] void try_main() {
        epoll::EPoll epoll{};
        tun::Tunnel tun{epoll, {
            { 5000, sock::ipFromString("127.0.0.1") },
        }};

        while (true) {
            epoll.poll(1000);
            tun.poll(handle_data);
        }
    }
}

int main() {
    try {
        try_main();
    } catch (const char* e) {
        std::cerr << e << " (" << std::strerror(errno) << ")\n";
        return 1;
    }
}
