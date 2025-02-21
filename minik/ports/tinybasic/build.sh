#!/usr/bin/env bash

set -e

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
NPROCS=$(nproc --all)

cd "${SCRIPT_DIR}"

GIT_URL="https://github.com/eddavis2/Tiny-Basic.git"
git clone "${GIT_URL}" && cd Tiny-Basic
git checkout 27435e6

# Apply patch file if any exist
if ls ../*.patch 1> /dev/null 2>&1; then
    patch -p1 < ../*.patch
fi

TOOLCHAIN_DIR="$(cd -- "${SCRIPT_DIR}/../../toolchain" && pwd)"
${TOOLCHAIN_DIR}/minik-x64-musl-tcc C/basic.c -o C/basic
