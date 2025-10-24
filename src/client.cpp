#include "client.hpp"
#include "epoll/epoll.hpp"
#include "tun.hpp"
#include "sock/sock.hpp"

#include <cstddef>
#include <cstdint>
#include <ios>
#include <iostream>

#include <netinet/in.h>

using namespace client;

namespace {
    void on_data(const sock::buf<tun::RECVBUF>& buf, size_t len) {
        std::cerr << "recv " << len << " bytes, starting with: "
            << std::hex << static_cast<int>(buf[0]) << std::dec << "\n";
    }
}

void client::main(uint16_t bport, uint16_t bport_len, in_addr_t peer) {
    epoll::Epoll epoll{};
    tun::Tunnel tunnel{on_data, peer, bport, bport_len};

    while (true) {
        for (uint16_t i = 0; i < bport_len; ++i)
            tunnel.checkConnection(epoll, i);
        epoll.poll(1000);
    }
}
