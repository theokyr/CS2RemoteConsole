#include "tui.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/callback_sink.h>

// PDCurses uses nc_getmouse, ncurses uses getmouse
#ifndef _WIN32
#define nc_getmouse getmouse
#endif

template <typename T>
T clamp(const T& value, const T& low, const T& high)
{
    return std::max(low, std::min(value, high));
}

TUI::TUI(std::atomic<bool>& runningFlag)
    : m_consoleWindow(nullptr), m_inputWindow(nullptr),
      m_running(runningFlag), m_needsResize(false), m_lastWidth(0), m_lastHeight(0),
      m_scrollPosition(0), m_consoleDirty(false)
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

    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    m_useExtendedColors = can_change_color() && COLORS > EXTENDED_COLOR_BASE;

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

        auto now = std::chrono::steady_clock::now();
        if (m_consoleDirty && (now - m_lastUpdate) >= std::chrono::milliseconds(16))
        {
            drawWindows();
            m_consoleDirty = false;
            m_lastUpdate = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void TUI::shutdown()
{
    m_running = false;
    destroyWindows();
    mousemask(0, NULL);
    endwin();
}

void TUI::setCommandCallback(std::function<void(const std::string&)> callback)
{
    m_commandCallback = callback;
}

void TUI::addConsoleMessage(int channelId, const std::string& message, uint32_t msgColor) //no background color required, this is only for PRNT as of now, which doesn't deliver background colors anyways
{
    std::lock_guard<std::mutex> lock(m_consoleMutex);
    ConsoleMessage cMessage;
    cMessage.channelId = channelId;
    cMessage.color = byteSwap32(msgColor); //byteswapping because apparently message colors have their bytes swapped relative to channel colors...Valve...
    cMessage.message = message;

    cMessage.timestamp = std::chrono::system_clock::now();

    m_consoleMessages.push_back(cMessage);
    if (m_consoleMessages.size() > MAX_CONSOLE_MESSAGES)
    {
        m_consoleMessages.pop_front();
    }
    m_consoleDirty = true;
}

void TUI::registerChannel(int id, const std::string& name, uint32_t color, uint32_t backgroundColor)
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
            if (m_nextColorPairId < COLOR_PAIRS)
            {
                channel.colorPairId = initializeColor(color, backgroundColor);
            }
            else
            {
                channel.colorPairId = 0;
                spdlog::warn("[TUI] Ran out of color pairs. Using default color for channel: {}", name);
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

    m_channels[id] = channel;
}

void TUI::setConsoleDirty(bool dirty)
{
    m_consoleDirty = dirty;
}

void TUI::createWindows()
{
    int height, width;
    getmaxyx(stdscr, height, width);

    m_consoleWindow = newwin(height - 3, width, 0, 0);
    m_inputWindow = newwin(3, width, height - 3, 0);

    scrollok(m_consoleWindow, TRUE);
    keypad(m_inputWindow, TRUE);
}

void TUI::destroyWindows()
{
    if (m_consoleWindow) delwin(m_consoleWindow);
    if (m_inputWindow) delwin(m_inputWindow);
}

void TUI::drawConsoleWindow()
{
    std::lock_guard<std::mutex> lock(m_consoleMutex);
    std::lock_guard<std::mutex> channelsLock(m_channelsMutex);
    werase(m_consoleWindow);

    int maxLines = getmaxy(m_consoleWindow);
    size_t startIndex = std::max(0, static_cast<int>(m_consoleMessages.size()) - maxLines - static_cast<int>(m_scrollPosition));

    for (int i = 0; i < maxLines && (startIndex + i) < m_consoleMessages.size(); ++i)
    {
        auto& currentMessage = m_consoleMessages[startIndex + i];
        auto channelIt = m_channels.find(currentMessage.channelId);

        std::string prefix;
        short colorPairId = 0;
        
        if (channelIt != m_channels.end())
        {
            colorPairId = channelIt->second.colorPairId;
            prefix = "[" + channelIt->second.name + "] ";
        }
        else
        {
            prefix = "[Unknown] ";
        }


        if (currentMessage.channelId == APPLICATION_SPECIAL_CHANNEL_ID)
        {
            prefix = "";
        }
        if (currentMessage.color)
        {
            colorPairId =  initializeColor(currentMessage.color);
        }

        if (colorPairId != 0)
            wattron(m_consoleWindow, COLOR_PAIR(colorPairId));

        mvwprintw(m_consoleWindow, i, 0, "%s%s", prefix.c_str(), currentMessage.message.c_str());

        if (colorPairId != 0)
            wattroff(m_consoleWindow, COLOR_PAIR(colorPairId));
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

void TUI::updateInputWindow()
{
    drawInputWindow();
    doupdate();
}

void TUI::drawWindows()
{
    drawConsoleWindow();
    drawInputWindow();
    doupdate();
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
            addConsoleMessage(APPLICATION_SPECIAL_CHANNEL_ID, "> " + m_inputBuffer);
            m_inputBuffer.clear();
            updateInputWindow();
        }
        break;
    case KEY_BACKSPACE:
    case 127:
    case 8: // Handle backspace key
        if (!m_inputBuffer.empty())
        {
            m_inputBuffer.pop_back();
            updateInputWindow();
        }
        break;
    case KEY_RESIZE:
        m_needsResize = true;
        break;
    case KEY_PPAGE: // Page Up
        scrollConsole(getmaxy(m_consoleWindow));
        break;
    case KEY_NPAGE: // Page Down
        scrollConsole(-getmaxy(m_consoleWindow));
        break;
    case KEY_MOUSE:
        {
            MEVENT event;
            if (nc_getmouse(&event) == OK)
            {
                if (event.bstate & BUTTON4_PRESSED) // Scroll wheel up
                {
                    scrollConsole(MOUSE_SCROLL_SPEED);
                }
                else if (event.bstate & BUTTON5_PRESSED) // Scroll wheel down
                {
                    scrollConsole(-MOUSE_SCROLL_SPEED);
                }
            }
        }
        break;
    default:
        if (ch >= 32 && ch <= 126)
        {
            m_inputBuffer += static_cast<char>(ch);
            updateInputWindow();
        }
        break;
    }
}

void TUI::setupLoggerCallbackSink()
{
    auto logger = spdlog::default_logger();
    if (logger)
    {
        logger->sinks().push_back(std::make_shared<spdlog::sinks::callback_sink_mt>([this](const spdlog::details::log_msg& msg)
        {
            auto severity = to_string_view(msg.level);
            std::stringstream ss;
            ss << "[" << std::string(severity.begin(), severity.end()) << "] " << std::string(msg.payload.begin(), msg.payload.end());

            addConsoleMessage(APPLICATION_SPECIAL_CHANNEL_ID, ss.str());
        }));
    }
}

void TUI::handleResize()
{
    endwin();
    refresh();
    clear();

    int height, width;
    getmaxyx(stdscr, height, width);

    resizeWindows(height, width);

    m_lastHeight = height;
    m_lastWidth = width;

    m_consoleDirty = true;
}

void TUI::resizeWindows(int height, int width)
{
    wresize(m_consoleWindow, height - 3, width);
    mvwin(m_consoleWindow, 0, 0);

    wresize(m_inputWindow, 3, width);
    mvwin(m_inputWindow, height - 3, 0);
}

void TUI::scrollConsole(int lines)
{
    int maxScroll = std::max(0, static_cast<int>(m_consoleMessages.size()) - getmaxy(m_consoleWindow));
    m_scrollPosition = static_cast<size_t>(clamp(static_cast<int>(m_scrollPosition) + lines, 0, maxScroll));
    m_consoleDirty = true;
}

short TUI::mapTo256Color(uint32_t color)
{
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;

    if (r == g && g == b)
    {
        if (r < 8) return 16;
        if (r > 248) return 231;
        return static_cast<short>(round(((r - 8) / 247.0) * 24) + 232);
    }

    return static_cast<short>(16 +
        36 * round(r / 255.0 * 5) +
        6 * round(g / 255.0 * 5) +
        round(b / 255.0 * 5));
}

short TUI::initializeColor(uint32_t color, uint32_t backgroundColor)
{
    long long combinedColor = (((long long)color) << 32) | (backgroundColor & 0xffffffffL);

    short colorPairId = 0;

    int r = (color >> 24) & 0xFF;
    int g = (color >> 16) & 0xFF;
    int b = (color >> 8) & 0xFF;

    int br = (backgroundColor >> 24) & 0xFF;
    int bg = (backgroundColor >> 16) & 0xFF;
    int bb = (backgroundColor >> 8) & 0xFF;

    if (m_colorCache.find(combinedColor) == m_colorCache.end())
    {
        colorPairId = m_nextColorPairId++;
        if (m_useExtendedColors)
        {
            int colorNumber = EXTENDED_COLOR_BASE + m_nextColorId++;
            int bColorNumber = -1;
            if (backgroundColor)
                bColorNumber = EXTENDED_COLOR_BASE + m_nextColorId++;
            init_color(colorNumber, r * 1000 / 255, g * 1000 / 255, b * 1000 / 255);
            init_color(bColorNumber, br * 1000 / 255, bg * 1000 / 255, bb * 1000 / 255);
            color_content(colorNumber, (short*)&r, (short*)&g, (short*)&b);
            init_pair(colorPairId, colorNumber, bColorNumber);
        }
        else
        {
            short nearestColor = mapTo256Color(color);
            short bNearestColor = -1;
            if (backgroundColor)
                bNearestColor = mapTo256Color(backgroundColor);
            init_pair(colorPairId, nearestColor, bNearestColor);
        }
        m_colorCache[combinedColor] = colorPairId;
    }
    else
    {

        colorPairId = m_colorCache.find(combinedColor)->second;
    }

    return colorPairId;
}
