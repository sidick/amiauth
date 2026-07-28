#!/bin/sh
# arexx-onhw.sh — headless on-target test of AmiAuthGUI's ARexx port (#46).
#
# Boots Workbench 3.2 + ReAction under Copperline (A1200/AGA/Kickstart 3.2,
# same profile as gui-smoke.sh), launches AmiAuthGUI resident with a seeded
# always-unlocked vault, then — in the same boot — runs a real ARexx script
# (tests/copperline/arexx-probe.rexx) via the WB image's resident RexxMast
# (`rx`), redirected to a RAM: file. A genuine ARexx interpreter task is
# required here: rexxsyslib.library's IsRexxMsg() only validates messages
# whose rm_TaskBlock was populated by a live ARexx task's own context, which
# a hand-rolled RexxMsg from a plain external C program never has (confirmed
# empirically — see arexxtest.c's header comment) — so this exercises the
# real dispatch code in src/gui/main.c/src/amiga/arexx.c exactly as a real
# user's script would, via the actual interpreter, not a simulation of one.
#
# Since Copperline's [ide] host-directory mount is an in-memory snapshot
# (guest writes never reach the host), the RAM: result file is relayed back
# over serial by tests/copperline/arexxtest (a tiny file-to-RawPutChar
# relay) before the emulator exits.
#
# Non-mutating: the Workbench install is a copy-on-write clone; source is
# never touched. Paths come from tests/gui/.env (shared with gui-smoke.sh).
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)          # tests/gui
ROOT=$(cd "$HERE/../.." && pwd)

[ -f "$HERE/.env" ] && . "$HERE/.env"
COPPERLINE=${COPPERLINE:-copperline}
KICK=${KICK:-${AMIAUTH_ROM:-}}
WB=${WB:-${AMIAUTH_WB_HDD:-}}
GUI=${GUI:-$ROOT/build/AmiAuthGUI}
CLI=${CLI:-$ROOT/build/amiauth-host}
AREXXTEST=${AREXXTEST:-$ROOT/build/arexxtest}
PROBE=${PROBE:-$ROOT/tests/copperline/arexx-probe.rexx}
SECS=${SECS:-60}

# INIT --open (no passphrase given) makes an always-unlocked test vault,
# same as gui-smoke.sh's own seeding.
EXPECT_STATUS='STATUS RC=0 RESULT=always-unlocked 1'

OUTDIR=$ROOT/build/arexx-onhw-run
BOOT=$OUTDIR/boot
LOG=$OUTDIR/serial.log
VAULT=$OUTDIR/AmiAuth.vault

fail() { echo "AREXX-ONHW FAIL: $1" >&2; exit 1; }

# --- preflight ---------------------------------------------------------------
command -v "$COPPERLINE" >/dev/null 2>&1 || fail "copperline not found (set COPPERLINE=, or brew install copperline)"
[ -n "$KICK" ] && [ -e "$KICK" ] || fail "Kickstart 3.2 ROM missing (KICK= / AMIAUTH_ROM in tests/gui/.env): '$KICK'"
[ -n "$WB" ] && [ -d "$WB" ]     || fail "Workbench 3.2 dir missing (WB= / AMIAUTH_WB_HDD): '$WB'"
[ -e "$WB/Libs/rexxsyslib.library" ] || fail "WB dir has no rexxsyslib.library: '$WB'"
[ -e "$WB/System/RexxMast" ] || fail "WB dir has no System/RexxMast (resident ARexx interpreter): '$WB'"
[ -x "$GUI" ] || fail "$GUI missing — build it first: make gui-docker"
[ -x "$CLI" ] || fail "$CLI missing — build it first: make cli"
[ -x "$AREXXTEST" ] || fail "$AREXXTEST missing — build it first: make arexx-onhw-docker"
[ -e "$PROBE" ] || fail "$PROBE missing"

rm -rf "$OUTDIR"; mkdir -p "$OUTDIR"
cleanup() { rm -rf "$BOOT"; }
trap cleanup EXIT INT TERM

