﻿#include "connection_remoteserver.h"
#include <iostream>
#include <chrono>
#include <spdlog/spdlog.h>

SOCKET remoteServerSock = INVALID_SOCKET;
std::atomic<bool> listeningRemoteServer(false);
std::atomic<bool> remoteServerConnected(false);
std::thread remoteServerListenerThread;
std::thread remoteServerConnectorThread;

ClientInfo globalClientInfo("");

bool connectToRemoteServer()
{
    const std::string ip = Config::getInstance().get("remote_server_ip", "127.0.0.1");
    const int port = Config::getInstance().getInt("remote_server_port", 42069);

    remoteServerSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (remoteServerSock == INVALID_SOCKET)
    {
        spdlog::error("[RemoteServerConnection] Failed to create socket for remote server: {}", WSAGetLastError());
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    if (connect(remoteServerSock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        spdlog::error("[RemoteServerConnection] Connection to remote server failed: {}", WSAGetLastError());
        closesocket(remoteServerSock);
        remoteServerSock = INVALID_SOCKET;
        return false;
    }

    spdlog::info("[RemoteServerConnection] Connected to remote server at {}:{}", ip, port);
    return true;
}

bool sendMessageToRemoteServer(const std::string& message)
{
    if (remoteServerSock == INVALID_SOCKET)
    {
        spdlog::error("[RemoteServerConnection] Cannot send message: Not connected to remote server");
        return false;
    }

    int sendResult = send(remoteServerSock, message.c_str(), static_cast<int>(message.length()), 0);
    if (sendResult == SOCKET_ERROR)
    {
        spdlog::error("[RemoteServerConnection] Failed to send message to remote server: {}", WSAGetLastError());
        return false;
    }

    return true;
}

void remoteServerConnectorLoop()
{
    const int reconnect_delay = Config::getInstance().getInt("remote_server_reconnect_delay", 5000);

    while (true)
    {
        if (!remoteServerConnected)
        {
            if (connectToRemoteServer())
            {
                remoteServerConnected = true;
                listeningRemoteServer = true;
                if (remoteServerListenerThread.joinable())
                {
                    remoteServerListenerThread.join();
                }
                remoteServerListenerThread = std::thread(listenForRemoteServerData);

                // Send player name if available
                if (!globalClientInfo.name.empty())
                {
                    std::string nameMessage = "PLAYERNAME:" + globalClientInfo.name;
                    if (sendMessageToRemoteServer(nameMessage))
                    {
                        spdlog::info("[RemoteServerConnection] Sent player name to remote server.");
                    }
                    else
                    {
                        spdlog::error("[RemoteServerConnection] Failed to send player name to remote server.");
                    }
                }
            }
            else
            {
                spdlog::error("[RemoteServerConnection] Failed to connect to remote server. Retrying in {} seconds...", reconnect_delay / 1000);
                std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_delay));
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

void listenForRemoteServerData()
{
    char buffer[1024];
    int bytesReceived;
    u_long mode = 1; // Set non-blocking mode
    ioctlsocket(remoteServerSock, FIONBIO, &mode);

    while (listeningRemoteServer)
    {
        bytesReceived = recv(remoteServerSock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            spdlog::info("[RemoteServerConnection] Received '{}' from remote server", buffer);
            sendPayloadToCS2Console(buffer);
        }
        else if (bytesReceived == 0)
        {
            spdlog::warn("[RemoteServerConnection] Connection closed by remote server");
            break;
        }
        else if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            spdlog::error("[RemoteServerConnection] recv failed from remote server: Error code {} ", WSAGetLastError());
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    spdlog::info("[RemoteServerConnection] Remote server listener thread stopping...");
    remoteServerConnected = false;
    listeningRemoteServer = false;
    closesocket(remoteServerSock);
    remoteServerSock = INVALID_SOCKET;
}

void cleanupRemoteServer()
{
    listeningRemoteServer = false;
    remoteServerConnected = false;
    if (remoteServerListenerThread.joinable())
    {
        remoteServerListenerThread.join();
    }
    if (remoteServerConnectorThread.joinable())
    {
        remoteServerConnectorThread.detach();
    }
    if (remoteServerSock != INVALID_SOCKET)
    {
        closesocket(remoteServerSock);
        remoteServerSock = INVALID_SOCKET;
    }
}
