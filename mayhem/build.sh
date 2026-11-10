#!/usr/bin/env bash
#
# mayhem/build.sh — build the RNDIS USB host driver fuzz harness (sanitized, libFuzzer + standalone) and its
# known-answer oracle (normal flags). Runs in /mayhem as `mayhem`. Re-runnable and offline: only the in-tree
# sources, the host stubs under mayhem/stubs/ and the base toolchain are used.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}"
: "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
export SANITIZER_FLAGS DEBUG_FLAGS CC LIB_FUZZING_ENGINE

SRC="${SRC:-/mayhem}"
cd "$SRC"

RN=components/usb/iot_usbh_rndis
INC="-Imayhem/stubs -Imayhem -Icomponents/usb/iot_usbh_cdc/include -I$RN/include -I$RN/priv_include \
-Icomponents/iot_eth/include -Icomponents/iot_eth/interface"
# The driver requires a CDC control transfer buffer of at least 1025 bytes (RNDIS_CONTROL_BUFFER_SIZE).
DEFS="-std=gnu11 -D_GNU_SOURCE -DCONFIG_USBH_CDC_CONTROL_TRANSFER_BUFFER_SIZE=1025 \
-DIOT_USBH_RNDIS_VER_MAJOR=0 -DIOT_USBH_RNDIS_VER_MINOR=3 -DIOT_USBH_RNDIS_VER_PATCH=1"

OBJ="$(mktemp -d)"
trap 'rm -rf "$OBJ"' EXIT

# Sanitized + coverage-instrumented objects. fuzzer-no-link instruments the driver itself (the engine is
# only linked into the harness), and stays on when SANITIZER_FLAGS is empty.
FUZZ_CFLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link -O1 $DEFS $INC"

$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.c -o "$OBJ/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c -x c "$STANDALONE_FUZZ_MAIN" -o "$OBJ/standalone_main.o"

$CC $FUZZ_CFLAGS -c "$RN/iot_usbh_rndis.c" -o "$OBJ/rndis.o"
$CC $FUZZ_CFLAGS -c mayhem/rndis_env.c -o "$OBJ/env.o"
$CC $FUZZ_CFLAGS -c mayhem/fuzz_rndis_control.c -o "$OBJ/harness.o"

objs="$OBJ/rndis.o $OBJ/env.o $OBJ/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$OBJ/harness.o" $objs -o /mayhem/fuzz_rndis_control
$CC $SANITIZER_FLAGS $DEBUG_FLAGS "$OBJ/standalone_main.o" "$OBJ/harness.o" $objs -o /mayhem/fuzz_rndis_control-standalone

# Oracle: NORMAL flags (no sanitizers, no coverage), independent of the above.
$CC -g -gdwarf-3 -O2 $DEFS $INC mayhem/oracle.c mayhem/rndis_env.c "$RN/iot_usbh_rndis.c" -o /mayhem/rndis_oracle
