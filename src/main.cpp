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
        // Initialize all UCX resources (context, worker, listener), wait for incoming
        // connection requests, create an endpoint once a request arrives and receive a single
        // message using ucp_tag_recv_nbx.
        if (auto status = demo->run(Base::Mode::Server, socket_address); status == UCS_OK) {
            ucs_info("First run successful");
        }

        // Close connection and cleanup all resources (endpoint, listener, worker, context).
        demo->cleanup();

        // Try to initialize all UCX resources again after they've been cleaned up.
        if (auto status = demo->run(Base::Mode::Server, socket_address); status != UCS_OK) {
            ucs_error("Second run failed with error %s", ucs_status_string(status));
        }

        // Close connection and cleanup all resources (endpoint, listener, worker, context).
        demo->cleanup();
    } else if (strcmp(argv[1], "client") == 0) {
        // Client connects with server, then sleeps 1 second. During this time
        // (specifically before the client closes its connection) the server closes
        // its listener and tries to reopen it, which fails.
        demo->run(Base::Mode::Client, socket_address);

        // Sleep for one second, during which the server closes its connection.
        sleep(1);

        // Close connection and cleanup all resources (endpoint, listener, worker, context).
        demo->cleanup();
    }

    delete demo;
    return 0;
}