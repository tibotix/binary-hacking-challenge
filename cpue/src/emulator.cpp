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
    uefi.prepare_long_mode(top);
    CPUE_ASSERT(m_kernel.init(cpu, top), "Failed to initialize kernel");

    // load user elf
    Loader loader{cpu};
    loader.load_user_elf(&m_binary, top);

    // start kernel (set rip, etc..)
    m_kernel.start(cpu, m_binary.entry_point());

    // create VT100 terminal and start it if requested
    VT100 vt100;
    if (m_serial) {
        connect_uart_devices(cpu.uart1(), vt100);
        vt100.start_loop_thread();
    }

    // start cpu loop, let's go!
    cpu.interpreter_loop();
}



}