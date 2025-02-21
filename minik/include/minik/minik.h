#ifndef MINIK_H
#define MINIK_H

#include "interrupts.h"
#include "io.h"
#include "syscalls.h"

inline void minik_init(void) {
  minik_init_interrupts();
  minik_init_io();
  minik_init_syscalls();
}

inline void minik_exit(void) { __asm__ volatile("hlt"); }

#endif // MINIK_H
