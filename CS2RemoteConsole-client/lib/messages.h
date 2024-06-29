#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>
#include <vector>
#include <cstdint>

struct Channel {
    int32_t id;
    int32_t unknown1;
    int32_t unknown2;
    int32_t verbosity_default;
    int32_t verbosity_current;
    uint32_t text_RGBA_override;
    char name[34];
};

struct Cvar {
    char name[64];
    uint32_t unknown;
    uint32_t flags;
    float rangemin;
    float rangemax;
    uint8_t padding;
};

struct AINF {
    std::vector<uint32_t> unknowns;
    uint8_t padding;
};

struct ADON {
    uint16_t unknown;
    uint16_t nameLength;
    std::string name;
};

struct CHAN {
    uint16_t numChannels;
    std::vector<Channel> channels;
};

struct PRNT {
    int32_t channelID;
    uint8_t unknown[24];
    std::string message;
};

struct CVAR {
    Cvar cvar;
};

struct CFGV {
    char variable[64];
    char value[65];
};

#endif // MESSAGES_H
