#include "server.h"
#include "utils.h"
#include <iostream>

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
            std::cout << "\nNew client connected: " << clientIP << ":" << clientPort
                << " at " << getFormattedTime(m_clients.back().connectionTime) << std::endl;

            std::thread(&handleClient, std::ref(m_clients.back()), std::ref(m_clients), std::ref(m_clientsMutex), std::ref(m_running)).detach();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Server::userInputHandler()
{
    std::string input;
    while (m_running)
    {
        std::cout << "Enter command to send to clients (or '/quit' to exit): ";
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
            for (const auto& client : m_clients)
            {
                std::cout << "  " << client.ip << ":" << client.port
                    << " (connected at " << getFormattedTime(client.connectionTime) << ")" << std::endl;
            }
        }
        else
        {
            broadcastToClients(input);
            std::cout << "Command sent to " << m_clients.size() << " clients." << std::endl;
        }
    }
}

void Server::broadcastToClients(const std::string& message)
{
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    auto it = m_clients.begin();
    while (it != m_clients.end())
    {
        int sendResult = send(it->socket, message.c_str(), static_cast<int>(message.length()), 0);
        if (sendResult == SOCKET_ERROR)
        {
            std::cerr << "send failed for client " << it->ip << ":" << it->port << " with error: " << SOCKET_ERROR_CODE << std::endl;
            shutdown(it->socket, SHUT_RDWR);
            closesocket(it->socket);
            it = m_clients.erase(it);
        }
        else
        {
            ++it;
        }
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

#ifdef _WIN32
    WSACleanup();
#endif

    std::cout << "All sockets have been closed and cleaned up." << std::endl;
}
