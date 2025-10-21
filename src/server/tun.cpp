#include "server/tun.hpp"
#include "constants.hpp"
#include "epoll.hpp"
#include "sock.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/types.h>

using namespace tun;

Tunnel::Tunnel(const std::vector<std::pair<uint16_t, in_addr_t>>& conns) {
    this->epfd = epoll::createEpollFd();

    this->conns.reserve(conns.size());
    for (const auto& [port, addr] : conns) {
        Connection conn{
            .fd = sock::openBoundDgramSocket(port),
            .peer = addr,
            .established = false
        };

        epoll::addFd(this->epfd, *conn.fd);
        const auto& entry = this->conns.emplace(*conn.fd, std::move(conn));
        this->conns_.push_back(&entry.first->second);
    }
}

void Tunnel::poll(const std::function<void(sock::buf<RECV_BUF>&, size_t)>& onData) {
    std::array<struct epoll_event, 16> events{};

    // poll and iterate through events
    const size_t n = epoll::poll(this->epfd, events);
    for (size_t i = 0; i < n; ++i) {
        const struct epoll_event& ev = events.at(i);
        if ((ev.events & EPOLLIN) == 0) // ignore non-readable events
            continue;

        const int fd = ev.data.fd;
        auto& conn = this->conns[fd];

        struct sockaddr_in inaddr{};
        const ssize_t nb = sock::read(fd, recvbuf, inaddr);

        // if the connection is not yet established,
        // run through the handshake process
        const bool valid = inaddr.sin_addr.s_addr == conn.peer;
        if (!conn.established) {
            // establish if source ip is valid
            // and a HSLEN-byte packet is received
            if (std::cmp_equal(nb, HSLEN) && valid) {
                std::cerr << "tunnel established on port " << ntohs(inaddr.sin_port) << "\n";
                conn.established = true;
                conn.addr = inaddr;
            } else {
                std::cerr << "invalid handshake on port " << ntohs(inaddr.sin_port) << "\n";
            }

            // write back 1 if established, else 0
            // to indicate success/failure of handshake
            const char sendbuf = valid ? 'Y' : 'N';
            sock::write(fd, &sendbuf, 1, inaddr);

            continue;
        }

        // drop invalid packets
        // FIXME: this should be handled better
        if (!valid) {
            std::cerr << "dropping invalid packet on port " << ntohs(inaddr.sin_port) << "\n";
            continue;
        }

        onData(recvbuf, static_cast<size_t>(nb));
    }
}

void Tunnel::write(const sock::buf<SEND_BUF>& buf, size_t n) {
    if (this->conns_.empty())
        throw "no tunnel connections available";

    this->rridx = (this->rridx + 1) % this->conns_.size();

    const auto& conn = this->conns_.at(this->rridx);
    sock::write(*conn->fd, buf, n, conn->addr);
}
