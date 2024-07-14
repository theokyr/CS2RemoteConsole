#include "server.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>

Server::Server(uint16_t initialPort, std::atomic<bool>& running)
    : m_initialPort(initialPort), m_port(initialPort), m_listenSocket(INVALID_SOCKET), m_running(running)
{
}

Server::~Server()
{
    cleanupSockets();
}
std::string trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) { return std::isspace(ch); });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
    return (start < end ? std::string(start, end) : std::string());
}
bool update_user_in_file(const std::string& filename, const ClientInfo& client) {
    std::vector<std::string> lines;
    std::ifstream infile(filename);
    std::string line;
    bool user_found = false;

    while (std::getline(infile, line)) {
        lines.push_back(line);
        if (trim(line) == client.name && lines.size() % 4 == 1) {
            user_found = true;
            lines.push_back(std::to_string(client.pingToGameServer));
            lines.push_back(std::to_string(client.totalLatency));
            lines.push_back(std::to_string(client.netBufferTicks));
            // Skip the next 3 lines in the input file
            std::getline(infile, line);
            std::getline(infile, line);
            std::getline(infile, line);
            std::cout << "User " << client.name << " found. Updating values." << std::endl;
        }
    }
    infile.close();

    if (!user_found) {
        std::cout << "User " << client.name << " not found. Adding to the log file." << std::endl;
        lines.push_back(client.name);
        lines.push_back(std::to_string(client.pingToGameServer));
        lines.push_back(std::to_string(client.totalLatency));
        lines.push_back(std::to_string(client.netBufferTicks));
    }

    std::ofstream outfile(filename);
    for (const auto& output_line : lines) {
        outfile << output_line << "\n";
    }
    outfile.close();

    return true;
}


