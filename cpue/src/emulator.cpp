#include "emulator.h"

#include "common.h"
#include "devices/vt100.h"
#include "uefi.h"


namespace CPUE {


void Emulator::start() const {
    CPU cpu;

    // configure long-mode and init kernel
    UEFI uefi{&cpu};
    uefi.prepare_long_mode();
    CPUE_ASSERT(m_kernel.init(), "Failed to initialize kernel");

    //    CPUE_ASSERT(m_binary.load(), "Failed to read binary file");
    //    auto data = m_binary.read();
    TODO_NOFAIL("map binary to memory");


    // create VT100 terminal and run it
    VT100 vt100;
    connect_uart_devices(cpu.uart1(), vt100);
    vt100.start_loop_thread();

    // start cpu loop, let's go!
    cpu.interpreter_loop();
}



}