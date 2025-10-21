#pragma once

#include <cstddef>

namespace tun {
    const size_t RECV_BUF = 65535; //!< receive buffer size
    const size_t SEND_BUF = 65535; //!< send buffer size
    const size_t HSLEN = 5; //!< handshake packet length
}
