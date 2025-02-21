#ifndef STL_H
#define STL_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed long long i64;

#define bool u8
#define true 1
#define false 0
#define nullptr 0x0

#define INT3() __asm__("int3")
#define NOP() __asm__("nop")

u64 strlen(char const *s);
void *memmove(void *dest, const void *src, u64 n);
void *memset(void *dest, int c, u64 n);
void assert(bool c);
void exit(int code);

#define ULLONG_MAX (~(u64)0)
#define ISSPACE(c)                                                             \
  ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r' || (c) == '\v' ||   \
   (c) == '\f')
#define ISALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define ISDIGIT(c) ((c) >= '0' && (c) <= '9')
#define ISUPPER(c) ((c) >= 'A' && (c) <= 'Z')

#define INTEGRAL_CONV_MAX_BUFFER_SIZE(bitsize, base)                           \
  (log(bitsize >= 64 ? ~((u64)0) : (((u64)1 << bitsize) - 1), base) + 2)
#define ULTOA_MAX_BUFFER_SIZE(base) (INTEGRAL_CONV_MAX_BUFFER_SIZE(64, base))
char *ultoa(u64 value, char *str, int base);
u64 strtoull(const char *str, char **endptr, int base);

inline u64 bitmask(u8 high);
inline u64 bytemask(u8 high);
inline u64 bits(u64 val, u8 high, u8 low);

inline void spin_wait(u64 nrounds) {
  while (nrounds--) {
  }
}

#endif // STL_H
