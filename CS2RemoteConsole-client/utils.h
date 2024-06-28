#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <direct.h>

#pragma once

void hexDump(const char* desc, const void* addr, int len);
// std::string getColorCode(uint32_t color);
// std::string getCategoryName(uint16_t category, uint32_t channelID);
uint32_t byteSwap32(uint32_t value);
uint16_t byteSwap16(uint16_t value);
std::string getCurrentDirectory();
bool setupConfig();
bool setupWinsock();
#endif // UTILS_H
