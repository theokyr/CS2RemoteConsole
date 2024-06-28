#include "cfgv.h"
#include <spdlog/spdlog.h>
#include "../utils.h"

void processCFGVMessage(const unsigned char* data, int size)
{
    if (size < 129)
    {
        spdlog::error("Incomplete CFGV message received");
        return;
    }

    CFGVMessage msg;
    msg.unknow.assign(data, data + 129);

    spdlog::info("Processed CFGV message");
}
