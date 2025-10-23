#include "udp.hpp"
#include "sock.hpp"

#include <cstddef>
#include <cstdint>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace sock::udp;

UdpSocket::UdpSocket()
        : fd(socket(AF_INET, SOCK_DGRAM, 0)) {
    if (this->fd < 0)
        throw sock_error("socket() failed");
}

UdpSocket::UdpSocket(uint16_t port) : UdpSocket() {
    const sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { .s_addr = INADDR_ANY }
    };
    if (bind(this->fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0)
        throw sock_error("bind() failed");
}

UdpSocket::~UdpSocket() {
    close(fd);
}

size_t UdpSocket::recv(char* buf, size_t len, sockaddr_in* addr) const {
    socklen_t addrlen = sizeof(*addr);

    const ssize_t ret = recvfrom(this->fd, buf, len, 0, reinterpret_cast<sockaddr*>(addr), &addrlen);
    if (ret < 0)
        throw sock_error("recvfrom() failed");

    return static_cast<size_t>(ret);
}

void UdpSocket::send(const char* buf, size_t len, const sockaddr_in& addr) const {
    const ssize_t ret = sendto(this->fd, buf, len, 0,
            reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (ret < 0)
        throw sock_error("sendto() failed");
}
