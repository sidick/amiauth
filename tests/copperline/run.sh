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
# Prereqs (referenced by path, never committed — same policy as the ROM):
#   - copperline on PATH (brew install copperline)
#   - a 512 KiB Kickstart 3.1 ROM               KICK=
#   - the cross-built harness                    SERIALTEST_M68K= (default build/serialtest)
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)

KICK=${KICK:-/Users/simond/Documents/Amiberry/Roms/amiga-os-310-a600.rom}
BIN=${SERIALTEST_M68K:-$ROOT/build/serialtest}
BENCH=${BENCH:-20}        # emulated seconds to run (boot + emit finish well before)

# RFC 4226 Appendix D: 6-digit HOTP for secret "12345678901234567890".
VECTORS="0=755224 1=287082 2=359152 3=969429 4=338314 5=254676 6=287922 7=162583 8=399871 9=520489"

for f in "$KICK" "$BIN"; do
    [ -e "$f" ] || { echo "FAIL: missing $f" >&2; exit 2; }
done
command -v copperline >/dev/null || { echo "FAIL: copperline not on PATH" >&2; exit 2; }

# --- stage the boot volume (just the harness binary) -------------------------
mkdir -p "$HERE/sys/C"
cp "$BIN" "$HERE/sys/C/serialtest"

OUT=$(mktemp)
cleanup() { rm -f "$OUT"; }
trap cleanup EXIT INT TERM

# --- boot windowless, serial -> our stdout (logs to stderr) ------------------
# --benchmark-until runs with no window until the given emulated time, then
# exits; boot + serialtest finish well before then. cd so `path = "sys"` in
# machine.toml resolves relative to this directory.
( cd "$HERE" && copperline --config machine.toml --noaudio --serial stdout \
    --benchmark-until "$BENCH" "$KICK" ) >"$OUT" 2>/dev/null \
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
