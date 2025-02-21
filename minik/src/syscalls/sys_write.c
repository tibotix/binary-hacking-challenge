#include <minik/syscalls.h>

#include <minik/io.h>

u64 sys$write(int fd, void const *buf, u64 count) {
  if (fd == 1 || fd == 2) {
    minik_write(buf, count);
    return count;
  }
  return 0;
}
