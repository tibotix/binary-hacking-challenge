#include <minik/math.h>

u64 log2(u64 val) { return log(val, 2); }

u64 log10(u64 val) { return log(val, 10); }

u64 log(u64 n, u64 base) {
  u64 log = 0;
  if (base < 2)
    return 0;

  while (n >= base) {
    n /= base;
    log++;
  }
  return log;
}

u64 align_to_next_pow2(u64 val) {
  if (val == 0)
    return 0;
  val--;
  val |= val >> 1;
  val |= val >> 2;
  val |= val >> 4;
  val |= val >> 8;
  val |= val >> 16;
  val |= val >> 32;
  return val + 1;
}
