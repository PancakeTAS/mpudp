#pragma once

#include "constants.hpp"
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

    /// represents a single tunnel connection
    struct Connection {
        own::owned_fd fd;

        in_addr_t peer{}; //!< address to expect packets from
        bool established{false}; //!< whether handshake is complete

        struct sockaddr_in addr{}; //!< address packets are received from
    };

    /// tunnel instance acting on a set of connections
    class Tunnel {
    public:
        /// setup a new tunnel endpoint with given connections
        Tunnel(const std::vector<std::pair<uint16_t, in_addr_t>>& conns);

        /// poll all connections for incoming data
        void poll(const std::function<void(sock::buf<RECV_BUF>&, size_t)>& onData);
        /// send data to the next connection in round-robin fashion
        void write(const sock::buf<SEND_BUF>& buf, size_t n);
    private:
        own::owned_fd epfd;
        sock::buf<RECV_BUF> recvbuf{};

        std::unordered_map<int, Connection> conns;
        std::vector<Connection*> conns_;

        size_t rridx{0}; //!< round-robin index
    };

}
