#include "config.hpp"
#include "side/client.hpp"
#include "side/server.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <variant>

int main() {
    // parse the configuration file
    std::variant<config::ClientConfig, config::ServerConfig> config;
    try {
        config = config::parse("mpudp.toml");
    } catch (const std::exception& e) {
        std::cerr << "An error occured while trying to parse the configuration file:\n> "
            << e.what() << '\n';
        return EXIT_FAILURE;
    }

    // start the appropriate mode
    try {
        if (std::holds_alternative<config::ClientConfig>(config)) {
            auto cconfig = std::get<config::ClientConfig>(config);

            client::main(cconfig);
        } else {
            auto sconfig = std::get<config::ServerConfig>(config);

            server::main(sconfig);
        }
    } catch (const std::exception& e) {
        std::cerr << "An error occured during runtime:\n> "
            << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
