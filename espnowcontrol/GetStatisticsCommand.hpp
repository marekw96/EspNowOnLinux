#pragma once

#include "ICmdCommand.hpp"
#include "ConnectionSocket.hpp"
#include <string_view>
#include <vector>

class GetStatisticsCommand : public ICmdCommand {
public:
    std::string_view get_name() const override;
    std::string_view get_description() const override;
    bool handle(ConnectionSocket& socket, args_t args) override;
};