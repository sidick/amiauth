#!/bin/sh
# bench.sh — measure PBKDF2-HMAC-SHA1 throughput on a stock 68000 under
# Copperline, to inform the vault KDF iteration policy (see docs/SECURITY.md).
#
# Needs copperline, docker (the amiga-gcc image), and a 512 KiB Kickstart ROM
# (timer.device EClock is not available under the bundled AROS, so this one dev
# tool needs a real ROM — override with KICK=). Invoked by `make pbkdf2-bench`.
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
KICK=${KICK:-/Users/simond/Documents/Amiberry/Roms/amiga-os-310-a600.rom}
IMG=${AMIGA_GCC_IMAGE:-stefanreinauer/amiga-gcc:latest}

[ -e "$KICK" ] || { echo "FAIL: need a Kickstart ROM (set KICK=)" >&2; exit 2; }
command -v copperline >/dev/null || { echo "FAIL: copperline not on PATH" >&2; exit 2; }

# Build the bench binary (m68k, -O2 as production).
docker run --rm --platform linux/amd64 -v "$ROOT":/work -w /work "$IMG" sh -lc \
  'PATH=/opt/amiga/bin:$PATH m68k-amigaos-gcc -std=c99 -O2 -Wall -m68000 -noixemul \
   -Isrc/core src/core/pbkdf2.c src/core/hmac.c src/core/sha1.c \
   tests/copperline/pbkdf2bench.c -o build/pbkdf2bench'

T=$(mktemp -d)
OUT=$(mktemp)
trap 'rm -rf "$T" "$OUT"' EXIT INT TERM
mkdir -p "$T/sys/C" "$T/sys/S"
cp "$ROOT/build/pbkdf2bench" "$T/sys/C/bench"
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

copperline --config "$T/m.toml" --noaudio --serial stdout --benchmark-until 200 "$KICK" \
    >"$OUT" 2>/dev/null || true
line=$(tr -d '\r' <"$OUT" | grep '^PBKDF2 ' | head -1 || true)
[ -n "$line" ] || { echo "FAIL: no PBKDF2 result (boot/timer issue)" >&2; exit 1; }

echo "raw: $line"
iters=$(echo "$line" | sed -n 's/.*iters=\([0-9]*\).*/\1/p')
ticks=$(echo "$line" | sed -n 's/.*ticks=\([0-9]*\).*/\1/p')
freq=$(echo "$line"  | sed -n 's/.*freq=\([0-9]*\).*/\1/p')
awk -v i="$iters" -v t="$ticks" -v f="$freq" 'BEGIN {
    sec = t / f; rate = i / sec;
    printf "PBKDF2(dkLen=64) on a stock 68000: %.1f iters/sec (%d iters in %.2fs)\n", rate, i, sec;
    printf "  ~1s target   = %d iters\n", rate;
    printf "  10000 iters  = %.0fs unlock\n", 10000 / rate;
}'
