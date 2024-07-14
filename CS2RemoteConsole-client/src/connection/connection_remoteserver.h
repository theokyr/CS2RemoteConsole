#ifndef CONNECTION_REMOTESERVER_H
#define CONNECTION_REMOTESERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <atomic>
#include <thread>
#include "connection_cs2console.h"
#include "../config.h"

#pragma once

#pragma comment(lib, "ws2_32.lib")

extern SOCKET remoteServerSock;
extern std::atomic<bool> listeningRemoteServer;
extern std::atomic<bool> remoteServerConnected;
extern std::thread remoteServerListenerThread;
extern std::thread remoteServerConnectorThread;

bool connectToRemoteServer();
void remoteServerConnectorLoop();
void listenForRemoteServerData();
bool sendMessageToRemoteServer(const std::string& message);
void cleanupRemoteServer();

struct ClientInfo
{
    std::string name;
    std::string gameServerIp;
    uint16_t gamePing;
    uint16_t totalLatency;
    uint16_t netBufferTicks; //0, 1, 2;
    std::string smooth; //"true" or "false", I am too lazy to convert back and forth


    ClientInfo(const std::string& i): name(i)
    {
    }
};

extern ClientInfo globalClientInfo;

#endif // CONNECTION_REMOTESERVER_H
