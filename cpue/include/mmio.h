#pragma once

#include "common.h"

namespace CPUE {


typedef void (*read)(int);

struct MMIORegister {
    ByteWidth width;
    void write(u64 value);
    u64 read();
};



}