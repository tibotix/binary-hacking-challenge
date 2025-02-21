#pragma once

#include <cstdio>
#include <thread>
#include <chrono>
#include <termios.h>
#include <poll.h>
#include "controllers/uart.h"
#include "fifo.h"
#include "common.h"

using namespace std::literals::chrono_literals;

namespace CPUE {

class VT100 final : public UARTDevice {
public:
    static constexpr u64 TERM_FIFO_CAPACITY = 1_kb;
    VT100() = default;
    ~VT100() override { stop_loop_thread(); }

    void disable_terminal_echo() { m_echo = false; }
    void enable_terminal_echo() { m_echo = true; }

    void start_loop_thread() { m_thread = std::thread(&VT100::loop, this); }
    void stop_loop_thread() {
        {
            std::scoped_lock _(m_mutex);
            m_should_stop = true;
        }
        if (m_thread.joinable())
            m_thread.join();
    }

protected:
    void rx(char const c) override {
        putchar(c);
        fflush(stdout);
    };

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
    void restore_terminal() { tcsetattr(STDIN_FILENO, TCSANOW, &old_tio); }
    void loop() {
        change_terminal();

        struct pollfd pfd;
        pfd.fd = STDIN_FILENO;
        pfd.events = POLLIN;

        char c;
        while (({
            std::scoped_lock _(m_mutex);
            !m_should_stop;
        })) {
            // if we have pending characters in fifo, wait at most 100ms for character, otherwise block completely
            int ret = poll(&pfd, 1, m_fifo.is_empty() ? -1 : 100);

            if (ret > 0 && (pfd.revents & POLLIN)) {
                if (read(STDIN_FILENO, &c, 1) == 1) {
                    (void)m_fifo.try_enqueue(c);
                }
            }

            // Send at most 5 queued items
            for (u8 i = 0; i < 5; ++i) {
                if (m_fifo.is_empty())
                    break;
                // if there are still some elements in fifo and CTS is not set, sleep a bit to give kernel time to catch up.
                if (!cts()) {
                    std::this_thread::sleep_for(50ms);
                    continue;
                }
                tx(m_fifo.take_first().value());
            }
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