#pragma once

#include "own.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include <sys/types.h>
#include <netinet/in.h>

namespace sock {
    template<size_t N>
    using buf = std::array<char, N>;
    /// open a UDP socket bound to the given port at any address
    own::owned_fd openBoundDgramSocket(uint16_t port);
    /// translate a string into an ip address
    in_addr_t ipFromString(const std::string& str);
    /// unsafe: read data from the udp socket
    ssize_t read(int fd, char* buf, size_t bufsize, struct sockaddr_in& addr);
    /// unsafe: write data to the udp socket
    void write(int fd, const char* buf, size_t nb, const struct sockaddr_in& addr);
    /// read data from the udp socket
    template<size_t N>
    ssize_t read(int fd, sock::buf<N>& buf, struct sockaddr_in& addr) {
        return read(fd, buf.data(), buf.size(), addr);
    }
    /// write data to the udp socket
    template<size_t N>
    void write(int fd, const sock::buf<N>& buf, size_t nb, const struct sockaddr_in& addr) {
        write(fd, buf.data(), nb, addr);
    }
}
