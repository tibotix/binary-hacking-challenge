#pragma once

#include "controllers/pic.h"
#include "common.h"
#include <thread>
#include <condition_variable>

namespace CPUE {


class UARTDevice {
public:
    friend void connect_uart_devices(UARTDevice&, UARTDevice&);
    UARTDevice() = default;
    virtual ~UARTDevice() = default;

    [[nodiscard]] bool is_connected() const { return m_opposite != nullptr; }

    virtual void tx(char const c) const {
        if (m_opposite) {
            m_opposite->rx(c);
        }
    };
    virtual void rx(char c) = 0;


private:
    void connect_to(UARTDevice* device) { m_opposite = device; }

    UARTDevice* m_opposite = nullptr;
};

inline void connect_uart_devices(UARTDevice& device1, UARTDevice& device2) {
    CPUE_ASSERT(!device1.is_connected() && !device2.is_connected(), "One of the devices is already connected");
    device1.connect_to(&device2);
    device2.connect_to(&device1);
}


// UART Controller for 1 UART connection to an external device
// TODO: own thread
class UARTController final : public UARTDevice {
public:
    UARTController(PIC* pic) : m_pic(pic) {
        auto handle = m_pic->request_connection();
        CPUE_ASSERT(handle.has_value(), "UARTController failed to request PIC connection");
        m_pic_handle = handle.value();
        m_thread = std::thread(&UARTController::loop, this);
    }
    ~UARTController() override {
        {
            std::scoped_lock _(m_mutex);
            m_should_stop = true;
        }
        m_thread.join();
    }
    void rx(char const c) override {
        // send data to the worker thread
        // std::lock_guard lk(m);
        // cv.notify_one();
        TODO_NOFAIL("provide c through MMIO");
    }

private:
    PICConnectionHandle m_pic_handle{0};
    PIC* m_pic;

private:
    void loop() {
        // wait until main() sends data
        std::unique_lock lk(m_mutex);
        cv.wait(lk, [] {
            return true;
        });

        // after the wait, we own the lock
        m_pic->raise_irq_pin(m_pic_handle);

        // manual unlocking is done before notifying, to avoid waking up
        // the waiting thread only to block again (see notify_one for details)
        lk.unlock();

        TODO_NOFAIL("either write or read");
    }
    std::mutex m_mutex;
    std::condition_variable cv;
    bool m_should_stop = false;
    std::thread m_thread;
};


}