#include <iostream>
#include <atomic>
#include <thread>
#include <csignal>
#include "config.h"
#include "payloads.h"
#include "utils.h"
#include "connection_cs2console.h"
#include "connection_remoteserver.h"
#pragma once

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

std::string getColorCode(uint32_t color)
{
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    uint8_t a = (color >> 24) & 0xFF;

    // If alpha is 0, return an empty string (no color change)
    if (a == 0)
    {
        return "";
    }

    // Convert RGB to ANSI color code
    std::stringstream ss;
    ss << "\033[38;2;" << static_cast<int>(r) << ";" << static_cast<int>(g) << ";" << static_cast<int>(b) << "m";
    return ss.str();
}

std::string getCategoryName(uint32_t unknown2)
{
    switch (unknown2)
    {
    case 0x04B07200: return "VConComm";
    case 0x02D6CB00: return "General";
    case 0x01B63100: return "Client";
    default:
        {
            std::stringstream ss;
            ss << "Unknown-0x" << std::setfill('0') << std::setw(8) << std::hex << unknown2;
            return ss.str();
        }
    }
}

uint32_t byteSwap32(uint32_t value)
{
    return ((value & 0xFF000000) >> 24) |
        ((value & 0x00FF0000) >> 8) |
        ((value & 0x0000FF00) << 8) |
        ((value & 0x000000FF) << 24);
}

uint16_t byteSwap16(uint16_t value)
{
    return (value >> 8) | (value << 8);
}

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