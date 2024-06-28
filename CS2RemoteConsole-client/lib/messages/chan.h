#ifndef CHAN_MESSAGE_H
#define CHAN_MESSAGE_H

#include <cstdint>
#include <vector>
#include <iostream>
#include <algorithm>
#include "../channel.h"
#include "../../utils.h"

#pragma once

#define CHAN_MAGIC 0x4348414E

struct CHANMessage
{
    uint16_t length;
    std::vector<Channel> channels;
};

void processCHANMessage(const unsigned char* data, int size);

#endif // CHAN_MESSAGE_H
