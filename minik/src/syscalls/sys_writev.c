#include <minik/syscalls.h>

u64 sys$writev(int fd, const struct iovec *iov, int iovcnt) {
  u64 count = 0;
  for (u64 i = 0; i < iovcnt; i++) {
    count += sys$write(fd, iov[i].iov_base, iov[i].iov_len);
  }
  return count;
}
