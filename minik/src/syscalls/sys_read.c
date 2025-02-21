#include <minik/syscalls.h>

#include <minik/io.h>

u64 sys$read(int fd, void *buf, u64 count) {
  if (fd == 0) {
    // activate interrupts so we can receive additional UART-RX interrupts
    // while waiting for characters to be input.
    STI();
    /*
     * Note:
     * we read until newline to emulate the _terminal line buffering_.
     */
    count = minik_read_until_newline(buf, count);
    return count;
  }
  return 0;
}
