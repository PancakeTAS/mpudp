#include "endpoint.hpp"
#include "../epoll/epoll.hpp"
#include "../sock/sock.hpp"

#include <cstdint>
#include <ctime>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <netinet/in.h>

using namespace endpoint;

Connection::~Connection() = default;

void EndpointHandler::onEvent(std::shared_ptr<sock::Fd>& fd, uint32_t events) {
    if ((events & EPOLLIN) == 0)
        return;

    auto& conn = dynamic_cast<Connection&>(*fd);
    sockaddr_in addr{};

    // read data from socket
    sock::buf<RECVBUF> recvbuf{}; // FIXME: maybe move to heap?
    const size_t len = conn.recv(recvbuf, recvbuf.size(), &addr);

    // if the tunnel is not yet established
    // run through the handshake process
    const bool valid = addr.sin_addr.s_addr == conn.peer();
    if (!conn.is_valid()) {
        // establish if source ip is valid
        // and a 5-byte handshake is received
        if (len == 5 && valid) {
            std::cerr << "tunnel established on port " << ntohs(conn.addr().sin_port) << "\n";
            conn.is_valid(true);
            conn.addr(addr);
        } else {
            std::cerr << "invalid handshake on port " << ntohs(conn.addr().sin_port) << "\n";
        }

        // write back Y if established, else N
        // to indicate success/failure of handshake
        const sock::buf<1> sendbuf = { conn.is_valid() ? 'Y' : 'N' };
        conn.send(sendbuf, 1, addr);

        return;
    }

    // drop invalid packets
    // FIXME: should be handled better
    if (!valid) {
        std::cerr << "dropped invalid packet on port " << ntohs(conn.addr().sin_port) << "\n";
        return;
    }

    this->on_data(recvbuf, len);
}

Endpoint::Endpoint(epoll::Epoll& epoll, DataCallback on_data,
        uint16_t bport, const std::vector<in_addr_t>& peers) {
    this->handler = std::make_shared<EndpointHandler>(std::move(on_data));

    this->conns.reserve(peers.size());
    for (size_t i = 0; i < peers.size(); ++i) {
        const auto& peer = peers.at(i);

        const auto& conn = this->conns.emplace_back(std::make_shared<Connection>(peer, bport + i));
        epoll.add(conn, this->handler, EPOLLIN);
    }
}

void Endpoint::write(const sock::buf<RECVBUF>& buf, size_t len) {
    if (this->conns.empty())
        throw std::runtime_error("no tunnel connections available");

    this->rr_idx = (this->rr_idx + 1) % this->conns.size();
    const auto& conn = this->conns.at(this->rr_idx);

    conn->send(buf, len, conn->addr());
}
