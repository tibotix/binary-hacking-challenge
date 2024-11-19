


# Building

```sh
git clone https://github.com/tibotix/binary-hacking-challenge.git
cd binary-hacking-challenge
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8
```