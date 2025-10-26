#include "server.hpp"
#include "../epoll/epoll.hpp"
#include "../tun/endpoint.hpp"
#include "../sock/sock.hpp"
#include "../config.hpp"

#include <cstddef>
#include <cstdint>

#include <memory>

#include <sys/socket.h>
#include <netinet/in.h>

using namespace server;

namespace {
    /// totally one-hundred percent memory-safe pointer to server state
    Server* sstate;
    /// handler for received data through tunnel
    void on_data(const sock::buf<MPUDP_RECVBUF>& buf, size_t len) {
        sstate->outgoingSocket.send(buf, len, sstate->saddr);
    }
    /// handler for received data from the server
    void on_inc_data(const sock::buf<MPUDP_RECVBUF>& buf, size_t len) {
        sstate->incomingEndpoint.write(buf, len);
    }
}
void server::main(const config::ServerConfig& config) {
    // initialize server state
    epoll::Epoll epoll{};
    auto s = std::shared_ptr<Server>(new Server { // NOLINT
        .incomingEndpoint = endpoint::Endpoint(epoll, on_data, config.baseport, config.connections),
        .outgoingSocket = sock::udp::UdpSocket(),
        .outgoingHandler = ServerHandler(on_inc_data),
        .saddr = sockaddr_in {
            .sin_family = AF_INET,
            .sin_port = htons(config.sendport),
            .sin_addr = { .s_addr = sock::stoia("127.0.0.1") }
        }
    });

    sstate = s.get();

    epoll.add( // register outgoing socket to epoll
        std::shared_ptr<sock::udp::UdpSocket>(s, &s->outgoingSocket),
        std::shared_ptr<ServerHandler>(s, &s->outgoingHandler),
        EPOLLIN
    );

    while (true)
        epoll.poll();
}

// boring server handler implementation
void ServerHandler::onEvent(std::shared_ptr<sock::Fd>& fd, uint32_t events) {
    if ((events & EPOLLIN) == 0)
        return;

    auto& conn = reinterpret_cast<sock::udp::UdpSocket&>(*fd);
    sockaddr_in addr{};

    sock::buf<MPUDP_RECVBUF> recvbuf; // FIXME: maybe move to heap?
    const size_t len = conn.recv(recvbuf, recvbuf.size(), &addr);

    this->on_data(recvbuf, len);
}
