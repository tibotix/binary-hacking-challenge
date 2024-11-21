#include <stdio.h>
#include <inttypes.h>

#include <capstone/capstone.h>
#include <elfio/elfio.hpp>

#include "cpu.h"

#define CODE "\x55\x48\x8b\x05\xb8\x13\x00\x00"

int test(int argc, char** argv) {
    csh handle;
    cs_insn* insn;
    size_t count;

    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK)
        return -1;
    count = cs_disasm(handle, (uint8_t const*)CODE, sizeof(CODE) - 1, 0x1000, 0, &insn);
    if (count > 0) {
        size_t j;
        for (j = 0; j < count; j++) {
            printf("0x%" PRIx64 ":\t%s\t\t%s\n", insn[j].address, insn[j].mnemonic, insn[j].op_str);
        }

        cs_free(insn, count);
    } else
        printf("ERROR: Failed to disassemble given code!\n");

    cs_close(&handle);

    // Create elfio reader
    ELFIO::elfio reader;
    // Load ELF data
    if (!reader.load(argv[1])) {
        std::cout << "Can't find or process ELF file " << argv[1] << std::endl;
        return 2;
    }

    return 0;
}