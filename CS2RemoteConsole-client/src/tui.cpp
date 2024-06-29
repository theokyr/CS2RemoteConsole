#include "tui.h"
#include <algorithm>

TUI::TUI() : m_logWindow(nullptr), m_inputWindow(nullptr)
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
    curs_set(0);
    start_color();

    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);

    createWindows();
}

void TUI::run()
{
    while (true)
    {
        drawLogWindow();
        drawInputWindow();
        handleInput();
    }
}

void TUI::shutdown()
{
    destroyWindows();
    // endwin();
}

void TUI::setCommandCallback(std::function<void(const std::string&)> callback)
{
    m_commandCallback = callback;
}

void TUI::addLogMessage(const std::string& message)
{
    m_logMessages.push_back(message);
    if (m_logMessages.size() > LINES - 3)
    {
        m_logMessages.erase(m_logMessages.begin());
    }
}

void TUI::addUserCommand(const std::string& command) {
    addLogMessage("> " + command);
}

void TUI::setOnPRNTReceivedCallback(std::function<void(const std::string&, const std::string&)> callback)
{
    m_prntReceivedCallback = [this](const std::string& source, const std::string& message)
    {
        addLogMessage(source + ": " + message);
    };
    callback = m_prntReceivedCallback;
}

void TUI::createWindows()
{
    m_logWindow = newwin(LINES - 3, COLS, 0, 0);
    m_inputWindow = newwin(3, COLS, LINES - 3, 0);
    scrollok(m_logWindow, TRUE);
}

void TUI::destroyWindows()
{
    delwin(m_logWindow);
    delwin(m_inputWindow);
}

void TUI::drawLogWindow()
{
    werase(m_logWindow);
    box(m_logWindow, 0, 0);
    mvwprintw(m_logWindow, 0, 2, " Log ");

    int y = 1;
    for (const auto& message : m_logMessages)
    {
        mvwprintw(m_logWindow, y++, 1, "%s", message.c_str());
    }

    wrefresh(m_logWindow);
}

void TUI::drawInputWindow()
{
    werase(m_inputWindow);
    box(m_inputWindow, 0, 0);
    mvwprintw(m_inputWindow, 0, 2, " Input ");
    mvwprintw(m_inputWindow, 1, 1, "> %s", m_inputBuffer.c_str());
    wrefresh(m_inputWindow);
}

void TUI::handleInput()
{
    int ch = wgetch(m_inputWindow);
    switch (ch)
    {
    case '\n':
        if (m_commandCallback)
        {
            m_commandCallback(m_inputBuffer);
        }
        m_inputBuffer.clear();
        break;
    case KEY_BACKSPACE:
    case 127:
        if (!m_inputBuffer.empty())
        {
            m_inputBuffer.pop_back();
        }
        break;
    default:
        if (std::isprint(ch))
        {
            m_inputBuffer += static_cast<char>(ch);
        }
        break;
    }
}
