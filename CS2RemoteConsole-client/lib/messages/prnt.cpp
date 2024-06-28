#include "prnt.h"
#include "../channel.h"
#include "../../utils.h"
#include <iostream>
#include <iomanip>

#pragma once

void processPRNTMessage(const unsigned char* data, int size)
{
    const int headerSize = 38; // 4 + 2 + 4 + 4 + 24 = 38 bytes
    int offset = 0;

    while (offset + headerSize <= size)
    {
        PRNTMessage msg;

        msg.magic = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset));
        if (msg.magic != PRNT_MAGIC)
        {
            std::cout << "Received non-PRNT message at offset " << offset << "\n";
            return;
        }

        msg.commandType = byteSwap16(*reinterpret_cast<const uint16_t*>(data + offset + 4));
        msg.messageSize = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset + 6));
        msg.channelID = byteSwap32(*reinterpret_cast<const uint32_t*>(data + offset + 10));

        std::memcpy(msg.unknown, data + offset + 14, 24);

        int messageContentLength = msg.messageSize - headerSize;
        if (messageContentLength <= 0 || msg.messageSize > static_cast<uint32_t>(size - offset))
        {
            std::cout << "Incomplete PRNT message received\n";
            return;
        }

        msg.message = std::string(reinterpret_cast<const char*>(data + offset + headerSize), messageContentLength);

        std::cout << getColorCode(msg.channelID);
        std::cout << "[" << getChannelName(msg.channelID) << "] " << msg.message;
        std::cout << "\033[0m" << std::endl; // Reset color

        offset += msg.messageSize;
    }
}
