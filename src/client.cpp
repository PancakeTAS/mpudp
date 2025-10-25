#include "client.hpp"
#include "epoll/epoll.hpp"
#include "sock/udp.hpp"
#include "tun.hpp"
#include "sock/sock.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <netinet/in.h>

using namespace client;

namespace {
    /// totally one-hundred percent memory-safe pointer to client state
    Client* cstate;
    /// handler for received data from tunnel
    void on_data(const sock::buf<tun::RECVBUF>& buf, size_t len) {
        if (!cstate->caddr.has_value()) {
            std::cerr << "dropping packet due to no client address\n";

            return;
        }

        cstate->incomingSocket.send(buf, len, *cstate->caddr);
    }
    /// handler for received data from clients
    void on_inc_data(const sock::buf<tun::RECVBUF>& buf, size_t len, const sockaddr_in& addr) {
        cstate->caddr.emplace(addr);

        cstate->outgoingTunnel.write(buf, len);
    }
}

void client::main(uint16_t bport, uint16_t bport_len, uint16_t tport, in_addr_t peer) {
    // initialize client state
    auto c = std::shared_ptr<Client>(new Client { // NOLINT
        .epoll = epoll::Epoll{},
        .outgoingTunnel = tun::Tunnel(on_data, peer, bport, bport_len),
        .incomingSocket = sock::udp::UdpSocket(tport),
        .incomingHandler = ClientHandler(on_inc_data)
    });

    cstate = c.get();

    c->epoll.add( // register incoming socket to epoll
        std::shared_ptr<sock::udp::UdpSocket>(c, &c->incomingSocket),
        std::shared_ptr<ClientHandler>(c, &c->incomingHandler),
        EPOLLIN
    );

    while (true) {
        for (uint16_t i = 0; i < bport_len; ++i)
            c->outgoingTunnel.checkConnection(c->epoll, i);
        c->epoll.poll(1000);
    }
}

// boring client handler implementation
void ClientHandler::onEvent(std::shared_ptr<sock::Fd>& fd, uint32_t events) {
    if ((events & EPOLLIN) == 0)
        return;

    auto& conn = dynamic_cast<sock::udp::UdpSocket&>(*fd);
    sockaddr_in addr{};

    sock::buf<tun::RECVBUF> recvbuf{}; // FIXME: maybe move to heap?
    const size_t len = conn.recv(recvbuf, recvbuf.size(), &addr);

    this->on_data(recvbuf, len, addr);
}
