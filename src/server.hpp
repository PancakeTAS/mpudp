#pragma once

#include <cstdint>
#include <vector>

#include <netinet/in.h>

namespace server {

    /// server-side main entry point
    /// @param bport base port number
    /// @param peers list of peer addresses
    /// @throws std::exception on error
    [[noreturn]] void main(uint16_t bport, const std::vector<in_addr_t>& peers);

}
