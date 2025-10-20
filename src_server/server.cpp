#include "epoll.hpp"
#include "own.hpp"
#include "sock.hpp"

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/types.h>

namespace {
    struct Tunnel {
        own::owned_fd fd;
        in_addr_t remote{};
        bool established{};
    };

    struct Tunnels {
        own::owned_fd epoll_fd;
        std::unordered_map<int, Tunnel> tunnels;
    };

    const size_t RECV_SEND_BUF = 65535;
    const ssize_t HSLEN = 5;
    void pollTunnels(Tunnels& tunnels, const std::function<void(sock::buf<RECV_SEND_BUF>&, size_t)>& onData) {
        static sock::buf<RECV_SEND_BUF> recvbuf, sendbuf;
        static std::array<struct epoll_event, 16> events;

        // poll and iterate through events
        const size_t n = epoll::poll(tunnels.epoll_fd, events);
        for (size_t i = 0; i < n; ++i) {
            const struct epoll_event& ev = events.at(i);
            if ((ev.events & EPOLLIN) == 0) // ignore non-readable events
                continue;

            const int fd = ev.data.fd;
            auto& tunnel = tunnels.tunnels[fd];

            struct sockaddr_in inaddr{};
            const ssize_t nb = sock::read(fd, recvbuf, inaddr);

            // if the tunnel is not yet established,
            // run through the handshake process
            const bool valid = inaddr.sin_addr.s_addr == tunnel.remote;
            if (!tunnel.established) {
                // establish if source ip is valid
                // and a HSLEN-byte packet is received
                if (nb == HSLEN && valid)
                    tunnel.established = true;

                // write back 1 if established, else 0
                // to indicate success/failure of handshake
                sendbuf[0] = valid ? 'Y' : 'N';
                sock::write(fd, sendbuf, 1, inaddr);

                continue;
            }

            // drop invalid packets
            // FIXME: this should be handled better
            if (!valid)
                continue;

            onData(recvbuf, static_cast<size_t>(nb));
        }
    }
}

namespace {
    void handle_data(sock::buf<RECV_SEND_BUF>& data, size_t len) {
        std::cerr << "recv " << len << " bytes\n";
    }
    [[noreturn]] void try_main() {
        Tunnels tunnels{
            .epoll_fd = epoll::createEpollFd(),
        };

        Tunnel tun1{
            .fd = sock::openBoundDgramSocket(5000),
            .remote = sock::ipFromString("127.0.0.1")
        };
        epoll::addFd(tunnels.epoll_fd, *tun1.fd);
        tunnels.tunnels[*tun1.fd] = std::move(tun1);

        Tunnel tun2{
            .fd = sock::openBoundDgramSocket(5001),
            .remote = sock::ipFromString("80.151.97.24")
        };
        epoll::addFd(tunnels.epoll_fd, *tun2.fd);
        tunnels.tunnels[*tun2.fd] = std::move(tun2);

        while (true)
            pollTunnels(tunnels, handle_data);
    }
}

int main() {
    try {
        try_main();
    } catch (const std::string& e) {
        std::cerr << e << " (" << std::strerror(errno) << ")\n";
        return 1;
    }
}
