#include "endpoint.hpp"
#include "../epoll/epoll.hpp"
#include "../sock/sock.hpp"
#include "../config.hpp"

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

    auto& conn = reinterpret_cast<Connection&>(*fd);
    sockaddr_in addr{};

    // read data from socket
    sock::buf<MPUDP_RECVBUF> recvbuf; // FIXME: maybe move to heap?
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
        uint16_t baseport, const std::vector<config::ServerConnectionConfig>& connections) {
    this->handler = std::make_shared<EndpointHandler>(std::move(on_data));

    this->conns.reserve(connections.size());
    for (size_t i = 0; i < connections.size(); ++i) {
        const auto& e = connections.at(i);

        const auto& conn = this->conns.emplace_back(std::make_shared<Connection>(e.peer, baseport + i));
        epoll.add(conn, this->handler, EPOLLIN);
    }

    std::vector<uint32_t> weights;
    weights.reserve(connections.size());
    for (const auto& conn : connections)
        weights.push_back(conn.weight);
    this->wrr = wrr::Selector(weights);
}

void Endpoint::write(const sock::buf<MPUDP_RECVBUF>& buf, size_t len) {
    if (this->conns.empty())
        throw std::runtime_error("no tunnel connections available");

    const auto next = static_cast<size_t>(this->wrr.next());
    const auto& conn = this->conns.at(next);
    if (conn->is_valid()) conn->send(buf, len, conn->addr());
}
