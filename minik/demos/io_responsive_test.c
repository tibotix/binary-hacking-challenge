#include <minik/atomic.h>
#include <minik/fifo.h>
#include <minik/interrupts.h>
#include <minik/io.h>
#include <minik/kmalloc.h>
#include <minik/minik.h>
#include <minik/stl.h>

void _start() {
  minik_init();

  minik_write_str(
      "Please type something very fast, and see how quick minik reacts.\n");
  minik_write_str(
      "For every character c you type it'll print a corresponding [c].\n");
  minik_write_str("The latest [c] seen on the screen should always be the "
                  "latest character you wrote.\n\n");

  char input;
  while (1) {
    minik_read(&input, 1);
    minik_write_str("\n[");
    minik_writec(input);
    minik_write_str("]");
  }

  minik_exit();
}