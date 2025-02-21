void _start() {
    int a = 0;
    for (int i = 0; i < 10; ++i) {
        a += i;
    }

    __asm__ volatile("mov %0, %%rdi" // Lade den Wert von `my_value` in das `rdi`-Register
                     :
                     : "r"(a) // `%0` wird durch den Wert von `my_value` ersetzt
                     : "rdi" // `rdi` wird verändert (Clobber)
    );

    __asm("hlt");
}
