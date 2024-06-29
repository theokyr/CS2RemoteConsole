#include <iostream>
#include <atomic>
#include <thread>
#include <csignal>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "config.h"
#include "payloads.h"
#include "utils.h"
#include "connection_cs2console.h"
#include "connection_remoteserver.h"
#include "logging.h"

std::atomic<bool> applicationRunning(true);

void signalHandler(int signum)
{
    auto logger = spdlog::get(LOGGER_APPLICATION);
    logger->info("[Main] Interrupt signal {} received.", signum);
    applicationRunning = false;
}

void userInputHandler()
{
    auto logger = spdlog::get(LOGGER_APPLICATION);
    while (applicationRunning)
    {
        // std::cout << ">> ";
        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) continue;

        if (input == "quit" || input == "exit" || input == "x")
        {
            logger->info("[Main] Exit command received. Initiating shutdown...");
            applicationRunning = false;
            break;
        }

        if (input.substr(0, 3) == "cmd")
        {
            if (input.length() > 4)
            {
                std::string command = input.substr(4);
                sendPayloadToCS2Console(command);
                logger->info("[Main] Sending command {} to CS2 Console...", command);
            }
            else
            {
                logger->error("[Main] Invalid command format. Use 'cmd <your_command>'");
            }
        }
        else
        {
            switch (input[0])
            {
            case 'y':
                if (!listeningCS2)
                {
                    listeningCS2 = true;
                    cs2ListenerThread = std::thread(listenForCS2ConsoleData);
                    logger->info("[Main] Started CS2 console output listening thread");
                }
                else
                {
                    listeningCS2 = false;
                    if (cs2ListenerThread.joinable()) cs2ListenerThread.join();
                    logger->info("[Main] Stopped CS2 console output listening thread");
                }
                break;
            default:
                logger->warn("Unknown command");
                break;
            }
        }
    }
}

void gracefulShutdown()
{
    auto logger = spdlog::get(LOGGER_APPLICATION);
    logger->info("Initiating graceful shutdown...");

    applicationRunning = false;

    cleanupCS2Console();
    cleanupRemoteServer();

    WSACleanup();

    std::cout << "CS2RemoteConsole shutdown complete. Bye - bye!..." << '\n';
}

void setupLogging()
{
    spdlog::set_level(spdlog::level::debug);

    std::vector<spdlog::sink_ptr> applicationSinks;
    applicationSinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_st>());
    applicationSinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/application.log"));
    auto applicationLogger = std::make_shared<spdlog::logger>(LOGGER_APPLICATION, begin(applicationSinks), end(applicationSinks));
    register_logger(applicationLogger);

    std::vector<spdlog::sink_ptr> protocolSinks;
    protocolSinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_st>());
    protocolSinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/protocol.log"));
    auto protocolLogger = std::make_shared<spdlog::logger>(LOGGER_VCON, begin(protocolSinks), end(protocolSinks));
    register_logger(protocolLogger);

    std::vector<spdlog::sink_ptr> remoteServerSinks;
    remoteServerSinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_st>());
    remoteServerSinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/remote.log"));
    auto remoteServerLogger = std::make_shared<spdlog::logger>(LOGGER_REMOTE_SERVER, begin(remoteServerSinks), end(remoteServerSinks));
    register_logger(remoteServerLogger);
}

int main()
{
    setupLogging();
    signal(SIGINT, signalHandler);

    if (!setupConfig() || !setupApplicationWinsock())
    {
        return 1;
    }

    cs2ConnectorThread = std::thread(cs2ConsoleConnectorLoop);
    remoteServerConnectorThread = std::thread(remoteServerConnectorLoop);

    userInputHandler();

    gracefulShutdown();
    return 0;
}
