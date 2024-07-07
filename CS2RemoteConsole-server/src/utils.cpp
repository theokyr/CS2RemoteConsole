#include "utils.h"
#include <iostream>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
bool initializeWinsock()
{
    WSADATA wsaData;
    return (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
}
#endif

void setNonBlocking(SOCKET socket)
{
#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(socket, FIONBIO, &mode) != 0)
    {
        std::cerr << "Failed to set socket to non-blocking mode." << std::endl;
    }
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Failed to get socket flags." << std::endl;
        return;
    }
    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Failed to set socket to non-blocking mode." << std::endl;
    }
#endif
}

std::string getFormattedTime(const std::chrono::system_clock::time_point& time)
{
    auto in_time_t = std::chrono::system_clock::to_time_t(time);
    std::stringstream ss;

#ifdef _WIN32
    struct tm timeinfo;
    localtime_s(&timeinfo, &in_time_t);
    ss << std::put_time(&timeinfo, "%Y-%m-%d %X");
#else
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
#endif

    return ss.str();
}
