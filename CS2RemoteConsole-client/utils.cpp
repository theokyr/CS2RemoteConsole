#include "utils.h"

std::vector<unsigned char> create_command_payload(const std::string& command)
{
    std::vector<unsigned char> payload = {'C', 'M', 'N', 'D', 0x00, 0xD4, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00};

    size_t max_command_length = 65535 - payload.size() - 1;
    std::string truncated_command = command.substr(0, max_command_length);

    payload.insert(payload.end(), truncated_command.begin(), truncated_command.end());
    payload.push_back(0x00);

    unsigned short total_length = static_cast<unsigned short>(payload.size());
    payload[8] = (total_length >> 8) & 0xFF;
    payload[9] = total_length & 0xFF;

    return payload;
}
