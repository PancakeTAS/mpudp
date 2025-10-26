#include "tun.hpp"
#include "../epoll/epoll.hpp"
#include "../sock/sock.hpp"
#include "../config.hpp"

#include <cstdint>
#include <ctime>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

using namespace tun;

Connection::~Connection() = default;

void TunnelHandler::onEvent(std::shared_ptr<sock::Fd>& fd, uint32_t events) {
    if ((events & EPOLLIN) == 0)
        return;

    auto& conn = reinterpret_cast<Connection&>(*fd);
    sockaddr_in addr{};

    // read data from socket
    sock::buf<RECVBUF> recvbuf{}; // FIXME: maybe move to heap?
    const size_t len = conn.recv(recvbuf, recvbuf.size(), &addr);

    // if the tunnel is not yet established
    // run through the handshake process
    if (conn.state() == ConnState::UNCONN) {
        // establish if a single 'Y' byte is received
        if (len == 1 && recvbuf[0] == 'Y') {
            std::cerr << "connection established on port " << ntohs(addr.sin_port) << '\n';
            conn.state(ConnState::VALID);
        } else {
            std::cerr << "connection handshake failed on port " << ntohs(addr.sin_port) << '\n';
            conn.state(ConnState::INVALID);
        }

        return; // skip further processing
    }

    // otherwise process data normally
    this->on_data(recvbuf, len);
}

Tunnel::Tunnel(DataCallback on_data, in_addr_t peer,
            uint16_t baseport, const std::vector<config::ClientConnectionConfig>& connections)
        : peer(peer), baseport(baseport) {
    this->handler = std::make_shared<TunnelHandler>(std::move(on_data));
    this->conns.resize(connections.size());

    std::vector<uint32_t> weights;
    weights.reserve(connections.size());
    for (const auto& conn : connections)
        weights.push_back(conn.weight);
    this->wrr = wrr::Selector(weights);
}

void Tunnel::checkConnection(epoll::Epoll& epoll, uint16_t idx) {
    auto& conn = this->conns.at(idx);

    if (conn && conn->state() == ConnState::VALID) // skip established
        return;

    // invalidate any connection which hasn't
    // completed the handshake in 5 seconds
    if (conn && conn->state() == ConnState::UNCONN) {
        const time_t now = std::time(nullptr);
        if (now - conn->htime() <= 5)
            return; // still waiting

        std::cerr << "connection timeout on port " << (this->baseport + idx) << '\n';
    }

    // past this point, connections are invalid.
    // if there actually was one to begin with
    // remove it from epoll and reset it
    if (conn) {
        epoll.remove(*conn);
        conn.reset();

        usleep(200000); // 200ms
    }

    // now recreate the connection...
    conn = std::make_unique<Connection>();
    conn->htime(std::time(nullptr));
    conn->state(ConnState::UNCONN);
    epoll.add(conn, this->handler, EPOLLIN);

    // ...and write handshake packet
    const sock::buf<5> handshake{};
    const sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(this->baseport + idx),
        .sin_addr = in_addr { .s_addr = this->peer },
    };
    conn->send(handshake, 5, addr);
}

void Tunnel::write(const sock::buf<RECVBUF>& buf, size_t len) {
    if (this->conns.empty())
        throw std::runtime_error("no tunnel connections available");

    const auto next = static_cast<size_t>(this->wrr.next());
    const auto& conn = this->conns.at(next);
    if (conn->state() != ConnState::VALID) {
        std::cerr << "dropping packet due to invalid connection on port " << (this->baseport + next) << '\n';
        return;
    }

    const sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(static_cast<uint16_t>(this->baseport + next)),
        .sin_addr = in_addr { .s_addr = this->peer },
    };
    conn->send(buf, len, addr);
}
