

#include "emulator.h"
#include "mmu.h"
#include "cpu.h"


int main(int argc, char** argv) {
    CPUE::CPU cpu{};
    CPUE::MMU mmu{&cpu, 1};
    mmu.init();
    // CPUE::Emulator e;
    // e.load_elf();
    // e.start();
}