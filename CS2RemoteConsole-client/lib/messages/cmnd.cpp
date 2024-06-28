#include "cmnd.h"
#include "../../utils.h"

#pragma once

CMNDMessage create_command_message(const std::string& command)
{
    CMNDMessage msg;
    msg.messageType = stringToUint32(CMND_MESSAGE_TYPE);
    msg.commandType = 0x00D4;
    msg.unknown = 0;
    msg.command = command;
    msg.messageSize = sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + command.length() + 1;
    return msg;
}
