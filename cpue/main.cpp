

#include "emulator.h"
#include "mmu.h"
#include "cpu.h"
#include "devices/vt100.h"


int main(int argc, char** argv) {
    CPUE::CPU cpu{};
    CPUE::MMU mmu{&cpu, 1};
    CPUE::VT100 vt100;
    CPUE::connect_uart_devices(cpu.uart1(), vt100);
    vt100.start_loop_thread();
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    // CPUE::Emulator e;
    // e.load_elf();
    // e.start();
}