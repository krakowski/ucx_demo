#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "messaging.h"

int main(int argc, char** argv) {

    // Require at least one parameter
    if (argc == 1) {
        return 1;
    }

    // Default listen address
    const char *listen_address = "127.0.0.1";
    uint16_t default_port = 2998;
    if (argc > 2) {
        default_port = atoi(argv[2]);
    }

    // Create listen address
    sockaddr_in socket_address{
            .sin_family = AF_INET,
            .sin_port   = htons(default_port)
    };

    // Set listen address
    inet_pton(AF_INET, listen_address, &socket_address.sin_addr);

    // Start client or server
    auto demo = new Messaging();
    if (strcmp(argv[1], "server") == 0) {
        // Server goes through run-cleanup cycle until error
        while (true) {
            if (auto status = demo->run(Base::Mode::Server, socket_address); status != UCS_OK) {
                break;
            }

            demo->cleanup();
        }
    } else if (strcmp(argv[1], "client") == 0) {
        // Client connects with server once and exits program
        demo->run(Base::Mode::Client, socket_address);
    }

    // Cleanup resources
    demo->cleanup();
    delete demo;

    return 0;
}