#pragma once

#include <cstdio>
#include <thread>
#include <termios.h>
#include "controllers/uart.h"

namespace CPUE {

class VT100 final : public UARTDevice {
public:
    VT100() = default;
    ~VT100() override { stop_loop_thread(); }

    void rx(char const c) override {
        putchar(c);
        fflush(stdout);
    };
    void disable_terminal_echo() {
        m_echo = false;
    }
    void enable_terminal_echo() {
        m_echo = true;
    }

    void start_loop_thread() { m_thread = std::thread(&VT100::loop, this); }
    void stop_loop_thread() {
        {
            std::scoped_lock _(m_mutex);
            m_should_stop = true;
        }
        if (m_thread.joinable())
            m_thread.join();
    }

private:
    void change_terminal() {
        tcgetattr(STDIN_FILENO, &old_tio);
        termios t = old_tio;
        auto flags = ICANON | (!m_echo ? ECHO : 0);
        t.c_lflag &= ~flags;
        t.c_cc[VMIN] = 1;
        t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    }
    void restore_terminal() {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    }
    void loop() {
        change_terminal();

        char c;
        while (({
            std::scoped_lock _(m_mutex);
            !m_should_stop;
        })) {
            if ((c = getchar()) != EOF)
                tx(c);
        }

        restore_terminal();
    }

    termios old_tio;
    bool m_echo = true;
    std::mutex m_mutex;
    bool m_should_stop = false;
    std::thread m_thread;
    InplaceFIFO<char, TERM_FIFO_CAPACITY> m_fifo;
};

}