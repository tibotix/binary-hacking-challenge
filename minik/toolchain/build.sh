#!/usr/bin/env bash

set -e

TCC_REPO_URL="https://repo.or.cz/tinycc.git"
MUSL_TCC_REPO_URL="https://github.com/GataOS/musl-tcc.git"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
NPROCS=$(nproc --all)

# Clone TCC
cd "${SCRIPT_DIR}"
git clone "${TCC_REPO_URL}" tinycc && cd tinycc
git checkout 2d210fef

# Build TCC
./configure
make -j"${NPROCS}"
TCC_DIR="${SCRIPT_DIR}/tinycc"

# Clone musl-tcc
cd "${SCRIPT_DIR}"
git clone "${MUSL_TCC_REPO_URL}" musl-tcc && cd musl-tcc

# Apply patch file if one exists
if ls ../*.patch 1> /dev/null 2>&1; then
    patch -p1 < ../*.patch
fi

# Build musl-tcc
./configure --target=x86_64 \
    CC="${TCC_DIR}/tcc" \
    AR="${TCC_DIR}/tcc -ar" \
    RANLIB="echo" \
    LIBCC="${TCC_DIR}/libtcc1.a"

make -j"${NPROCS}"
MUSL_TCC_DIR="${SCRIPT_DIR}/musl-tcc"


LDSCRIPT="${SCRIPT_DIR}/ldscript.ld"
CFLAGS="-I ${MUSL_TCC_DIR}/include -I ${MUSL_TCC_DIR}/obj/include -I ${MUSL_TCC_DIR}/arch/x86_64 -I ${MUSL_TCC_DIR}/arch/generic -nostdinc -g -O0"
CC="${TCC_DIR}/tcc"
LDFLAGS="-nostdlib -static ${MUSL_TCC_DIR}/lib/Scrt1.o ${LDSCRIPT}"
LD="${TCC_DIR}/tcc"
# Create tcc wrapper script
TCC_WRAPPER_SCRIPT="${SCRIPT_DIR}/minik-x64-musl-tcc"
echo "#!/usr/bin/env bash" > "${TCC_WRAPPER_SCRIPT}"
echo "${CC} \$@ ${CFLAGS} ${LDFLAGS}" >> "${TCC_WRAPPER_SCRIPT}"
chmod +x "${TCC_WRAPPER_SCRIPT}"
# Create ld wrapper script
TCC_LD_WRAPPER_SCRIPT="${SCRIPT_DIR}/minik-x64-musl-tcc-ld"
echo "#!/usr/bin/env bash" > "${TCC_LD_WRAPPER_SCRIPT}"
echo "${LD} \$@ ${LDFLAGS}" >> "${TCC_LD_WRAPPER_SCRIPT}"
chmod +x "${TCC_LD_WRAPPER_SCRIPT}"
# Create ldscript.ld
echo "GROUP ( ${MUSL_TCC_DIR}/lib/libc.a ${TCC_DIR}/libtcc1.a )" > "${LDSCRIPT}"




