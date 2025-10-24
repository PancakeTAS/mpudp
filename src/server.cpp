#include "server.hpp"
#include "epoll/epoll.hpp"
#include "server/endpoint.hpp"
#include "sock/sock.hpp"

#include <cstddef>
#include <cstdint>
#include <ios>
#include <iostream>

#include <netinet/in.h>
#include <vector>

using namespace server;

namespace {
    void on_data(const sock::buf<endpoint::RECVBUF>& buf, size_t len) {
        std::cerr << "recv " << len << " bytes, starting with: "
            << std::hex << static_cast<int>(buf[0]) << std::dec << "\n";
    }
}

void server::main(uint16_t bport, const std::vector<in_addr_t>& peers) {
    epoll::Epoll epoll{};
    const endpoint::Endpoint endpoint{epoll, on_data, bport, peers};

    while (true)
        epoll.poll();
}