# --- fresh always-unlocked test vault, one known account ---------------------
SECRET=GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ
export AMIAUTH_PREFS_DIR="$OUTDIR/prefs"
"$CLI" -v "$VAULT" INIT --open >/dev/null || fail "vault INIT"
"$CLI" -v "$VAULT" ADD "otpauth://totp/GitHub:smoke?secret=$SECRET&digits=6&period=30" \
    >/dev/null || fail "vault ADD"

# --- clone the WB (copy-on-write) and stage binaries + script + vault -------
cp -Rc "$WB" "$BOOT" 2>/dev/null || { rm -rf "$BOOT"; cp -R "$WB" "$BOOT"; }
cp "$GUI"       "$BOOT/AmiAuthGUI"
cp "$AREXXTEST" "$BOOT/arexxtest"
cp "$PROBE"     "$BOOT/arexx-probe.rexx"
cp "$VAULT"     "$BOOT/AmiAuth.vault"

# Launch AmiAuthGUI resident (backgrounded) after LoadWB, give it a few
# seconds to open its window and the ARexx port, run the probe script via
# the resident RexxMast (rx), redirecting SAY output to a RAM: file, then
# relay that file out over serial. All synchronous within one Startup-Sequence.
SEQ="$BOOT/S/Startup-Sequence"
[ -f "$SEQ" ] || fail "clone missing S/Startup-Sequence"
awk '{ print }
     /^LoadWB/ { print "SetEnv AmiAuth/vault SYS:Decoy-does-not-exist.vault"
                 print "Run >NIL: SYS:AmiAuthGUI VAULT=SYS:AmiAuth.vault"
                 print "Wait 3"
                 print "rx SYS:arexx-probe.rexx >RAM:arexx-result.txt"
                 print "SYS:arexxtest RAM:arexx-result.txt" }' "$SEQ" > "$SEQ.new" && mv "$SEQ.new" "$SEQ"
grep -q 'SYS:arexxtest' "$SEQ" || fail "could not patch Startup-Sequence (no LoadWB line?)"

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

# --- boot headless, serial -> our stdout, exit at SECS -----------------------
echo "AREXX-ONHW: booting A1200/OS 3.2 under Copperline, probing AmiAuthGUI's ARexx port..."
"$COPPERLINE" --config "$CFG" --noaudio --serial stdout --benchmark-until "$SECS" "$KICK" \
    >"$LOG" 2>&1 || { tail -20 "$LOG" >&2; fail "copperline exited non-zero"; }

tr -d '\r' <"$LOG" >"$LOG.n" && mv "$LOG.n" "$LOG"   # serial sends CRLF; drop CR
echo "----- serial capture -----"; cat "$LOG"; echo "--------------------------"

grep -q '^BEGIN$' "$LOG" || fail "no BEGIN marker — arexxtest didn't run (raise SECS?)"
grep -q '^NOFILE$' "$LOG" && fail "RAM:arexx-result.txt missing — rx probe script didn't run/produce output"
grep -q '^END$' "$LOG" || fail "no END marker (raise SECS?)"

fails=0
assert_line() {
    grep -qxF "$1" "$LOG" || { echo "FAIL: expected line '$1' not found" >&2; fails=$((fails + 1)); }
}
assert_line "$EXPECT_STATUS"
grep -q '^LIST RC=0 RESULT=GitHub:smoke$' "$LOG" || { echo "FAIL: LIST result wrong" >&2; fails=$((fails + 1)); }
grep -q '^GETCODE RC=0 RESULT=[0-9]\{6\}$' "$LOG" || { echo "FAIL: GETCODE didn't return a 6-digit code" >&2; fails=$((fails + 1)); }
grep -q '^TIMELEFT RC=0 RESULT=[0-9][0-9]\{0,1\}$' "$LOG" || { echo "FAIL: TIMELEFT result wrong" >&2; fails=$((fails + 1)); }
assert_line "NOTFOUND RC=10 RESULT="
assert_line "UNKNOWN RC=10 RESULT="
assert_line "QUIT RC=0 RESULT="

[ "$fails" -eq 0 ] || exit 1
echo "AREXX-ONHW PASS: AMIAUTH.1 answered STATUS/LIST/GETCODE/TIMELEFT/QUIT correctly on real 68k (via a real rx script)."
