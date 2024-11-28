#pragma once

#include <cstdio>
#include <thread>
#include <termios.h>
#include "controllers/uart.h"

namespace CPUE {

class VT100 final : public UARTDevice {
public:
    VT100() {
        // Disable terminal buffering and echoing
        struct termios t {};
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag &= ~ICANON;
        t.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    }

    void rx(char const c) override {
        putchar(c);
        fflush(stdout);
    };

    void start_loop_thread() { m_thread = std::thread(&VT100::loop, this); }
    void stop_loop_thread() {
        {
            std::scoped_lock _(m_mutex);
            m_should_stop = true;
        }
        m_thread.join();
    }

private:
    void loop() {
        char c;
        while (({
            std::scoped_lock _(m_mutex);
            !m_should_stop;
        })) {
            if ((c = getchar()) != EOF)
                tx(c);
        }
    }
    std::mutex m_mutex;
    bool m_should_stop = false;
    std::thread m_thread;
};

}