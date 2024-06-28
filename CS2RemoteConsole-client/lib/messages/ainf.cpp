#include "ainf.h"
#include "../../utils.h"
#include <spdlog/spdlog.h>

void processAINFMessage(const unsigned char* data, int size)
{
    if (size < sizeof(AINFMessage))
    {
        spdlog::error("Incomplete AINF message received");
        return;
    }

    AINFMessage msg;
    const int32_t* dataInt32 = reinterpret_cast<const int32_t*>(data);
    msg.unknown1 = byteSwap32(dataInt32[0]);
    msg.unknown2 = byteSwap32(dataInt32[1]);
    msg.unknown3 = byteSwap32(dataInt32[2]);
    msg.unknown4 = byteSwap32(dataInt32[3]);
    msg.unknown5 = byteSwap32(dataInt32[4]);
    msg.unknown6 = byteSwap32(dataInt32[5]);
    msg.unknown7 = byteSwap32(dataInt32[6]);
    msg.unknown8 = byteSwap32(dataInt32[7]);
    msg.unknown9 = byteSwap32(dataInt32[8]);
    msg.unknown10 = byteSwap32(dataInt32[9]);
    msg.unknown11 = byteSwap32(dataInt32[10]);
    msg.unknown12 = byteSwap32(dataInt32[11]);
    msg.unknown13 = byteSwap32(dataInt32[12]);
    msg.unknown14 = byteSwap32(dataInt32[13]);
    msg.unknown15 = byteSwap32(dataInt32[14]);
    msg.unknown16 = byteSwap32(dataInt32[15]);
    msg.unknown17 = byteSwap32(dataInt32[16]);
    msg.unknown18 = byteSwap32(dataInt32[17]);
    msg.unknown19 = byteSwap32(dataInt32[18]);
    msg.padding = data[sizeof(AINFMessage) - 1];

    spdlog::info("Processed AINF message");
}
