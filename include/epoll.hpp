#pragma once

#include <cstdint>

#include <memory>
#include <sys/epoll.h>

namespace epoll {
    /// mediocre performance object-oriented epoll wrapper
    class EPoll {
    public:
        /// create an epoll instance
        EPoll();

        /// add file descriptor to epoll instance. automatically removed on close.
        void add(int fd, const std::unique_ptr<uint32_t>& event_flag, uint32_t events = EPOLLIN) const;
        /// poll epoll instance for events.
        void poll(int timeout_ms = -1) const;
    private:
        std::shared_ptr<int> epfd;
    };
}
