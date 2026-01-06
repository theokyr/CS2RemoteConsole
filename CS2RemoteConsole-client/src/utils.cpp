#include <iostream>
#include "config.h"
#include "utils.h"
#include "connection/connection_cs2console.h"
#include "connection/connection_remoteserver.h"

std::string getCurrentDirectory()
{
    char buffer[FILENAME_MAX];
#ifdef _WIN32
    if (_getcwd(buffer, FILENAME_MAX) != nullptr)
#else
    if (getcwd(buffer, FILENAME_MAX) != nullptr)
#endif
    {
        return std::string(buffer);
    }
    return "";
}

bool setupApplicationSockets()
{
    if (!platformSocketInit())
    {
        std::cerr << "[Main] Socket initialization failed" << '\n';
        return false;
    }
    return true;
}
