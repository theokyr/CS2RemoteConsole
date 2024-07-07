#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "utils.h"
#include <string>
#include <chrono>
#include <vector>
#include <mutex>
#include <atomic>

struct ClientInfo
{
    std::string name;

    SOCKET socket;
    std::string ip;
    uint16_t port;
    std::chrono::system_clock::time_point connectionTime;

    ClientInfo(SOCKET s, const std::string& i, uint16_t p)
        : socket(s), ip(i), port(p), connectionTime(std::chrono::system_clock::now())
    {
    }
};

void handleClient(ClientInfo& client, std::vector<ClientInfo>& clients, std::mutex& clientsMutex, std::atomic<bool>& running);

#endif // CLIENT_HANDLER_H
