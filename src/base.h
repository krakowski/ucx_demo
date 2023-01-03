#ifndef OPENUCX_EXAMPLES_BASE_H
#define OPENUCX_EXAMPLES_BASE_H

#include <atomic>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ucp/api/ucp.h>
#include <ucs/type/status.h>
#include <ucs/debug/log_def.h>


class Base {

private:

    ucp_listener_h listener = nullptr;

    ucs_status_t initialize();

    ucs_status_t run_client(sockaddr_in address);

    ucs_status_t run_server(sockaddr_in address);

protected:

    ucp_context_h context = nullptr;

    ucp_worker_h worker = nullptr;

    ucp_ep_h endpoint = nullptr;

    ucs_status_t wait_on_request(ucs_status_ptr_t request);

    virtual ucs_status_t on_client_ready() = 0;

    virtual ucs_status_t on_server_ready() = 0;

public:

    enum class Mode {
        Client,
        Server
    };

    ucs_status_t run(Mode mode, sockaddr_in address);

    void cleanup();
};


#endif //OPENUCX_EXAMPLES_BASE_H
