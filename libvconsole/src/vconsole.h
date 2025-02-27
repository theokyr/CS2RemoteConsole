﻿#ifndef VCONSOLE_H
#define VCONSOLE_H

#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <iterator>
#include "messages.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#pragma once

struct VConChunk
{
    char type[4];
    uint32_t version;
    uint16_t length;
    uint16_t handle;
};

class VConsole
{
public:
    VConsole();
    ~VConsole();

    std::string stripNonAscii(const std::string& input)
    {
        std::string output;
        std::copy_if(input.begin(), input.end(), std::back_inserter(output),
                     [](unsigned char c) { return c <= 127; });
        return output;
    }

    bool connect(const std::string& ip = "127.0.0.1", int port = 29000);
    void disconnect();
    bool sendCmd(const std::string& cmd);
    int readChunk(std::vector<char>& outputBuf);

    std::vector<Channel>* getChannels()
    {
        return &channels;
    }

    void setOnPRNTReceived(std::function<void(const PRNT&)> callback);
    void setOnCVARsLoaded(std::function<void(const std::vector<Cvar>&)> callback);
    void setOnADONReceived(std::function<void(const std::string&)> callback);
    void setOnDisconnected(std::function<void()> callback);
    void setOnCHANReceived(std::function<void(const CHAN&)> callback);

    bool processIncomingData();
    SOCKET getSocket() const { return clientSocket; }

    bool isConnected() const
    {
        if (clientSocket == INVALID_SOCKET)
            return false;

        // Perform a non-blocking check on the socket
        char buf;
        int result = recv(clientSocket, &buf, 1, MSG_PEEK);
        if (result == 0 || (result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK))
            return false;

        return true;
    }

private:
    SOCKET clientSocket;

    std::function<void(const PRNT&)> onPRNTReceived;
    std::function<void(const std::vector<Cvar>&)> onCVARsLoaded;
    std::function<void(const std::string&)> onADONReceived;
    std::function<void()> onDisconnected;
    std::function<void(const CHAN&)> onCHANReceived;

    std::vector<Channel> channels;
    std::vector<Cvar> cvars;
    std::string adonName;

    void processPacket(const std::string& msgType, const std::vector<char>& chunkBuf);

    AINF parseAINF(const std::vector<char>& chunkBuf);
    ADON parseADON(const std::vector<char>& chunkBuf);
    CHAN parseCHAN(const std::vector<char>& chunkBuf);
    PRNT parsePRNT(const std::vector<char>& chunkBuf);
    CVAR parseCVAR(const std::vector<char>& chunkBuf);
    CFGV parseCFGV(const std::vector<char>& chunkBuf);
};

bool setupWinsock();
std::vector<unsigned char> createCommandPayload(const std::string& command);

#endif // VCONSOLE_H
