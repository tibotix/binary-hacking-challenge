


# Building

```sh
git clone https://github.com/tibotix/binary-hacking-challenge.git
cd binary-hacking-challenge
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
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
            virtual adress (segmentation)
            
        - Need to add i/o-Instructions:
            1 byte, 2 byte, 4 byte, 8byte

# Optional
    swapping as array or directory
    
