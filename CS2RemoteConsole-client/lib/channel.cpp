#include "channel.h"
#include <sstream>
#include <iomanip>

#pragma once

std::vector<Channel> channels;

std::string getChannelName(uint32_t channelID)
{
    for (const auto& channel : channels)
    {
        if (channel.id == channelID)
        {
            return channel.name;
        }
    }
    return "Unknown-" + std::to_string(channelID);
}

std::string getColorCode(uint32_t channelID)
{
    for (const auto& channel : channels)
    {
        if (channel.id == channelID)
        {
            uint8_t r = (channel.RGBA_Override >> 16) & 0xFF;
            uint8_t g = (channel.RGBA_Override >> 8) & 0xFF;
            uint8_t b = channel.RGBA_Override & 0xFF;
            std::stringstream ss;
            ss << "\033[38;2;" << static_cast<int>(r) << ";" << static_cast<int>(g) << ";" << static_cast<int>(b) << "m";
            return ss.str();
        }
    }
    return "\033[0m"; // Default color
}
