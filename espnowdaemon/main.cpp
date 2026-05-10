#include <asio.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <utility>

using namespace asio;
using asio::ip::tcp;

constexpr auto CONTROL_PORT = 19997;

class control_session
    : public std::enable_shared_from_this<control_session>
{
public:
    using pointer = std::shared_ptr<control_session>;

    static pointer create(asio::io_context &io_context) {
        return std::make_shared<control_session>(io_context);
    }

    tcp::socket &get_socket() { return socket_; }

    void start() {
        std::cout << "New control session started" << std::endl;

        asio::async_write(socket_, asio::buffer("Hello\n"),
            std::bind(&control_session::handle_write, shared_from_this(),
                asio::placeholders::error, asio::placeholders::bytes_transferred));
    }

    control_session(asio::io_context &io_context)
    : socket_(io_context) {}

private:
    void handle_write(const std::error_code& ec, size_t bytes_transferred)
    {

    }

    asio::ip::tcp::socket socket_;
};

class control_server
{
public:
    control_server(asio::io_context &io_context)
    : io_context_(io_context)
    , acceptor_(io_context, tcp::endpoint(tcp::v4(), CONTROL_PORT)) {
        start();
    }

    void start() {
        auto session = control_session::create(io_context_);

        acceptor_.async_accept(
            session->get_socket(),
            std::bind(&control_server::handle_accept, this, session, asio::placeholders::error));
    }

    void handle_accept(control_session::pointer session, std::error_code ec)
    {
        if (!ec) {
            session->start();
        }

        start();
    }

private:
    asio::io_context &io_context_;
    tcp::acceptor acceptor_;
};

int main(int argc, char *argv[])
{
    try {
        asio::io_context io_context(1);
        control_server server(io_context);

        io_context.run();
    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}