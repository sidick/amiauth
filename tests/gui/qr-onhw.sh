#!/bin/sh
# qr-onhw.sh — headless on-target test of the QR-image pipeline under Copperline.
#
# Boots Workbench 3.2 (A1200/AGA/KS 3.2), runs SYS:qr-onhw which loads a staged
# QR PNG via datatypes.library (src/amiga/qrimage.c) and decodes it (src/qr),
# emitting the result over serial via RawPutChar. We assert the decoded URI. This
# validates the datatypes glue against the *real* picture.datatype — the risk the
# host decoder test can't cover. Build the v39 fallback path with:
#   make qr-onhw-docker QR_ONHW_DEFS=-DQRIMAGE_FORCE_V39 && make qr-onhw-smoke
#
# Non-mutating: the WB install is a copy-on-write clone; source is never touched.
# Paths come from tests/gui/.env (shared with the other GUI harnesses).
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)          # tests/gui
ROOT=$(cd "$HERE/../.." && pwd)

[ -f "$HERE/.env" ] && . "$HERE/.env"
COPPERLINE=${COPPERLINE:-copperline}
KICK=${KICK:-${AMIAUTH_ROM:-}}
WB=${WB:-${AMIAUTH_WB_HDD:-}}
BIN=${BIN:-$ROOT/build/qr-onhw}
PNG=${PNG:-$HERE/qr-sample.png}
# The decode is slow on the FPU-less CPU (soft-float geometry) — allow generous
# emulated run time for boot + load + decode.
SECS=${SECS:-260}

EXPECT='otpauth://totp/AmiAuth:demo?secret=JBSWY3DPEHPK3PXP&issuer=AmiAuth'

OUTDIR=$ROOT/build/qr-onhw-run
BOOT=$OUTDIR/boot
LOG=$OUTDIR/serial.log

fail() { echo "QR-ONHW FAIL: $1" >&2; exit 1; }

command -v "$COPPERLINE" >/dev/null 2>&1 || fail "copperline not found"
[ -n "$KICK" ] && [ -e "$KICK" ] || fail "Kickstart 3.2 ROM missing (KICK=/AMIAUTH_ROM): '$KICK'"
[ -n "$WB" ] && [ -d "$WB" ]     || fail "Workbench 3.2 dir missing (WB=/AMIAUTH_WB_HDD): '$WB'"
[ -e "$WB/Classes/DataTypes/png.datatype" ] || echo "QR-ONHW WARN: WB has no png.datatype — the load may fail" >&2
[ -x "$BIN" ] || fail "$BIN missing — build it: make qr-onhw-docker"
[ -e "$PNG" ] || fail "$PNG missing"

rm -rf "$OUTDIR"; mkdir -p "$OUTDIR"
cleanup() { rm -rf "$BOOT"; }
trap cleanup EXIT INT TERM

# --- clone WB (copy-on-write) and stage the test binary + image --------------
cp -Rc "$WB" "$BOOT" 2>/dev/null || { rm -rf "$BOOT"; cp -R "$WB" "$BOOT"; }
cp "$BIN" "$BOOT/qr-onhw"
cp "$PNG" "$BOOT/qr-sample.png"

# Run the test after LoadWB (datatypes assigns are up by then), then exit.
SEQ="$BOOT/S/Startup-Sequence"
[ -f "$SEQ" ] || fail "clone missing S/Startup-Sequence"
awk '{ print }
     /^LoadWB/ { print "SYS:qr-onhw" }' "$SEQ" > "$SEQ.new" && mv "$SEQ.new" "$SEQ"
grep -q 'SYS:qr-onhw' "$SEQ" || fail "could not patch Startup-Sequence (no LoadWB line?)"

# --- config: A1200 / AGA / KS 3.2, boot from the clone -----------------------
CFG="$OUTDIR/copperline.toml"
cat > "$CFG" <<EOF
[machine]
profile = "A1200"

[memory]
fast = "8M"

[ide]
master = { path = "$BOOT", name = "Workbench" }
EOF

# --- boot headless, serial -> our log, run until SECS emulated seconds --------
echo "QR-ONHW: booting A1200/OS 3.2 under Copperline, running SYS:qr-onhw..."
"$COPPERLINE" --config "$CFG" --serial stdout --benchmark-until "$SECS" "$KICK" \
    >"$LOG" 2>/dev/null || { tail -20 "$LOG" >&2; fail "copperline exited non-zero"; }

tr -d '\r' <"$LOG" >"$LOG.n" && mv "$LOG.n" "$LOG"
echo "----- serial capture -----"; grep -E '^(DATATYPES|LOAD|DEC|URI|END)' "$LOG" || true
echo "--------------------------"

grep -q '^END' "$LOG" || fail "no END marker — qr-onhw didn't run (raise SECS?)"
grep -q '^DATATYPES=yes' "$LOG" || fail "datatypes.library did not open on the target"
grep -q "^URI=$EXPECT\$" "$LOG" || fail "decoded URI mismatch (see $LOG)"

echo "QR-ONHW PASS: datatypes loaded the PNG and the QR decoded to the expected URI."
