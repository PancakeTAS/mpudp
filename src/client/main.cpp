#include "client/tun.hpp"
#include "constants.hpp"
#include "epoll.hpp"
#include "sock.hpp"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>

namespace {
    void handle_data(sock::buf<tun::RECV_BUF>& data, size_t len) {
        std::cerr << "recv " << len << " bytes\n";
    }
    [[noreturn]] void try_main() {
        epoll::EPoll epoll{};
        tun::Tunnel tun{sock::ipFromString("127.0.0.1")};

        const sock::buf<tun::SEND_BUF> sendbuf{};
        auto last = std::chrono::steady_clock::now();
        while (true) {
            tun.validateConnection(epoll, 5000);

            epoll.poll(1000);
            tun.poll(handle_data);

            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last).count() >= 1) {
                tun.write(sendbuf, 20);
                last = now;
            }
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
