#include <cstdio>
#include <stdlib.h>
#include <cstdint>


namespace CPUE {


typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;



void fail(char const* msg) {
    if (msg != NULL) {
        printf("%s\n");
    }
    exit(1);
}

}