#pragma once

#include "epoll/epoll.hpp"
#include "sock/sock.hpp"
#include "sock/udp.hpp"
#include "tun.hpp"

#include <cstdint>
#include <memory>

#include <netinet/in.h>

namespace client {

    /// callback for when data is received
    using DataCallback = std::function<void(const sock::buf<tun::RECVBUF>& buf, size_t len, const sockaddr_in& addr)>;

    /// handler for events from clients
    class ClientHandler : public epoll::EventHandler {
    public:
        /// create a client handler
        /// @param callback callback for received data
        ClientHandler(DataCallback callback)
            : on_data(std::move(callback)) {}

        /// handle an event
        void onEvent(std::shared_ptr<sock::Fd>& fd, uint32_t events) override;
    private:
        DataCallback on_data;
    };

    /// Structure containing client state
    struct Client {
        epoll::Epoll epoll; //!< single epoll instance

        tun::Tunnel outgoingTunnel; //!< tunnel to server

        sock::udp::UdpSocket incomingSocket; //!< socket for client
        ClientHandler incomingHandler; //!< client handler
        std::optional<sockaddr_in> caddr; //!< client address
    };

    /// client-side main entry point
    /// @param bport base port number
    /// @param bport_len number of ports to use
    /// @param tport target port number
    /// @param peer peer address
    /// @throws std::exception on error
    [[noreturn]] void main(uint16_t bport, uint16_t bport_len, uint16_t tport, in_addr_t peer);

}
