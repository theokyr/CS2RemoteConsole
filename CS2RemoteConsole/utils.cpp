#include "utils.h"
#include <iostream>

std::vector<unsigned char> create_payload(const std::vector<unsigned char>& bytes)
{
    return bytes;
}

int send_payload(SOCKET sock, const std::vector<unsigned char>& payload)
{
    if (send(sock, reinterpret_cast<const char*>(payload.data()), static_cast<int>(payload.size()), 0) == SOCKET_ERROR)
    {
        std::cerr << "Failed to send data" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    return 0;
}

std::vector<unsigned char> create_command_payload(const std::string& command)
{
    std::vector<unsigned char> payload = {'C', 'M', 'N', 'D', 0x00, 0xD4, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00};

    // Truncate the command if it's too long
    size_t max_command_length = 65535 - payload.size() - 1; // -1 for null terminator
    std::string truncated_command = command.substr(0, max_command_length);

    // Add the command string
    payload.insert(payload.end(), truncated_command.begin(), truncated_command.end());
    payload.push_back(0x00); // Null terminator

    // Calculate and set the correct length
    unsigned short total_length = static_cast<unsigned short>(payload.size());
    payload[8] = (total_length >> 8) & 0xFF; // High byte of length
    payload[9] = total_length & 0xFF; // Low byte of length

    return payload;
}
