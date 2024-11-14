

- basic stack overflow (no pie, no canary)
- format string leaks (leak canary, leak aslr -> base address libc/own binary)
- canary bruteforce (forking server)
- canary leak through format string
- ROP -> ret2libc system
- GOT overwrite (__stack_chk_fail also part of GOT) (only when partial RELRO)

NX
RELRO (GOT Overwrite)
Canary (stack protector)
PIE (ASLR)



ASLR Leak
Canary bypassen: Leak
ROP -> ret2libc

Integer overflow -> OOB Write


Idea:
 - numerical instable algo
 - calculator
 - terminal game
