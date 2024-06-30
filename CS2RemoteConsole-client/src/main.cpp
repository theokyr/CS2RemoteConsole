#include <iostream>
#include <atomic>
#include <thread>
#include <csignal>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "config.h"
#include "payloads.h"
#include "utils.h"
#include "singletons.h"
#include "connection_cs2console.h"
#include "connection_remoteserver.h"
#include "logging.h"
#include "tui.h"
#include "tui_sink.h"
#include <windows.h>

std::atomic<bool> applicationRunning(true);
TUI tui;

void signalHandler(int signum)
{
    auto logger = spdlog::get(LOGGER_APPLICATION);
    logger->info("[Main] Interrupt signal {} received.", signum);
    applicationRunning = false;
}

void gracefulShutdown()
{
    auto logger = spdlog::get(LOGGER_APPLICATION);
    logger->info("Initiating graceful shutdown...");

    applicationRunning = false;

    tui.shutdown();

    cleanupCS2Console();
    cleanupRemoteServer();

    WSACleanup();

    std::cout << "CS2RemoteConsole shutdown complete. Bye-bye!..." << std::endl;
}

void setupLogging()
{
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/application.log");
    auto tui_sink = std::make_shared<tui_sink_mt>(tui);
    
    auto default_logger = std::make_shared<spdlog::logger>("default", file_sink);
    spdlog::set_level(spdlog::level::debug);
    default_logger->sinks().push_back(tui_sink);
    spdlog::set_default_logger(default_logger);
    
    auto application_logger = std::make_shared<spdlog::logger>(LOGGER_APPLICATION, tui_sink);
    auto vconsole_logger = std::make_shared<spdlog::logger>(LOGGER_VCON, tui_sink);
    auto remote_server_logger = std::make_shared<spdlog::logger>(LOGGER_REMOTE_SERVER, tui_sink);
    
    spdlog::register_logger(application_logger);
    spdlog::register_logger(vconsole_logger);
    spdlog::register_logger(remote_server_logger);
}

int main()
{
    try
    {

        if (!setupConfig() || !setupApplicationWinsock())
        {
            return 1;
        }
        
        tui.init();
        
        setupLogging();
        
        auto logger = spdlog::get(LOGGER_APPLICATION);
        logger->info("Starting CS2RemoteConsole application");
        
        signal(SIGINT, signalHandler);

        tui.setCommandCallback([](const std::string& command)
        {
            sendPayloadToCS2Console(command);
        });

        auto& vconsole = VConsoleSingleton::getInstance();
        vconsole.setOnCHANReceived([&](const CHAN& CHAN)
        {
            for (const auto& channel : CHAN.channels)
            {
                tui.registerChannel(channel.id, channel.name, channel.text_RGBA_override);
            }
        });

        vconsole.setOnPRNTReceived([&](const PRNT& PRNT)
        {
            tui.addConsoleMessage(PRNT.channelID, PRNT.message);
        });

        cs2ConnectorThread = std::thread(cs2ConsoleConnectorLoop);
        remoteServerConnectorThread = std::thread(remoteServerConnectorLoop);

        tui.run();
        
        gracefulShutdown();
    }
    catch (const std::exception& e)
    {
        spdlog::error("Unhandled exception: {}", e.what());
        cleanupCS2Console();
        cleanupRemoteServer();
        WSACleanup();
        return 1;
    }
    catch (...)
    {
        spdlog::error("Unhandled unknown exception.");
        cleanupCS2Console();
        cleanupRemoteServer();
        WSACleanup();
        return 1;
    }
    return 0;
}