bool Server::start()
{
    const int MAX_PORT = 65535; // Maximum valid port number

    while (m_port <= MAX_PORT)
    {
        std::cout << "Attempting to bind to port " << m_port << "..." << std::endl;

        m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_listenSocket == INVALID_SOCKET)
        {
            std::cerr << "Error creating socket: " << SOCKET_ERROR_CODE << std::endl;
            return false;
        }

        int opt = 1;
        if (setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0)
        {
            std::cerr << "setsockopt(SO_REUSEADDR) failed: " << SOCKET_ERROR_CODE << std::endl;
            cleanupSockets();
            return false;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(m_port);

        if (bind(m_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        {
            std::cerr << "Bind failed on port " << m_port << ": " << SOCKET_ERROR_CODE << std::endl;

            cleanupSockets();

            // Try the next port
            m_port++;

            if (m_port > MAX_PORT)
            {
                std::cerr << "Exhausted all available ports. Failed to start server." << std::endl;
                return false;
            }

            continue;
        }

        // If we reach here, bind was successful
        break;
    }

    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "Listen failed: " << SOCKET_ERROR_CODE << std::endl;
        cleanupSockets();
        return false;
    }

    setNonBlocking(m_listenSocket);

    std::cout << "Server is listening on port " << m_port << std::endl;
    if (m_port != m_initialPort)
    {
        std::cout << "Note: The server is using a different port than initially specified." << std::endl;
    }
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
    char buffer[8096];
    while (m_running)
    {
        int bytesReceived = recv(client.socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0)
        {
            //buffer[bytesReceived] = '\0'; //since I am a war criminal now and send a null terminator, we won't need this.

            std::vector<std::string> commands;
            for (const char* p = buffer; p != buffer + bytesReceived; p += commands.back().size() + 1)
                commands.push_back(p);


            for(std::string message : commands)
                handleClientMessage(client, message);
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
        if (client.name != content)
        {
            client.name = content;
            std::cout << "Updated player name for client "
                << ANSI_COLOR_IP_PORT << client.ip << ":" << client.port << ANSI_COLOR_RESET
                << " to: "
                << ANSI_COLOR_USERNAME << client.name << ANSI_COLOR_RESET << std::endl;

            std::string broadcastMessage = "Player " + client.name + " has joined the game.";
            broadcastToClients(broadcastMessage, &client);
        }
    }
    else if (command == "GAMEIP")
    {
        if (client.gameServerIp != content)
        {
            client.gameServerIp = content;
            std::cout << "Updated game ip for client "
                << ANSI_COLOR_IP_PORT << client.ip << ":" << client.port << ANSI_COLOR_RESET
                << " to: "
                << ANSI_COLOR_GAME_IP << client.gameServerIp << ANSI_COLOR_RESET << std::endl;
        } 
    }
    else if (command == "GAMEPING")
    {
        if (client.pingToGameServer != std::stoi(content))
        {
            client.pingToGameServer = std::stoi(content);
            std::cout << "Updated game ping for client "
                << ANSI_COLOR_IP_PORT << client.ip << ":" << client.port << ANSI_COLOR_RESET
                << " to: "
                << ANSI_COLOR_GAME_PING << client.pingToGameServer << ANSI_COLOR_RESET << std::endl;
        }
    }
    else if (command == "TOTALLATENCY")
    {
        if (client.totalLatency != std::stoi(content))
        {
            client.totalLatency = std::stoi(content);
            std::cout << "Updated total latency for client "
                << ANSI_COLOR_IP_PORT << client.ip << ":" << client.port << ANSI_COLOR_RESET
                << " to: "
                << ANSI_COLOR_TOTAL_LATENCY << client.totalLatency << ANSI_COLOR_RESET << std::endl;
        }
        
    }
    else if (command == "NETBUFFER")
    {
        if (client.netBufferTicks != std::stoi(content))
        {
            client.netBufferTicks = std::stoi(content);
            std::cout << "Updated net buffer size for client "
                << ANSI_COLOR_IP_PORT << client.ip << ":" << client.port << ANSI_COLOR_RESET
                << " to: "
                << ANSI_COLOR_NETBUFFER << client.netBufferTicks << ANSI_COLOR_RESET << std::endl;
        }
    }
    else if (command == "SMOOTH")
    {
        if (client.smooth != content)
        {
            client.smooth = content;
            std::cout << "Updated smoothing setting for client "
                << ANSI_COLOR_IP_PORT << client.ip << ":" << client.port << ANSI_COLOR_RESET
                << " to: "
                << ANSI_COLOR_SMOOTH << client.smooth << ANSI_COLOR_RESET << std::endl;
        }
    }
    else
    {
        std::cout << "Received from client "
            << ANSI_COLOR_IP_PORT << client.ip << ":" << client.port << ANSI_COLOR_RESET
            << ": " << message << std::endl;
        // Handle other message types here
    }

    //this is really stupid and would always write...whatever.
    if (client.pingToGameServer != 0)
    {
        if (update_user_in_file("playerData.txt", client)) {
            std::cout << "Updated or added information for " << client.name << std::endl;
        }
        else {
            std::cerr << "Failed to update or add information for " << client.name << std::endl;
        }
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
            std::cout << ANSI_COLOR_BACKGROUND_HEADER << "Connected clients:" << std::endl;

            std::cout << ANSI_COLOR_COMMON_HEADER << std::left << std::setw(5) << "Index" << ANSI_COLOR_RESET
                << ANSI_COLOR_BACKGROUND_HEADER << "| "
                << ANSI_COLOR_BACKGROUND_HEADER << ANSI_COLOR_COMMON_HEADER << std::left << std::setw(10) << "Name" << ANSI_COLOR_RESET
                << ANSI_COLOR_BACKGROUND_HEADER << "| "
                << ANSI_COLOR_BACKGROUND_HEADER << ANSI_COLOR_COMMON_HEADER << std::left << std::setw(20) << "Client ip:port" << ANSI_COLOR_RESET
                << ANSI_COLOR_BACKGROUND_HEADER << "| "
                << ANSI_COLOR_BACKGROUND_HEADER << ANSI_COLOR_COMMON_HEADER << std::left << std::setw(20) << "Game server ip:port" << ANSI_COLOR_RESET
                << ANSI_COLOR_BACKGROUND_HEADER << "| "
                << ANSI_COLOR_BACKGROUND_HEADER << ANSI_COLOR_COMMON_HEADER << std::left << std::setw(10) << "Game ping" << ANSI_COLOR_RESET
                << ANSI_COLOR_BACKGROUND_HEADER << "| "
                << ANSI_COLOR_BACKGROUND_HEADER << ANSI_COLOR_COMMON_HEADER << std::left << std::setw(14) << "Total latency" << ANSI_COLOR_RESET
                << ANSI_COLOR_BACKGROUND_HEADER << "| "
                << ANSI_COLOR_BACKGROUND_HEADER << ANSI_COLOR_COMMON_HEADER << std::left << std::setw(15) << "Net buffer size" << ANSI_COLOR_RESET
                << ANSI_COLOR_BACKGROUND_HEADER << "| "
                << ANSI_COLOR_BACKGROUND_HEADER << ANSI_COLOR_COMMON_HEADER << std::left << std::setw(16) << "Error smoothing" << ANSI_COLOR_RESET
                << ANSI_COLOR_BACKGROUND_HEADER << "| "
                << ANSI_COLOR_BACKGROUND_HEADER << ANSI_COLOR_COMMON_HEADER << std::left << std::setw(20) << "Time" << ANSI_COLOR_RESET << "|"
                << "\n";


            int index = 0;
            for (const auto& client : m_clients)
            {
                std::string indexStr = "[" + std::to_string(index++) + "]";
                std::string clientIpPort = client.ip + ":" + std::to_string(client.port);

                std::cout << ANSI_COLOR_ID << std::left << std::setw(5) << indexStr << ANSI_COLOR_RESET << "| "
                    << ANSI_COLOR_USERNAME << std::left << std::setw(10) << (client.name.empty() ? "Unknown" : client.name) << ANSI_COLOR_RESET
                    << "| "
                    << ANSI_COLOR_IP_PORT << std::left << std::setw(20) << clientIpPort << ANSI_COLOR_RESET
                    << "| "
                    << ANSI_COLOR_GAME_IP << std::left << std::setw(20) << client.gameServerIp << ANSI_COLOR_RESET
                    << "| "
                    << ANSI_COLOR_GAME_PING << std::left << std::setw(10) << client.pingToGameServer << ANSI_COLOR_RESET
                    << "| "
                    << ANSI_COLOR_TOTAL_LATENCY << std::left << std::setw(14) << client.totalLatency << ANSI_COLOR_RESET
                    << "| "
                    << ANSI_COLOR_NETBUFFER << std::left << std::setw(15) << client.netBufferTicks << ANSI_COLOR_RESET
                    << "| "
                    << ANSI_COLOR_SMOOTH << std::left << std::setw(16) << client.smooth << ANSI_COLOR_RESET
                    << "| "
                    << ANSI_COLOR_TIMESTAMP << std::left << std::setw(20) << getFormattedTime(client.connectionTime) << ANSI_COLOR_RESET
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
