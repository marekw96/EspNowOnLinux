#include "control_server.hpp"
#include <functional>

using asio::ip::tcp;

constexpr auto CONTROL_PORT = 19997;

control_server::control_server(asio::io_context &io_context, device_manager& device_manager, event_dispatcher& dispatcher)
: io_context_(io_context)
, acceptor_(io_context, tcp::endpoint(tcp::v4(), CONTROL_PORT))
, device_manager_(device_manager)
, dispatcher_(dispatcher) {
    start();
}

void control_server::start() {
    auto session = control_session::create(io_context_, device_manager_, dispatcher_);

    acceptor_.async_accept(
        session->get_socket(),
        std::bind(&control_server::handle_accept, this, session, asio::placeholders::error));
}

void control_server::handle_accept(control_session::pointer session, std::error_code ec)
{
    if (!ec) {
        session->start();
    }

    start();
}
