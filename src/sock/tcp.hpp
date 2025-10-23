#pragma once

#include "sock.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

#include <netinet/in.h>

namespace sock::tcp {

    /// simple tcp socket wrapper
    class TcpSocket {
    public:
        /// create a new tcp socket
        /// @param addr the address to connect to
        /// @param port the port to connect to
        /// @throws sock_error on failure
        TcpSocket(in_addr_t addr, uint16_t port);

        /// create a bound tcp socket
        /// @param port the port to bind to
        /// @throws sock_error on failure
        TcpSocket(uint16_t port);

        /// receive data from the socket
        /// @param buf the buffer to receive data into
        /// @param len optionally the number of bytes to receive
        /// @return the number of bytes received
        /// @throws sock_error on failure
        template<size_t N>
        size_t recv(buf<N>& buf, std::optional<size_t> len = std::nullopt) const {
            return recv(buf.data(), len.value_or(N));
        }

        /// write data to the socket
        /// @param buf the buffer to send data from
        /// @param len the number of bytes to send
        /// @throws sock_error on failure
        template<size_t N>
        void send(const buf<N>& buf, size_t len) const {
            send(buf.data(), len);
        }

        /// accept a new connection
        /// @param addr address of the new client
        /// @return a new TcpSocket representing the accepted connection
        TcpSocket accept(sockaddr_in& addr) const;

        // non-copyable and non-movable
        TcpSocket(const TcpSocket&) = delete;
        TcpSocket& operator=(const TcpSocket&) = delete;
        TcpSocket(TcpSocket&&) = delete;
        TcpSocket& operator=(TcpSocket&&) = delete;
        ~TcpSocket();
    private:
        int fd;

        explicit TcpSocket(int fd) : fd(fd) {}
        size_t recv(char* buf, size_t len) const;
        void send(const char* buf, size_t len) const;
    };

}
