#pragma once

#include "constants.hpp"
#include "own.hpp"
#include "sock.hpp"

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <unordered_map>

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
    };

    /// tunnel instance acting on a set of connections
    class Tunnel {
    public:
        /// setup a new tunnel client to remote address
        Tunnel(in_addr_t remote);

        /// validate or (re)-add a connection to the tunnel
        void validateConnection(uint16_t port);
        /// poll all connections for incoming data
        void poll(const std::function<void(sock::buf<RECV_BUF>&, size_t)>& onData);
    private:
        own::owned_fd epfd;
        sock::buf<RECV_BUF> recvbuf{};

        in_addr_t remote{};

        std::unordered_map<int, Connection> conns;
    };

}
