#pragma once
#include <string_view>
#include <span>
#include "ConnectionSocket.hpp"

using args_t = std::span<std::string_view>;

class ICmdCommand {
    public:
        virtual ~ICmdCommand() = default;
        virtual std::string_view get_name() const = 0;
        virtual std::string_view get_description() const = 0;
        virtual bool handle(ConnectionSocket& socket, args_t args) = 0;
};