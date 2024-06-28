#ifndef CMND_MESSAGE_H
#define CMND_MESSAGE_H

#include <cstdint>
#include <string>

#pragma once

#define CMND_MAGIC 0x444E4D43 // "CMND" in little endian

struct CMNDMessage
{
    uint32_t magic;
    uint16_t commandType;
    uint32_t messageSize;
    uint16_t unknown;
    std::string command;
};

CMNDMessage create_command_message(const std::string& command);

#endif // CMND_MESSAGE_H
