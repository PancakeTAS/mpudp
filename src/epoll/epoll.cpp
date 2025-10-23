#include "epoll.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>

#include <unistd.h>
#include <sys/epoll.h>

using namespace epoll;

Epoll::Epoll() : epfd(epoll_create1(0)) {
    if (this->epfd < 0)
        throw epoll_error("epoll_create1() failed");
}

void Epoll::add(int fd, std::shared_ptr<EventHandler> handler, uint32_t events) {
    auto* user = new EpollData {
        .fd = fd,
        .handler = std::move(handler)
    };

    // insert into map or throw if already exists
    auto entry = this->fds.emplace(fd, user);
    if (!entry.second) {
        delete user;

        throw epoll_error("tried adding already registered fd", 0);
    }

    // register to epoll
    epoll_event ev{
        .events = events,
        .data = { .ptr = user }
    };
    if (epoll_ctl(this->epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        this->fds.erase(entry.first);
        delete user;

        throw epoll_error("epoll_ctl() failed");
    }
}

void Epoll::modify(int fd, std::shared_ptr<EventHandler> handler, uint32_t events) {
    // find in map or throw if not found
    auto entry = this->fds.find(fd);
    if (entry == this->fds.end())
        throw epoll_error("tried modifying unregistered fd", 0);

    // update epoll events
    auto& user = entry->second;
    epoll_event ev{
        .events = events,
        .data = { .ptr = user }
    };
    if (epoll_ctl(this->epfd, EPOLL_CTL_MOD, fd, &ev) < 0)
        throw epoll_error("epoll_ctl() failed");

    // update user data
    user->handler = std::move(handler);
}

void Epoll::remove(int fd) {
    // remove from map or throw if not found
    auto entry = this->fds.find(fd);
    if (entry == this->fds.end())
        throw epoll_error("tried removing unregistered fd", 0);

    // remove from epoll
    if (epoll_ctl(this->epfd, EPOLL_CTL_DEL, fd, nullptr) < 0)
        throw epoll_error("epoll_ctl() failed");

    // delete user data
    delete entry->second;
    this->fds.erase(entry);
}

void Epoll::poll(int timeout, std::span<epoll_event> events) const {
    // fetch events
    const int n = epoll_wait(this->epfd, events.data(), static_cast<int>(events.size()), timeout);
    if (n < 0)
        throw epoll_error("epoll_wait() failed");

    // call corresponding handler
    for (size_t i = 0; std::cmp_less(i ,n); i++) {
        auto& ev = events[i];

        auto* user = static_cast<EpollData*>(ev.data.ptr);
        user->handler->onEvent(user->fd, ev.events);
    }
}

Epoll::~Epoll() {
    for (auto& [fd, user] : this->fds)
        delete user;
    close(this->epfd);
}

epoll_error::~epoll_error() = default;
