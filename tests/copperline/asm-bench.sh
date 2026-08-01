#!/bin/sh
# asm-bench.sh — compare the portable C SHA-1 compress function against the
# hand-written 68000 asm one (#47), via PBKDF2-HMAC-SHA1, same boot, same
# CPU, same params, real 68000 (A500). Dev-only groundwork/verification for
# crypto_dispatch.h's runtime asm dispatch — does not itself change anything
# shipped. Invoked by `make asm-bench`.
#
# Needs copperline (or COPPERLINE= pointing at another build), docker (the
# amiga-gcc image), and a 512 KiB Kickstart ROM (timer.device EClock is not
# available under the bundled AROS, so this one dev tool needs a real ROM —
# override with KICK=).
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
COPPERLINE=${COPPERLINE:-copperline}
KICK=${KICK:-/Users/simond/Documents/Amiberry/Roms/amiga-os-310-a600.rom}
IMG=${AMIGA_GCC_IMAGE:-stefanreinauer/amiga-gcc:latest}

[ -e "$KICK" ] || { echo "FAIL: need a Kickstart ROM (set KICK=)" >&2; exit 2; }
command -v "$COPPERLINE" >/dev/null || { echo "FAIL: $COPPERLINE not found" >&2; exit 2; }

# Build the bench binary (m68k, -O2 as production, plain 68000 — same
# baseline the asm targets). sha256.c/sha512.c must link for the same reason
# noted in bench.sh (hmac.c defines the SHA-256/512 HMAC variants
# unconditionally).
docker run --rm --platform linux/amd64 -v "$ROOT":/work -w /work "$IMG" sh -lc \
  'PATH=/opt/amiga/bin:$PATH m68k-amigaos-gcc -std=c99 -O2 -Wall -m68000 -noixemul \
   -Isrc/core src/core/pbkdf2.c src/core/hmac.c src/core/sha1.c \
   src/core/sha256.c src/core/sha512.c src/core/sha1_asm.s \
   tests/copperline/asmbench.c -o build/asmbench'

T=$(mktemp -d)
OUT=$(mktemp)
trap 'rm -rf "$T" "$OUT"' EXIT INT TERM
mkdir -p "$T/sys/C" "$T/sys/S"
cp "$ROOT/build/asmbench" "$T/sys/C/bench"
printf 'bench\n' > "$T/sys/S/Startup-Sequence"
cat > "$T/m.toml" <<EOF
[machine]
profile = "A500"
[cpu]
model = "68000"
[memory]
chip = "512K"
slow = "512K"
[floppy]
drives = 1
[[filesys]]
path = "$T/sys"
volume = "Bench"
bootpri = 10
[emulation]
warp_speed = "max"
EOF

"$COPPERLINE" --config "$T/m.toml" --noaudio --serial stdout --benchmark-until 300 "$KICK" \
    >"$OUT" 2>/dev/null || true

c_line=$(tr -d '\r' <"$OUT" | grep '^PBKDF2 impl=c ' | head -1 || true)
asm_line=$(tr -d '\r' <"$OUT" | grep '^PBKDF2 impl=asm ' | head -1 || true)

[ -n "$c_line" ] || { echo "FAIL: no C result (boot/timer issue?)" >&2; cat "$OUT" >&2; exit 1; }
[ -n "$asm_line" ] || { echo "FAIL: no asm result" >&2; cat "$OUT" >&2; exit 1; }

echo "raw: $c_line"
echo "raw: $asm_line"

report() {
    label=$1 line=$2
    iters=$(echo "$line" | sed -n 's/.*iters=\([0-9]*\).*/\1/p')
    ticks=$(echo "$line" | sed -n 's/.*ticks=\([0-9]*\).*/\1/p')
    freq=$(echo "$line"  | sed -n 's/.*freq=\([0-9]*\).*/\1/p')
    awk -v l="$label" -v i="$iters" -v t="$ticks" -v f="$freq" 'BEGIN {
        sec = t / f; rate = i / sec;
        printf "%-4s PBKDF2(dkLen=64) on a stock 68000: %.1f iters/sec (%d iters in %.2fs)\n", l, rate, i, sec;
    }'
}
report "C"   "$c_line"
report "asm" "$asm_line"
