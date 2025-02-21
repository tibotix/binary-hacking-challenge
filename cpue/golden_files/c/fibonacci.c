int fibo(int n) {
    if (n == 0 || n == 1) // Base condition
        return n;
    else
        return fibo(n - 1) + fibo(n - 2);
}


void _start() {
    int n = 15;

    int fibonacci = fibo(n);
    __asm__ volatile("mov %0, %%rax" : : "r"(fibonacci) : "rax");
    __asm__("hlt");
}
