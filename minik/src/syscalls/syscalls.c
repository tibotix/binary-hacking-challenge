#include <minik/syscalls.h>

void syscall_isr(u64 error_code, saved_user_regs *regs) {
  u64 syscall_nr = regs->rax;
  if (syscall_nr < sizeof(syscall_table)) {
    syscall_handler sys_handler = syscall_table[syscall_nr];
    regs->rax = sys_handler(regs->rdi, regs->rsi, regs->rdx, regs->rcx,
                            regs->r8, regs->r9);
  }
  SEND_EOI();
}

void minik_init_syscalls(void) {
  minik_register_isr_handler(SYSCALL_IRQ, &syscall_isr);
}
