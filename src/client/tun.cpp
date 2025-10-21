#include "client/tun.hpp"
#include "constants.hpp"
#include "epoll.hpp"
#include "sock.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <utility>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace tun;

Tunnel::Tunnel(in_addr_t remote) : remote{remote} {
    // nothing to do :3
}

void Tunnel::validateConnection(epoll::EPoll& epoll, uint16_t port) {
    auto conn = std::ranges::find_if(conns,
        [port](const auto& conn) {
            return conn.port == port;
        });
    // validate the tunnel if it already exists
    if (conn != this->conns.end()) {
        if (conn->state == ConnState::VALID) // skip established tunnels
            return;

        // remove any tunnel which was explicitly marked
        // as invalid or hasn't established in 5s
        const time_t now = std::time(nullptr);
        if (conn->state == ConnState::INVALID || now - conn->hshake_tsamp > 5) {
            if (now - conn->hshake_tsamp > 5) // kind of redundant check, but whatever
                std::cerr << "tunnel timeout on port " << conn->port << "\n";

            this->conns.erase(conn);
        } else {
            return; // still waiting
        }
    }

    // (re)-add the tunnel
    Connection newconn{
        .fd = sock::openDgramSocket(),
        .port = port,
        .hshake_tsamp = std::time(nullptr),
        .state = ConnState::UNCONN,
        .event_flag = std::make_unique<uint32_t>(0),
    };

    // write handshake packet
    const sock::buf<HSLEN> handshake{};
    const sockaddr_in inaddr{
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { .s_addr = this->remote },
    };
    sock::write(*newconn.fd, handshake, HSLEN, inaddr);

    epoll.add(*newconn.fd, newconn.event_flag);
    this->conns.push_back(std::move(newconn));
}

void Tunnel::poll(const std::function<void(sock::buf<RECV_BUF>&, size_t)>& onData) {
    struct sockaddr_in inaddr{};

    for (auto& conn : this->conns) {
        if ((*conn.event_flag & EPOLLIN) == 0) // skip non-readable events
            continue;
        *conn.event_flag = 0;

        // read data from the socket
        const ssize_t nb = sock::read(*conn.fd, recvbuf, inaddr);

        // if the tunnel is not yet established,
        // run through the handshake process
        if (conn.state == ConnState::UNCONN) {
            // establish if a single 'Y' byte is received
            if (nb == 1 && recvbuf[0] == 'Y') {
                std::cerr << "tunnel established on port " << conn.port << "\n";
                conn.state = ConnState::VALID;
            } else {
                std::cerr << "tunnel cancelled on port " << conn.port << "\n";
                conn.state = ConnState::INVALID; // mark as invalid
            }

            continue; // skip further processing until established
        }

        // otherwise, process the received data
        onData(recvbuf, static_cast<size_t>(nb));
    }
}

void Tunnel::write(const sock::buf<SEND_BUF>& buf, size_t n) {
    if (this->conns.empty())
        throw "no tunnel connections available";

    this->rridx = (this->rridx + 1) % this->conns.size();
    const auto& conn = this->conns[this->rridx];

    const struct sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(conn.port),
        .sin_addr = { .s_addr = this->remote },
    };
    sock::write(*conn.fd, buf, n, addr);
}
