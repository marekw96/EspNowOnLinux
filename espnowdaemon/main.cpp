#include <asio.hpp>
#include <iostream>

#include "device_manager.hpp"
#include "control_server.hpp"

int main(int argc, char *argv[])
{
    // try {
        asio::io_context io_context(1);

        device_manager device_manager(io_context);
        control_server server(io_context, device_manager);

        io_context.run();
    // }
    // catch (std::exception &e) {
    //     std::cerr << "Exception: " << e.what() << std::endl;
    //     return 1;
    // }

    return 0;
}