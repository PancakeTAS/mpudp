#include "client/tun.hpp"
#include "constants.hpp"
#include "epoll.hpp"
#include "own.hpp"
#include "sock.hpp"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

#include <netinet/in.h>
#include <sys/types.h>

namespace {
    // void handle_data(sock::buf<tun::RECV_BUF>& data, size_t len) {
    //     std::cerr << "recv " << len << " bytes\n";
    // }
    [[noreturn]] void try_main() {
        epoll::EPoll epoll{};

        tun::Tunnel tun{
            sock::ipFromString("136.243.2.114")
        };

        const own::owned_fd incoming_sock = sock::openBoundDgramSocket(51280);
        const int fd = *incoming_sock;

        sock::buf<tun::SEND_BUF> sendbuf{};
        std::unique_ptr<uint32_t> incoming_data = std::make_unique<uint32_t>(0);
        epoll.add(fd, incoming_data);

        struct sockaddr_in addr{};

        while (true) {
            tun.validateConnection(epoll, 5000);
            tun.validateConnection(epoll, 5001);


            epoll.poll(1000);
            tun.poll([fd=fd,paddr=&addr](sock::buf<tun::RECV_BUF>& data, size_t len) {
                sock::write(fd, data, len, *paddr);
            });

            if (*incoming_data & EPOLLIN) {
                *incoming_data = 0;

                const ssize_t n = sock::read(fd, sendbuf, addr);
                tun.write(sendbuf, static_cast<size_t>(n));
            }
        }
    }
}


int main() {
    try {
        try_main();
    } catch (const char* e) {
        std::cerr << e << " (" << std::strerror(errno) << ")\n";
        return 1;
    }
}
