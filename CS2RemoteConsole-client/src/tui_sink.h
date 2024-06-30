// tui_sink.h
#ifndef TUI_SINK_H
#define TUI_SINK_H

#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>
#include "tui.h"

template<typename Mutex>
class tui_sink : public spdlog::sinks::base_sink<Mutex>
{
public:
    explicit tui_sink(TUI& tui) : m_tui(tui) {}

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        m_tui.addLogMessage(fmt::to_string(formatted));
    }

    void flush_() override {}

private:
    TUI& m_tui;
};

using tui_sink_mt = tui_sink<std::mutex>;
using tui_sink_st = tui_sink<spdlog::details::null_mutex>;

#endif // TUI_SINK_H