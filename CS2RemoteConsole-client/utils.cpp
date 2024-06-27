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