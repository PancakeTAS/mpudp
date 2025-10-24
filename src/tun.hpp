#pragma once

#include "epoll/epoll.hpp"
#include "sock/sock.hpp"
#include "sock/udp.hpp"

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <memory>
#include <vector>

#include <netinet/in.h>

namespace tun {

    const size_t RECVBUF = 65535; //!< size of the receive buffer

    /// state of a single connection
    enum class ConnState {
        UNCONN, //!< no handshake yet
        VALID, //!< established and valid
        INVALID //!< failed handshake
    };

    /// connection instance
    class Connection : public sock::udp::UdpSocket {
    public:
        /// create a socket
        Connection() = default;

        /// get the current state
        /// @return current state
        [[nodiscard]] ConnState state() const { return this->state_; }
        /// set the current state
        /// @param state new state
        void state(ConnState state) { this->state_ = state; }

        /// get handshake time
        /// @return handshake time
        [[nodiscard]] time_t htime() const { return this->htime_; }
        /// set handshake time
        /// @param t new handshake time
        void htime(time_t t) { this->htime_ = t; }

        // non-copyable and non-movable
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection(Connection&&) = delete;
        Connection& operator=(Connection&&) = delete;
        ~Connection() override;
    private:
        ConnState state_{ConnState::INVALID};
        time_t htime_{}; //!< time of handshake
    };

    /// callback for when data is received
    using DataCallback = std::function<void(const sock::buf<RECVBUF>& buf, size_t len)>;

    /// handler for tunnel events
    class TunnelHandler : public epoll::EventHandler {
    public:
        /// create a handler
        /// @param callback callback for data events
        TunnelHandler(DataCallback callback)
            : on_data(std::move(callback)) {}

        /// handle an event
        void onEvent(std::shared_ptr<sock::Fd>& fd, uint32_t events) override;
    private:
        DataCallback on_data;
    };

    /// tunnel instance
    class Tunnel {
    public:
        /// create a tunnel
        /// @param peer peer address
        /// @param on_data callback for data events
        /// @param bport base port
        /// @param ports number of ports
        Tunnel(DataCallback on_data, in_addr_t peer,
            uint16_t bport, uint16_t ports);

        /// check and potentially update a connection
        /// @param epoll epoll instance
        /// @param idx index of connection (used to calculate port)
        void checkConnection(epoll::Epoll& epoll, uint16_t idx);

        /// write data to the tunnel
        /// @param buf buffer to write
        /// @param len length of data
        void write(const sock::buf<RECVBUF>& buf, size_t len);
    private:
        in_addr_t peer;
        std::shared_ptr<TunnelHandler> handler;

        std::vector<std::shared_ptr<Connection>> conns;
        uint16_t bport{}; //!< lowest port
        uint16_t rr_idx{}; //!< round-robin index
    };


}
