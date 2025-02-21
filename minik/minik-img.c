#include <minik/minik.h>

typedef void (*main_func)(int, char **);

void _start(void *user_binary_entry_point) {
  minik_init();

  // TODO: switch to ring3
  char const *progname = "progname";
  int argc = 1;
  char *argv[0x10] = {(char *)progname};
  minik_write_str(
      "You are about to call the entry point of your binary-elf.\n");
  minik_write_str("How many arguments do you want to pass to it? ");
  char input[0x10];
  u64 chars_read = minik_read_until_newline(input, 0x10);
  char *endptr = (char *)input + chars_read;
  argc += strtoull(input, &endptr, 10);
  if (argc < 0 || argc >= sizeof(argv) - 1)
    minik_exit();
  for (u64 i = 1; i < argc; i++) {
    minik_write_str("Please input argument ");
    minik_write_ull(i, 10);
    minik_write_str(": ");
    char *buffer = kmalloc(0x50);
    u64 count = minik_read_until_newline(buffer, 0x50);
    if (count > 0)
      buffer[count - 1] = 0;
    argv[i] = buffer;
  }
  minik_write_str("Let's go :)\n\n\n");
  argv[argc] = nullptr;

  ((main_func)user_binary_entry_point)(argc, argv);
  minik_exit();
}