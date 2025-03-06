

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