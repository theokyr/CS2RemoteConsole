#include "connection_remoteserver.h"
#include "connection_RemoteServer.h"
#include "payloads.h"
#include <iostream>
#include <ws2tcpip.h>

SOCKET remoteServerSock = INVALID_SOCKET;
std::atomic<bool> listeningRemoteServer(false);
std::atomic<bool> remoteServerConnected(false);
std::thread remoteServerListenerThread;
std::thread remoteServerConnectorThread;

bool connectToRemoteServer()
{
    const std::string ip = Config::getInstance().get("remote_server_ip", "127.0.0.1");
    const int port = Config::getInstance().getInt("remote_server_port", 42069);

    remoteServerSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (remoteServerSock == INVALID_SOCKET)
    {
        std::cerr << "[Connection] [RemoteServer] Failed to create socket for remote server: " << WSAGetLastError() << '\n';
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    if (connect(remoteServerSock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        std::cerr << "[Connection] [RemoteServer] Connection to remote server failed: " << WSAGetLastError() << '\n';
        closesocket(remoteServerSock);
        remoteServerSock = INVALID_SOCKET;
        return false;
    }

    std::cout << "[Connection] [RemoteServer] Connected to remote server at " << ip << ":" << port << '\n';
    return true;
}

void remoteServerConnectorLoop()
{
    const int reconnect_delay = Config::getInstance().getInt("remote_server_reconnect_delay", 5000);

    while (!remoteServerConnected)
    {
        if (connectToRemoteServer())
        {
            remoteServerConnected = true;
            listeningRemoteServer = true;
            remoteServerListenerThread = std::thread(listenForRemoteServerData);
        }
        else
        {
            std::cout << "[Connection] [RemoteServer] Failed to connect to remote server. Retrying in " << reconnect_delay / 1000 << " seconds...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_delay));
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
            std::cout << "\n[Connection] [RemoteServer] Received from remote server: " << buffer << '\n' << ">> ";
            // Forward the command to CS2 console
            auto payload = create_command_payload(buffer);
            sendPayloadToCS2Console(payload);
        }
        else if (bytesReceived == 0)
        {
            std::cout << "\n[Connection] [RemoteServer] Connection closed by remote server" << '\n';
            break;
        }
        else if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            std::cerr << "[Connection] [RemoteServer] recv failed from remote server: " << WSAGetLastError() << '\n';
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "[Connection] [RemoteServer] Remote server listener thread stopping..." << '\n';
    remoteServerConnected = false;
    listeningRemoteServer = false;
    closesocket(remoteServerSock);
    remoteServerSock = INVALID_SOCKET;

    // Restart the connector thread
    remoteServerConnectorThread = std::thread(remoteServerConnectorLoop);
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
        remoteServerConnectorThread.join();
    }
    if (remoteServerSock != INVALID_SOCKET)
    {
        closesocket(remoteServerSock);
        remoteServerSock = INVALID_SOCKET;
    }
}
