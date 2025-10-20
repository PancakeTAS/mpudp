#include "client/tun.hpp"
#include "constants.hpp"
#include "sock.hpp"

#include <cerrno>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>

namespace {
    void handle_data(sock::buf<tun::RECV_BUF>& data, size_t len) {
        std::cerr << "recv " << len << " bytes\n";
    }
    [[noreturn]] void try_main() {
        tun::Tunnel tun{sock::ipFromString("127.0.0.1")};

        while (true) {
            tun.validateConnection(5000);

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
