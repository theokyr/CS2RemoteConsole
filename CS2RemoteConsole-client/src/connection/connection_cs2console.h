﻿#ifndef CONNECTION_CS2CONSOLE_H
#define CONNECTION_CS2CONSOLE_H

#include <atomic>
#include <thread>
#include <vector>
#include "vconsole.h"
#include "../singletons.h"

#pragma comment(lib, "libvconsole.lib")

#pragma once

extern std::atomic<bool> listeningCS2;
extern std::atomic<bool> cs2ConsoleConnected;
extern std::atomic<bool> running;
extern std::thread cs2ConnectorThread;
extern std::thread cs2ListenerThread;

bool connectToCS2Console();
void cs2ConsoleConnectorLoop();
void listenForCS2ConsoleData();
int sendPayloadToCS2Console(const std::string& payload);
void cleanupCS2Console();
void initializeCS2Connection();

#endif // CONNECTION_CS2CONSOLE_H
