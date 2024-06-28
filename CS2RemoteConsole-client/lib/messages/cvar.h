#pragma once

#ifndef CVAR_H
#define CVAR_H

#include <cstdint>
#include <string>

const std::string CVAR_MESSAGE_TYPE = "CVAR";

struct Cvar
{
    std::string name;
    int32_t unknown;
    int32_t flags;
    float rangemin;
    float rangemax;
    uint8_t padding;
};

void processCVARMessage(const unsigned char* data, int size);

#endif // CVAR_H
