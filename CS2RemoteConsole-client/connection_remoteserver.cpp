#include "connection_remoteserver.h"
#include "payloads.h"
#include <iostream>
#include <ws2tcpip.h>
#include <chrono>
#include <spdlog/spdlog.h>

#include "logging.h"

SOCKET remoteServerSock = INVALID_SOCKET;
std::atomic<bool> listeningRemoteServer(false);
std::atomic<bool> remoteServerConnected(false);
std::thread remoteServerListenerThread;
std::thread remoteServerConnectorThread;

bool connectToRemoteServer()
{
    auto logger = spdlog::get(LOGGER_REMOTE_SERVER);

    const std::string ip = Config::getInstance().get("remote_server_ip", "127.0.0.1");
    const int port = Config::getInstance().getInt("remote_server_port", 42069);

    remoteServerSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (remoteServerSock == INVALID_SOCKET)
    {
        logger->error("[Connection] [RemoteServer] Failed to create socket for remote server: {}", WSAGetLastError());
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    if (connect(remoteServerSock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        logger->error("[Connection] [RemoteServer] Connection to remote server failed: {}", WSAGetLastError());
        closesocket(remoteServerSock);
        remoteServerSock = INVALID_SOCKET;
        return false;
    }

    logger->info("[Connection] [RemoteServer] Connected to remote server at {}:{}", ip, port);
    return true;
}

void remoteServerConnectorLoop()
{
    auto logger = spdlog::get(LOGGER_REMOTE_SERVER);

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
            }
            else
            {
                logger->error("[Connection] [RemoteServer] Failed to connect to remote server. Retrying in {} seconds...", reconnect_delay / 1000);
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
    auto logger = spdlog::get(LOGGER_REMOTE_SERVER);

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
            logger->info("[Connection] [RemoteServer] Received '{}' from remote server", buffer);
            // Forward the command to CS2 console
            auto payload = create_command_payload(buffer);
            sendPayloadToCS2Console(payload);
        }
        else if (bytesReceived == 0)
        {
            logger->warn("[Connection] [RemoteServer] Connection closed by remote server");
            break;
        }
        else if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            logger->error("[Connection] [RemoteServer] recv failed from remote server: Error code {} ", WSAGetLastError());
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    logger->info("[Connection] [RemoteServer] Remote server listener thread stopping...");
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
