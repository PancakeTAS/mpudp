#include "epoll/epoll.hpp"

#include "sock/sock.hpp"
#include "tun.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <netinet/in.h>

namespace {
    void on_data(const sock::buf<tun::RECVBUF>& buf, size_t len) {
        std::cerr << "recv " << len << " bytes, starting with: "
            << std::hex << static_cast<int>(buf[0]) << std::dec << "\n";
    }
}

int main() {
    const in_addr_t PEER_ADDR = sock::stoia("127.0.0.1");
    const uint16_t PEER_BASE_PORT = 8888;
    const uint16_t PEER_PORTS = 2;

    epoll::Epoll epoll{};
    tun::Tunnel tunnel{PEER_ADDR, on_data, PEER_BASE_PORT, PEER_PORTS};

    while (true) {
        for (uint16_t i = 0; i < PEER_PORTS; i++)
            tunnel.checkConnection(epoll, i);
        epoll.poll(1000);
    }
}
