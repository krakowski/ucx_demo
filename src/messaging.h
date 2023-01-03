#ifndef OPENUCX_EXAMPLES_MESSAGING_H
#define OPENUCX_EXAMPLES_MESSAGING_H

#include "base.h"
#include <vector>

class Messaging: public Base {


protected:

    ucs_status_t on_client_ready() override;

    ucs_status_t on_server_ready() override;
};


#endif //OPENUCX_EXAMPLES_MESSAGING_H
