#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include "../../common/platform.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#pragma once

std::string getCurrentDirectory();
bool setupApplicationSockets();

#endif // UTILS_H
