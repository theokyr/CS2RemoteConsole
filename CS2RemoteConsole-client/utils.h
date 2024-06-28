#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <direct.h>

#pragma once

void hexDump(const char* desc, const void* addr, int len);
std::string getColorCode(uint32_t color);
std::string getCategoryName(uint32_t unknown2);
uint32_t byteSwap32(uint32_t value);
uint16_t byteSwap16(uint16_t value);
std::vector<unsigned char> create_command_payload(const std::string& command);
std::string getCurrentDirectory();
bool setupConfig();
bool setupWinsock();
#endif // UTILS_H
