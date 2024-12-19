#pragma once

#include <thread>
#include <condition_variable>
#include <chrono>
#include <queue>
#include "forward.h"
#include "controllers/pic.h"
#include "common.h"
#include "address.h"
#include "fifo.h"

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


/**
 * UART Controller for 1 UART connection to an external device.
 *
 * This is a simplified version of the 16550D UART Transceiver
 * We only implement the FIFO-Interrupt-based operation mode
 *
 * Registers (all registers are 1byte wide):
 *  - addr_offset: <abr> <descr>
 *  - 0x0: THR (Transmit-Holding-Register)
 *  - 0x0: RBR (Receive-Buffer-Register)
 *  - 0x1: IER (Interrupt-Enable-Register)
 *  - 0x2: IIR (Interrupt-Identification-Register)
 *  - 0x2: FCR (FIFO-Control-Register)
 *  - 0x5: LSR (Line-Status-Register)
 *  - (internal) TSR (Transmit-Shift-Register)
 *      -> if data is written to TSR, it is immediately transmitted over the wire
 *
 * FIFOs:
 *  - RX-FIFO (as soon as the RBR is written to, it is moved into the RX-FIFO. If RX-FIFO is full, an overrun error occurs)
 *  - TX-FIFO (as soon as the THR is written to, it is moved into the TX-FIFO. If TX-FIFO is full, an overrun error occurs)
 *
 * Interrupts:
 * Can be configured through IER (enabling interrupt when bit set to 0x1):
 *  - 0x0: Enable Received Data Available Interrupt (and timeout interrupts in the FIFO mode)
 *  - 0x1: Enable Transmitter Holding Register Empty Interrupt
 *  - 0x2: Enable Receiver Line Status Interrupt
 *  - 0x3: Enable MODEM Status Interrupt
 * The currently active interrupt is set in IIR:
 *  - 0b0001 (priority -): No interrupt
 *  - 0b0110 (priority highest): Receiver Line Status Interrupt
 *      Description: Overrun Error or Parity Error or Framing Error or Break Interrupt
 *      Reset on: Reading the Line Status Register
 *  - 0b0100 (priority second): Received Data Available
 *      Description: Receiver Data Available or Trigger Level Reached
 *      Reset on: Reading the Receiver Buffer Register or the FIFO Drops Below the Trigger Level
 *  - 0b1100 (priority second): Character Timeout Indication (only in FIFO-Mode)
 *      Description: No Characters Have Been Removed From or Input to the RCVR FIFO During the Last 4 Char. times
 *                   and there Is at Least 1 Char. in It During This Time
 *      Reset on: Reading the Receiver
 *  - 0b0010 (priority third): Transmitter Holding Register Empty
 *      Description: Transmitter Holding Register Empty
 *      Reset on: Reading the IIR Register (if source of interrupt) or Writing into the Transmitter Holding Register
 *  - 0b0000 (priority fourth): MODEM Status (Not implemented)
 *      Description: Clear to Send or Data Set Ready or Ring Indicator or Data Carrier Detect
 *      Reset on: Reading the MODEM Status Register
 *
 *
 */
class UARTController final : public UARTDevice {
public:
    // Non-FIFO mode is essentially FIFO-mode with capacity 1 (resembles the THR/RBR registers)
    static constexpr u8 FIFO_CAPACITY = 16;
    static constexpr PhysicalAddress MMIO_REG_BASE = 0xffff'ffff'0000'0000_pa;
    static constexpr u8 MAX_PENDING_INTERRUPTS = 8;

    explicit UARTController(CPU* cpu);
    ~UARTController() override;
    void rx(char c) override;


private:
    void init_mmio_registers();
    void assert_in_fifo_mode() const { CPUE_ASSERT(m_fcr.concrete.enable_fifo == 1, "UART controller is not in fifo-mode."); }

    u8 read_rbr();
    void write_thr(u8 value);
    void write_ier(u8 value);
    u8 read_iir();
    void write_fcr(u8 value);
    u8 read_lsr();

    void update_data_ready_bit_and_rda_interrupt_if_necessary() {
        m_lsr.concrete.dr = !m_rx_fifo.is_empty();
        if (m_ier.is_rda_enabled() && m_rx_fifo.size() >= m_fcr.concrete.trigger_level())
            record_interrupt(Interrupts::RECEIVED_DATA_AVAILABLE);
        if (m_ier.is_rda_enabled() && m_rx_fifo.size() < m_fcr.concrete.trigger_level())
            clear_interrupt(Interrupts::RECEIVED_DATA_AVAILABLE);
    }
    void update_thre_interrupt_if_necessary() {
        if (m_ier.is_thre_enabled() && m_tx_fifo.is_empty())
            record_interrupt(Interrupts::TRANSMITTER_HOLDING_REGISTER_EMPTY);
        if (m_ier.is_thre_enabled() && m_tx_fifo.size() > 0)
            clear_interrupt(Interrupts::TRANSMITTER_HOLDING_REGISTER_EMPTY);
    }

