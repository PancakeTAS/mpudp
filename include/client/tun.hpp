#pragma once

#include "constants.hpp"
#include "epoll.hpp"
#include "own.hpp"
#include "sock.hpp"

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>

#include <memory>
#include <netinet/in.h>

namespace tun {

    /// state of a single tunnel connection
    enum class ConnState {
        UNCONN, //!< state prior to handshake
        VALID, //!< established and valid
        INVALID //!< error state
    };

    /// represents a single tunnel connection
    struct Connection {
        own::owned_fd fd;
        uint16_t port{};

        time_t hshake_tsamp{}; //!< timestamp of handshake attempt
        ConnState state{ConnState::UNCONN};

        std::unique_ptr<uint32_t> event_flag; //!< set by epoll
    };

    /// tunnel instance acting on a set of connections
    class Tunnel {
    public:
        /// setup a new tunnel client to remote address
        Tunnel(in_addr_t remote);

        /// validate or (re)-add a connection to the tunnel
        void validateConnection(epoll::EPoll& epoll, uint16_t port);
        /// poll all connections for incoming data
        void poll(const std::function<void(sock::buf<RECV_BUF>&, size_t)>& onData);
        /// send data to the next connection in round-robin fashion
        void write(const sock::buf<SEND_BUF>& buf, size_t n);
    private:
        sock::buf<RECV_BUF> recvbuf{};

        in_addr_t remote{};

        std::vector<Connection> conns;

        size_t rridx{0}; //!< round-robin index
    };

}
