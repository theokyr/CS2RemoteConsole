#ifndef PRNT_MESSAGE_H
#define PRNT_MESSAGE_H

#include <cstdint>
#include <string>

#pragma once

const std::string PRNT_MESSAGE_TYPE = "PRNT";

struct PRNTMessage
{
    uint32_t messageType;
    uint16_t commandType;
    uint32_t messageSize;
    uint32_t channelID;
    uint8_t unknown[24];
    std::string message;
};

void processPRNTMessage(const unsigned char* data, int size);

#endif // PRNT_MESSAGE_H
