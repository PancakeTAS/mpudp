#include "config.hpp"
#include "sock/sock.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <toml++.hpp>

using namespace config;

namespace {
    template<typename T> // helper for getting keys or throwing
    T get(const toml::table& toml, const std::string& key) {
        const auto value = toml[key];
        if (!value.is<T>())
            throw std::runtime_error("Missing or invalid key: " + key);
        return value.ref<T>();
    }

    uint32_t gcd(uint32_t a, uint32_t b) { // helper for gcd
        while (b != 0) {
            const uint32_t temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }
}

std::variant<ClientConfig, ServerConfig> config::parse(const std::string& filename) {
    auto toml = toml::parse_file(filename);

    const bool has_client = toml["client"].is_table();
    const bool has_server = toml["server"].is_table();

    if (has_client == has_server || (!has_client && !has_server))
        throw std::runtime_error("Must contain exactly one [client] or [server] section");

    if (has_client) { // parse client config
        const auto tclient = *toml["client"].as_table();

        // ensure configuration contains a "connection" array
        const auto tclient_connection = tclient["connection"];
        if (!tclient_connection.is_array())
            throw std::runtime_error("Client configuration must contain a connection array");

        // then parse all entries
        std::vector<ClientConnectionConfig> conns;
        for (const auto& tconn : *tclient_connection.as_array()) {
            if (!tconn.is_table())
                throw std::runtime_error("Invalid connection entry in client configuration");
            const auto tconnt = *tconn.as_table();

            const ClientConnectionConfig cconn{
                .weight = static_cast<uint32_t>(get<int64_t>(tconnt, "weight")),
            };
            conns.push_back(cconn);
        }

        // normalize weights by GCD
        uint32_t div{};
        for (const auto& cconn : conns)
            div = gcd(div, cconn.weight);
        for (auto& cconn : conns)
            cconn.weight /= div;

        // finally, construct the client config
        return ClientConfig {
            .peer = sock::stoia(get<std::string>(tclient, "peer")),
            .baseport = static_cast<uint16_t>(get<int64_t>(tclient, "baseport")),
            .listenport = static_cast<uint16_t>(get<int64_t>(tclient, "listenport")),
            .connections = std::move(conns)
        };
    } // else parse server config

    const auto tserver = *toml["server"].as_table();

    // ensure configuration contains a "connection" array
    const auto tserver_connection = tserver["connection"];
    if (!tserver_connection.is_array())
        throw std::runtime_error("Server configuration must contain a connection array");

    // then parse all entries
    std::vector<ServerConnectionConfig> conns;
    for (const auto& tconn : *tserver_connection.as_array()) {
        if (!tconn.is_table())
            throw std::runtime_error("Invalid connection entry in server configuration");
        const auto tconnt = *tconn.as_table();

        const ServerConnectionConfig sconn{
            .peer = sock::stoia(get<std::string>(tconnt, "peer")),
            .weight = static_cast<uint32_t>(get<int64_t>(tconnt, "weight")),
        };
        conns.push_back(sconn);
    }

    // normalize weights by GCD
    uint32_t div{};
    for (const auto& sconn : conns)
        div = gcd(div, sconn.weight);
    for (auto& sconn : conns)
        sconn.weight /= div;

    // finally, construct the server config
    return ServerConfig {
        .baseport = static_cast<uint16_t>(get<int64_t>(tserver, "baseport")),
        .sendport = static_cast<uint16_t>(get<int64_t>(tserver, "sendport")),
        .connections = std::move(conns)
    };
}
