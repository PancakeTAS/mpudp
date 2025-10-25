#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <netinet/in.h>

namespace config {
    /// client connection element [[client.connection]]
    struct ClientConnectionConfig {
        bool active{}; //!< PLACEHOLDER, DO NOT USE
    };

    /// client configuration element [client]
    struct ClientConfig {
        in_addr_t peer{}; //!< address of the tunnel endpoint
        uint16_t baseport{}; //!< lowest port of the tunnel
        uint16_t listenport{}; //!< port to listen for incoming data on
        std::vector<ClientConnectionConfig> connections; //!< all configured connections
    };

    /// server connection element [[server.connection]]
    struct ServerConnectionConfig {
        in_addr_t peer{}; //!< client address expected on this connection
    };

    /// server configuration element [server]
    struct ServerConfig {
        uint16_t baseport{}; //!< lowest port of the tunnel
        uint16_t sendport{}; //!< port to send outgoing data to
        std::vector<ServerConnectionConfig> connections; //!< all configured connections
    };

    /// parse configuration file
    /// @param filename path to configuration file
    /// @return parsed configuration
    /// @throws std::exception on failure
    std::variant<ClientConfig, ServerConfig> parse(const std::string& filename);
}
