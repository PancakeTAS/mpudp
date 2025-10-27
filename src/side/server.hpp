#pragma once

#include "../epoll/epoll.hpp"
#include "../tun/endpoint.hpp"
#include "../sock/sock.hpp"
#include "../sock/udp.hpp"
#include "../config.hpp"

#include <cstdint>
#include <memory>

#include <netinet/in.h>

namespace server {

    /// callback for when data is received
    using DataCallback = std::function<void(sock::buf<MPUDP_RECVBUF>& buf, size_t len)>;

    /// handler for events from the server
    class ServerHandler : public epoll::EventHandler {
    public:
        /// create a server handler
        /// @param callback callback for received data
        ServerHandler(DataCallback callback)
            : on_data(std::move(callback)) {}

        /// handle an event
        void onEvent(std::shared_ptr<sock::Fd>& fd, uint32_t events) override;
    private:
        DataCallback on_data;
    };

    /// Structure containing server state
    struct Server {
        endpoint::Endpoint incomingEndpoint; //!< endpoint of client

        sock::udp::UdpSocket outgoingSocket; //!< socket to server
        ServerHandler outgoingHandler; //!< server handler
        sockaddr_in saddr; //!< server address
    };

    /// server-side main entry point
    /// @param config server configuration
    /// @throws std::exception on error
    [[noreturn]] void main(const config::ServerConfig& config);

}
