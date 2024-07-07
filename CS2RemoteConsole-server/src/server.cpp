#include "server.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <algorithm>

Server::Server(uint16_t port, std::atomic<bool>& running)
    : m_port(port), m_listenSocket(INVALID_SOCKET), m_running(running)
{
}

Server::~Server()
{
    cleanupSockets();
}

bool Server::start()
{
    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET)
    {
        std::cerr << "Error creating socket: " << SOCKET_ERROR_CODE << std::endl;
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(m_port);

    if (bind(m_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Bind failed: " << SOCKET_ERROR_CODE << std::endl;
        cleanupSockets();
        return false;
    }

    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "Listen failed: " << SOCKET_ERROR_CODE << std::endl;
        cleanupSockets();
        return false;
    }

    setNonBlocking(m_listenSocket);

    std::cout << "Server is listening on port " << m_port << "..." << std::endl;
    return true;
}

void Server::run()
{
    m_acceptThread = std::thread(&Server::acceptClients, this);
    userInputHandler();

    std::cout << "Shutting down server..." << std::endl;
    if (m_acceptThread.joinable())
    {
        m_acceptThread.join();
    }
}

void Server::acceptClients()
{
    while (m_running)
    {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(m_listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET)
        {
#ifdef _WIN32
            if (SOCKET_ERROR_CODE != WSAEWOULDBLOCK)
#else
            if (SOCKET_ERROR_CODE != EWOULDBLOCK && SOCKET_ERROR_CODE != EAGAIN)
#endif
            {
                std::cerr << "accept failed with error: " << SOCKET_ERROR_CODE << std::endl;
            }
        }
        else
        {
            setNonBlocking(clientSocket);
            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
            uint16_t clientPort = ntohs(clientAddr.sin_port);

            std::lock_guard<std::mutex> lock(m_clientsMutex);
            m_clients.emplace_back(clientSocket, clientIP, clientPort);
            std::cout << "\nNew client connected: "
                << ANSI_COLOR_IP_PORT << clientIP << ":" << clientPort << ANSI_COLOR_RESET
                << " at "
                << ANSI_COLOR_TIMESTAMP << getFormattedTime(m_clients.back().connectionTime) << ANSI_COLOR_RESET
                << std::endl;

            std::thread(&Server::handleClient, this, std::ref(m_clients.back())).detach();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Server::handleClient(ClientInfo& client)
{
    char buffer[1024];
    while (m_running)
    {
        int bytesReceived = recv(client.socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            handleClientMessage(client, buffer);
        }
        else if (bytesReceived == 0)
        {
            std::cout << "Client "
                << ANSI_COLOR_IP_PORT << client.ip << ":" << client.port << ANSI_COLOR_RESET
                << " disconnected." << std::endl;
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

    removeClient(client);
}

void Server::handleClientMessage(ClientInfo& client, const std::string& message)
{
    size_t colonPos = message.find(':');
    if (colonPos == std::string::npos)
    {
        std::cout << "Received invalid message format from client "
            << ANSI_COLOR_IP_PORT << client.ip << ":" << client.port << ANSI_COLOR_RESET
            << ": " << message << std::endl;
        return;
    }

    std::string command = message.substr(0, colonPos);
    std::string content = message.substr(colonPos + 1);

    if (command == "PLAYERNAME")
    {
        client.name = content;
        std::cout << "Updated player name for client "
            << ANSI_COLOR_IP_PORT << client.ip << ":" << client.port << ANSI_COLOR_RESET
            << " to: "
            << ANSI_COLOR_USERNAME << client.name << ANSI_COLOR_RESET << std::endl;

        std::string broadcastMessage = "Player " + client.name + " has joined the game.";
        broadcastToClients(broadcastMessage, &client);
    }
    else
    {
        std::cout << "Received from client "
            << ANSI_COLOR_IP_PORT << client.ip << ":" << client.port << ANSI_COLOR_RESET
            << ": " << message << std::endl;
        // Handle other message types here
    }
}

void Server::userInputHandler()
{
    std::string input;
    while (m_running)
    {
        std::cout << ANSI_COLOR_COMMON << "Enter command to send to clients (or '/quit' to exit): " << ANSI_COLOR_RESET;
        std::getline(std::cin, input);

        if (input == "/quit")
        {
            m_running = false;
            break;
        }
        else if (input == "/list")
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            std::cout << "Connected clients:" << std::endl;
            int index = 0;
            for (const auto& client : m_clients)
            {
                std::cout << ANSI_COLOR_ID << "[" << index++ << "]" << ANSI_COLOR_RESET << " "
                    << ANSI_COLOR_USERNAME << (client.name.empty() ? "Unknown" : client.name) << ANSI_COLOR_RESET
                    << " - "
                    << ANSI_COLOR_IP_PORT << client.ip << ":" << client.port << ANSI_COLOR_RESET
                    << " - "
                    << ANSI_COLOR_TIMESTAMP << getFormattedTime(client.connectionTime) << ANSI_COLOR_RESET
                    << "\n";
            }
        }
        else
        {
            broadcastToClients(input);
            std::cout << ANSI_COLOR_COMMON << "Command sent to " << ANSI_COLOR_RESET
                << ANSI_COLOR_ID << m_clients.size() << ANSI_COLOR_RESET
                << ANSI_COLOR_COMMON << " clients." << ANSI_COLOR_RESET << std::endl;
        }
    }
}

void Server::broadcastToClients(const std::string& message, const ClientInfo* excludeClient)
{
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (auto& client : m_clients)
    {
        if (excludeClient && client.socket == excludeClient->socket)
        {
            continue;
        }
        int sendResult = send(client.socket, message.c_str(), static_cast<int>(message.length()), 0);
        if (sendResult == SOCKET_ERROR)
        {
            std::cerr << "send failed for client " << client.ip << ":" << client.port << " with error: " << SOCKET_ERROR_CODE << std::endl;
        }
    }
}

void Server::removeClient(const ClientInfo& client)
{
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    auto it = std::find_if(m_clients.begin(), m_clients.end(),
                           [&client](const ClientInfo& c) { return c.socket == client.socket; });
    if (it != m_clients.end())
    {
        shutdown(it->socket, SHUT_RDWR);
        closesocket(it->socket);
        m_clients.erase(it);
    }
}

void Server::cleanupSockets()
{
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (const auto& client : m_clients)
    {
        shutdown(client.socket, SHUT_RDWR);
        closesocket(client.socket);
    }
    m_clients.clear();

    if (m_listenSocket != INVALID_SOCKET)
    {
        shutdown(m_listenSocket, SHUT_RDWR);
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    std::cout << "All sockets have been closed and cleaned up." << std::endl;
}
