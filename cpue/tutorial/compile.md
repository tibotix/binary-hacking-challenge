
### With no-pie:
gcc stack3.c -o stack3 -no-pie

### With no stack canary:
gcc stack2.c -o stack2 -no-pie -fno-stack-protector

### With full RELRO:
gcc stack1.c -o stack1 -Wl,-z,relro,-z,now -no-pie -fno-stack-protector
