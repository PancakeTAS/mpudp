#include "sock.hpp"

#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace sock;

in_addr_t sock::stoia(const std::string& straddr) {
    in_addr addr{};
    if (inet_pton(AF_INET, straddr.c_str(), &addr) != 1)
        throw sock_error("inet_pton() failed for address: " + straddr);

    return addr.s_addr;
}

Fd::~Fd() {
    if (fd != -1)
        close(fd);
}

sock_error::~sock_error() = default;
