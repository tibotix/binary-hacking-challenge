#!/usr/bin/env bash

set -e

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
NPROCS=$(nproc --all)

cd "${SCRIPT_DIR}"

GIT_URL="https://github.com/skx/lighthouse-of-doom.git"
git clone "${GIT_URL}" && cd lighthouse-of-doom
git checkout 5ec5c12

# Apply patch file if any exist
if ls ../*.patch 1> /dev/null 2>&1; then
    patch -p1 < ../*.patch
fi

TOOLCHAIN_DIR="$(cd -- "${SCRIPT_DIR}/../../toolchain" && pwd)"
make lighthouse CC="${TOOLCHAIN_DIR}/minik-x64-musl-tcc" -j"${NPROCS}"
