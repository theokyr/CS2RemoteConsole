#include "chan.h"

extern std::vector<Channel> channels;

void processCHANMessage(const unsigned char* data, int size)
{
    int offset = 0;
    CHANMessage chanMsg;

    chanMsg.length = byteSwap16(*reinterpret_cast<const uint16_t*>(data + offset));
    offset += 2;

    std::cout << "Length: " << chanMsg.length << std::endl;

    for (int i = 0; i < chanMsg.length; ++i)
    {
        Channel channel;

        channel.id = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset));
        offset += 4;
        std::cout << "Channel ID: " << channel.id << std::endl;

        channel.unknown1 = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset));
        offset += 4;
        std::cout << "Unknown1: " << channel.unknown1 << std::endl;

        channel.unknown2 = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset));
        offset += 4;
        std::cout << "Unknown2: " << channel.unknown2 << std::endl;

        channel.verbosity_default = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset));
        offset += 4;
        std::cout << "Verbosity Default: " << channel.verbosity_default << std::endl;

        channel.verbosity_current = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset));
        offset += 4;
        std::cout << "Verbosity Current: " << channel.verbosity_current << std::endl;

        uint8_t rgba[4];
        std::memcpy(rgba, data + offset, 4);
        channel.RGBA_Override = *reinterpret_cast<uint32_t*>(rgba);
        offset += 4;
        std::cout << "RGBA Override: " << std::hex << channel.RGBA_Override << std::dec << std::endl;

        char nameBuf[35] = {0};
        int nameLen = 0;
        while (nameLen < 34 && data[offset + nameLen] != 0)
        {
            nameBuf[nameLen] = data[offset + nameLen];
            nameLen++;
        }
        channel.name = std::string(nameBuf);
        offset += 34;
        std::cout << "Channel Name: " << channel.name << std::endl;

        chanMsg.channels.push_back(channel);

        auto it = std::find_if(channels.begin(), channels.end(),
                               [&channel](const Channel& c) { return c.id == channel.id; });
        if (it != channels.end())
        {
            *it = channel;
        }
        else
        {
            channels.push_back(channel);
        }
    }
}
