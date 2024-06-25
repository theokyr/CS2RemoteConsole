#ifndef CONNECTION_CS2CONSOLE_H
#define CONNECTION_CS2CONSOLE_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <atomic>
#include <thread>
#include <vector>
#include "config.h"

#pragma comment(lib, "ws2_32.lib")

extern SOCKET cs2ConsoleSock;
extern std::atomic<bool> listeningCS2;
extern std::thread cs2ListenerThread;

bool connectToCS2Console();
void listenForCS2ConsoleData();
int sendPayloadToCS2Console(const std::vector<unsigned char>& payload);
void cleanupCS2Console();

#endif // CONNECTION_CS2CONSOLE_H