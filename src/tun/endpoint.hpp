#pragma once

#include "../epoll/epoll.hpp"
#include "../sock/sock.hpp"
#include "../sock/udp.hpp"
#include "../config.hpp"

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <memory>
#include <vector>

#include <netinet/in.h>

namespace endpoint {

    const size_t RECVBUF = 65535; //!< size of the receive buffer

    /// connection instance
    class Connection : public sock::udp::UdpSocket {
    public:
        /// create a socket
        Connection(in_addr_t peer, uint16_t port)
            : sock::udp::UdpSocket(port), peer_(peer) {}

        /// get peer address
        /// @return peer address
        [[nodiscard]] in_addr_t peer() const { return this->peer_; }

        /// get actual address
        /// @return actual address
        [[nodiscard]] const sockaddr_in& addr() const { return this->addr_; }
        /// set actual address
        /// @param addr new actual address
        void addr(const sockaddr_in& addr) { this->addr_ = addr; }

        /// get validity
        /// @return validity
        [[nodiscard]] bool is_valid() const { return this->is_valid_; }
        /// set validity
        /// @param t new validity
        void is_valid(bool t) { this->is_valid_ = t; }

        // non-copyable and non-movable
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection(Connection&&) = delete;
        Connection& operator=(Connection&&) = delete;
        ~Connection() override;
    private:
        in_addr_t peer_{}; //!< expected peer address
        sockaddr_in addr_{}; //!< actual peer address
        bool is_valid_{};
    };

    /// callback for when data is received
    using DataCallback = std::function<void(const sock::buf<RECVBUF>& buf, size_t len)>;

    /// handler for endpoint events
    class EndpointHandler : public epoll::EventHandler {
    public:
        /// create a handler
        /// @param callback callback for data events
        EndpointHandler(DataCallback callback)
            : on_data(std::move(callback)) {}

        /// handle an event
        void onEvent(std::shared_ptr<sock::Fd>& fd, uint32_t events) override;
    private:
        DataCallback on_data;
    };

    /// tunnel endpoint instance
    class Endpoint {
    public:
        /// create a endpoint
        /// @param epoll epoll instance
        /// @param on_data callback for data events
        /// @param baseport base port
        /// @param connections connections configurations
        Endpoint(epoll::Epoll& epoll, DataCallback on_data,
            uint16_t baseport, const std::vector<config::ServerConnectionConfig>& connections);

        /// write data to the endpoint
        /// @param buf buffer to write
        /// @param len length of data
        void write(const sock::buf<RECVBUF>& buf, size_t len);
    private:
        std::shared_ptr<EndpointHandler> handler;

        std::vector<std::shared_ptr<Connection>> conns;
        uint16_t rr_idx{}; //!< round-robin index
    };


}
