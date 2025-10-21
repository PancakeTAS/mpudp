#include "server/tun.hpp"
#include "constants.hpp"
#include "epoll.hpp"
#include "sock.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include <netinet/in.h>
#include <sys/types.h>

using namespace tun;

Tunnel::Tunnel(epoll::EPoll& epoll, const std::vector<std::pair<uint16_t, in_addr_t>>& conns) {
    this->conns.reserve(conns.size());
    for (const auto& [port, addr] : conns) {
        Connection conn{
            .fd = sock::openBoundDgramSocket(port),
            .peer = addr,
            .established = false,
            .event_flag = std::make_unique<uint32_t>(0),
        };

        epoll.add(*conn.fd, conn.event_flag);
        this->conns.push_back(std::move(conn));
    }
}

void Tunnel::poll(const std::function<void(sock::buf<RECV_BUF>&, size_t)>& onData) {
    struct sockaddr_in inaddr{};

    for (auto& conn : this->conns) {
        if ((*conn.event_flag & EPOLLIN) == 0) // ignore non-readable events
            continue;
        *conn.event_flag = 0; // reset event flag

        // read from socket
        const ssize_t nb = sock::read(*conn.fd, recvbuf, inaddr);

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

            // write back Y if established, else N
            // to indicate success/failure of handshake
            const char sendbuf = valid ? 'Y' : 'N';
            sock::write(*conn.fd, &sendbuf, 1, inaddr);

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
    if (this->conns.empty())
        throw "no tunnel connections available";

    this->rridx = (this->rridx + 1) % this->conns.size();
    const auto& conn = this->conns[this->rridx];

    sock::write(*conn.fd, buf, n, conn.addr);
}
