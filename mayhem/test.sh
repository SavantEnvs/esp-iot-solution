#!/usr/bin/env bash
#
# mayhem/test.sh — RUN the known-answer oracle (already built by mayhem/build.sh): it enumerates simulated
# RNDIS USB devices through the real iot_usbh_rndis driver (checked-in response streams under the target's
# testsuite/) and prints what the driver reports; the output is diffed against golden files. The oracle is a
# project executable built with normal flags, so a neutered program prints nothing and fails the diff.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
SRC="${SRC:-/mayhem}"
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$skipped" "$pending" "$other"
  [ "$failed" -eq 0 ]
}

passed=0; failed=0
bin=/mayhem/rndis_oracle
SUITE=mayhem/fuzz_rndis_control-mined-d910006/testsuite

if [ ! -x "$bin" ]; then
  echo "missing $bin — mayhem/build.sh did not build it" >&2
  emit_ctrf "rndis-oracle" 0 1; exit $?
fi

for name in enumerate-link-up enumerate-link-down; do
  got="$("$bin" "$SUITE/$name" 2>&1)"
  if [ "$got" = "$(cat "mayhem/oracle/expected_$name.txt")" ]; then
    echo "oracle $name: output matches golden"; passed=$((passed + 1))
  else
    echo "oracle $name: output differs from mayhem/oracle/expected_$name.txt" >&2
    diff <(printf '%s\n' "$got") "mayhem/oracle/expected_$name.txt" >&2 || true
    failed=$((failed + 1))
  fi
done

# A device that never answers must leave the driver without a link or a MAC address.
got="$("$bin" /dev/null 2>&1)"
if [ "$got" = "$(cat mayhem/oracle/expected_silent-device.txt)" ]; then
  echo "oracle silent-device: output matches golden"; passed=$((passed + 1))
else
  echo "oracle silent-device: output differs from mayhem/oracle/expected_silent-device.txt" >&2
  failed=$((failed + 1))
fi

emit_ctrf "rndis-oracle" "$passed" "$failed"
