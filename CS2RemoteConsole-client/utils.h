#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <direct.h>

#pragma once

// uint32_t byteSwap32(uint32_t value);
// uint16_t byteSwap16(uint16_t value);
// std::string uint32ToString(uint32_t value);
// uint32_t stringToUint32(const std::string& str);
std::string getCurrentDirectory();
bool setupApplicationWinsock();
#endif // UTILS_H
