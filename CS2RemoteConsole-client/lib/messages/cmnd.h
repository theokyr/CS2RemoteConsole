#ifndef CMND_MESSAGE_H
#define CMND_MESSAGE_H

#include <cstdint>
#include <string>

#pragma once

const std::string CMND_MESSAGE_TYPE = "CMND";

struct CMNDMessage
{
    uint32_t messageType;
    uint16_t commandType;
    uint32_t messageSize;
    uint16_t unknown;
    std::string command;
};

CMNDMessage create_command_message(const std::string& command);

#endif // CMND_MESSAGE_H
