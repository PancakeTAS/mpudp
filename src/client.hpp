#pragma once

#include <cstdint>

#include <netinet/in.h>

namespace client {

    /// client-side main entry point
    /// @param bport base port number
    /// @param bport_len number of ports to use
    /// @param peer peer address
    /// @throws std::exception on error
    [[noreturn]] void main(uint16_t bport, uint16_t bport_len, in_addr_t peer);

}
