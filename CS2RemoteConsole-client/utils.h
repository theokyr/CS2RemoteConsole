﻿#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <direct.h>

std::vector<unsigned char> create_command_payload(const std::string& command);
std::string getCurrentDirectory();

#endif // UTILS_H