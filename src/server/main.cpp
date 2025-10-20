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
        tun::Tunnel tun{{
            { 5000, sock::ipFromString("127.0.0.1") },
            { 5001, sock::ipFromString("127.0.0.2") },
        }};

        while (true)
            tun.poll(handle_data);
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
