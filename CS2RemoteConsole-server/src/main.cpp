#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <csignal>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define SOCKET_ERROR_CODE WSAGetLastError()
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#define SOCKET_ERROR_CODE errno
#endif

std::vector<SOCKET> clients;
std::mutex clientsMutex;
std::atomic<bool> applicationRunning(true);
SOCKET listenSocket;

void signalHandler(int signum)
{
    std::cout << "\nInterrupt signal (" << signum << ") received.\n";
    applicationRunning = false;
    // Close the listening socket to unblock the accept() call
    closesocket(listenSocket);
}

#ifdef _WIN32
bool initializeWinsock()
{
    WSADATA wsaData;
    return (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
}

void cleanupWinsock()
{
    WSACleanup();
}
#endif

void handleClient(SOCKET clientSocket)
{
    char buffer[1024];
    while (applicationRunning)
    {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            std::cout << "Received from client: " << buffer << std::endl;
        }
        else if (bytesReceived == 0 || bytesReceived == SOCKET_ERROR)
        {
            break;
        }
    }

    std::lock_guard<std::mutex> lock(clientsMutex);
    clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
    closesocket(clientSocket);
}

void acceptClients(SOCKET listenSocket)
{
    while (applicationRunning)
    {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET)
        {
            if (applicationRunning)
            {
                std::cerr << "accept failed: " << SOCKET_ERROR_CODE << std::endl;
            }
            break; 
        }

        std::cout << "New client connected." << std::endl;
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientSocket);
        }
        std::thread(handleClient, clientSocket).detach();
    }
}

void broadcastToClients(const std::string& message)
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (const auto& clientSocket : clients)
    {
        send(clientSocket, message.c_str(), static_cast<int>(message.length()), 0);
    }
}

void userInputHandler()
{
    std::string input;
    while (applicationRunning)
    {
        std::cout << "Enter command to send to clients (or 'quit' to exit): ";
        std::getline(std::cin, input);

        if (input == "quit")
        {
            applicationRunning = false;
            closesocket(listenSocket);
            break;
        }

        broadcastToClients(input);
        std::cout << "Command sent to " << clients.size() << " clients." << std::endl;
    }
}

int main()
{
    signal(SIGINT, signalHandler);

#ifdef _WIN32
    if (!initializeWinsock())
    {
        std::cerr << "Failed to initialize Winsock\n";
        return 1;
    }
#endif

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
    {
        std::cerr << "Error creating socket: " << SOCKET_ERROR_CODE << std::endl;
#ifdef _WIN32
        cleanupWinsock();
#endif
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(42069);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Bind failed: " << SOCKET_ERROR_CODE << std::endl;
        closesocket(listenSocket);
#ifdef _WIN32
        cleanupWinsock();
#endif
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "Listen failed: " << SOCKET_ERROR_CODE << std::endl;
        closesocket(listenSocket);
#ifdef _WIN32
        cleanupWinsock();
#endif
        return 1;
    }

    std::cout << "Server is listening on port 42069..." << std::endl;

    std::thread acceptThread(acceptClients, listenSocket);
    userInputHandler();

    acceptThread.join();

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (const auto& clientSocket : clients)
        {
            closesocket(clientSocket);
        }
        clients.clear();
    }

    closesocket(listenSocket);

#ifdef _WIN32
    cleanupWinsock();
#endif

    return 0;
}
