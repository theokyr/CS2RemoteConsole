#ifndef AINF_H
#define AINF_H

#include <cstdint>
#include <string>

#pragma once

const std::string AINF_MESSAGE_TYPE = "AINF";

struct AINFMessage
{
    int32_t unknown1;
    int32_t unknown2;
    int32_t unknown3;
    int32_t unknown4;
    int32_t unknown5;
    int32_t unknown6;
    int32_t unknown7;
    int32_t unknown8;
    int32_t unknown9;
    int32_t unknown10;
    int32_t unknown11;
    int32_t unknown12;
    int32_t unknown13;
    int32_t unknown14;
    int32_t unknown15;
    int32_t unknown16;
    int32_t unknown17;
    int32_t unknown18;
    int32_t unknown19;
    uint8_t padding;
};

void processAINFMessage(const unsigned char* data, int size);

#endif // AINF_H
