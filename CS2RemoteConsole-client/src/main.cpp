#include <iostream>
#include <atomic>
#include <thread>
#include <csignal>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/callback_sink.h>

#include "config.h"
#include "constants.h"
#include "utils.h"
#include "singletons.h"
#include "connection/connection_cs2console.h"
#include "connection/connection_remoteserver.h"
#include "tui/tui.h"
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
        std::stringstream ss;
        ss << application_name << ".log";
        applicationSinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(ss.str()));
        auto applicationLogger = std::make_shared<spdlog::logger>("CS2RemoteConsole-Client", begin(applicationSinks), end(applicationSinks));
        spdlog::set_default_logger(applicationLogger);

        if (!setupConfig() || !setupApplicationWinsock())
        {
            return 1;
        }

        tui.init();
        tui.registerChannel(APPLICATION_SPECIAL_CHANNEL_ID, "Log", 0xD0D0D0FF, 0x2D0034FF); //
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

        spdlog::info("[Main] Starting {}", application_name);

        cs2ConnectorThread = std::thread(cs2ConsoleConnectorLoop);
        remoteServerConnectorThread = std::thread(remoteServerConnectorLoop);

        tui.run();

        gracefulShutdown();
    }
    catch (const std::exception& e)
    {
        spdlog::error("[Main] Unhandled exception: {}", e.what());
        cleanupCS2Console();
        cleanupRemoteServer();
        WSACleanup();
        return 1;
    }
    catch (...)
    {
        spdlog::error("[Main] Unhandled unknown exception.");
        cleanupCS2Console();
        cleanupRemoteServer();
        WSACleanup();
        return 1;
    }
    return 0;
}
