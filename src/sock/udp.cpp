#include "udp.hpp"
#include "sock.hpp"

#include <cstddef>
#include <cstdint>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace sock::udp;

UdpSocket::UdpSocket() {
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        throw sock_error("socket() failed");

    this->set(fd);
}

UdpSocket::UdpSocket(uint16_t port) : UdpSocket() {
    const sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { .s_addr = INADDR_ANY }
    };
    if (bind(*this, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0)
        throw sock_error("bind() failed");
}

size_t UdpSocket::recv(char* buf, size_t len, size_t skip, sockaddr_in* addr) const {
    socklen_t addrlen = sizeof(*addr);

    const ssize_t ret = recvfrom(*this, buf + skip, len - skip, 0, reinterpret_cast<sockaddr*>(addr), &addrlen);
    if (ret < 0)
        throw sock_error("recvfrom() failed");

    return static_cast<size_t>(ret);
}

void UdpSocket::send(const char* buf, size_t len, size_t skip, const sockaddr_in& addr) const {
    const ssize_t ret = sendto(*this, buf + skip, len - skip, MSG_ZEROCOPY,
            reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (ret < 0)
        throw sock_error("sendto() failed");
}

void UdpSocket::noop() {}
