#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include <chrono>
#include "payloads.h"

#pragma comment(lib, "ws2_32.lib")

SOCKET sock;
std::atomic<bool> running(true);
std::atomic<bool> listening(false);
std::thread listenerThread;
std::thread sanityCheckThread;

void listenForData();
void userInputHandler();
void sendSanityCheck();
int sendPayload(const std::vector<unsigned char>& payload);
bool connectToServer(const char* ip, int port);

int main()
{
    const char* ip = "127.0.0.1";
    const int port = 29000;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    if (!connectToServer(ip, port))
    {
        WSACleanup();
        return 1;
    }

    sanityCheckThread = std::thread(sendSanityCheck);
    userInputHandler();

    running = false;
    if (sanityCheckThread.joinable()) sanityCheckThread.join();
    if (listenerThread.joinable()) listenerThread.join();
    closesocket(sock);
    WSACleanup();
    return 0;
}

bool connectToServer(const char* ip, int port)
{
    while (true)
    {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET)
        {
            std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
            return false;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &server_addr.sin_addr);

        if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            std::cerr << "Connection failed. Retrying in 5 seconds..." << std::endl;
            closesocket(sock);
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        else
        {
            std::cout << "Connected to server on port " << port << std::endl;
            return true;
        }
    }
}

int sendPayload(const std::vector<unsigned char>& payload)
{
    if (send(sock, reinterpret_cast<const char*>(payload.data()), static_cast<int>(payload.size()), 0) == SOCKET_ERROR)
    {
        std::cerr << "Failed to send data" << std::endl;
        return 1;
    }
    return 0;
}

void listenForData()
{
    char buffer[1024];
    int bytesReceived;
    u_long mode = 1; // Set non-blocking mode
    ioctlsocket(sock, FIONBIO, &mode);

    while (listening)
    {
        bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            std::cout << "\nReceived: " << buffer << std::endl << ">> ";
        }
        else if (bytesReceived == 0)
        {
            std::cout << "\nConnection closed by server" << std::endl;
            listening = false;
            running = false;
        }
        else if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
            listening = false;
            running = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void userInputHandler()
{
    while (running)
    {
        std::cout << ">> ";
        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) continue;

        if (input.substr(0, 3) == "cmd")
        {
            if (input.length() > 4)
            {
                std::string command = input.substr(4);
                auto payload = create_command_payload(command);
                sendPayload(payload);
                std::cout << "Sent command: " << command << std::endl;
            }
            else
            {
                std::cout << "Invalid command format. Use 'cmd <your_command>'" << std::endl;
            }
        }
        else
        {
            switch (input[0])
            {
            case '0':
                sendPayload(command_smooth_disable_payload);
                std::cout << "Sent smooth disable command" << std::endl;
                break;
            case '1':
                sendPayload(command_smooth_enable_payload);
                std::cout << "Sent smooth enable command" << std::endl;
                break;
            case 'y':
                if (!listening)
                {
                    listening = true;
                    listenerThread = std::thread(listenForData);
                    std::cout << "Started console output listening thread" << std::endl;
                }
                else
                {
                    listening = false;
                    if (listenerThread.joinable()) listenerThread.join();
                    std::cout << "Stopped console output listening thread" << std::endl;
                }
                break;
            case 'x':
                running = false;
                listening = false;
                return;
            default:
                std::cout << "Unknown command" << std::endl;
                break;
            }
        }
    }
}

void sendSanityCheck()
{
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        if (running)
        {
            sendPayload(command_say_sanity_check_payload);
            std::cout << "Sent sanity check command" << std::endl;
        }
    }
}
