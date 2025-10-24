#include "client.hpp"
#include "server.hpp"
#include "sock/sock.hpp"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include <bits/getopt_core.h>
#include <netinet/in.h>

namespace {
    enum class Mode {
        CLIENT, SERVER
    };

    struct Args {
        Mode mode{Mode::CLIENT};
        uint16_t baseport{5000};
        uint16_t count{1}; // client only
        in_addr_t peer{}; // client only
        std::vector<in_addr_t> peers; // server only
    };

    void print_usage(const std::string& prog) {
        std::cerr << "Usage: " << prog << " -c -b <baseport> -l <count> <peer>\n"
            << "or: " << prog << " -s -b <baseport> -p <peer1> -p <peer2> ...\n"
            << "Open a multiport UDP tunnel to a peer, or listen for incoming connections from a list of peers.\n\n"
            << "Each peer will be opened on a separate port incrementally.\n\n"
            << "Options:\n"
            << "  -c                Client mode: Open a tunnel to a peer\n"
            << "  -s                Server mode: Listen for incoming connections from peers\n"
            << "  -b <baseport>     Base port number (default: 5000)\n"
            << "  -l <count>        Client only: Number of ports to use when connecting (default: 1)\n"
            << "  -p <peer>         Server only: Peer address to listen for (can be specified multiple times)\n\n"
            << "Examples:\n"
            << "    " << prog << " -c -b 6000 -l 1 127.0.0.1\n"
            << "    " << prog << " -s -b 6000 -p 127.0.0.1\n\n"
            << "Report bugs to: /dev/null\n"
            << "Source code: https://github.com/PancakeTAS/mpudp\n";
    }

    [[noreturn]] void client_main(const Args& args) {
        try {
            client::main(args.baseport, args.count, args.peer);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
            exit(EXIT_FAILURE);
        }
    }

    [[noreturn]] void server_main(const Args& args) {
        try {
            server::main(args.baseport, args.peers);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char* argv[]) {
    Args args{};

    // parse arguments
    int opt{};
    while ((opt = getopt(argc, argv, "csb:l:p:")) != -1) {
        switch (opt) {
            case 'c':
                args.mode = Mode::CLIENT;
                break;
            case 's':
                args.mode = Mode::SERVER;
                break;
            case 'b':
                if (std::stoi(optarg) < 1) {
                    print_usage(*argv);
                    return EXIT_FAILURE;
                }

                args.baseport = static_cast<uint16_t>(std::stoi(optarg));
                break;
            case 'l':
                if (std::stoi(optarg) < 1) {
                    print_usage(*argv);
                    return EXIT_FAILURE;
                }

                args.count = static_cast<uint16_t>(std::stoi(optarg));
                break;
            case 'p': {
                args.peers.push_back(sock::stoia(optarg));
                break;
            }
            default:
                print_usage(*argv);
                return EXIT_FAILURE;
        }
    }

    const std::span<char*> arguments(argv, static_cast<size_t>(argc));
    if (args.mode == Mode::CLIENT) {
        if (optind >= argc) {
            print_usage(*argv);
            return EXIT_FAILURE;
        }

        args.peer = sock::stoia(arguments[static_cast<size_t>(optind)]);
    }

    if (args.mode == Mode::SERVER && args.peers.empty()) {
        print_usage(*argv);
        return EXIT_FAILURE;
    }

    // start tunnels
    if (args.mode == Mode::CLIENT) {
        std::cerr << "Opening tunnel to "
            << arguments[static_cast<size_t>(optind)] << " on ports "
            << args.baseport << " to "
            << (args.baseport + args.count - 1) << '\n';

        client_main(args);
    } else {
        std::cerr << "Listening for incoming tunnels on ports "
            << args.baseport << " to "
            << (args.baseport + args.peers.size() - 1) << '\n';

        server_main(args);
    }
}
