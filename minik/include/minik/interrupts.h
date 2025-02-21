#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "paging.h"
#include "stl.h"

// PIC MMIO:
u8 PIC_MMIO_PAGE[PAGE_SIZE] __attribute__((__aligned__(PAGE_SIZE)));
static u64 PIC_MMIO_PAGE_FRAME = 0xff00f; // addr is 0xff00'ff00
#define MMIO_EOI_ADDR ({ (u8 *)((u64)(&PIC_MMIO_PAGE) + 0xf00); })
#define MMIO_ICW4_ADDR ({ (u8 *)((u64)(&PIC_MMIO_PAGE) + 0xf01); })
#define ENABLE_AEOI() ({ *MMIO_ICW4_ADDR |= 0b10; })
#define DISABLE_AEOI() ({ *MMIO_ICW4_ADDR &= ~(0b10); })
#define SEND_EOI() ({ *MMIO_EOI_ADDR = 0x0; })

struct idt_register {
  u16 limit;
  u64 base;
} __attribute__((packed));

inline struct idt_register minik_get_idtr(void) {
  struct idt_register idtr;
  __asm__ volatile("sidt %0" : "=m"(idtr));
  return idtr;
}

typedef struct __attribute__((__aligned__(8))) {
  u64 rax;
  u64 rbx;
  u64 rbp;
  u64 rcx;
  u64 rdx;
  u64 rdi;
  u64 rsi;
  u64 r8;
  u64 r9;
  u64 r10;
  u64 r11;
  u64 r12;
  u64 r13;
  u64 r14;
  u64 r15;
} saved_user_regs;

static void isr_default_handler(u64 error_code, saved_user_regs *regs) {
  return;
}
extern void ihandler_tramp();
typedef void (*isr_handler)(u64, saved_user_regs *);
struct isr {
  isr_handler handler;
};

struct isr isr_table[0xff];

void minik_init_interrupts(void);
inline void minik_register_isr_handler(u8 irq, isr_handler handler) {
  isr_table[irq].handler = handler;
}

#endif // INTERRUPTS_H
