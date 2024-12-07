#include "controllers/uart.h"
#include "cpu.h"

using namespace std::literals::chrono_literals;

namespace CPUE {


UARTController::UARTController(CPU* cpu) : m_cpu(cpu) {
    auto handle = m_cpu->pic().request_connection();
    CPUE_ASSERT(handle.has_value(), "UARTController failed to request PIC connection");
    m_pic_handle = handle.value();
    init_mmio_registers();
    m_thread = std::thread(&UARTController::loop, this);
}
UARTController::~UARTController() {
    {
        std::scoped_lock _(m_mutex);
        m_should_stop = true;
    }
    m_thread.join();
}

void UARTController::init_mmio_registers() {
    // THR/RBR
    m_cpu->mmu().mmio().map_mmio_register(MMIO_REG_BASE + 0x0, {
                                                                   .width = ByteWidth::WIDTH_BYTE,
                                                                   .read_func = [](void* data) -> BigEndian<u64> {
                                                                       return static_cast<UARTController*>(data)->read_rbr();
                                                                   },
                                                                   .write_func =
                                                                       [](void* data, BigEndian<u64> value) {
                                                                           static_cast<UARTController*>(data)->write_thr((u8)value.value);
                                                                       },
                                                                   .data = this,
                                                               });

    // IER
    m_cpu->mmu().mmio().map_mmio_register(MMIO_REG_BASE + 0x1, {
                                                                   .width = ByteWidth::WIDTH_BYTE,
                                                                   .read_func = [](void* data) -> BigEndian<u64> {
                                                                       auto* this_ = static_cast<UARTController*>(data);
                                                                       std::scoped_lock _(this_->m_mutex);
                                                                       return this_->m_ier.value;
                                                                   },
                                                                   .write_func =
                                                                       [](void* data, BigEndian<u64> value) {
                                                                           static_cast<UARTController*>(data)->write_ier((u8)value.value);
                                                                       },
                                                                   .data = this,
                                                               });

    // IIR/FCR
    m_cpu->mmu().mmio().map_mmio_register(MMIO_REG_BASE + 0x2, {
                                                                   .width = ByteWidth::WIDTH_BYTE,
                                                                   .read_func = [](void* data) -> BigEndian<u64> {
                                                                       return static_cast<UARTController*>(data)->read_iir();
                                                                   },
                                                                   .write_func =
                                                                       [](void* data, BigEndian<u64> value) {
                                                                           static_cast<UARTController*>(data)->write_fcr((u8)value.value);
                                                                       },
                                                                   .data = this,
                                                               });

    // LSR
    m_cpu->mmu().mmio().map_mmio_register(MMIO_REG_BASE + 0x5, {
                                                                   .width = ByteWidth::WIDTH_BYTE,
                                                                   .read_func = [](void* data) -> BigEndian<u64> {
                                                                       return static_cast<UARTController*>(data)->read_lsr();
                                                                   },
                                                                   .write_func =
                                                                       [](void* data, BigEndian<u64> value) {
                                                                           fail("Attempted to write to LSR which is not allowed");
                                                                       },
                                                                   .data = this,
                                                               });
}


u8 UARTController::read_rbr() {
    std::scoped_lock _(m_mutex);

    // When a timeout interrupt has occurred it is cleared and the timer reset when the CPU reads one character
    // from the RCVR FIFO.
    m_last_cpu_read_time_point = std::chrono::steady_clock::now();
    clear_interrupt(Interrupts::CHARACTER_TIMEOUT_INDICATION);

    auto opt = m_rx_fifo.take_first();
    update_data_ready_bit_and_rda_interrupt_if_necessary();
    // if no bytes are available to read, the returned value is undefined (simply return 0).
    // callers should check the dr bit beforehand to know whether data is available to read or not.
    return opt.value_or(0);
}
void UARTController::write_thr(u8 value) {
    std::scoped_lock _(m_mutex);
    assert_in_fifo_mode();
    if (!m_tx_fifo.enqueue(value))
        return record_overrun_error();
    update_thre_interrupt_if_necessary();
    cv.notify_one();
}
void UARTController::write_ier(u8 value) {
    std::scoped_lock _(m_mutex);
    m_ier.value = value;
    if (m_ier.is_rda_enabled()) {
        clear_interrupt(Interrupts::RECEIVED_DATA_AVAILABLE);
        clear_interrupt(Interrupts::CHARACTER_TIMEOUT_INDICATION);
    }
    if (m_ier.is_thre_enabled())
        clear_interrupt(Interrupts::TRANSMITTER_HOLDING_REGISTER_EMPTY);
    if (m_ier.is_rls_enabled())
        clear_interrupt(Interrupts::RECEIVER_LINE_STATUS);
    if (m_ier.is_ms_enabled())
        clear_interrupt(Interrupts::MODEM_STATUS);
}
u8 UARTController::read_iir() {
    std::scoped_lock _(m_mutex);
    /**
     * When the CPU accesses the IIR, the UART freezes all interrupts and indicates the highest priority pending
     * interrupt to the CPU. While this CPU access is occurring, the UART records new interrupts, but does not change
     * its current indication until the access is complete. Table 1 shows the contents of the IIR. Details on each bit
     * follow.
     */
    while (!m_interrupt_queue.empty() && !m_active_interrupt_map.test(m_interrupt_queue.top().number)) {
        m_interrupt_queue.pop();
    }
    if (m_interrupt_queue.empty())
        return 1;
    auto i = m_interrupt_queue.top();
    if (i == Interrupts::TRANSMITTER_HOLDING_REGISTER_EMPTY)
        clear_interrupt(i);
    return i.iir_value;
}
void UARTController::write_fcr(u8 value) {
    std::scoped_lock _(m_mutex);
    m_fcr.value = value;
    if (m_fcr.concrete.clear_rx_fifo) {
        m_rx_fifo.clear();
        update_data_ready_bit_and_rda_interrupt_if_necessary();
    }
    if (m_fcr.concrete.clear_tx_fifo) {
        m_tx_fifo.clear();
        update_thre_interrupt_if_necessary();
    }
    m_fcr.concrete.clear_rx_fifo = m_fcr.concrete.clear_tx_fifo = 0;
    assert_in_fifo_mode();
}
u8 UARTController::read_lsr() {
    std::scoped_lock _(m_mutex);
    // The OE indicator is set to a logic 1 upon detection of an overrun condition and
    // reset whenever the CPU reads the contents of the Line Status Register
    m_lsr.concrete.oe = 0;
    clear_interrupt(Interrupts::RECEIVER_LINE_STATUS);
    return m_lsr.value;
}

void UARTController::rx(char const c) {
    std::scoped_lock _(m_mutex);

    // When a timeout interrupt has not occurred the timeout timer is reset after a new character is received or after
    // the CPU reads the RCVR FIFO.
    if (!m_active_interrupt_map.test(Interrupts::CHARACTER_TIMEOUT_INDICATION.number))
        m_last_char_received_time_point = std::chrono::steady_clock::now();

    if (!m_rx_fifo.enqueue(c))
        return record_overrun_error();
    update_data_ready_bit_and_rda_interrupt_if_necessary();
    cv.notify_one();
}

void UARTController::record_overrun_error() {
    std::scoped_lock _(m_mutex);
    m_lsr.concrete.oe = 1;
    if (m_ier.is_rls_enabled()) {
        record_interrupt(Interrupts::RECEIVER_LINE_STATUS);
    }
}

void UARTController::record_interrupt(Interrupt interrupt) {
    CPUE_ASSERT(m_interrupt_queue.size() < MAX_PENDING_INTERRUPTS, "Too many pending interrupts in UARTController.");
    // don't insert the same interrupt twice.
    if (m_active_interrupt_map.test(interrupt.number))
        return;
    m_interrupt_queue.push(interrupt);
    m_active_interrupt_map.set(interrupt.number);
    m_cpu->pic().raise_irq_pin(m_pic_handle);
}

void UARTController::clear_interrupt(Interrupt interrupt) {
    m_active_interrupt_map.reset(interrupt.number);
    if (m_active_interrupt_map.none())
        m_cpu->pic().clear_irq_pin(m_pic_handle);
}


void UARTController::loop() {
    for (;;) {
        std::unique_lock lk(m_mutex);
        cv.wait(lk, [this] {
            return !m_tx_fifo.is_empty() || !m_rx_fifo.is_empty() || m_should_stop;
        });
        // after the wait, we own the lock

        if (m_should_stop)
            break;

        // send all enqueued bytes
        while (!m_tx_fifo.is_empty()) {
            tx(m_tx_fifo.take_first().value());
        }
        update_thre_interrupt_if_necessary();

        if (!m_rx_fifo.is_empty()) {
            auto cpue_read_time_diff = std::chrono::steady_clock::now() - m_last_cpu_read_time_point;
            auto char_received_time_diff = std::chrono::steady_clock::now() - m_last_char_received_time_point;
            if (m_ier.is_rda_enabled() && (cpue_read_time_diff > 500ms || char_received_time_diff > 500ms)) {
                record_interrupt(Interrupts::CHARACTER_TIMEOUT_INDICATION);
            }
        }
    }
}


}