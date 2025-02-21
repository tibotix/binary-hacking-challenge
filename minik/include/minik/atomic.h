#ifndef KATOMIC_H
#define KATOMIC_H

#include "kmalloc.h"
#include "stl.h"

#define CLI() __asm__ __volatile__("cli")
#define STI() __asm__ __volatile__("sti")

/**
 * Returns 64bit read from *addr.
 */
inline u64 atomic_read64(void *addr) {
  u64 value;
  __asm__ volatile("movq (%1), %0" : "=r"(value) : "r"(addr) : "memory");
  return value;
}

/**
 * Returns 8bit read from *addr.
 */
inline u8 atomic_read8(void *addr) {
  u8 value;
  __asm__ volatile("movb (%1), %0" : "=r"(value) : "r"(addr) : "memory");
  return value;
}

/**
 * Writes 64bits of value to *addr.
 */
inline void atomic_write64(void *addr, u64 value) {
  __asm__ volatile("movq %1, (%0)" : : "r"(addr), "r"(value) : "memory");
}

/**
 * Writes 8bits of value to *addr.
 */
inline void atomic_write8(void *addr, u8 value) {
  __asm__ volatile("movb %1, (%0)" : : "r"(addr), "r"(value) : "memory");
}

/**
 * Compares *addr with expected and...
 *  - if they match, loads new_value into *addr
 *  - else, loads new_value into expected
 * Returns 1 if *addr matched with expected, 0 otherwise
 */
inline u8 compare_and_swap64(u64 *addr, u64 expected, u64 new_value) {
  u8 success;
  __asm__ volatile("lock cmpxchg %3, %1 \n"
                   "sete %0"
                   : "=r"(success), "+m"(*addr), "+a"(expected)
                   : "r"(new_value)
                   : "memory", "cc");
  return success;
}

/**
 * Compares *addr with expected and...
 *  - if they match, loads new_value into *addr
 *  - else, loads new_value into expected
 * Returns 1 if *addr matched with expected, 0 otherwise
 */
inline u8 compare_and_swap8(u8 *addr, u8 expected, u8 new_value) {
  u8 success;
  __asm__ volatile("lock cmpxchg %3, %1 \n"
                   "sete %0"
                   : "=r"(success), "+m"(*addr), "+a"(expected)
                   : "r"(new_value)
                   : "memory", "cc");
  return success;
}

/*
 * Exchanges *addr with val.
 * Returns the old contents of *addr.
 */
inline u8 test_and_set8(u8 *addr, u8 val) {
  asm volatile("xchg %0, %1" : "+r"(val), "+m"(*addr) : : "memory");
  return val;
}

typedef struct {
  u8 val;
} spinlock;

inline spinlock *spinlock_new() {
  spinlock *lock = (spinlock *)kmalloc(sizeof(spinlock));
  lock->val = 0;
  return lock;
}
/*
 * Try to acquire the spinlock.
 * Returns true if the lock was successfully acquired, false otherwise.
 */
inline bool spinlock_try_lock(spinlock *lock) {
  return test_and_set8(&lock->val, 1) == 0;
}
inline void spinlock_lock(spinlock *lock) {
  while (!spinlock_try_lock(lock)) {
  }
}
inline void spinlock_unlock(spinlock *lock) { test_and_set8(&lock->val, 0); }

#endif