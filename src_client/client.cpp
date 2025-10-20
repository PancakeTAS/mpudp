#include "epoll.hpp"
#include "own.hpp"
#include "sock.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace {
    struct Tunnel {
        own::owned_fd fd;
        uint16_t port{};
        time_t last_active{};
        bool established{};
        bool cancelled{};
    };

    struct Tunnels {
        own::owned_fd epoll_fd;
        in_addr_t remote;
        std::unordered_map<int, Tunnel> tunnels;
    };

    const size_t RECV_BUF = 65535;
    void pollTunnels(Tunnels& tunnels) {
        static sock::buf<RECV_BUF> recvbuf;
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
            if (!tunnel.established && !tunnel.cancelled) {
                // establish if a single 'Y' byte is received
                if (nb == 1 && recvbuf[0] == 'Y')
                    tunnel.established = true;
                else // FIXME: better state handling
                    tunnel.cancelled = true; // mark as cancelled

                continue; // skip further processing until established
            }

            // TODO: process data
        }
    }
}

namespace {
    const size_t HSLEN = 5;
    void validate_tunnel(Tunnels& tunnels, uint16_t port) {
        auto it = std::ranges::find_if(tunnels.tunnels,
            [port](const auto& pair) {
                return pair.second.port == port;
            }); // FIXME: optimize lookup
        // validate the tunnel if it already exists
        if (it != tunnels.tunnels.end()) {
            auto& tunnel = it->second;
            if (tunnel.established) // skip established tunnels
                return;

            if (tunnel.cancelled) {
                epoll::removeFd(tunnels.epoll_fd, *tunnel.fd);
                tunnels.tunnels.erase(it);
            }

            // remove tunnels which haven't established in 5s
            const time_t now = std::time(nullptr);
            if (now - tunnel.last_active > 5) {
                epoll::removeFd(tunnels.epoll_fd, *tunnel.fd);
                tunnels.tunnels.erase(it);
            }
        }

        // (re)-add the tunnel
        Tunnel tun{
            .fd = sock::openDgramSocket(),
            .port = port,
            .last_active = std::time(nullptr)
        };

        // write handshake packet
        const sock::buf<HSLEN> handshake{};
        const sockaddr_in inaddr{
            .sin_family = AF_INET,
            .sin_port = htons(port),
            .sin_addr = { .s_addr = tunnels.remote },
        };
        sock::write(*tun.fd, handshake, HSLEN, inaddr);

        epoll::addFd(tunnels.epoll_fd, *tun.fd);
        tunnels.tunnels[*tun.fd] = std::move(tun);
    }
    [[noreturn]] void try_main() {
        Tunnels tunnels{
            .epoll_fd = epoll::createEpollFd(),
            .remote = sock::ipFromString("127.0.0.1")
        };

        while (true) {
            validate_tunnel(tunnels, 5000);
            validate_tunnel(tunnels, 5001);

            pollTunnels(tunnels);
        }
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
