#ifndef MAIN_H
#define MAIN_H

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "config.h"
#include "payloads.h"
#include "utils.h"

#pragma comment(lib, "ws2_32.lib")

extern SOCKET cs2ConsoleSock;
extern SOCKET remoteServerSock;
extern std::atomic<bool> running;
extern std::atomic<bool> listeningCS2;
extern std::atomic<bool> listeningRemoteServer;
extern std::atomic<bool> remoteServerConnected;
extern std::thread cs2ListenerThread;
extern std::thread remoteServerListenerThread;
extern std::thread remoteServerConnectorThread;
extern std::thread sanityCheckThread;

void signalHandler(int signum);
bool setupConfig();
bool setupWinsock();
void cleanupWinsock();
bool connectToCS2Console();
bool connectToRemoteServer();
void remoteServerConnectorLoop();
void listenForCS2ConsoleData();
void listenForRemoteServerData();
int sendPayload(SOCKET sock, const std::vector<unsigned char>& payload);
void userInputHandler();
void sendSanityCheck();
void gracefulShutdown();

#endif // MAIN_H