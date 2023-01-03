#include "base.h"

ucs_status_t Base::run(Base::Mode mode, sockaddr_in address) {

    // Initialize context and worker
    if (auto status = initialize(); status != UCS_OK) {
        return status;
    }

    // Start client or server depending on specified mode
    switch (mode) {
        case Mode::Client:
            return run_client(address);
        case Mode::Server:
            return run_server(address);
    }

    return UCS_ERR_INVALID_PARAM;
}

ucs_status_t Base::initialize() {

    ucs_info("Initializing resources");

    // Activate Tag Matching Feature
    ucp_params_t context_params {
            .field_mask        = UCP_PARAM_FIELD_FEATURES,
            .features          = UCP_FEATURE_TAG
    };

    // Read configuration
    ucp_config_t *configuration = nullptr;
    if (auto status = ucp_config_read(nullptr, nullptr, &configuration); status != UCS_OK) {
        ucs_error("Reading configuration failed");
        return status;
    }

    // Initialize context and release configuration
    ucs_info("Initializing context...");
    if (auto status = ucp_init(&context_params, configuration, &context); status != UCS_OK) {
        ucp_config_release(configuration);
        ucs_error("Initializing context failed");
        return status;
    }

    ucp_config_release(configuration);

    // Use worker in single-threaded mode
    ucp_worker_params_t worker_params{
            .field_mask  = UCP_WORKER_PARAM_FIELD_THREAD_MODE,
            .thread_mode = UCS_THREAD_MODE_SINGLE
    };

    // Create worker
    ucs_info("Initializing worker...");
    if (auto status = ucp_worker_create(context, &worker_params, &worker); status != UCS_OK) {
        ucs_error("Initializing worker failed");
        return status;
    }

    ucs_info("Initialization complete");
    return UCS_OK;
}

ucs_status_t Base::run_client(sockaddr_in address) {

    ucs_info("Initializing endpoint...");

    // Prepare endpoint parameters
    ucp_ep_params_t endpoint_parameters {
            .field_mask  = UCP_EP_PARAM_FIELD_FLAGS |
                           UCP_EP_PARAM_FIELD_SOCK_ADDR|
                           UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE,
            .err_mode    = UCP_ERR_HANDLING_MODE_PEER,
            .flags       = UCP_EP_PARAMS_FLAGS_CLIENT_SERVER |
                           UCP_EP_PARAMS_FLAGS_SEND_CLIENT_ID,
            .sockaddr {
                    .addr    = (const struct sockaddr*) &address,
                    .addrlen = sizeof(address)
            }
    };

    if (auto status = ucp_ep_create(worker, &endpoint_parameters, &endpoint); status != UCS_OK) {
        ucs_error("Creating endpoint failed");
        return status;
    }

    ucs_info("Endpoint initialized");
    return on_client_ready();
}

ucs_status_t Base::run_server(sockaddr_in address) {

    ucs_info("Initializing listener...");

    // Set listen address
    std::atomic<ucp_conn_request_h> connection_request(nullptr);
    ucp_listener_params_t listener_parameters {
            .field_mask  = UCP_LISTENER_PARAM_FIELD_SOCK_ADDR |
                           UCP_LISTENER_PARAM_FIELD_CONN_HANDLER,
            .sockaddr {
                    .addr    = (const struct sockaddr*) &address,
                    .addrlen = sizeof(address)
            },
            .conn_handler {
                    .cb      = [](ucp_conn_request_h conn_request, void *arg) -> void {
                        auto *connection_request = (std::atomic<ucp_conn_request*>*) arg;
                        connection_request->store(conn_request);
                    },
                    .arg     = &connection_request
            }
    };

    // Create listener
    if (auto status = ucp_listener_create(worker, &listener_parameters, &listener); status != UCS_OK) {
        ucs_error("Creating listener failed");
        return status;
    }

    // Progress until connection attempt is made
    ucs_info("Waiting on client connection...");
    while (connection_request.load() == nullptr) {
        ucp_worker_progress(worker);
    }

    ucs_info("Accepting client connection...");

    // Prepare endpoint parameters
    ucp_ep_params_t endpoint_parameters {
            .field_mask   = UCP_EP_PARAM_FIELD_CONN_REQUEST |
                            UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE,
            .err_mode     = UCP_ERR_HANDLING_MODE_PEER,
            .conn_request = connection_request.load()
    };

    // Accept connection
    if (auto status = ucp_ep_create(worker, &endpoint_parameters, &endpoint); status != UCS_OK) {
        ucs_error("Creating endpoint failed");
        return status;
    }

    return on_server_ready();
}

void Base::cleanup() {

// Close endpoint
if (endpoint != nullptr) {
    ucp_request_param_t request_params {
            .op_attr_mask   = UCP_OP_ATTR_FIELD_FLAGS,
            .flags          = UCP_EP_CLOSE_FLAG_FORCE
    };

    if (auto status = wait_on_request(ucp_ep_close_nbx(endpoint, &request_params)); status != UCS_OK) {
        ucs_error("Force-closing endpoint failed with error %s", ucs_status_string(status));
    }

    endpoint = nullptr;
    ucs_info("Endpoint closed");
}

// Close listener
if (listener != nullptr) {
    ucp_listener_destroy(listener);
    listener = nullptr;
    ucs_info("Listener closed");
}

// Close worker
if (worker != nullptr) {
    ucp_worker_destroy(worker);
    worker = nullptr;
    ucs_info("Worker closed");
}

// Close context
if (context != nullptr) {
    ucp_cleanup(context);
    context = nullptr;
    ucs_info("Context closed");
}
}

ucs_status_t Base::wait_on_request(ucs_status_ptr_t request) {

    // Operation was completed immediately
    if (request == nullptr) {
        return UCS_OK;
    }

    // Check if we got an error
    if UCS_PTR_IS_ERR(request) {
        return UCS_PTR_STATUS(request);
    }

    // Wait on completion
    while (ucp_request_check_status(request) != UCS_OK) {
        ucp_worker_progress(worker);
    }

    // Release request and return OK
    ucp_request_release(request);
    return UCS_OK;
}
