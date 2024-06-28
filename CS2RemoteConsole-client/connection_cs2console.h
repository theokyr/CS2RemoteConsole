#ifndef CONNECTION_CS2CONSOLE_H
#define CONNECTION_CS2CONSOLE_H

#include "lib/messages/chan.h"
#include "lib/messages/prnt.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <ws2tcpip.h>

#include "config.h"
#include "payloads.h"
#include "utils.h"

#pragma once

#pragma comment(lib, "ws2_32.lib")

constexpr size_t HEADER_SIZE = 6; // 4 bytes for type, 2 for size
constexpr size_t MAX_VCON_PACKET_SIZE = 8000000; // Maximum VConsole packet size

extern SOCKET cs2ConsoleSock;
extern std::atomic<bool> listeningCS2;
extern std::atomic<bool> cs2ConsoleConnected;
extern std::thread cs2ListenerThread;
extern std::thread cs2ConnectorThread;
extern std::atomic<bool> running;

bool connectToCS2Console();
void cs2ConsoleConnectorLoop();
void listenForCS2ConsoleData();
int sendPayloadToCS2Console(const std::vector<unsigned char>& payload);
void cleanupCS2Console();

#endif // CONNECTION_CS2CONSOLE_H
