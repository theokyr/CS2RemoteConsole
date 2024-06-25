#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include "payloads.h"
#include "utils.h"

#pragma comment(lib, "ws2_32.lib")

SOCKET sock;
std::atomic<bool> running(true);

void listenForData();

int main() {
    const int dest_port = 29000;

    Sleep(5000);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(dest_port);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed with error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Get the assigned local port
    sockaddr_in local_addr;
    int local_addr_len = sizeof(local_addr);
    if (getsockname(sock, (sockaddr*)&local_addr, &local_addr_len) == SOCKET_ERROR) {
        std::cerr << "Failed to get local port" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    int source_port = ntohs(local_addr.sin_port);

    // Start the listener thread
    std::thread listenerThread(listenForData);

    // Send payloads
    if (!send_payload(sock, init_payload))
        std::cout << "Init sent successfully to port " << dest_port << std::endl;
    Sleep(100);
    if (!send_payload(sock, cmd_payload))
        std::cout << "Command sent successfully to port " << dest_port << std::endl;

    // Signal the listener thread to stop and join it
    std::cout << "Press Enter to stop listening..." << std::endl;
    std::cin.get();
    running = false;
    listenerThread.join();

    closesocket(sock);
    WSACleanup();
    return 0;
}

void listenForData() {
    char buffer[1024];
    int bytesReceived;
    while (running) {
        bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0'; // Null-terminate the received data
            std::cout << "Received: " << buffer << std::endl;
        } else if (bytesReceived == 0) {
            std::cout << "Connection closed" << std::endl;
            running = false;
        } else {
            if (WSAGetLastError() != WSAEWOULDBLOCK) { // Ignore non-blocking mode error
                std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
                running = false;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Small sleep to avoid busy-waiting
    }
}
