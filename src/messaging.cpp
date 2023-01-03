#include "messaging.h"

ucp_request_param_t request_parameters {};

ucs_status_t Messaging::on_client_ready() {
    std::string message = "Hello UCX";
    auto request = ucp_tag_send_nbx(
            endpoint,
            message.c_str(),
            message.size() + 1,
            0,
            &request_parameters
    );

    if (auto status = wait_on_request(request); status != UCS_OK) {
        ucs_error("Sending message failed");
        return status;
    }

    ucs_info("Message sent");
    return UCS_OK;
}

ucs_status_t Messaging::on_server_ready() {
    std::vector<char> message(32);
    auto request = ucp_tag_recv_nbx(
            worker,
            message.data(),
            message.size(),
            0,
            0,
            &request_parameters
    );

    if (auto status = wait_on_request(request); status != UCS_OK) {
        ucs_error("Receiving message failed");
        return status;
    }

    ucs_info("Received %s", message.data());
    return UCS_OK;
}
