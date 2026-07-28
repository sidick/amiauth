#!/bin/sh
# amissl-bench.sh — compare AmiSSL's PKCS5_PBKDF2_HMAC_SHA1 against our own
# pbkdf2_hmac_sha1, same boot, same CPU, same params. Dev-only groundwork for
# issue #85 ("optional AmiSSL crypto provider") — informs whether that's
# worth building, does not itself change anything shipped. Invoked by
# `make amissl-bench`.
#
# AmiSSL needs 68020+/OS3.0+ (no 68000 path exists), so unlike pbkdf2-bench
# this boots an accelerated profile, not the plain A500 baseline.
#
# Boots from a copy-on-write clone of the same Workbench 3.2 install used by
# tests/gui/gui-smoke.sh (tests/gui/.env: AMIAUTH_WB_HDD/AMIAUTH_ROM), not a
# synthetic minimal disk: AmiSSL's OpenAmiSSLTags() reliably crashes ramlib
# (CPU exception ACPU_CHK, 0x80000006) on a from-scratch boot volume that
# only has amisslmaster.library + the CPU-specific amissl_v*.library copied
# into LIBS: — it also needs the `AmiSSL:` assign (`Assign AmiSSL: SYS:AmiSSL`
# + `Assign LIBS: AmiSSL:Libs ADD`, done by this WB's S:User-Startup) that a
# minimal boot doesn't set up, evidently for something InitAmiSSL() reads
# even with no networking (config/cert store lookup, most likely). The real
# WB install already has AmiSSL installed and its assigns wired up, so this
# reuses that known-good environment instead of re-deriving it. Confirmed
# 2026-07-28.
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
[ -f "$ROOT/tests/gui/.env" ] && . "$ROOT/tests/gui/.env"

COPPERLINE=${COPPERLINE:-copperline}
KICK=${KICK:-${AMIAUTH_ROM:-}}
WB=${WB:-${AMIAUTH_WB_HDD:-}}
IMG=${AMIGA_GCC_IMAGE:-stefanreinauer/amiga-gcc:latest}
BENCH=${BENCH:-90}      # emulated seconds; WB boot + both PBKDF2 runs need headroom

[ -n "$KICK" ] && [ -e "$KICK" ] || { echo "FAIL: Kickstart ROM missing (KICK= / AMIAUTH_ROM in tests/gui/.env): '$KICK'" >&2; exit 2; }
[ -n "$WB" ] && [ -d "$WB" ] || { echo "FAIL: Workbench dir missing (WB= / AMIAUTH_WB_HDD in tests/gui/.env): '$WB'" >&2; exit 2; }
[ -f "$WB/S/User-Startup" ] || { echo "FAIL: $WB/S/User-Startup missing (need the AmiSSL assign it sets up)" >&2; exit 2; }
command -v "$COPPERLINE" >/dev/null || { echo "FAIL: $COPPERLINE not found" >&2; exit 2; }

# Fetch (or reuse the cached) AmiSSL SDK — only the Developer headers +
# libamisslstubs.a are needed to link the bench binary; the runtime
# amisslmaster.library/amissl_v*.library come from the WB clone itself.
sdk_out=$("$HERE/fetch-amissl-sdk.sh")
SDK_DEV=$(echo "$sdk_out" | sed -n 1p)
[ -d "$SDK_DEV" ] || { echo "FAIL: fetch-amissl-sdk.sh did not return a valid SDK path" >&2; exit 1; }

# Build the bench binary (m68k, -O2, 68020 target — AmiSSL has no 68000
# path). hmac.c defines the SHA-256/512 HMAC variants unconditionally, so
# sha256.c/sha512.c must link even though this benchmark only exercises SHA-1.
docker run --rm --platform linux/amd64 \
  -v "$ROOT":/work -v "$SDK_DEV":/amissl-sdk -w /work "$IMG" sh -lc \
  'PATH=/opt/amiga/bin:$PATH m68k-amigaos-gcc -std=c99 -O2 -Wall -m68020 -noixemul \
   -Isrc/core -I/amissl-sdk/include \
   src/core/pbkdf2.c src/core/hmac.c src/core/sha1.c \
   src/core/sha256.c src/core/sha512.c \
   tests/copperline/amisslbench.c \
   -L/amissl-sdk/lib/AmigaOS3 -lamisslstubs \
   -o build/amisslbench'

T=$(mktemp -d)
OUT=$(mktemp)
trap 'rm -rf "$T" "$OUT"' EXIT INT TERM

# --- clone the WB (copy-on-write) and stage the bench binary -----------------
cp -Rc "$WB" "$T/boot" 2>/dev/null || cp -R "$WB" "$T/boot"
cp "$ROOT/build/amisslbench" "$T/boot/bench"

# Run right after S:User-Startup (which sets up the AmiSSL:/LIBS: assigns)
# and before LoadWB — a Workbench screen isn't needed for this, so skip the
# time it'd take to load one.
SEQ="$T/boot/S/Startup-Sequence"
awk '/^LoadWB/ { print "SYS:bench" } { print }' "$SEQ" > "$SEQ.new" && mv "$SEQ.new" "$SEQ"
grep -q '^SYS:bench$' "$SEQ" || { echo "FAIL: could not patch Startup-Sequence (no LoadWB line?)" >&2; exit 1; }

cat > "$T/cfg.toml" <<EOF
[machine]
profile = "A1200"
[memory]
fast = "8M"
[ide]
master = { path = "$T/boot", name = "Workbench" }
[emulation]
warp_speed = "max"
EOF

"$COPPERLINE" --config "$T/cfg.toml" --noaudio --serial stdout --benchmark-until "$BENCH" "$KICK" \
    >"$OUT" 2>/dev/null || true

builtin_line=$(tr -d '\r' <"$OUT" | grep '^BUILTIN ' | head -1 || true)
amissl_line=$(tr -d '\r' <"$OUT" | grep '^AMISSL '  | head -1 || true)
err_line=$(tr -d '\r' <"$OUT" | grep '^ERR: '        | head -1 || true)

[ -n "$builtin_line" ] || { echo "FAIL: no BUILTIN result (boot/timer issue?)" >&2; cat "$OUT" >&2; exit 1; }
[ -n "$err_line" ] && echo "note: $err_line" >&2
[ -n "$amissl_line" ] || { echo "FAIL: no AMISSL result — see note above" >&2; exit 1; }

echo "raw: $builtin_line"
echo "raw: $amissl_line"

report() {
    label=$1 line=$2
    iters=$(echo "$line" | sed -n 's/.*iters=\([0-9]*\).*/\1/p')
    ticks=$(echo "$line" | sed -n 's/.*ticks=\([0-9]*\).*/\1/p')
    freq=$(echo "$line"  | sed -n 's/.*freq=\([0-9]*\).*/\1/p')
    awk -v l="$label" -v i="$iters" -v t="$ticks" -v f="$freq" 'BEGIN {
        sec = t / f; rate = i / sec;
        printf "%-8s PBKDF2(dkLen=64): %.1f iters/sec (%d iters in %.2fs)\n", l, rate, i, sec;
    }'
}
report "BUILTIN" "$builtin_line"
report "AMISSL"  "$amissl_line"
