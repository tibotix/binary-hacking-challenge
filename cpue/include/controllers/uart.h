#pragma once

#include <thread>
#include <atomic>
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

    void tx(char const c) const {
        if (m_opposite) {
            m_opposite->rx(c);
        }
    };
    [[nodiscard]] bool cts() const { return m_cts; }
    void set_rts(bool rts) const {
        if (m_opposite)
            m_opposite->set_cts(rts);
    }

protected:
    virtual void rx(char c) = 0;
    void set_cts(bool cts) { m_cts = cts; }

private:
    void connect_to(UARTDevice* device) { m_opposite = device; }

    UARTDevice* m_opposite = nullptr;
    std::atomic<bool> m_cts = true;
};

inline void connect_uart_devices(UARTDevice& device1, UARTDevice& device2) {
    CPUE_ASSERT(!device1.is_connected() && !device2.is_connected(), "One of the devices is already connected");
    device1.connect_to(&device2);
    device2.connect_to(&device1);
}


/**
 * UART Controller for 1 UART connection to an external device.
 *
 * This is a simplified version of the TL16C750 UART Transceiver
 * We only implement the FIFO-Interrupt-based operation mode
 *
 * Registers (all registers are 1byte wide):
 *  - addr_offset: <abr> <descr>
 *  - 0x0: THR (Transmit-Holding-Register)
 *  - 0x0: RBR (Receive-Buffer-Register)
 *  - 0x1: IER (Interrupt-Enable-Register)
 *  - 0x2: IIR (Interrupt-Identification-Register)
 *  - 0x2: FCR (FIFO-Control-Register)
 *  - 0x3: LCR (Line-Control-Register) (partially implemented)
 *  - 0x4: MCR (Modem-Control-Register)
 *  - 0x5: LSR (Line-Status-Register)
 *  - 0x6: MSR (Modem-Status-Register) (not implemented)
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
 *  - 0x4: Enables sleep mode (not implemented)
 *  - 0x5: Enables low power mode (not implemented)
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
    static constexpr u8 MAX_FIFO_CAPACITY = 64; // 16750 allows either 16byte or 64byte FIFOs
    static constexpr PhysicalAddress MMIO_REG_BASE = 0xff00'0000_pa;
    static constexpr u8 MAX_PENDING_INTERRUPTS = 8;

    UARTController(CPU* cpu, u8 id);
    ~UARTController() override;

protected:
    void rx(char c) override;


private:
    void init_mmio_registers();
    void assert_in_fifo_mode() const { CPUE_ASSERT(m_fcr.concrete.enable_fifo == 1, "UART controller is not in fifo-mode."); }

    u8 read_rbr();
    void write_thr(u8 value);
    u8 read_ier() const;
    void write_ier(u8 value);
    u8 read_iir();
    void write_fcr(u8 value);
    u8 read_lsr();

    void update_data_ready_bit_and_rda_interrupt_if_necessary() {
        bool rx_fifo_trigger_level_reached = m_rx_fifo.size() >= m_fcr.concrete.trigger_level();
        // When auto-RST is enabled:
        // When the receiver FIFO level reaches the trigger level, RTS is deasserted.
        // NOTE: we also reassert RTS on clearing the RX-FIFO through FCR. (This deviates from spec, which says it's only updated on RBR read)
        if (m_mcr.c.afe && m_mcr.c.rts)
            set_rts(!rx_fifo_trigger_level_reached);
        m_lsr.concrete.dr = !m_rx_fifo.is_empty();
        if (m_ier.is_rda_enabled() && rx_fifo_trigger_level_reached)
            record_interrupt(Interrupts::RECEIVED_DATA_AVAILABLE);
        else
            clear_interrupt(Interrupts::RECEIVED_DATA_AVAILABLE);
    }
    void update_thre_interrupt_if_necessary() {
        m_lsr.concrete.thre = m_tx_fifo.is_empty();
        if (m_ier.is_thre_enabled() && m_tx_fifo.is_empty())
            record_interrupt(Interrupts::TRANSMITTER_HOLDING_REGISTER_EMPTY);
        else
            clear_interrupt(Interrupts::TRANSMITTER_HOLDING_REGISTER_EMPTY);
    }

    [[nodiscard]] u8 fifo_capacity() const { return m_fcr.concrete.extended_fifo ? MAX_FIFO_CAPACITY : 16; }

    void loop();

private:
    struct Interrupt {
        u8 number;
        u8 iir_value;
        u8 priority;

        bool operator==(Interrupt const& other) const { return number == other.number; }
    };
    struct Interrupts {
        static constexpr Interrupt RECEIVER_LINE_STATUS = {0, 0b0110, 1};
        static constexpr Interrupt RECEIVED_DATA_AVAILABLE = {1, 0b0100, 2};
        static constexpr Interrupt CHARACTER_TIMEOUT_INDICATION = {2, 0b1100, 2};
        static constexpr Interrupt TRANSMITTER_HOLDING_REGISTER_EMPTY = {3, 0b0010, 3};
        static constexpr Interrupt MODEM_STATUS = {4, 0b0000, 4};
    };
    static constexpr std::array<Interrupt, 5> SORTED_INTERRUPTS = [] {
        std::array interrupts = {Interrupts::RECEIVER_LINE_STATUS, Interrupts::RECEIVED_DATA_AVAILABLE, Interrupts::CHARACTER_TIMEOUT_INDICATION,
            Interrupts::TRANSMITTER_HOLDING_REGISTER_EMPTY, Interrupts::MODEM_STATUS};
        constexpr auto cmp = [](const Interrupt& a, const Interrupt& b) {
            return a.priority < b.priority;
        };
        std::sort(interrupts.begin(), interrupts.end(), cmp);
        return interrupts;
    }();
    void record_overrun_error();
    void record_interrupt(Interrupt interrupt);
    void clear_interrupt(Interrupt interrupt);
    std::bitset<8> m_active_interrupt_map = 0;


    // FIFO-Control-Register
    union FCR {
        struct {
            u8 enable_fifo : 1; // We only implement FIFO-Mode, so this must be 1
            u8 clear_rx_fifo : 1; // If this is 1, rx-fifo is cleared (this bit is self-clearing)
            u8 clear_tx_fifo : 1; // If this is 1, tx-fifo is cleared (this bit is self-clearing)
            u8 : 1; // we don't implement rxrdy/txrdy pins
            u8 : 1; // reserved
            /**
            * When this bit is set 64-byte mode of operation is selected. When cleared, the 16-byte mode is
            * selected. A write to FCR bit 5 is protected by setting the line control register (LCR) bit 7 = 1. LCR bit 7 needs
            * to cleared for normal operation.
            */
            u8 extended_fifo : 1;
            u8 trigger_level_indicator : 2; // 0b00=1, 0b01=4, 0b10=8, 0b11=14,

            u8 trigger_level() const {
                switch (trigger_level_indicator) {
                    case 0b00: return 1;
                    case 0b01: return extended_fifo ? 16 : 4;
                    case 0b10: return extended_fifo ? 32 : 8;
                    case 0b11: return extended_fifo ? 56 : 14;
                    default: fail("invalid trigger level");
                }
            }
        } concrete;
        u8 value;
    };
    FCR m_fcr = {.value = 0b00000001};

    // Interrupt-Enable-Register
    struct IER {
        u8 value;

        bool is_rda_enabled() const { return bits(value, 0, 0); } // Received Data Available (and Character Timeout Indicator in FIFO-mode)
        bool is_thre_enabled() const { return bits(value, 1, 1); } // Transmitter Holding Register Empty
        bool is_rls_enabled() const { return bits(value, 2, 2); } // Receiver Line Status
        bool is_ms_enabled() const { return bits(value, 3, 3); } // Modem status
    };
    IER m_ier = {.value = 0b0000};

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
            /**
            * Transmitter Holding Register Empty (THRE) indicator. Bit 5 indicates that the UART is ready
            * to accept a new character for transmission. The THRE bit is set to a logic 1 when a
            * character is transferred from the Transmitter Holding Register into the Transmitter Shift Register. The bit is reset
            * to logic 0 concurrently with the loading of the Transmitter Holding Register by the CPU. In the FIFO mode this
            * bit is set when the XMIT FIFO is empty; it is cleared when at least 1 byte is written to the XMIT FIFO.
            */
            u8 thre : 1; // Transmitter Holding Register Empty indicator.
            u8 : 1; // Transmitter empty indicator (set if both THR and TSR (Transmitter-Shift-Register) are empty) (not implemented)
        } concrete;
        u8 value;
    };
    LSR m_lsr = {.value = 0b1100000};

    union LCR {
        struct {
            u8 : 2; // Serial Character Word Length (not implemented)
            u8 : 1; // Number of Stop Bits Generated (not implemented)
            u8 : 1; // Parity enable bit (not implemented)
            u8 : 1; // Even parity select bit (not implemented)
            u8 : 1; // Stick parity bit (not implemented)
            u8 : 1; // Break control bit (not implemented)
            /**
            * This bit is the divisor latch access bit (DLAB). Bit 7 must be set to access the divisor latches of the
            * baud generator during a read or write or access bit 5 of the FCR. Bit 7 must be cleared during a read or write
            * to access the receiver buffer, the THR, or the IER.
            */
            u8 dlab : 1;
        } c;
        u8 value;
    };
    LCR m_lcr = {.value = 0b00000000};

    union MCR {
        struct {
            /**
            * When any of bits 0 through 3 is set, the associated output is forced low;
            * a cleared bit forces the associated output high.
            * (we don't implement these)
            */
            u8 dtr : 1; // This bit (DTR) controls the !DTR output.
            u8 rts : 1; // This bit (RTS) controls !RTS output.
            u8 out1 : 1; // This bit (OUT1) controls !OUT1 signal.
            u8 out2 : 1; // This bit (OUT2) controls the !OUT2 signal.
            u8 loop : 1; // This bit (LOOP) provides a local loop back feature for diagnostic testing of the ACE. (not implemented)
            /**
            * This bit (AFE) is the autoflow control enable. When bit 5 is set, the autoflow control,
            * as described in the detailed description, is enabled.
            * The receiver and transmitter interrupts are fully operational.
            * The modem control interrupts are also operational, but the modem control interrupt sources are now the
            * lower four bits of the MCR instead of the four modem control inputs. All interrupts are still controlled by the
            * IER.
            * The ACE flow can be configured by programming bits 1 and 5 of the MCR as shown in Table 8.
            * MCR BIT 5(AFE) | MCR BIT 1(RTS) | ACE FLOW CONFIGURATION
            * 1              | 1              | Auto-RTS and auto-CTS enabled (autoflow control enabled)
            * 1              | 0              | Auto-CTS only enabled
            * 0              | X              | Auto-RTS and auto-CTS disabled
            * When bit 5 of the FCR is cleared, there is a 16-byte AFC. When bit 5 of the FCR is set, there is a 64-byte AFC.
            */
            u8 afe : 1;
        } c;
        u8 value;
    };
    MCR m_mcr = {.value = 0b00000000};

    InplaceFIFO<u8, MAX_FIFO_CAPACITY> m_rx_fifo;
    InplaceFIFO<u8, MAX_FIFO_CAPACITY> m_tx_fifo;

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