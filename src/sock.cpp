#include "sock.hpp"
#include "own.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

own::owned_fd sock::openDgramSocket() {
    const int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        throw "socket() failed";

    own::owned_fd fd{new int(sockfd)};
    return fd;
}

own::owned_fd sock::openBoundDgramSocket(uint16_t port) {
    own::owned_fd fd = sock::openDgramSocket();

    const struct sockaddr_in saddr_in{
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = {
            .s_addr = INADDR_ANY,
        }
    };
    if (bind(*fd, reinterpret_cast<const struct sockaddr*>(&saddr_in), sizeof(saddr_in)) < 0)
        throw "bind() failed";
    return fd;
}

in_addr_t sock::ipFromString(const std::string& str) {
    struct in_addr addr{};
    if (inet_pton(AF_INET, str.c_str(), &addr) != 1)
        throw "inet_pton() failed";
    return addr.s_addr;
}

ssize_t sock::read(int fd, char* buf, size_t bufsize, struct sockaddr_in& addr) {
    socklen_t addr_len = sizeof(addr);

    const ssize_t n = recvfrom(fd, buf, bufsize, 0,
        reinterpret_cast<struct sockaddr*>(&addr), &addr_len);
    if (n < 0)
        throw "recvfrom() failed";

    return n;
}

void sock::write(int fd, const char* buf, size_t nb, const struct sockaddr_in& addr) {
    const ssize_t n = sendto(fd, buf, nb, 0, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr));
    if (n < 0)
        throw "sendto() failed";
}
