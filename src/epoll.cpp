#include "epoll.hpp"
#include "own.hpp"

#include <cstddef>

#include <sys/epoll.h>

own::owned_fd epoll::createEpollFd() {
    const int efd = epoll_create1(0);
    if (efd < 0)
        throw "epoll_create1() failed";

    own::owned_fd fd{new int(efd)};
    return fd;
}

void epoll::addFd(own::owned_fd& epoll_fd, int fd) {
    struct epoll_event event{
        .events = EPOLLIN,
        .data = {
            .fd = fd,
        },
    };
    if (epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
        throw "epoll_ctl() failed";
}

void epoll::removeFd(own::owned_fd& epoll_fd, int fd) {
    if (epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, fd, nullptr) < 0)
        throw "epoll_ctl() failed";
}

size_t epoll::poll(own::owned_fd& epoll_fd, struct epoll_event* events, size_t max_events) {
    const int n = epoll_wait(*epoll_fd, events, static_cast<int>(max_events), 500);
    if (n < 0)
        throw "epoll_wait() failed";
    return static_cast<size_t>(n);
}