    void loop();

private:
    struct Interrupt {
        u8 number;
        u8 iir_value;
        u8 priority;

        bool operator==(Interrupt const& other) const { return number == other.number; }
    };
    struct Interrupts {
        static constexpr Interrupt RECEIVER_LINE_STATUS = {0, 0b0110, 0b110};
        static constexpr Interrupt RECEIVED_DATA_AVAILABLE = {1, 0b0100, 0b100};
        static constexpr Interrupt CHARACTER_TIMEOUT_INDICATION = {2, 0b1100, 0b100};
        static constexpr Interrupt TRANSMITTER_HOLDING_REGISTER_EMPTY = {3, 0b0010, 0b010};
        static constexpr Interrupt MODEM_STATUS = {4, 0b0000, 0b000};
    };
    void record_overrun_error();
    void record_interrupt(Interrupt interrupt);
    void clear_interrupt(Interrupt interrupt);
    struct InterruptComparator {
        bool operator()(Interrupt const& lhs, Interrupt const& rhs) const { return lhs.priority < rhs.priority; }
    };
    std::priority_queue<Interrupt, std::vector<Interrupt>, InterruptComparator> m_interrupt_queue;
    std::bitset<8> m_active_interrupt_map = 0;


    // FIFO-Control-Register
    union FCR {
        struct {
            u8 enable_fifo : 1; // We only implement FIFO-Mode, so this must be 1
            u8 clear_rx_fifo : 1; // If this is 1, rx-fifo is cleared (this bit is self-clearing)
            u8 clear_tx_fifo : 1; // If this is 1, tx-fifo is cleared (this bit is self-clearing)
            u8 : 1; // we don't implement rxrdy/txrdy pins
            u8 : 2; // reserved
            u8 trigger_level_indicator : 2; // 0b00=1, 0b01=4, 0b10=8, 0b11=14,

            u8 trigger_level() const {
                switch (trigger_level_indicator) {
                    case 0b00: return 1;
                    case 0b01: return 4;
                    case 0b10: return 8;
                    case 0b11: return 14;
                    default: fail("invalid trigger level");
                }
            }
        } concrete;
        u8 value;
    };
    FCR m_fcr = {.value = 0b01000001};

    // Interrupt-Enable-Register
    struct IER {
        u8 value;

        bool is_rda_enabled() const { return bits(value, 0, 0); } // Received Data Available (and Character Timeout Indicator in FIFO-mode)
        bool is_thre_enabled() const { return bits(value, 1, 1); } // Transmitter Holding Register Empty
        bool is_rls_enabled() const { return bits(value, 2, 2); } // Receiver Line Status
        bool is_ms_enabled() const { return bits(value, 3, 3); } // Modem status
    };
    IER m_ier = {.value = 0b1111};

    union LSR {
        struct {
            /**
             * Data Ready (DR) indicator. Bit is set to a logic 1 whenever a complete incoming
             * character has been received and transferred into the Receiver Buffer Register or the FIFO. Bit 0 is reset to a
             * logic 0 by reading all of the data in the Receiver Buffer Register or the FIFO.
             */
            u8 dr : 1;
            /**
             * Overrun Error (OE) indicator. Bit 1 indicates that data in the Receiver Buffer Register was
             * not read by the CPU before the next character was transferred into the Receiver Buffer Register, thereby
             * destroying the previous character. The OE indicator is set to a logic 1 upon detection of an overrun condition and
             * reset whenever the CPU reads the contents of the Line Status Register. If the FIFO mode data continues to fill
             * the FIFO beyond the trigger level, an overrun error will occur only after the FIFO is full and the next character
             * has been completely received in the shift register. OE is indicated to the CPU as soon as it happens. The
             * character in the shift register is overwritten, but it is not transferred to the FIFO.
             */
            u8 oe : 1;
            u8 pe : 1; // Parity Error indicator. Not implemented.
            u8 fe : 1; // Framing Error indicator. Not implemented.
            u8 bi : 1; // Break Interrupt indicator. Not implemented.
        } concrete;
        u8 value;
    };
    LSR m_lsr = {.value = 0x0};

    InplaceFIFO<u8, FIFO_CAPACITY> m_rx_fifo;
    InplaceFIFO<u8, FIFO_CAPACITY> m_tx_fifo;

    std::chrono::time_point<std::chrono::steady_clock> m_last_cpu_read_time_point;
    std::chrono::time_point<std::chrono::steady_clock> m_last_char_received_time_point;

    std::mutex m_mutex;
    std::condition_variable cv;
    bool m_should_stop = false;
    std::thread m_thread;

    PICConnectionHandle m_pic_handle{0};
    CPU* m_cpu;
};
}