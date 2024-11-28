#pragma once

#include "common.h"

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
class UARTController final : public UARTDevice {
public:
    UARTController() = default;
    void rx(char const c) override {
        tx(c);
        // TODO: raise interrupt on pic
        // printf("%c", c);
        // fflush(stdout);
    }
};


}