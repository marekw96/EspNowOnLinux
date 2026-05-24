#include <iostream>
#include <string>
#include <string_view>
#include <asio.hpp>
#include "ICmdCommand.hpp"
#include "AddUartDeviceCommand.hpp"

constexpr auto CONTROL_PORT = 19997;
constexpr auto CONTROL_IP = "localhost";

auto to_vector(int argc, char* argv[]){
    return std::vector<std::string_view>(argv, argv + argc);
}

auto find_command(std::string_view arg, const std::vector<std::unique_ptr<ICmdCommand>> &commands){
    return std::find_if(commands.begin(), commands.end(), [&](const std::unique_ptr<ICmdCommand>& cmd){
        return cmd->get_name() == arg;
    });
}

int main(int argc, char *argv[]) {
    auto args = to_vector(argc, argv);
    args_t args_view = std::span(args.begin(), args.end()).subspan(1);
    std::vector<std::unique_ptr<ICmdCommand>> commands;
    commands.push_back(std::make_unique<AddUartDeviceCommand>());

    auto command = find_command(args_view[0], commands);
    if(command == commands.end()){
        std::cerr << "Command not found" << std::endl;
        std::cerr << "Available commands:" << std::endl;
        for(const auto& command : commands){
            std::cerr << '\t' << command->get_description() << std::endl;
        }
        return -1;
    }

    try {
        asio::io_context io_context;
        asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(CONTROL_IP, std::to_string(CONTROL_PORT));

        asio::ip::tcp::socket socket(io_context);
        asio::connect(socket, endpoints);

        std::cout << "Connected to " << CONTROL_IP << ":" << CONTROL_PORT << std::endl;

        auto result = command->get()->handle(args_view.subspan(1), socket);
        if(!result) {
            std::cerr << "Failed to handle command" << std::endl;
            return -1;
        }
    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
