#!/bin/sh
# gui-smoke.sh — headless Copperline GUI smoke test for AmiAuthGUI.
#
# Boots Workbench 3.2 + ReAction under Copperline (A1200/AGA/Kickstart 3.2),
# auto-launches AmiAuthGUI from the boot Startup-Sequence, screenshots the
# result, and asserts the ReAction window actually rendered. No VNC, no
# clicking, no window interaction: deterministic and CI-friendly.
#
# Non-mutating: the Workbench install is a copy-on-write clone (APFS `cp -c`);
# the source WB dir and the repo are never modified. The clone is thrown away
# on exit (Copperline mounts a host dir as an in-memory FFS anyway).
#
# Paths come from tests/gui/.env (shared with the amiberry harness); override
# any with env vars:
#   COPPERLINE  copperline binary            (default: copperline on PATH)
#   KICK        512 KiB Kickstart 3.2 ROM    (default: $AMIAUTH_ROM)
#   WB          bootable WB 3.2 dir + ReAction(default: $AMIAUTH_WB_HDD)
#   GUI         AmiAuthGUI m68k binary       (default: build/AmiAuthGUI)
#   CLI         host CLI for vault seeding   (default: build/amiauth-host)
#   SECS        emulated seconds before shot (default: 50)
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)          # tests/gui
ROOT=$(cd "$HERE/../.." && pwd)

[ -f "$HERE/.env" ] && . "$HERE/.env"
COPPERLINE=${COPPERLINE:-copperline}
KICK=${KICK:-${AMIAUTH_ROM:-}}
WB=${WB:-${AMIAUTH_WB_HDD:-}}
GUI=${GUI:-$ROOT/build/AmiAuthGUI}
CLI=${CLI:-$ROOT/build/amiauth-host}
SECS=${SECS:-50}

OUTDIR=$ROOT/build/gui-smoke
BOOT=$OUTDIR/boot
SHOT=$OUTDIR/AmiAuthGUI.png
LOG=$OUTDIR/serial.log
VAULT=$OUTDIR/AmiAuth.vault

fail() { echo "GUI-SMOKE FAIL: $1" >&2; exit 1; }

# --- preflight ---------------------------------------------------------------
command -v "$COPPERLINE" >/dev/null 2>&1 || fail "copperline not found (set COPPERLINE=, or brew install copperline)"
[ -n "$KICK" ] && [ -e "$KICK" ] || fail "Kickstart 3.2 ROM missing (KICK= / AMIAUTH_ROM in tests/gui/.env): '$KICK'"
[ -n "$WB" ] && [ -d "$WB" ]     || fail "Workbench 3.2 dir missing (WB= / AMIAUTH_WB_HDD): '$WB'"
[ -e "$WB/Classes/window.class" ] || fail "WB dir has no ReAction classes (Classes/window.class): '$WB'"
[ -x "$GUI" ] || fail "$GUI missing — build it first: make gui-docker"
[ -x "$CLI" ] || fail "$CLI missing — build it first: make cli"

rm -rf "$OUTDIR"; mkdir -p "$OUTDIR"
cleanup() { rm -rf "$BOOT"; }                # keep SHOT/LOG; drop the clone
trap cleanup EXIT INT TERM

# --- fresh always-unlocked test vault (known account) ------------------------
SECRET=GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ
export AMIAUTH_PREFS_DIR="$OUTDIR/prefs"
"$CLI" -v "$VAULT" INIT --open >/dev/null || fail "vault INIT"
"$CLI" -v "$VAULT" ADD "otpauth://totp/GitHub:smoke?secret=$SECRET&digits=6&period=30" \
    >/dev/null || fail "vault ADD"

# --- clone the WB (copy-on-write) and stage the payload ----------------------
cp -Rc "$WB" "$BOOT" 2>/dev/null || { rm -rf "$BOOT"; cp -R "$WB" "$BOOT"; }
cp "$GUI"   "$BOOT/AmiAuthGUI"
cp "$VAULT" "$BOOT/AmiAuth.vault"

# Launch after LoadWB (the ReAction window needs the Workbench screen up).
# The vault is passed via the VAULT= Shell argument (#42); the pref is
# deliberately set to a nonexistent decoy, so the window only renders if the
# argument is honoured *and* outranks the saved pref — a broken VAULT= would
# land in the first-run requester instead and fail the colour assertion.
SEQ="$BOOT/S/Startup-Sequence"
[ -f "$SEQ" ] || fail "clone missing S/Startup-Sequence"
awk '{ print }
     /^LoadWB/ { print "SetEnv AmiAuth/vault SYS:Decoy-does-not-exist.vault"
                 print "Run >NIL: SYS:AmiAuthGUI VAULT=SYS:AmiAuth.vault" }' "$SEQ" > "$SEQ.new" && mv "$SEQ.new" "$SEQ"
grep -q 'SYS:AmiAuthGUI' "$SEQ" || fail "could not patch Startup-Sequence (no LoadWB line?)"

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

# --- boot headless, screenshot after SECS emulated seconds -------------------
echo "GUI-SMOKE: booting A1200/OS 3.2 under Copperline, launching AmiAuthGUI..."
"$COPPERLINE" --config "$CFG" --serial stdout --screenshot-after "$SECS" "$SHOT" "$KICK" \
    >"$LOG" 2>&1 || { tail -20 "$LOG" >&2; fail "copperline exited non-zero"; }

grep -q 'screenshot saved' "$LOG" || { tail -20 "$LOG" >&2; fail "no screenshot saved"; }
[ -s "$SHOT" ] || fail "screenshot not written: $SHOT"

# --- assert the ReAction window rendered -------------------------------------
# The centre of a bare WB 3.2 desktop is one flat colour; the AmiAuthGUI window
# (borders, account list, code display, fuelgauge) fills it with many. Crop the
# window region (excludes the right-hand system monitor and the top clock, whose
# text changes every second) and count distinct colours. Calibrated: WB-only=1,
# window present=4+.
if command -v magick >/dev/null 2>&1; then
    W=$(magick identify -format '%w' "$SHOT"); H=$(magick identify -format '%h' "$SHOT")
    x0=$(awk "BEGIN{print int(0.42*$W)}"); y0=$(awk "BEGIN{print int(0.36*$H)}")
    cw=$(awk "BEGIN{print int(0.15*$W)}"); ch=$(awk "BEGIN{print int(0.32*$H)}")
    K=$(magick "$SHOT" -crop "${cw}x${ch}+${x0}+${y0}" +repage -format '%k' info:)
    SD=$(magick "$SHOT" -crop "${cw}x${ch}+${x0}+${y0}" +repage -format '%[fx:standard_deviation]' info:)
    echo "GUI-SMOKE: window-region unique_colours=$K stddev=$SD"
    awk "BEGIN{exit !($K>=3)}" || fail "no ReAction window in the centre region (unique_colours=$K) — see $SHOT"
else
    echo "GUI-SMOKE: WARN 'magick' not found — skipping window-content assertion (structural checks only)"
fi

echo "GUI-SMOKE PASS: WB 3.2 booted and AmiAuthGUI's ReAction window rendered."
echo "  screenshot: $SHOT"
