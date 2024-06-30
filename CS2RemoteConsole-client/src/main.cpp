#include <iostream>
#include <atomic>
#include <thread>
#include <csignal>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/callback_sink.h>

#include "config.h"
#include "payloads.h"
#include "utils.h"
#include "singletons.h"
#include "connection_cs2console.h"
#include "connection_remoteserver.h"
#include "logging.h"
#include "tui.h"
#include <windows.h>

std::atomic<bool> applicationRunning(true);
TUI tui;

void signalHandler(int signum)
{
    spdlog::info("[Main] Interrupt signal {} received.", signum);
    applicationRunning = false;
}

void gracefulShutdown()
{
    spdlog::info("Initiating graceful shutdown...");

    applicationRunning = false;

    tui.shutdown();

    cleanupCS2Console();
    cleanupRemoteServer();

    WSACleanup();

    std::cout << "CS2RemoteConsole shutdown complete. Bye-bye!..." << std::endl;
}

int main()
{
    try
    {
        spdlog::set_level(spdlog::level::debug);
        std::vector<spdlog::sink_ptr> applicationSinks;
        applicationSinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/application.log"));
        auto applicationLogger = std::make_shared<spdlog::logger>(LOGGER_APPLICATION, begin(applicationSinks), end(applicationSinks));
        set_default_logger(applicationLogger);

        if (!setupConfig() || !setupApplicationWinsock())
        {
            return 1;
        }

        tui.init();
        tui.registerChannel(APPLICATION_SPECIAL_CHANNEL_ID, "Log", 4285057279);
        tui.setupLoggerCallbackSink();

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

        spdlog::info("Starting CS2RemoteConsole application");

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
