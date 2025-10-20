#pragma once

#include "own.hpp"

#include <array>
#include <cstddef>

#include <sys/epoll.h>

namespace epoll {
    /// create an epoll fd
    own::owned_fd createEpollFd();
    /// add file descriptor to epoll instance
    void addFd(own::owned_fd& epoll_fd, int fd);
    /// remove file descriptor from epoll instance
    void removeFd(own::owned_fd& epoll_fd, int fd);
    /// unsafe: poll epoll instance for events
    size_t poll(own::owned_fd& epoll_fd, struct epoll_event* events, size_t max_events);
    /// poll epoll instance for events
    template<size_t N>
    size_t poll(own::owned_fd& epoll_fd, std::array<struct epoll_event, N>& events) {
        return poll(epoll_fd, events.data(), events.size());
    }
}
