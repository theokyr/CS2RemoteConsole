#ifndef MESSAGES_H
#define MESSAGES_H
#include <cstdint>
#include <string>

#pragma once

struct CMNDMessage
{
    // TODO
};

extern uint32_t PRNT_MAGIC;

struct PRNTMessage
{
    uint32_t magic; // "PRNT" magic number
    uint16_t commandType; // Command type
    uint32_t messageSize; // Total size of the message including header
    uint32_t timestamp; // Timestamp of the message
    uint64_t unknown1; // Unknown field
    uint16_t category; // Message category
    uint32_t color; // Color code for the message
    uint32_t unknown2; // Unknown field
    std::string message; // Actual message content
};
#endif
