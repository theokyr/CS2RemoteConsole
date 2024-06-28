#ifndef ADON_H
#define ADON_H

#include <cstdint>
#include <string>

#pragma once

const std::string ADON_MESSAGE_TYPE = "ADON";

struct ADONMessage
{
    int16_t unknown;
    int16_t length;
    std::string name;
};

void processADONMessage(const unsigned char* data, int size);

#endif // ADON_H
