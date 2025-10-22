#include "constants.hpp"
#include "epoll.hpp"
#include "own.hpp"
#include "server/tun.hpp"
#include "sock.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace {
    [[noreturn]] void try_main() {
        epoll::EPoll epoll{};

        tun::Tunnel tun{epoll, {
            { 5000, sock::ipFromString("80.151.97.24") },
            { 5001, sock::ipFromString("93.229.85.103") }
        }};

        const own::owned_fd outgoing_sock = sock::openDgramSocket();
        const int fd = *outgoing_sock;

        sock::buf<tun::RECV_BUF> recvbuf{};
        std::unique_ptr<uint32_t> incoming_data = std::make_unique<uint32_t>(0);
        epoll.add(fd, incoming_data);

        const struct sockaddr_in addr{
            .sin_family = AF_INET,
            .sin_port = htons(51280),
            .sin_addr = {
                .s_addr = sock::ipFromString("127.0.0.1")
            }
        };

        while (true) {
            epoll.poll(1000);

            tun.poll([=](sock::buf<tun::RECV_BUF>& data, size_t len) {
                sock::write(fd, data, len, addr);
            });

            if (*incoming_data & EPOLLIN) {
                *incoming_data = 0;

                struct sockaddr_in naddr{};
                const ssize_t n = sock::read(fd, recvbuf, naddr);
                tun.write(recvbuf, static_cast<size_t>(n));
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
