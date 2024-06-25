#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include <chrono>
#include "payloads.h"
#include "utils.h"

#pragma comment(lib, "ws2_32.lib")

SOCKET sock;
std::atomic<bool> running(true);
std::atomic<bool> listening(false);
std::thread listenerThread;
std::thread sanityCheckThread;

void listenForData();
void userInputHandler();
void sendSanityCheck();

int main()
{
    const int dest_port = 29000;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    while (true)
    {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET)
        {
            std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return 1;
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(dest_port);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

        if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            std::cerr << "Connection failed with error: " << WSAGetLastError() << ". Retrying in 5 seconds..." << std::endl;
            closesocket(sock);
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }
        else
        {
            std::cout << "Connected to server on port " << dest_port << std::endl;
            break;
        }
    }

    // Start the sanity check thread
    sanityCheckThread = std::thread(sendSanityCheck);

    // Start the user input handler thread
    std::thread inputThread(userInputHandler);
    inputThread.join();

    // Clean up
    running = false;
    if (sanityCheckThread.joinable())
    {
        sanityCheckThread.join();
    }
    if (listenerThread.joinable())
    {
        listenerThread.join();
    }
    closesocket(sock);
    WSACleanup();
    return 0;
}

void listenForData()
{
    char buffer[1024];
    int bytesReceived;
    while (listening)
    {
        bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0'; // Null-terminate the received data
            std::cout << "\nReceived: " << buffer << std::endl << ">> ";
        }
        else if (bytesReceived == 0)
        {
            std::cout << "\nConnection closed by server" << std::endl;
            listening = false;
            running = false;
        }
        else
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                // Ignore non-blocking mode error
                std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
                listening = false;
                running = false;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Small sleep to avoid busy-waiting
    }
}

void userInputHandler()
{
    while (running)
    {
        std::cout << ">> ";
        char input;
        std::cin >> input;

        switch (input)
        {
        case '0':
            send_payload(sock, command_smooth_disable_payload);
            std::cout << "Sent smooth disable command" << std::endl;
            break;
        case '1':
            send_payload(sock, command_smooth_enable_payload);
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
                if (listenerThread.joinable())
                {
                    listenerThread.join();
                }
                std::cout << "Stopped console output listening thread" << std::endl;
            }
            break;
        case 'x':
            running = false;
            listening = false;
            if (listenerThread.joinable())
            {
                listenerThread.join();
            }
            std::cout << "Exiting program..." << std::endl;
            return;
        default:
            std::cout << "Unknown command" << std::endl;
            break;
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
            // Check again to avoid sending after program is supposed to stop
            send_payload(sock, command_say_sanity_check_payload);
            std::cout << "Sent sanity check command" << std::endl;
        }
    }
}
