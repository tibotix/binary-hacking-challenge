#include "controllers/uart.h"
#include "cpu.h"

namespace CPUE {


UARTController::UARTController(CPU* cpu) : m_cpu(cpu) {
    auto handle = m_cpu->pic().request_connection();
    CPUE_ASSERT(handle.has_value(), "UARTController failed to request PIC connection");
    m_pic_handle = handle.value();
    TODO_NOFAIL("map mmio registers");
    m_thread = std::thread(&UARTController::loop, this);
}
UARTController::~UARTController() {
    {
        std::scoped_lock _(m_mutex);
        m_should_stop = true;
    }
    m_thread.join();
}


void UARTController::rx(char const c) {
    // send data to the worker thread
    // std::lock_guard lk(m);
    // cv.notify_one();
    TODO_NOFAIL("provide c through MMIO");
}


void UARTController::loop() {
    // wait until main() sends data
    std::unique_lock lk(m_mutex);
    cv.wait(lk, [] {
        return true;
    });
    // after the wait, we own the lock
    m_cpu->pic().raise_irq_pin(m_pic_handle);
    // manual unlocking is done before notifying, to avoid waking up
    // the waiting thread only to block again (see notify_one for details)
    lk.unlock();
    TODO_NOFAIL("either write or read");
}


}