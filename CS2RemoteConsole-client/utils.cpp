#include <iostream>
#include <atomic>
#include <thread>
#include <csignal>
#include "config.h"
#include "payloads.h"
#include "utils.h"
#include "connection_cs2console.h"
#include "connection_remoteserver.h"

void hexDump(const char* desc, const void* addr, int len)
{
    int i;
    unsigned char buff[17];
    const unsigned char* pc = (const unsigned char*)addr;

    if (desc != nullptr)
        std::cout << desc << ":\n";

    for (i = 0; i < len; i++)
    {
        if ((i % 16) == 0)
        {
            if (i != 0)
                std::cout << "  " << buff << "\n";
            std::cout << "  " << std::setw(4) << std::setfill('0') << std::hex << i << " ";
        }

        std::cout << " " << std::setw(2) << std::setfill('0') << std::hex << (int)pc[i];

        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    while ((i % 16) != 0)
    {
        std::cout << "   ";
        i++;
    }

    std::cout << "  " << buff << "\n";
}

uint32_t byteSwap32(uint32_t val)
{
    return ((val & 0xFF000000) >> 24) |
        ((val & 0x00FF0000) >> 8) |
        ((val & 0x0000FF00) << 8) |
        ((val & 0x000000FF) << 24);
}

uint16_t byteSwap16(uint16_t val)
{
    return (val >> 8) | (val << 8);
}

// Color codes and category mappings
// std::string getColorCode(uint32_t color)
// {
//     uint8_t r = color & 0xFF;
//     uint8_t g = (color >> 8) & 0xFF;
//     uint8_t b = (color >> 16) & 0xFF;
//     uint8_t a = (color >> 24) & 0xFF;
//
//     if (a == 0)
//     {
//         return "\033[0m"; // Reset to default color
//     }
//
//     // Special color handling
//     if (color == 0xFFFF00FF)
//     {
//         return "\033[1;33m"; // Bright yellow for warnings
//     }
//     else if (color == 0xFF0000FF)
//     {
//         return "\033[1;31m"; // Bright red for errors
//     }
//     else if (color == 0xFF00FFFF)
//     {
//         return "\033[1;35m"; // Bright magenta for special messages
//     }
//
//     // Default to RGB color
//     return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
// }
//
// std::string getCategoryName(uint16_t category, uint32_t channelID)
// {
//     switch (category)
//     {
//     case 1: return "EngineServiceManager";
//     case 2: return "General";
//     case 3: return "Developer";
//     case 4: return "Console";
//     default:
//         switch (channelID)
//         {
//         case 0x0059410C: return "VConComm";
//         case 0x0059410B: return "Host";
//         case 0x00594107: return "GameInput";
//         case 0x00594104: return "MainMenu";
//         case 0x00594102: return "GameUI";
//         case 0x00593FF3: return "EngineService";
//         default:
//             std::stringstream ss;
//             ss << "Unknown-" << std::hex << category << "-" << channelID;
//             return ss.str();
//         }
//     }
// }

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

// std::vector<unsigned char> create_command_payload(const std::string& command)
// {
//     CMNDMessage msg = create_command_message(command);
//     std::vector<unsigned char> payload(msg.messageSize);
//
//     size_t offset = 0;
//     memcpy(payload.data() + offset, &msg.magic, sizeof(msg.magic));
//     offset += sizeof(msg.magic);
//     memcpy(payload.data() + offset, &msg.commandType, sizeof(msg.commandType));
//     offset += sizeof(msg.commandType);
//     memcpy(payload.data() + offset, &msg.messageSize, sizeof(msg.messageSize));
//     offset += sizeof(msg.messageSize);
//     memcpy(payload.data() + offset, &msg.unknown, sizeof(msg.unknown));
//     offset += sizeof(msg.unknown);
//     memcpy(payload.data() + offset, msg.command.c_str(), msg.command.length() + 1);
//
//     return payload;
// }

std::string getCurrentDirectory()
{
    char buffer[FILENAME_MAX];
    if (_getcwd(buffer, FILENAME_MAX) != nullptr)
    {
        return std::string(buffer);
    }
    return "";
}

bool setupConfig()
{
    std::vector<std::string> config_paths = {
        "config.ini",
        getCurrentDirectory() + "\\config.ini"
    };

    for (const auto& path : config_paths)
    {
        try
        {
            std::cout << "[Main] Attempting to load config from: " << path << '\n';
            Config::getInstance().load(path);
            std::cout << "[Main] Config loaded successfully from: " << path << '\n';
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[Main] Failed to load config from " << path << ": " << e.what() << '\n';
        }
    }

    std::cerr << "[Main] Failed to load config from any location." << '\n';
    return false;
}

bool setupWinsock()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "[Main] WSAStartup failed" << '\n';
        return false;
    }
    return true;
}
