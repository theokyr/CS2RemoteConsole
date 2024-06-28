#include "connection_cs2console.h"

#include "logging.h"
#include "lib/messages/adon.h"
#include "lib/messages/ainf.h"
#include "lib/messages/cfgv.h"
#include "lib/messages/cvar.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#pragma once

SOCKET cs2ConsoleSock = INVALID_SOCKET;
std::atomic<bool> listeningCS2(false);
std::atomic<bool> cs2ConsoleConnected(false);
std::thread cs2ListenerThread;
std::thread cs2ConnectorThread;

bool connectToCS2Console()
{
    auto logger = spdlog::get(LOGGER_VCON);
    const std::string ip = Config::getInstance().get("cs2_console_ip", "127.0.0.1");
    const int port = Config::getInstance().getInt("cs2_console_port", 29000);

    cs2ConsoleSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (cs2ConsoleSock == INVALID_SOCKET)
    {
        logger->error("Failed to create socket for CS2 console: Error Code {} ", WSAGetLastError());
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    if (connect(cs2ConsoleSock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        logger->error("Connection to CS2 console failed: Error Code {} ", WSAGetLastError());
        closesocket(cs2ConsoleSock);
        cs2ConsoleSock = INVALID_SOCKET;
        return false;
    }

    logger->info("Connected to CS2 console at {}:{}", ip, port);
    return true;
}

void cs2ConsoleConnectorLoop()
{
    auto logger = spdlog::get(LOGGER_VCON);
    const int reconnect_delay = Config::getInstance().getInt("cs2_console_reconnect_delay", 5000);
    const int sanity_check_interval = Config::getInstance().getInt("debug_sanity_interval", 5000);

    bool debug_sanity = Config::getInstance().getInt("debug_sanity_enabled", 0) == 1;
    if (debug_sanity)
    {
        logger->info("Debug sanity check enabled!");
    }

    auto last_sanity_check = std::chrono::steady_clock::now();

    while (running)
    {
        if (!cs2ConsoleConnected)
        {
            if (connectToCS2Console())
            {
                cs2ConsoleConnected = true;
            }
            else
            {
                logger->debug("Failed to connect to CS2 console. Retrying in {} seconds...", reconnect_delay / 1000);
                std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_delay));
                continue;
            }
        }

        if (debug_sanity && cs2ConsoleConnected)
        {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_sanity_check).count() >= sanity_check_interval)
            {
                sendPayloadToCS2Console(command_say_sanity_check_payload);
                logger->debug("Sent sanity check command to CS2 console");
                last_sanity_check = now;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void listenForCS2ConsoleData()
{
    auto logger = spdlog::get(LOGGER_VCON);
    std::vector<unsigned char> buffer(MAX_VCON_PACKET_SIZE);
    u_long mode = 1; // Set non-blocking mode
    ioctlsocket(cs2ConsoleSock, FIONBIO, &mode);

    while (listeningCS2)
    {
        int bytesReceived = recv(cs2ConsoleSock, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()) - 1, 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            logger->debug("Received {} bytes", bytesReceived);

            size_t processedBytes = 0;
            while (processedBytes + HEADER_SIZE <= bytesReceived)
            {
                char msgType[5] = {0};
                std::memcpy(msgType, buffer.data() + processedBytes, 4);
                uint16_t packetSize = ntohs(*reinterpret_cast<uint16_t*>(buffer.data() + processedBytes + 4));

                if (processedBytes + packetSize > bytesReceived)
                {
                    // Incomplete packet, wait for more data
                    break;
                }

                logger->debug("Processing packet: Type: {}, Size: {}", msgType, packetSize);

                if (strcmp(msgType, "PRNT") == 0)
                {
                    processPRNTMessage(buffer.data() + processedBytes, packetSize);
                }
                else if (strcmp(msgType, "CHAN") == 0)
                {
                    processCHANMessage(buffer.data() + processedBytes, packetSize);
                }
                else if (strcmp(msgType, "AINF") == 0)
                {
                    processAINFMessage(buffer.data() + processedBytes, packetSize);
                }
                else if (strcmp(msgType, "ADON") == 0)
                {
                    processADONMessage(buffer.data() + processedBytes, packetSize);
                }
                else if (strcmp(msgType, "CFGV") == 0)
                {
                    processCFGVMessage(buffer.data() + processedBytes, packetSize);
                }
                else if (strcmp(msgType, "CVAR") == 0)
                {
                    processCVARMessage(buffer.data() + processedBytes, packetSize);
                }
                else
                {
                    logger->warn("Unknown message type: {}", msgType);
                }

                processedBytes += packetSize;
            }
        }
        else if (bytesReceived == 0)
        {
            logger->info("Connection closed by CS2 console");
            break;
        }
        else if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            logger->error("recv failed from CS2 console: {}", WSAGetLastError());
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    logger->info("CS2 console listener thread stopping...");
}

int sendPayloadToCS2Console(const std::vector<unsigned char>& payload)
{
    auto logger = spdlog::get(LOGGER_VCON);

    if (cs2ConsoleSock == INVALID_SOCKET)
    {
        logger->error("Cannot send payload: Not connected to CS2 console");
        return 1;
    }

    if (send(cs2ConsoleSock, reinterpret_cast<const char*>(payload.data()), static_cast<int>(payload.size()), 0) == SOCKET_ERROR)
    {
        logger->error("Failed to send data to CS2 console: Error code {}", WSAGetLastError());
        cs2ConsoleConnected = false;
        closesocket(cs2ConsoleSock);
        cs2ConsoleSock = INVALID_SOCKET;
        return 1;
    }
    return 0;
}

void cleanupCS2Console()
{
    listeningCS2 = false;
    cs2ConsoleConnected = false;
    if (cs2ListenerThread.joinable())
    {
        cs2ListenerThread.join();
    }
    if (cs2ConnectorThread.joinable())
    {
        cs2ConnectorThread.detach();
    }
    if (cs2ConsoleSock != INVALID_SOCKET)
    {
        closesocket(cs2ConsoleSock);
        cs2ConsoleSock = INVALID_SOCKET;
    }
}
