#pragma once

#include "own.hpp"
#include "sock.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <netinet/in.h>

namespace tun {

    const size_t RECV_BUF = 65535; //!< receive buffer size
    const size_t HSLEN = 5; //!< handshake packet length

    /// represents a single tunnel connection
    struct Connection {
        own::owned_fd fd;

        in_addr_t peer{}; //!< address to expect packets from
        bool established{false}; //!< whether handshake is complete
    };

    /// tunnel instance acting on a set of connections
    class Tunnel {
    public:
        /// setup a new tunnel endpoint with given connections
        Tunnel(const std::vector<std::pair<uint16_t, in_addr_t>>& conns);

        /// poll all connections for incoming data
        void poll(const std::function<void(sock::buf<RECV_BUF>&, size_t)>& onData);
    private:
        own::owned_fd epfd;
        sock::buf<RECV_BUF> recvbuf{};

        std::unordered_map<int, Connection> conns;
    };

}
