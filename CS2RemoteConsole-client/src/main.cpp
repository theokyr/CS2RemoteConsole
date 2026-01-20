#include <iostream>
#include <atomic>
#include <thread>
#include <csignal>
#include <regex>
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
#include "../../common/platform.h"

std::atomic<bool> applicationRunning(true);
TUI tui(applicationRunning);

void signalHandler(int signum)
{
    spdlog::info("[Main] Interrupt signal {} received.", signum);
    applicationRunning = false;
    running = false;  // Stop connection loops
}

void gracefulShutdown()
{
    spdlog::info("Initiating graceful shutdown...");

    applicationRunning = false;

    cleanupCS2Console();
    cleanupRemoteServer();

    platformSocketCleanup();

    std::cout << "CS2RemoteConsole shutdown complete. Bye-bye!..." << std::endl;
}

void handlePRNT(const PRNT& prnt)
{
    static const std::regex nameRegex(R"(name = (.+))");
    std::smatch match;

    if (std::regex_search(prnt.message, match, nameRegex))
    {
        globalClientInfo.name = match[1].str();
        std::cout << "Player name updated: " << globalClientInfo.name << std::endl;

        // Send the name to the remote server
        std::string nameMessage = "PLAYERNAME:" + globalClientInfo.name;
        if (sendMessageToRemoteServer(nameMessage))
        {
            std::cout << "Sent player name to remote server." << std::endl;
        }
        else
        {
            std::cerr << "Failed to send player name to remote server." << std::endl;
        }
    }
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

        if (!setupConfig() || !setupApplicationSockets())
        {
            return 1;
        }

        tui.init();
        tui.registerChannel(APPLICATION_SPECIAL_CHANNEL_ID, "Log", 0xD0D0D0FF, 0x2D0034FF); //light grey and purple
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
            tui.addConsoleMessage(PRNT.channelID, PRNT.message, PRNT.color);
            if (globalClientInfo.name.empty())
            {
                handlePRNT(PRNT);
            }
        });

        spdlog::info("[Main] Starting {}", application_name);

        cs2ConnectorThread = std::thread(cs2ConsoleConnectorLoop);

        if (Config::getInstance().getInt("remote_server_enabled", 0) == 1)
        {
            remoteServerConnectorThread = std::thread(remoteServerConnectorLoop);
        }

        tui.run();

        gracefulShutdown();
    }
    catch (const std::exception& e)
    {
        spdlog::error("[Main] Unhandled exception: {}", e.what());
        cleanupCS2Console();
        cleanupRemoteServer();
        platformSocketCleanup();
        return 1;
    }
    catch (...)
    {
        spdlog::error("[Main] Unhandled unknown exception.");
        cleanupCS2Console();
        cleanupRemoteServer();
        platformSocketCleanup();
        return 1;
    }
    return 0;
}
