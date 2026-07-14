#!/bin/sh
# run.sh — headless Copperline on-target smoke test for the AmiAuth core.
#
# Boots a stock A500/68000 from ./sys (a throwaway RAM volume built from this
# host directory). The Startup-Sequence runs C:serialtest, which computes the
# RFC 4226 HOTP vectors and emits them over serial via RawPutChar. Copperline
# forwards serial to its stdout (`--serial stdout`); we assert every vector.
#
# No Workbench files, handlers, or Mounts are needed — RawPutChar is the ROM
# debug path straight to the Paula serial registers.
#
# Prereqs:
#   - copperline on PATH (brew install copperline)
#   - the cross-built harness                    SERIALTEST_M68K= (default build/serialtest)
#   - KICK= (optional): a 512 KiB Kickstart ROM. If unset, boots Copperline's
#     bundled AROS Kickstart replacement — redistributable, so CI needs no ROM.
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)

KICK=${KICK:-}            # empty => bundled AROS (no licensed ROM needed)
BIN=${SERIALTEST_M68K:-$ROOT/build/serialtest}
BENCH=${BENCH:-40}        # emulated seconds to run; enough for the slower AROS boot

# RFC 4226 Appendix D: 6-digit HOTP for secret "12345678901234567890".
VECTORS="0=755224 1=287082 2=359152 3=969429 4=338314 5=254676 6=287922 7=162583 8=399871 9=520489"

[ -e "$BIN" ] || { echo "FAIL: missing $BIN" >&2; exit 2; }
[ -z "$KICK" ] || [ -e "$KICK" ] || { echo "FAIL: KICK set but missing: $KICK" >&2; exit 2; }
command -v copperline >/dev/null || { echo "FAIL: copperline not on PATH" >&2; exit 2; }
[ -n "$KICK" ] && echo "ROM: $KICK" || echo "ROM: bundled AROS"

# --- stage the boot volume (just the harness binary) -------------------------
mkdir -p "$HERE/sys/C"
cp "$BIN" "$HERE/sys/C/serialtest"

OUT=$(mktemp)
cleanup() { rm -f "$OUT"; }
trap cleanup EXIT INT TERM

# --- boot windowless, serial -> our stdout (logs to stderr) ------------------
# --benchmark-until runs with no window until the given emulated time, then
# exits; boot + serialtest finish well before then. cd so `path = "sys"` in
# machine.toml resolves relative to this directory. A ROM arg overrides the
# config's (absent) rom; with none, Copperline boots its bundled AROS.
set -- --config machine.toml --noaudio --serial stdout --benchmark-until "$BENCH"
[ -n "$KICK" ] && set -- "$@" "$KICK"
( cd "$HERE" && copperline "$@" ) >"$OUT" 2>/dev/null \
    || { echo "FAIL: copperline exited non-zero" >&2; cat "$OUT" >&2; exit 3; }

tr -d '\r' <"$OUT" >"$OUT.n" && mv "$OUT.n" "$OUT"   # serial sends CRLF; drop CR
echo "----- serial capture -----"; cat "$OUT"; echo "--------------------------"
grep -q '^END' "$OUT" 2>/dev/null || { echo "FAIL: no END marker (raise BENCH?)" >&2; exit 1; }

# --- verify every vector -----------------------------------------------------
fails=0
for v in $VECTORS; do
    n=${v%=*}; code=${v#*=}
    if grep -q "^HOTP${n}=${code}$" "$OUT"; then
        :
    else
        got=$(sed -n "s/^HOTP${n}=//p" "$OUT" | head -1)
        echo "FAIL: counter $n expected $code, got '${got:-<none>}'" >&2
        fails=$((fails + 1))
    fi
done

[ "$fails" -eq 0 ] || exit 1
echo "PASS: all 10 RFC 4226 HOTP vectors correct on 68000 (via serial/RawPutChar)"
