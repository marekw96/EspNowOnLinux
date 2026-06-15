#include <asio.hpp>
#include <iostream>

#include "device_manager.hpp"
#include "control_server.hpp"
#include "utility/event_dispatcher.hpp"

int main(int argc, char *argv[])
{
    // try {
        event_dispatcher event_dispatcher;
        asio::io_context io_context(1);

        device_manager device_manager(io_context, event_dispatcher);
        control_server server(io_context, device_manager, event_dispatcher);

        io_context.run();
    // }
    // catch (std::exception &e) {
    //     std::cerr << "Exception: " << e.what() << std::endl;
    //     return 1;
    // }

    return 0;
}