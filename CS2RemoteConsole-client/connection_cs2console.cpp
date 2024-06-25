#include "connection_cs2console.h"
#include <iostream>
#include <ws2tcpip.h>

SOCKET cs2ConsoleSock = INVALID_SOCKET;
std::atomic<bool> listeningCS2(false);
std::thread cs2ListenerThread;

bool connectToCS2Console()
{
    const std::string ip = Config::getInstance().get("cs2_console_ip", "127.0.0.1");
    const int port = Config::getInstance().getInt("cs2_console_port", 29000);
    const int reconnect_delay = Config::getInstance().getInt("cs2_console_reconnect_delay", 5000);

    while (true)
    {
        cs2ConsoleSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (cs2ConsoleSock == INVALID_SOCKET)
        {
            std::cerr << "Failed to create socket for CS2 console: " << WSAGetLastError() << '\n';
            return false;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

        if (connect(cs2ConsoleSock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            std::cerr << "Connection to CS2 console failed. Retrying in " << reconnect_delay / 1000 << " seconds...\n";
            closesocket(cs2ConsoleSock);
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_delay));
        }
        else
        {
            std::cout << "Connected to CS2 console at " << ip << ":" << port << '\n';
            return true;
        }
    }
}

void listenForCS2ConsoleData()
{
    char buffer[1024];
    int bytesReceived;
    u_long mode = 1; // Set non-blocking mode
    ioctlsocket(cs2ConsoleSock, FIONBIO, &mode);

    while (listeningCS2)
    {
        bytesReceived = recv(cs2ConsoleSock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            std::cout << "\nReceived from CS2 console: " << buffer << '\n' << ">> ";
        }
        else if (bytesReceived == 0)
        {
            std::cout << "\nConnection closed by CS2 console" << '\n';
            break;
        }
        else if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            std::cerr << "recv failed from CS2 console: " << WSAGetLastError() << '\n';
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "CS2 console listener thread stopping..." << '\n';
}

int sendPayloadToCS2Console(const std::vector<unsigned char>& payload)
{
    if (send(cs2ConsoleSock, reinterpret_cast<const char*>(payload.data()), static_cast<int>(payload.size()), 0) == SOCKET_ERROR)
    {
        std::cerr << "Failed to send data to CS2 console" << '\n';
        return 1;
    }
    return 0;
}

void cleanupCS2Console()
{
    listeningCS2 = false;
    if (cs2ListenerThread.joinable())
    {
        cs2ListenerThread.join();
    }
    if (cs2ConsoleSock != INVALID_SOCKET)
    {
        closesocket(cs2ConsoleSock);
        cs2ConsoleSock = INVALID_SOCKET;
    }
}
