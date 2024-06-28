#ifndef CFGV_H
#define CFGV_H

#include <cstdint>
#include <string>
#include <vector>

#pragma once

const std::string CFGV_MESSAGE_TYPE = "CFGV";

struct CFGVMessage
{
    std::vector<uint8_t> unknow;
};

void processCFGVMessage(const unsigned char* data, int size);

#endif // CFGV_H
