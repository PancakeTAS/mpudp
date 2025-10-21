#include "epoll.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include <sys/epoll.h>
#include <unistd.h>

using namespace epoll;

EPoll::EPoll() {
    const int epfd = epoll_create1(0);
    if (epfd < 0)
        throw "epoll_create1() failed";

    this->epfd = std::shared_ptr<int>(
        new int(epfd),
        [](const int* epfd) {
            close(*epfd);
            delete epfd;
        });
}

void EPoll::add(int fd, const std::unique_ptr<uint32_t>& event_flag, uint32_t events) const {
    struct epoll_event event{
        .events = events,
        .data = {
            .ptr = event_flag.get()
        },
    };
    if (epoll_ctl(*this->epfd, EPOLL_CTL_ADD, fd, &event) < 0)
        throw "epoll_ctl() failed";

    *event_flag = 0;
}

void EPoll::poll(int timeout_ms) const {
    std::array<struct epoll_event, EPOLL_MAXEVENTS> events{};

    const int n = epoll_wait(*this->epfd, events.data(), EPOLL_MAXEVENTS, timeout_ms);
    if (n < 0)
        throw "epoll_wait() failed";

    for (size_t i = 0; std::cmp_less(i, n); i++) {
        auto* ev = &events.at(i);
        *static_cast<uint32_t*>(ev->data.ptr) = ev->events;
    }
}
