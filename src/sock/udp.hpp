#pragma once

#include "sock.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

#include <netinet/in.h>

namespace sock::udp {

    /// simple udp socket wrapper
    class UdpSocket : public Fd {
    public:
        /// create a new udp socket
        /// @throws sock_error on failure
        UdpSocket();

        /// create a bound udp socket
        /// @param port the port to bind to
        /// @throws sock_error on failure
        UdpSocket(uint16_t port);

        /// receive data from the socket
        /// @param buf the buffer to receive data into
        /// @param len optionally the number of bytes to receive
        /// @param skip number of bytes to skip (this will decrease len automatically)
        /// @param addr optionally the address to receive from
        /// @return the number of bytes received
        /// @throws sock_error on failure
        template<size_t N>
        size_t recv(buf<N>& buf, std::optional<size_t> len, size_t skip,
                sockaddr_in* addr = nullptr) const {
            return recv(buf.data(), len.value_or(N), skip, addr);
        }

        /// write data to the socket
        /// @param buf the buffer to send data from
        /// @param len the number of bytes to send
        /// @param skip number of bytes to skip (this will decrease len automatically)
        /// @param addr the address to send to
        /// @throws sock_error on failure
        template<size_t N>
        void send(const buf<N>& buf, size_t len, size_t skip, const sockaddr_in& addr) const {
            send(buf.data(), len, skip, addr);
        }
    private:
        size_t recv(char* buf, size_t len, size_t skip, sockaddr_in* addr) const;
        void send(const char* buf, size_t len, size_t skip, const sockaddr_in& addr) const;

        virtual void noop(); // vtable anchor
    };

}
