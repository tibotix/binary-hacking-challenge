

#include "emulator.h"



int main(int argc, char** argv) {
    CPUE::Emulator e;
    e.load_elf();
    e.start();
}