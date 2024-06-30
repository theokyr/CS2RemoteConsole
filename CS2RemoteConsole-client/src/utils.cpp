#include <iostream>
#include "config.h"
#include "payloads.h"
#include "utils.h"
#include "connection_cs2console.h"
#include "connection_remoteserver.h"

std::string getCurrentDirectory()
{
    char buffer[FILENAME_MAX];
    if (_getcwd(buffer, FILENAME_MAX) != nullptr)
    {
        return std::string(buffer);
    }
    return "";
}

bool setupApplicationWinsock()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "[Main] WSAStartup failed" << '\n';
        return false;
    }
    return true;
}
