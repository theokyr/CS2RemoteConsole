#include "client_handler.h"
#include <iostream>
#include <algorithm>
#include <thread>

void handleClient(ClientInfo& client, std::vector<ClientInfo>& clients, std::mutex& clientsMutex, std::atomic<bool>& running)
{
    char buffer[1024];
    while (running)
    {
        int bytesReceived = recv(client.socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            std::cout << "Received from client " << client.ip << ":" << client.port << ": " << buffer << std::endl;
        }
        else if (bytesReceived == 0)
        {
            std::cout << "Client " << client.ip << ":" << client.port << " disconnected." << std::endl;
            break;
        }
        else if (bytesReceived == SOCKET_ERROR)
        {
#ifdef _WIN32
            if (SOCKET_ERROR_CODE != WSAEWOULDBLOCK)
#else
            if (SOCKET_ERROR_CODE != EWOULDBLOCK && SOCKET_ERROR_CODE != EAGAIN)
#endif
            {
                std::cerr << "recv failed for client " << client.ip << ":" << client.port << " with error: " << SOCKET_ERROR_CODE << std::endl;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::lock_guard<std::mutex> lock(clientsMutex);
    auto it = std::find_if(clients.begin(), clients.end(),
                           [&client](const ClientInfo& c) { return c.socket == client.socket; });
    if (it != clients.end())
    {
        shutdown(it->socket, SHUT_RDWR);
        closesocket(it->socket);
        clients.erase(it);
    }
}
