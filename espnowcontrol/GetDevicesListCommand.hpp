#pragma once
#include "ICmdCommand.hpp"

class GetDevicesListCommand : public ICmdCommand {
    public:
        std::string_view get_name() const override;
        std::string_view get_description() const override;
        bool handle(ConnectionSocket& socket, args_t args) override;
};
