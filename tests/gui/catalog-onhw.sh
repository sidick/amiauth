#!/bin/sh
# catalog-onhw.sh — on-target smoke test for the locale.library catalog
# plumbing (#67): AmiAuthGUI's CLI companion, AmiAuth, runs a vault command
# whose output goes through MSG()/catalog_get() (ADD's "Added %s:%s\n" - see
# locale/AmiAuth.cd), and the result is relayed back over serial via
# tests/copperline/arexxtest (a RAM: file -> RawPutChar relay, reused
# unchanged from the #46 ARexx on-target test).
#
# Two binaries run in the SAME boot, both asserted to produce the identical
# English default output:
#   - build/AmiAuth        - locale.library present, no matching catalog
#     installed (the common real-world case - most installs are English,
#     same as AmiAuth's built-in strings). Exercises the full
#     OpenLibrary/OpenCatalog/GetCatalogStr chain "successfully" skipping
#     catalog loading (see catalog.c's OC_BuiltInLanguage note) without
#     crashing or corrupting anything.
#   - build/AmiAuth-nolib   - built with -DAMIAUTH_LOCALE_LIBNAME set to a
#     name that doesn't exist (`make catalog-nolib-onhw`), so catalog_open()'s
#     OpenLibrary() genuinely fails on real hardware. Exercises the
#     LocaleBase==NULL fallback for real, without deleting the WB clone's
#     actual locale.library - which earlier attempts found breaks unrelated
#     boot components (the clone's own Locale-prefs tool pops a modal error
#     and hangs headless boot). Never shipped; test-only.
#
# What this does NOT (and, in this project's fixtures, currently cannot)
# cover as an automated regression test: a catalog actually overriding the
# built-in text. AmigaOS intentionally skips loading any on-disk catalog
# when OC_BuiltInLanguage matches the current locale's language (documented
# behaviour, see catalog.c) - and this project's WB clone (tests/gui/.env's
# AMIAUTH_WB_HDD) has no other "<language>.language" module installed, so
# the system locale can't meaningfully be anything but English here. This
# WAS verified manually during development: deliberately mismatching
# OC_BuiltInLanguage against the system locale (forcing a real disk load)
# confirmed OpenCatalog()/GetCatalogStr() correctly return the translated
# string, with substitution parameters intact, on real 68k hardware.
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
CLI_M68K=${CLI_M68K:-$ROOT/build/AmiAuth}
CLI_NOLIB=${CLI_NOLIB:-$ROOT/build/AmiAuth-nolib}
AREXXTEST=${AREXXTEST:-$ROOT/build/arexxtest}
SECS=${SECS:-40}

OUTDIR=$ROOT/build/catalog-onhw-run
BOOT=$OUTDIR/boot
LOG=$OUTDIR/serial.log

fail() { echo "CATALOG-ONHW FAIL: $1" >&2; exit 1; }

command -v "$COPPERLINE" >/dev/null 2>&1 || fail "copperline not found (set COPPERLINE=, or brew install copperline)"
[ -n "$KICK" ] && [ -e "$KICK" ] || fail "Kickstart 3.2 ROM missing (KICK= / AMIAUTH_ROM in tests/gui/.env): '$KICK'"
[ -n "$WB" ] && [ -d "$WB" ]     || fail "Workbench 3.2 dir missing (WB= / AMIAUTH_WB_HDD): '$WB'"
[ -e "$WB/Libs/locale.library" ] || fail "WB dir has no locale.library: '$WB'"
[ -x "$CLI_M68K" ]  || fail "$CLI_M68K missing — build it first: make m68k-docker"
[ -x "$CLI_NOLIB" ] || fail "$CLI_NOLIB missing — build it first: make catalog-nolib-onhw-docker"
[ -x "$AREXXTEST" ] || fail "$AREXXTEST missing — build it first: make arexx-onhw-docker"

rm -rf "$OUTDIR"; mkdir -p "$OUTDIR"
cleanup() { rm -rf "$BOOT"; }
trap cleanup EXIT INT TERM

cp -Rc "$WB" "$BOOT" 2>/dev/null || cp -R "$WB" "$BOOT"
cp "$CLI_M68K"  "$BOOT/AmiAuth"
cp "$CLI_NOLIB" "$BOOT/AmiAuth-nolib"
cp "$AREXXTEST" "$BOOT/arexxtest"

SEQ="$BOOT/S/Startup-Sequence"
[ -f "$SEQ" ] || fail "clone missing S/Startup-Sequence"
awk '{ print }
     /^LoadWB/ {
       print "SYS:AmiAuth INIT OPEN VAULT=RAM:t.vault >RAM:catout.txt"
       print "SYS:AmiAuth ADD \"otpauth://totp/Test:foo?secret=GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ\" VAULT=RAM:t.vault >>RAM:catout.txt"
       print "SYS:AmiAuth-nolib INIT OPEN VAULT=RAM:t2.vault >RAM:catout2.txt"
       print "SYS:AmiAuth-nolib ADD \"otpauth://totp/Test:foo?secret=GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ\" VAULT=RAM:t2.vault >>RAM:catout2.txt"
       print "SYS:arexxtest RAM:catout.txt"
       print "SYS:arexxtest RAM:catout2.txt"
     }' "$SEQ" > "$SEQ.new" && mv "$SEQ.new" "$SEQ"

CFG="$OUTDIR/copperline.toml"
cat > "$CFG" <<EOF
[machine]
profile = "A1200"

[memory]
fast = "8M"

[ide]
master = { path = "$BOOT", name = "Workbench" }
EOF

echo "CATALOG-ONHW: booting A1200/OS 3.2 under Copperline, exercising MSG()-routed CLI output..."
"$COPPERLINE" --config "$CFG" --noaudio --serial stdout --benchmark-until "$SECS" "$KICK" \
    >"$LOG" 2>&1 || { tail -20 "$LOG" >&2; fail "copperline exited non-zero"; }
tr -d '\r' <"$LOG" >"$LOG.n" && mv "$LOG.n" "$LOG"

echo "----- serial capture -----"; cat "$LOG"; echo "--------------------------"

begins=$(grep -c '^BEGIN$' "$LOG" || true)
ends=$(grep -c '^END$' "$LOG" || true)
[ "$begins" -eq 2 ] || fail "expected 2 BEGIN markers (one per arexxtest relay), got $begins — did both AmiAuth commands run? (raise SECS?)"
[ "$ends" -eq 2 ]   || fail "expected 2 END markers, got $ends (raise SECS?)"
grep -q '^NOFILE$' "$LOG" && fail "a RAM:catout*.txt relay is missing — one of the CLI commands didn't run"

added=$(grep -cxF 'Added Test:foo' "$LOG" || true)
[ "$added" -eq 2 ] || fail "expected 'Added Test:foo' twice (locale.library present AND absent), got $added — MSG(MSG_CLI_ADDED) regressed"

echo "CATALOG-ONHW PASS: MSG()-routed CLI output correct on real 68k, with locale.library both present and absent."
