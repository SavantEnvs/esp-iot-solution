#!/usr/bin/env bash
#
# mayhem/build.sh — build the ELF-loader fuzz harnesses (sanitized, libFuzzer + standalone) and the
# known-answer oracle (normal flags) for components/elf_loader. Runs in /mayhem as `mayhem`.
# Re-runnable and offline: only the in-tree sources and the base toolchain are used.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}"
: "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
: "${MAYHEM_JOBS:=$(nproc)}"
export SANITIZER_FLAGS DEBUG_FLAGS CC LIB_FUZZING_ENGINE MAYHEM_JOBS

SRC="${SRC:-/mayhem}"
cd "$SRC"

EL=components/elf_loader
INC="-I$EL/tests/host/include -I$EL/include -Imayhem"
COMMON="-std=gnu11 -D_GNU_SOURCE -DCONFIG_ELF_DYNAMIC_LOAD_SHARED_OBJECT=1 -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast"
ARCH_FLAGS_xtensa="-DCONFIG_IDF_TARGET_ARCH_XTENSA=1 -DCONFIG_ELF_LOADER_BUS_ADDRESS_MIRROR=1"
ARCH_FLAGS_riscv="-DCONFIG_IDF_TARGET_ARCH_RISCV=1"

OBJ="$(mktemp -d)"
trap 'rm -rf "$OBJ"' EXIT

# Sanitized + coverage-instrumented objects. fuzzer-no-link instruments the library code itself
# (the engine is only linked into the harness), and stays on when SANITIZER_FLAGS is empty.
FUZZ_CFLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link -O1"

$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.c -o "$OBJ/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c -x c "$STANDALONE_FUZZ_MAIN" -o "$OBJ/standalone_main.o"

for arch in xtensa riscv; do
    eval "arch_flags=\$ARCH_FLAGS_$arch"
    cflags="$FUZZ_CFLAGS $COMMON $arch_flags $INC"

    $CC $cflags -c "$EL/src/esp_elf_validate.c" -o "$OBJ/validate_$arch.o"
    $CC $cflags -c "$EL/src/esp_elf.c"          -o "$OBJ/loader_$arch.o"
    $CC $cflags -c "$EL/src/arch/esp_elf_$arch.c" -o "$OBJ/arch_$arch.o"
    $CC $cflags -c mayhem/loader_stubs.c        -o "$OBJ/stubs_$arch.o"
    $CC $cflags -c mayhem/fuzz_elf_validate.c   -o "$OBJ/h_validate_$arch.o"
    $CC $cflags -c mayhem/fuzz_elf_load.c       -o "$OBJ/h_load_$arch.o"

    validate_objs="$OBJ/validate_$arch.o $OBJ/lsan_off.o"
    load_objs="$OBJ/loader_$arch.o $OBJ/validate_$arch.o $OBJ/arch_$arch.o $OBJ/stubs_$arch.o $OBJ/lsan_off.o"

    $CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$OBJ/h_validate_$arch.o" $validate_objs \
        -o "/mayhem/fuzz_elf_validate_$arch"
    $CC $SANITIZER_FLAGS $DEBUG_FLAGS "$OBJ/standalone_main.o" "$OBJ/h_validate_$arch.o" $validate_objs \
        -o "/mayhem/fuzz_elf_validate_$arch-standalone"
    $CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$OBJ/h_load_$arch.o" $load_objs \
        -o "/mayhem/fuzz_elf_load_$arch"
    $CC $SANITIZER_FLAGS $DEBUG_FLAGS "$OBJ/standalone_main.o" "$OBJ/h_load_$arch.o" $load_objs \
        -o "/mayhem/fuzz_elf_load_$arch-standalone"

    # Oracle: the project's NORMAL flags (no sanitizers, no coverage), independent of the above.
    $CC -O1 $COMMON $arch_flags $INC \
        mayhem/oracle.c "$EL/src/esp_elf.c" "$EL/src/esp_elf_validate.c" \
        "$EL/src/arch/esp_elf_$arch.c" mayhem/loader_stubs.c \
        -o "/mayhem/elf_oracle_$arch"
done
