#!/usr/bin/env python

import os
from tempfile import NamedTemporaryFile
from base64 import b64decode


def main():
    print("Welcome to the ultra-secure, blazingly-fast, x86 VM.")
    ram = input("How much RAM do you want to have? (MB): ")

    try:
        ram = int(ram)
    except ValueError:
        print("Invalid RAM value :( - bye")
        return

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
        os.system(f"/app/cpue --kernel=none -r'{ram}' '{f.name}' 2>/dev/null")


if __name__ == "__main__":
    main()