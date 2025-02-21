#include <minik/stl.h>

u64 strlen(char const *s) {
  u64 i = 0;
  while (s[i] != '\0') {
    ++i;
  }
  return i;
}

void *memmove(void *dest, const void *src, u64 n) {
  u8 *d = (u8 *)dest;
  u8 const *s = (u8 const *)src;
  if (d < s) {
    while (n--) {
      *d++ = *s++;
    }
  } else {
    d += n;
    s += n;
    while (n--) {
      *(--d) = *(--s);
    }
  }
  return dest;
}

void *memset(void *dest, int c, u64 n) {
  u8 *ptr = (u8 *)dest;
  while (n--) {
    *ptr++ = (u8)c;
  }
  return dest;
}

void assert(bool c) {
  if (!c)
    exit(1);
}

void exit(int code) { __asm__("hlt"); }

char *ultoa(u64 value, char *str, int base) {
  const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
  int i, j;
  unsigned remainder;
  char c;

  /* Check base is supported. */
  if ((base < 2) || (base > 36)) {
    str[0] = '\0';
    return nullptr;
  }

  /* Convert to string. Digits are in reverse order.  */
  i = 0;
  do {
    remainder = value % base;
    str[i++] = digits[remainder];
    value = value / base;
  } while (value != 0);
  str[i] = '\0';

  /* Reverse string.  */
  for (j = 0, i--; j < i; j++, i--) {
    c = str[j];
    str[j] = str[i];
    str[i] = c;
  }

  return str;
}

u64 strtoull(const char *nptr, char **endptr, int base) {
  const char *s = nptr;
  u64 acc;
  int c;
  u64 cutoff;
  int neg = 0, any, cutlim;

  /*
   * See strtol for comments as to the logic used.
   */
  do {
    c = *s++;
  } while (ISSPACE(c));
  if (c == '-') {
    neg = 1;
    c = *s++;
  } else if (c == '+')
    c = *s++;
  if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
    c = s[1];
    s += 2;
    base = 16;
  }
  if (base == 0)
    base = c == '0' ? 8 : 10;
  cutoff = (u64)ULLONG_MAX / (u64)base;
  cutlim = (u64)ULLONG_MAX % (u64)base;
  for (acc = 0, any = 0;; c = *s++) {
    if (ISDIGIT(c))
      c -= '0';
    else if (ISALPHA(c))
      c -= ISUPPER(c) ? 'A' - 10 : 'a' - 10;
    else
      break;
    if (c >= base)
      break;
    if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
      any = -1;
    else {
      any = 1;
      acc *= base;
      acc += c;
    }
  }
  if (any < 0) {
    acc = ULLONG_MAX;
    //	    errno = ERANGE;
  } else if (neg)
    acc = -acc;
  if (endptr != 0)
    *endptr = (char *)(any ? s - 1 : nptr);
  return (acc);
}

u64 bitmask(u8 high) { return (high >= 64) ? ~0ULL : ((u64)(1) << high) - 1; }
u64 bytemask(u8 high) { return bitmask(high * 8); }
u64 bits(u64 val, u8 high, u8 low) { return (val & bitmask(high + 1)) >> low; }
