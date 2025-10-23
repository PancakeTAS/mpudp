#include "tcp.hpp"
#include "sock.hpp"

#include <cstddef>
#include <cstdint>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace sock::tcp;

TcpSocket::TcpSocket(in_addr_t addr, uint16_t port)
        : fd(socket(AF_INET, SOCK_STREAM, 0)) {
    if (this->fd < 0)
        throw sock_error("socket() failed");

    const sockaddr_in server_addr{
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { .s_addr = addr }
    };
    if (connect(this->fd, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        close(this->fd);

        throw sock_error("connect() failed");
    }
}

TcpSocket::TcpSocket(uint16_t port)
        : fd(socket(AF_INET, SOCK_STREAM, 0)) {
    if (this->fd < 0)
        throw sock_error("socket() failed");

    const sockaddr_in server_addr{
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { .s_addr = INADDR_ANY }
    };
    if (bind(this->fd, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        close(this->fd);

        throw sock_error("bind() failed");
    }

    if (listen(this->fd, SOMAXCONN) < 0) {
        close(this->fd);

        throw sock_error("listen() failed");
    }
}

TcpSocket::~TcpSocket() {
    close(fd);
}

size_t TcpSocket::recv(char* buf, size_t len) const {
    const ssize_t ret = ::recv(this->fd, buf, len, 0);
    if (ret < 0)
        throw sock_error("recv() failed");

    return static_cast<size_t>(ret);
}

void TcpSocket::send(const char* buf, size_t len) const {
    const ssize_t ret = ::send(this->fd, buf, len, 0);
    if (ret < 0)
        throw sock_error("send() failed");
}

TcpSocket TcpSocket::accept(sockaddr_in& addr) const {
    socklen_t addrlen = sizeof(addr);

    const int cfd = ::accept(this->fd, reinterpret_cast<sockaddr*>(&addr), &addrlen);
    if (cfd < 0)
        throw sock_error("accept() failed");

    return TcpSocket(cfd);
}
