#pragma once

#include <array>
#include <cstddef>
#include <cstring>
#include <stdexcept>

#include <netinet/in.h>
#include <unistd.h>

namespace sock {

    template <size_t N>
    using buf = std::array<char, N>; //!< char buffer

    /// fd wrapper
    class Fd {
    public:
        /// default constructor
        Fd() = default;

        /// update the fd
        void set(int nfd) {
            if (this->fd != -1)
                close(fd);
            this->fd = nfd;
        }

        /// get the underlying fd
        operator int() const { return this->fd; }

        /// non-copyable, non-movable
        Fd(const Fd&) = delete;
        Fd& operator=(const Fd&) = delete;
        Fd(Fd&&) = delete;
        Fd& operator=(Fd&&) = delete;
        virtual ~Fd();
    private:
        int fd{-1};
    };

    /// convert a string ipv4 address to a in_addr_t
    /// @param straddr the string ipv4 address
    /// @return the in_addr_t representation of the address
    /// @throws sock_error on failure
    in_addr_t stoia(const std::string& straddr);

    /// exception class for socket errors
    class sock_error : public std::runtime_error {
    public:
        /// create a new sock_error
        /// @param what the error message
        sock_error(const std::string& what)
            : std::runtime_error(what + ": " + std::strerror(errno)), code(errno) {}

        /// get the errno associated with the error
        [[nodiscard]] int get_errno() const { return code; }

        sock_error(const sock_error&) = default;
        sock_error& operator=(const sock_error&) = default;
        sock_error(sock_error&&) = default;
        sock_error& operator=(sock_error&&) = default;
        ~sock_error() override;
    private:
        int code;
    };

}
