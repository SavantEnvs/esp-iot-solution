#!/usr/bin/env bash
#
# mayhem/test.sh — RUN the elf_loader functional tests (already built by mayhem/build.sh).
#  1. upstream's host suite (components/elf_loader/tests/host/test_elf_validate.py), which compiles and
#     executes the real validator/loader/relocation tests with the host compiler;
#  2. a known-answer oracle per architecture: the built elf_oracle_<arch> binary validates and loads
#     the checked-in ELF fixtures plus fixed corruptions, and its output is diffed against golden files.
# The oracle binary is a dynamically linked project executable, so a neutered program prints nothing
# and fails the diff.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
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

passed=0; failed=0; skipped=0
FIXTURES=examples/elf_loader/elf_loader_example/main

for arch in xtensa riscv; do
  bin="/mayhem/elf_oracle_$arch"
  if [ ! -x "$bin" ]; then
    echo "missing $bin — mayhem/build.sh did not build it" >&2; failed=$((failed + 1)); continue
  fi
  got="$("$bin" "$FIXTURES/test_$arch.elf" 2>&1)"
  if [ "$got" = "$(cat "mayhem/oracle/expected_$arch.txt")" ]; then
    echo "oracle $arch: output matches golden"; passed=$((passed + 1))
  else
    echo "oracle $arch: output differs from mayhem/oracle/expected_$arch.txt" >&2
    diff <(printf '%s\n' "$got") "mayhem/oracle/expected_$arch.txt" >&2 || true
    failed=$((failed + 1))
  fi
done

log="$(PYTHONDONTWRITEBYTECODE=1 CC="${CC:-clang}" python3 -m unittest discover \
        -s components/elf_loader/tests/host -p test_elf_validate.py 2>&1)"
rc=$?
printf '%s\n' "$log" | tail -25
ran="$(printf '%s\n' "$log" | sed -n 's/^Ran \([0-9]\+\) tests\? in .*/\1/p' | tail -1)"
if [ -z "$ran" ] || [ "$ran" -lt 30 ]; then
  echo "upstream suite ran ${ran:-0} tests (expected >= 30)" >&2; failed=$((failed + 1))
else
  skip="$(printf '%s\n' "$log" | sed -n 's/.*skipped=\([0-9]\+\).*/\1/p' | tail -1)"
  nfail="$(printf '%s\n' "$log" | sed -n 's/^FAILED (.*failures=\([0-9]\+\).*/\1/p' | tail -1)"
  nerr="$(printf '%s\n' "$log" | sed -n 's/^FAILED (.*errors=\([0-9]\+\).*/\1/p' | tail -1)"
  skip="${skip:-0}"; bad=$(( ${nfail:-0} + ${nerr:-0} ))
  [ "$rc" -ne 0 ] && [ "$bad" -eq 0 ] && bad=1
  passed=$(( passed + ran - skip - bad )); skipped=$(( skipped + skip )); failed=$(( failed + bad ))
fi

emit_ctrf "unittest+oracle" "$passed" "$failed" "$skipped"
