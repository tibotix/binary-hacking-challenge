
This is a small *minik*ernel that can run under the cpue VM.
It has basic functionality for I/O and memory mapping APIs.
It provides syscalls using IDT entry 0x80.
Implemented syscalls:
- write
- writev
- read
- readv

Tested with musl-libc using a patched version of tinycc to emit `int 0x80` instructions instead of `syscall` for 64-bit syscalls.
You can find demos under /demos.

# Building toolchain

```sh
cd toolchain && ./build.sh
```

# Building minik

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8
```
