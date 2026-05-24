#pragma once
#include "ICmdCommand.hpp"

class AddUartDeviceCommand : public ICmdCommand {
    public:
        std::string_view get_name() const override;
        std::string_view get_description() const override;
        bool handle(args_t args, asio::ip::tcp::socket& socket) override;
};
