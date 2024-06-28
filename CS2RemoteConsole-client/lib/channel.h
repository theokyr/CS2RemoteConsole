#ifndef CHANNEL_H
#define CHANNEL_H

#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Channel {
    uint32_t id;
    uint32_t unknown1;
    uint32_t unknown2;
    uint32_t verbosity_default;
    uint32_t verbosity_current;
    uint32_t RGBA_Override;
    std::string name;
};

extern std::vector<Channel> channels;

std::string getChannelName(uint32_t channelID);
std::string getColorCode(uint32_t channelID);

#endif // CHANNEL_H
