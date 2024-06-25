#include <iostream>
#include <atomic>
#include <thread>
#include <csignal>
#include "config.h"
#include "payloads.h"
#include "utils.h"
#include "connection_cs2console.h"
#include "connection_remoteserver.h"

std::atomic<bool> running(true);
std::thread sanityCheckThread;

void signalHandler(int signum)
{
    std::cout << "\n[Main] Interrupt signal (" << signum << ") received.\n";
    running = false;
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

void sendSanityCheck()
{
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        if (running)
        {
            sendPayloadToCS2Console(command_say_sanity_check_payload);
            std::cout << "[Main] Sent sanity check command to CS2 console" << '\n';
        }
    }
    std::cout << "[Main] Sanity check thread stopping..." << '\n';
}

void userInputHandler()
{
    while (running)
    {
        std::cout << ">> ";
        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) continue;

        if (input == "quit" || input == "exit" || input == "x")
        {
            std::cout << "[Main] Exit command received. Initiating shutdown..." << '\n';
            running = false;
            break;
        }

        if (input.substr(0, 3) == "cmd")
        {
            if (input.length() > 4)
            {
                std::string command = input.substr(4);
                auto payload = create_command_payload(command);
                sendPayloadToCS2Console(payload);
                std::cout << "[Main] Sent command to CS2 console: " << command << '\n';
            }
            else
            {
                std::cout << "[Main] Invalid command format. Use 'cmd <your_command>'" << '\n';
            }
        }
        else
        {
            switch (input[0])
            {
            case '0':
                sendPayloadToCS2Console(command_smooth_disable_payload);
                std::cout << "[Main] Sent smooth disable command" << '\n';
                break;
            case '1':
                sendPayloadToCS2Console(command_smooth_enable_payload);
                std::cout << "[Main] Sent smooth enable command" << '\n';
                break;
            case 'y':
                if (!listeningCS2)
                {
                    listeningCS2 = true;
                    cs2ListenerThread = std::thread(listenForCS2ConsoleData);
                    std::cout << "[Main] Started CS2 console output listening thread" << '\n';
                }
                else
                {
                    listeningCS2 = false;
                    if (cs2ListenerThread.joinable()) cs2ListenerThread.join();
                    std::cout << "[Main] Stopped CS2 console output listening thread" << '\n';
                }
                break;
            default:
                std::cout << "[Main] Unknown command" << '\n';
                break;
            }
        }
    }
}

void gracefulShutdown()
{
    std::cout << "[Main] Initiating graceful shutdown..." << '\n';

    running = false;

    if (sanityCheckThread.joinable())
    {
        std::cout << "[Main] Waiting for sanity check thread to finish..." << '\n';
        sanityCheckThread.join();
    }

    cleanupCS2Console();
    cleanupRemoteServer();

    WSACleanup();

    std::cout << "[Main] Graceful shutdown complete. Bye-bye!" << '\n';
}

int main()
{
    signal(SIGINT, signalHandler);

    if (!setupConfig() || !setupWinsock())
    {
        return 1;
    }
    if (!connectToCS2Console())
    {
        WSACleanup();
        return 1;
    }

    remoteServerConnectorThread = std::thread(remoteServerConnectorLoop);

    bool debug_sanity = Config::getInstance().getInt("debug_sanity", 0) == 1;
    if (debug_sanity)
    {
        std::cout << "[Main] Debug sanity check enabled!" << '\n';
        sanityCheckThread = std::thread(sendSanityCheck);
    }

    userInputHandler();

    gracefulShutdown();
    return 0;
}
