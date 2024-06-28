#include "connection_cs2console.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

#pragma once

SOCKET cs2ConsoleSock = INVALID_SOCKET;
std::atomic<bool> listeningCS2(false);
std::atomic<bool> cs2ConsoleConnected(false);
std::thread cs2ListenerThread;
std::thread cs2ConnectorThread;

bool connectToCS2Console() {
    const std::string ip = Config::getInstance().get("cs2_console_ip", "127.0.0.1");
    const int port = Config::getInstance().getInt("cs2_console_port", 29000);

    cs2ConsoleSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (cs2ConsoleSock == INVALID_SOCKET)
    {
        std::cerr << "[Connection] [CS2Console] Failed to create socket for CS2 console: " << WSAGetLastError() << '\n';
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    if (connect(cs2ConsoleSock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        std::cerr << "[Connection] [CS2Console] Connection to CS2 console failed: " << WSAGetLastError() << '\n';
        closesocket(cs2ConsoleSock);
        cs2ConsoleSock = INVALID_SOCKET;
        return false;
    }

    std::cout << "[Connection] [CS2Console] Connected to CS2 console at " << ip << ":" << port << '\n';
    return true;
}

void cs2ConsoleConnectorLoop() {
    const int reconnect_delay = Config::getInstance().getInt("cs2_console_reconnect_delay", 5000);
    const int sanity_check_interval = Config::getInstance().getInt("debug_sanity_interval", 5000);

    bool debug_sanity = Config::getInstance().getInt("debug_sanity_enabled", 0) == 1;
    if (debug_sanity)
    {
        std::cout << "[Connection] [CS2Console] Debug sanity check enabled!" << '\n';
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
                std::cout << "[Connection] [CS2Console] Failed to connect to CS2 console. Retrying in " << reconnect_delay / 1000 << " seconds...\n";
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
                std::cout << "[Connection] [CS2Console] Sent sanity check command to CS2 console" << '\n';
                last_sanity_check = now;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void listenForCS2ConsoleData() {
    auto logger = spdlog::basic_logger_mt("Connection-CS2Console", "logs/connection_cs2console.log");

    std::vector<unsigned char> buffer(0x800000);
    u_long mode = 1; // Set non-blocking mode
    ioctlsocket(cs2ConsoleSock, FIONBIO, &mode);

    while (listeningCS2) {
        int bytesReceived = recv(cs2ConsoleSock, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()) - 1, 0);
        if (bytesReceived > 0) {
            logger->info("[Data] Bytes received: {}", bytesReceived);
            int offset = 0;
            while (offset < bytesReceived) {
                if (offset + sizeof(uint32_t) > bytesReceived) {
                    logger->error("[Error] Incomplete packet, exiting loop");
                    break;
                }

                uint32_t magic = byteSwap32(*reinterpret_cast<const uint32_t*>(buffer.data() + offset));
                logger->info("[Data] Magic number: {:#x} at offset {}", magic, offset);

                if (magic == PRNT_MAGIC) {
                    logger->info("[Processing] PRNT message at offset {}", offset);
                    processPRNTMessage(buffer.data() + offset, bytesReceived - offset);
                } else if (magic == CHAN_MAGIC) {
                    logger->info("[Processing] CHAN message at offset {}", offset);
                    processCHANMessage(buffer.data() + offset + 12, bytesReceived - offset - 12); // Skip the header
                } else {
                    logger->error("[Error] Unknown magic number, exiting loop");
                    break;
                }

                // Check if packetSize calculation is within bounds
                if (offset + 10 > bytesReceived) {
                    logger->error("[Error] Incomplete packet size information, exiting loop");
                    break;
                }

                uint32_t packetSize = byteSwap32(*reinterpret_cast<const uint32_t*>(buffer.data() + offset + 6));
                logger->info("[Data] Packet size: {}", packetSize);

                if (packetSize == 0 || offset + packetSize > bytesReceived) {
                    logger->error("[Error] Invalid packet size, exiting loop");
                    break;
                }

                offset += packetSize;
            }
        } else if (bytesReceived == 0) {
            logger->info("\n[Connection] [CS2Console] Connection closed by CS2 console");
            break;
        } else if (WSAGetLastError() != WSAEWOULDBLOCK) {
            logger->error("[Connection] [CS2Console] recv failed from CS2 console: {}", WSAGetLastError());
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Reduced sleep time
    }
    logger->info("[Connection] [CS2Console] CS2 console listener thread stopping...");
}

int sendPayloadToCS2Console(const std::vector<unsigned char>& payload) {
    if (cs2ConsoleSock == INVALID_SOCKET)
    {
        std::cerr << "[Connection] [CS2Console] Cannot send payload: Not connected to CS2 console" << '\n';
        return 1;
    }

    if (send(cs2ConsoleSock, reinterpret_cast<const char*>(payload.data()), static_cast<int>(payload.size()), 0) == SOCKET_ERROR)
    {
        std::cerr << "[Connection] [CS2Console] Failed to send data to CS2 console: " << WSAGetLastError() << '\n';
        cs2ConsoleConnected = false;
        closesocket(cs2ConsoleSock);
        cs2ConsoleSock = INVALID_SOCKET;
        return 1;
    }
    return 0;
}

void cleanupCS2Console() {
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
