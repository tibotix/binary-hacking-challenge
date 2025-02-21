#include "emulator.h"

#include "common.h"
#include "devices/vt100.h"
#include "uefi.h"


namespace CPUE {


void Emulator::start() {
    CPU cpu{m_available_pages};

    // configure long-mode and init kernel
    u64 top = 0;
    UEFI uefi{&cpu};
    CPUE_TRACE("preparing long mode...");
    uefi.prepare_long_mode(top);
    CPUE_TRACE("initializing kernel...");
    CPUE_ASSERT(m_kernel.init(cpu, top), "Failed to initialize kernel");

    // load user elf
    CPUE_TRACE("Loading user elf...");
    Loader loader{cpu};
    loader.load_user_elf(&m_binary, top);

    // start kernel (set rip, etc..)
    CPUE_TRACE("Starting kernel...");
    // TODO: start at real entry point if we have enough instructions+syscalls support.
    auto user_binary_entry_point = m_binary.find_symbol_address("main").value_or(m_binary.entry_point());
    m_kernel.start(cpu, user_binary_entry_point);

    // create VT100 terminal and start it if requested
    VT100 vt100;
    if (m_serial) {
        connect_uart_devices(cpu.uart0(), vt100);
        vt100.start_loop_thread();
    }

    // start cpu loop, let's go!
    cpu.interpreter_loop();
}



}