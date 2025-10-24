#pragma once

#include "../sock/sock.hpp"

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <unordered_map>

#include <sys/epoll.h>

namespace epoll {

    /// abstract base class for handling events
    class EventHandler {
    public:
        /// default constructor
        EventHandler() = default;

        /// called when an event occurs on the file descriptor
        /// @param fd the file descriptor on which the event occurred
        /// @param events the events that occurred (bitmask of EPOLLIN, EPOLLOUT, etc.)
        virtual void onEvent(std::shared_ptr<sock::Fd>& fd, uint32_t events) = 0;

        // non-copyable and non-movable
        EventHandler(const EventHandler&) = delete;
        EventHandler& operator=(const EventHandler&) = delete;
        EventHandler(EventHandler&&) = delete;
        EventHandler& operator=(EventHandler&&) = delete;
        virtual ~EventHandler();
    };

    /// zero-overhead epoll wrapper (not thread-safe)
    class Epoll {
    public:
        /// create a new epoll instance
        /// @throws epoll_error on failure
        Epoll();

        /// add a file descriptor
        /// @param fd the file descriptor to add
        /// @param handler the event handler to associate
        /// @param events the events to monitor (default: EPOLLIN)
        /// @throws epoll_error on failure
        void add(std::shared_ptr<sock::Fd> fd, std::shared_ptr<EventHandler> handler, uint32_t events);

        /// modify a file descriptor
        /// @param fd the file descriptor to modify
        /// @param handler the new event handler to associate
        /// @param events the new events to monitor
        /// @throws epoll_error on failure
        void modify(const sock::Fd& fd, std::shared_ptr<EventHandler> handler, uint32_t events);

        /// remove a file descriptor
        /// @param fd the file descriptor to remove
        /// @throws epoll_error on failure
        void remove(const sock::Fd& fd);

        /// poll for events
        /// @param timeout the timeout in milliseconds (-1 for infinite)
        /// @throws epoll_error on failure
        template<size_t N = 64>
        void poll(int timeout = -1) const {
            std::array<epoll_event, N> events;
            this->poll(timeout, std::span(events.data(), N));
        }

        // non-copyable and non-movable
        Epoll(const Epoll&) = delete;
        Epoll& operator=(const Epoll&) = delete;
        Epoll(Epoll&&) = delete;
        Epoll& operator=(Epoll&&) = delete;
        ~Epoll();
    private:
        int epfd;

        struct EpollData {
            std::shared_ptr<sock::Fd> fd;
            std::shared_ptr<EventHandler> handler;
        };
        std::unordered_map<int, EpollData*> fds;

        void poll(int timeout, std::span<epoll_event> events) const;
    };

    /// exception class for epoll errors
    class epoll_error : public std::runtime_error {
    public:
        /// create a new epoll_error
        /// @param what the error message
        /// @param err the optional errno value
        epoll_error(const std::string& what, int err = errno)
            : std::runtime_error(what + ": " + std::strerror(err)), code(err) {}

        /// get the optional errno associated with the error
        /// @return the errno value
        [[nodiscard]] int get_errno() const { return code; }

        epoll_error(const epoll_error&) = default;
        epoll_error& operator=(const epoll_error&) = default;
        epoll_error(epoll_error&&) = default;
        epoll_error& operator=(epoll_error&&) = default;
        ~epoll_error() override;
    private:
        int code;
    };

}
