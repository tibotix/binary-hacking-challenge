#include <minik/syscalls.h>

#include <minik/io.h>

u64 sys$readv(int fd, const struct iovec *iov, int iovcnt) {
  // TODO: check fd
  u64 count = 0;
  u64 c = 0;
  for (u64 i = 0; i < iovcnt; i++) {
    char *base = iov[i].iov_base;
    c = minik_read_until_newline(base, iov[i].iov_len);
    count += c;
    if (c > 0 && base[c - 1] == '\n')
      break;
  }
  return count;
}