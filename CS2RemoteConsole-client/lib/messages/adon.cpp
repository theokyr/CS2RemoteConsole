#include "adon.h"
#include <spdlog/spdlog.h>
#include "../utils.h"

void processADONMessage(const unsigned char* data, int size)
{
    if (size < 4)
    {
        spdlog::error("Incomplete ADON message received");
        return;
    }

    ADONMessage msg;
    msg.unknown = byteSwap16(*reinterpret_cast<const int16_t*>(data));
    msg.length = byteSwap16(*reinterpret_cast<const int16_t*>(data + 2));

    if (size < 4 + msg.length)
    {
        spdlog::error("Incomplete ADON message received");
        return;
    }

    msg.name = std::string(reinterpret_cast<const char*>(data + 4), msg.length);

    spdlog::info("Processed ADON message: {}", msg.name);
}
