#include <minik/syscalls.h>

#define TIOCGWINSZ 0x5413

int sys$ioctl(int fd, int op, ...) {
  if (op == TIOCGWINSZ) {
    // get terminal size
    return 0;
  }
  __asm__("hlt");
  return 0;
}