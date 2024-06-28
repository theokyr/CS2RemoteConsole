#include "cvar.h"
#include <spdlog/spdlog.h>
#include "../utils.h"

void processCVARMessage(const unsigned char* data, int size)
{
    if (size < 74)
    {
        spdlog::error("Incomplete CVAR message received");
        return;
    }

    Cvar msg;
    msg.name = std::string(reinterpret_cast<const char*>(data), 64);
    msg.unknown = byteSwap32(*reinterpret_cast<const int32_t*>(data + 64));
    msg.flags = byteSwap32(*reinterpret_cast<const int32_t*>(data + 68));
    msg.rangemin = *reinterpret_cast<const float*>(data + 72);
    msg.rangemax = *reinterpret_cast<const float*>(data + 76);
    msg.padding = data[80];

    spdlog::info("Processed CVAR message: {} with range [{} - {}] and flags {}", msg.name, msg.rangemin, msg.rangemax, msg.flags);
}
