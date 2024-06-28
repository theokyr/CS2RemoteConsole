#include "prnt.h"
#include "../channel.h"
#include "../../utils.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <spdlog/spdlog.h>

#pragma once

void processPRNTMessage(const unsigned char* data, int size)
{
    const int headerSize = 38; // 4 + 2 + 4 + 4 + 24 = 38 bytes
    int offset = 4; // Start after the message type

    while (offset + headerSize <= size)
    {
        PRNTMessage msg;

        msg.messageType = byteSwap32(*reinterpret_cast<const uint32_t*>(data));
        msg.commandType = byteSwap16(*reinterpret_cast<const uint16_t*>(data + offset));
        msg.messageSize = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset + 2));
        msg.channelID = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset + 6));

        std::memcpy(msg.unknown, data + offset + 10, 24);

        int messageContentLength = msg.messageSize - headerSize;
        if (messageContentLength <= 0 || msg.messageSize > static_cast<uint32_t>(size - offset))
        {
            spdlog::warn("Incomplete PRNT message received");
            return;
        }

        msg.message = std::string(reinterpret_cast<const char*>(data + offset + headerSize), messageContentLength);

        // Process the message here (e.g., log it, display it, etc.)
        spdlog::info("[{}] {}", getChannelName(msg.channelID), msg.message);

        offset += msg.messageSize;
    }
}
