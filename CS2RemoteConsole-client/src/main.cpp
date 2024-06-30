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

void tuiThread()
{
    tui.init();
    tui.run();
}

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

    std::cout << "CS2RemoteConsole shutdown complete. Bye - bye!..." << '\n';
}

void setupLogging()
{
    spdlog::set_level(spdlog::level::debug);

    auto tui_sink = std::make_shared<tui_sink_mt>(tui);

    std::vector<spdlog::sink_ptr> applicationSinks;
    applicationSinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/application.log"));
    applicationSinks.push_back(tui_sink);
    auto applicationLogger = std::make_shared<spdlog::logger>(LOGGER_APPLICATION, begin(applicationSinks), end(applicationSinks));
    register_logger(applicationLogger);

    std::vector<spdlog::sink_ptr> protocolSinks;
    protocolSinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/protocol.log"));
    protocolSinks.push_back(tui_sink);
    auto protocolLogger = std::make_shared<spdlog::logger>(LOGGER_VCON, begin(protocolSinks), end(protocolSinks));
    register_logger(protocolLogger);

    std::vector<spdlog::sink_ptr> remoteServerSinks;
    remoteServerSinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/remote.log"));
    remoteServerSinks.push_back(tui_sink);
    auto remoteServerLogger = std::make_shared<spdlog::logger>(LOGGER_REMOTE_SERVER, begin(remoteServerSinks), end(remoteServerSinks));
    register_logger(remoteServerLogger);
}

int main()
{
    try
    {
        setupLogging();
        std::thread uiThread(tuiThread);

        Sleep(1000);

        
        signal(SIGINT, signalHandler);

        if (!setupConfig() || !setupApplicationWinsock())
        {
            return 1;
        }

        tui.setCommandCallback([](const std::string& command)
        {
            sendPayloadToCS2Console(command);
        });

        cs2ConnectorThread = std::thread(cs2ConsoleConnectorLoop);
        remoteServerConnectorThread = std::thread(remoteServerConnectorLoop);

        auto& vconsole = VConsoleSingleton::getInstance();
        vconsole.setOnCHANReceived([](const CHAN& CHAN)
        {
            for (const auto& channel : CHAN.channels)
            {
                tui.registerChannel(channel.id, channel.name, channel.text_RGBA_override);
            }
        });

        vconsole.setOnPRNTReceived([](const PRNT& PRNT)
        {
            tui.addConsoleMessage(PRNT.channelID, PRNT.message);
        });

        while (applicationRunning)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

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
