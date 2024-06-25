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

extern SOCKET sock;
extern std::atomic<bool> running;
extern std::atomic<bool> listening;
extern std::thread listenerThread;
extern std::thread sanityCheckThread;

bool setupConfig();
bool setupWinsock();
void cleanupWinsock();
bool connectToServer(const char* ip, int port);
int sendPayload(const std::vector<unsigned char>& payload);
void listenForData();
void userInputHandler();
void sendSanityCheck();
void gracefulShutdown();

#endif // MAIN_H
