#include "chan.h"

#include <spdlog/spdlog.h>

void processCHANMessage(const unsigned char* data, int size)
{
    // Ensure we have enough data for the message type, packet size, and length (4 + 2 + 2 = 8 bytes)
    if (size < 8)
    {
        spdlog::error("Insufficient data size for CHAN message");
        return;
    }

    // Skip the message type (4 bytes) and packet size (2 bytes)
    int offset = 6;

    // Read the number of channels
    uint16_t channelCount;
    std::memcpy(&channelCount, data + offset, sizeof(uint16_t));
    channelCount = byteSwap16(channelCount);
    offset += 2;

    spdlog::debug("CHAN message: {} channels", channelCount);

    for (int i = 0; i < channelCount; ++i)
    {
        if (size < offset + 58) // 4 + 4 + 4 + 4 + 4 + 4 + 34 = 58 bytes per channel
        {
            spdlog::error("Insufficient data size for channel {}", i);
            return;
        }

        Channel channel;

        channel.id = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset));
        offset += 4;

        channel.unknown1 = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset));
        offset += 4;

        channel.unknown2 = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset));
        offset += 4;

        channel.verbosity_default = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset));
        offset += 4;

        channel.verbosity_current = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset));
        offset += 4;

        channel.RGBA_Override = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset));
        offset += 4;

        channel.name = std::string(reinterpret_cast<const char*>(data + offset), 34);
        channel.name = channel.name.c_str(); // Trim null characters
        offset += 34;

        spdlog::debug("Channel: ID={}, Unknown1={}, Unknown2={}, Default Verbosity={}, Current Verbosity={}, RGBA={:08X}, Name='{}'",
                      channel.id, channel.unknown1, channel.unknown2, channel.verbosity_default,
                      channel.verbosity_current, channel.RGBA_Override, channel.name);

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
