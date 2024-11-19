#!/usr/bin/env python

import os
from tempfile import NamedTemporaryFile
from base64 import b64decode


def main():
    print("Welcome to the ultra-secure, blazingly-fast, x86 VM.")
    print("Please input your ELF binary base64-encoded, followed by END")

    # Read in ELF
    elf = b""
    while True:
        line = input().strip().encode()

        # Break at end
        if line == b"END":
            break

        elf += line

    try:
        elf = b64decode(elf)
    except:
        print("Invalid base64 :( - bye")
        return

    with NamedTemporaryFile() as f:
        # Write to file
        f.write(elf)
        f.flush()

        # Emulate
        os.system(f"/app/cpue '{f.name}'")


if __name__ == "__main__":
    main()