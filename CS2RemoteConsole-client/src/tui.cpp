﻿#include "tui.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>

#include "logging.h"

TUI::TUI()
    : m_logWindow(nullptr), m_consoleWindow(nullptr), m_inputWindow(nullptr),
      m_running(true), m_needsResize(false), m_lastWidth(0), m_lastHeight(0)
{
}

TUI::~TUI()
{
    shutdown();
}

void TUI::init()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    start_color();
    use_default_colors();
    nodelay(stdscr, TRUE);

    // Check for extended color support
    if (can_change_color() && COLORS > 0x1000000 && COLOR_PAIRS > 0x1000000)
    {
        m_maxColorPairs = COLOR_PAIRS - 1; // Reserve pair 0
        m_useExtendedColors = true;
    }
    else
    {
        m_maxColorPairs = 255; // Standard 256 color mode, reserve pair 0
        m_useExtendedColors = false;
    }

    createWindows();
    getmaxyx(stdscr, m_lastHeight, m_lastWidth);
}

void TUI::run()
{
    while (m_running)
    {
        if (m_needsResize)
        {
            handleResize();
            m_needsResize = false;
        }

        handleInput();
        drawWindows();

        // Add a small delay to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
}

void TUI::shutdown()
{
    m_running = false;
    destroyWindows();
}

void TUI::setCommandCallback(std::function<void(const std::string&)> callback)
{
    m_commandCallback = callback;
}

void TUI::addLogMessage(const std::string& message)
{
    std::lock_guard<std::mutex> lock(m_logMutex);
    m_logMessages.push_back(message);
    if (m_logMessages.size() > MAX_LOG_MESSAGES)
    {
        m_logMessages.erase(m_logMessages.begin());
    }
}

void TUI::addConsoleMessage(int channelId, const std::string& message)
{
    std::lock_guard<std::mutex> lock(m_consoleMutex);
    ConsoleMessage cMessage;
    cMessage.channelId = channelId;
    cMessage.message = message;

    m_consoleMessages.push_back(cMessage);
    if (m_consoleMessages.size() > MAX_CONSOLE_MESSAGES)
    {
        m_consoleMessages.erase(m_consoleMessages.begin());
    }
}

void TUI::registerChannel(int channelId, const std::string& name, uint32_t color)
{
    std::lock_guard<std::mutex> lock(m_channelsMutex);

    ConsoleChannel channel;
    channel.name = name;
    channel.color = color;

    if (color != 0)
    {
        auto it = m_colorCache.find(color);
        if (it == m_colorCache.end())
        {
            if (m_nextColorPairId < m_maxColorPairs)
            {
                short colorPairId = m_nextColorPairId++;

                if (m_useExtendedColors)
                {
                    int r = (color >> 16) & 0xFF;
                    int g = (color >> 8) & 0xFF;
                    int b = color & 0xFF;
                    init_extended_color(colorPairId, r, g, b);
                    init_extended_pair(colorPairId, colorPairId, -1);
                }
                else
                {
                    // Map to the nearest color in 256 color mode
                    int r = (color >> 16) & 0xFF;
                    int g = (color >> 8) & 0xFF;
                    int b = color & 0xFF;
                    short nearestColor = 16 + (r / 43 * 36) + (g / 43 * 6) + (b / 43);
                    init_pair(colorPairId, nearestColor, -1);
                }

                m_colorCache[color] = colorPairId;
                channel.colorPairId = colorPairId;
            }
            else
            {
                // We've run out of color pairs, use a default
                channel.colorPairId = 0;
            }
        }
        else
        {
            channel.colorPairId = it->second;
        }
    }
    else
    {
        channel.colorPairId = 0;
    }

    m_channels[channelId] = channel;
}

void TUI::createWindows()
{
    int height, width;
    getmaxyx(stdscr, height, width);

    resizeWindows(height, width);
}

void TUI::resizeWindows(int height, int width)
{
    // Calculate window sizes
    int inputHeight = 3;
    int remainingHeight = height - inputHeight;
    int logHeight = remainingHeight / 3;
    int consoleHeight = remainingHeight - logHeight;

    // If windows don't exist, create them
    if (!m_logWindow)
    {
        m_logWindow = newwin(logHeight, width, 0, 0);
        scrollok(m_logWindow, TRUE);
    }
    if (!m_consoleWindow)
    {
        m_consoleWindow = newwin(consoleHeight, width, logHeight, 0);
        scrollok(m_consoleWindow, TRUE);
    }
    if (!m_inputWindow)
    {
        m_inputWindow = newwin(inputHeight, width, height - inputHeight, 0);
    }

    // Resize existing windows
    wresize(m_logWindow, logHeight, width);
    wresize(m_consoleWindow, consoleHeight, width);
    wresize(m_inputWindow, inputHeight, width);

    // Move windows to their correct positions
    mvwin(m_logWindow, 0, 0);
    mvwin(m_consoleWindow, logHeight, 0);
    mvwin(m_inputWindow, height - inputHeight, 0);
}

void TUI::destroyWindows()
{
    if (m_logWindow) delwin(m_logWindow);
    if (m_consoleWindow) delwin(m_consoleWindow);
    if (m_inputWindow) delwin(m_inputWindow);
}

void TUI::drawWindows()
{
    drawLogWindow();
    drawConsoleWindow();
    drawInputWindow();
    doupdate();
}

void TUI::drawLogWindow()
{
    std::lock_guard<std::mutex> lock(m_logMutex);
    werase(m_logWindow);
    box(m_logWindow, 0, 0);
    mvwprintw(m_logWindow, 0, 2, " Log ");

    int maxLines = getmaxy(m_logWindow) - 2;
    int startIndex = std::max(0, static_cast<int>(m_logMessages.size()) - maxLines);
    for (int i = 0; i < maxLines && (startIndex + i) < static_cast<int>(m_logMessages.size()); ++i)
    {
        mvwprintw(m_logWindow, i + 1, 1, "%s", m_logMessages[startIndex + i].c_str());
    }

    wnoutrefresh(m_logWindow);
}

void TUI::drawConsoleWindow()
{
    std::lock_guard<std::mutex> lock(m_consoleMutex);
    std::lock_guard<std::mutex> channelsLock(m_channelsMutex);
    werase(m_consoleWindow);
    box(m_consoleWindow, 0, 0);
    mvwprintw(m_consoleWindow, 0, 2, " Console ");

    int maxLines = getmaxy(m_consoleWindow) - 2;
    int startIndex = std::max(0, static_cast<int>(m_consoleMessages.size()) - maxLines);

    for (int i = 0; i < maxLines && (startIndex + i) < static_cast<int>(m_consoleMessages.size()); ++i)
    {
        auto& currentMessage = m_consoleMessages[startIndex + i];
        auto channelIt = m_channels.find(currentMessage.channelId);

        if (channelIt != m_channels.end())
        {
            const auto& channel = channelIt->second;

            if (channel.colorPairId != 0)
            {
                wcolor_set(m_consoleWindow, channel.colorPairId, NULL);
                mvwprintw(m_consoleWindow, i + 1, 1, "[%s] %s", channel.name.c_str(), currentMessage.message.c_str());
                wcolor_set(m_consoleWindow, 0, NULL);
            }
            else
            {
                mvwprintw(m_consoleWindow, i + 1, 1, "[%s] %s", channel.name.c_str(), currentMessage.message.c_str());
            }
        }
        else
        {
            mvwprintw(m_consoleWindow, i + 1, 1, "[Unknown] %s", currentMessage.message.c_str());
        }
    }

    wnoutrefresh(m_consoleWindow);
}

void TUI::drawInputWindow()
{
    werase(m_inputWindow);
    box(m_inputWindow, 0, 0);
    mvwprintw(m_inputWindow, 0, 2, " Input ");
    mvwprintw(m_inputWindow, 1, 1, "> %s", m_inputBuffer.c_str());
    wmove(m_inputWindow, 1, m_inputBuffer.length() + 3);
    wnoutrefresh(m_inputWindow);
}

void TUI::handleInput()
{
    int ch = wgetch(stdscr);
    if (ch == ERR) return; // No input available

    switch (ch)
    {
    case '\n':
        if (m_commandCallback && !m_inputBuffer.empty())
        {
            m_commandCallback(m_inputBuffer);
            std::string channelName = std::string("> ");
            addConsoleMessage(-1, m_inputBuffer);
            m_inputBuffer.clear();
        }
        break;
    case KEY_BACKSPACE:
    case 127:
    case 8: // Handle backspace key
        if (!m_inputBuffer.empty())
        {
            m_inputBuffer.pop_back();
        }
        break;
    case KEY_RESIZE:
        m_needsResize = true;
        break;
    default:
        if (ch >= 32 && ch <= 126)
        {
            m_inputBuffer += static_cast<char>(ch);
        }
        break;
    }
}

void TUI::handleResize()
{
    refresh();
    clear();

    int height, width;
    getmaxyx(stdscr, height, width);
    resize_term(height, width);

    resizeWindows(height, width);

    m_lastHeight = height;
    m_lastWidth = width;

    drawWindows();
}
