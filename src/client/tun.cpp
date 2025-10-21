#include "client/tun.hpp"
#include "constants.hpp"
#include "epoll.hpp"
#include "sock.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <iostream>
#include <utility>

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace tun;

Tunnel::Tunnel(in_addr_t remote)
        : epfd{epoll::createEpollFd()}, remote{remote} {
    // nothing to do :3
}

void Tunnel::validateConnection(uint16_t port) {
    auto it = std::ranges::find_if(conns,
        [port](const auto& pair) {
            return pair.second.port == port;
        });
    // validate the tunnel if it already exists
    if (it != this->conns.end()) {
        auto& conn = it->second;
        if (conn.state == ConnState::VALID) // skip established tunnels
            return;

        // remove any tunnel which was explicitly marked as invalid
        // or hasn't established in 5s
        const time_t now = std::time(nullptr);
        if (conn.state == ConnState::INVALID || now - conn.hshake_tsamp > 5) {
            if (now - conn.hshake_tsamp > 5) // kind of redundant check, but whatever
                std::cerr << "tunnel timeout on port " << conn.port << "\n";

            epoll::removeFd(this->epfd, *conn.fd);
            auto it_ = std::ranges::find(this->conns_, &it->second);
            if (it_ != conns_.end())
                conns_.erase(it_);
            this->conns.erase(it);
        } else {
            return; // still waiting
        }
    }

    // (re)-add the tunnel
    Connection conn{
        .fd = sock::openDgramSocket(),
        .port = port,
        .hshake_tsamp = std::time(nullptr),
        .state = ConnState::UNCONN
    };

    // write handshake packet
    const sock::buf<HSLEN> handshake{};
    const sockaddr_in inaddr{
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { .s_addr = this->remote },
    };
    sock::write(*conn.fd, handshake, HSLEN, inaddr);

    epoll::addFd(this->epfd, *conn.fd);
    const auto& entry = this->conns.emplace(*conn.fd, std::move(conn));
    this->conns_.push_back(&entry.first->second);
}

void Tunnel::poll(const std::function<void(sock::buf<RECV_BUF>&, size_t)>& onData) {
    static std::array<struct epoll_event, 16> events;

    // poll and iterate through events
    const size_t n = epoll::poll(this->epfd, events);
    for (size_t i = 0; i < n; ++i) {
        const struct epoll_event& ev = events.at(i);
        if ((ev.events & EPOLLIN) == 0) // ignore non-readable events
            continue;

        const int fd = ev.data.fd;
        auto& tunnel = this->conns[fd];

        struct sockaddr_in inaddr{};
        const ssize_t nb = sock::read(fd, recvbuf, inaddr);

        // if the tunnel is not yet established,
        // run through the handshake process
        if (tunnel.state == ConnState::UNCONN) {
            // establish if a single 'Y' byte is received
            if (nb == 1 && recvbuf[0] == 'Y') {
                std::cerr << "tunnel established on port " << tunnel.port << "\n";
                tunnel.state = ConnState::VALID;
            } else {
                std::cerr << "tunnel cancelled on port " << tunnel.port << "\n";
                tunnel.state = ConnState::INVALID; // mark as invalid
            }

            continue; // skip further processing until established
        }

        onData(recvbuf, static_cast<size_t>(nb));
    }
}

void Tunnel::write(const sock::buf<SEND_BUF>& buf, size_t n) {
    if (this->conns_.empty())
        throw "no tunnel connections available";

    this->rridx = (this->rridx + 1) % this->conns_.size();

    const auto& conn = this->conns_.at(this->rridx);
    const struct sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(conn->port),
        .sin_addr = { .s_addr = this->remote },
    };
    sock::write(*conn->fd, buf, n, addr);
}
