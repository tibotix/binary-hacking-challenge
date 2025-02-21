#include <minik/minik.h>
#include <minik/stl.h>

void _start() {
  minik_init();

  char const *msg = "Hello World";
  u64 size = strlen(msg);
  u64 ret;

  minik_write_str("Performing syscall write(1, '");
  minik_write_str(msg);
  minik_write_str("', ");
  minik_write_ull(size, 10);
  minik_write_str(")\n");

  __asm__ __volatile__("mov $1, %%rax\n" // rax = 1 (syscall write)
                       "mov $1, %%rdi\n" // rdi = fd (stdout)
                       "int $0x80\n"     // Syscall ausführen
                       : "=a"(ret)
                       : "S"(msg), "d"(size) // Eingaben
                       : "rdi", "memory"     // Clobber-Register
  );

  minik_write_str("\nret: ");
  minik_write_ull(ret, 10);
  minik_write_str("\n");

  minik_exit();
}
