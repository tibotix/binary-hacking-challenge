


# Building

```sh
git clone https://github.com/tibotix/binary-hacking-challenge.git
cd binary-hacking-challenge
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=1 .. # -DENABLE_TRACE=1 to enable tracing
make -j8
```

# What do we need
    Test-cases
    MMU
    TLB && update-TLB
    CPU-Loop
    Interpreter loop
    elfparses

# MMU
    paging -> Page-table
        - page-size
        - hierarchy
        - API's:
            mapping memory space (mmap)
            virtual address (segmentation)

        - Need to add i/o-Instructions:
            1 byte, 2 byte, 4 byte, 8byte

# Optional
    swapping as array or directory


# Testing Framework
https://github.com/catchorg/Catch2


# Useful Links

https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
https://www.felixcloutier.com/x86/

https://github.com/tibotix/Speicherverwaltung
