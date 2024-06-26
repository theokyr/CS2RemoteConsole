#ifndef CONNECTION_REMOTESERVER_H
#define CONNECTION_REMOTESERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <atomic>
#include <thread>
#include "connection_cs2console.h"
#include "config.h"

#pragma comment(lib, "ws2_32.lib")

extern SOCKET remoteServerSock;
extern std::atomic<bool> listeningRemoteServer;
extern std::atomic<bool> remoteServerConnected;
extern std::thread remoteServerListenerThread;
extern std::thread remoteServerConnectorThread;

bool connectToRemoteServer();
void remoteServerConnectorLoop();
void listenForRemoteServerData();
void cleanupRemoteServer();

#endif // CONNECTION_REMOTESERVER_H