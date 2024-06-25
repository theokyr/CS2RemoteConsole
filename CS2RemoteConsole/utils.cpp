#include "utils.h"
#include <iostream>

std::vector<unsigned char> create_payload(const std::vector<unsigned char>& bytes) {
    return bytes;
}

int send_payload(SOCKET sock, const std::vector<unsigned char>& payload) {
    if (send(sock, reinterpret_cast<const char*>(payload.data()), static_cast<int>(payload.size()), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send data" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    return 0;
}
