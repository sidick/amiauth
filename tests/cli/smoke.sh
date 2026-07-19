#!/bin/sh
# CLI smoke test — exercises the always-unlocked vault round-trip end to end
# (INIT/ADD/LIST/GET/REMOVE). Dependency-free: it cross-checks the vault-backed
# GET against the vault-less CODE command for the same secret, rather than
# needing an external reference. The encrypted path needs a TTY and is covered
# by the vault unit tests, not here.
set -eu

BIN="${AMIAUTH_BIN:-build/amiauth-host}"
SECRET="GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"
TMP="$(mktemp -d)"
VAULT="$TMP/smoke.vault"
export AMIAUTH_PREFS_DIR="$TMP/prefs"
trap 'rm -rf "$TMP"' EXIT

fail() { echo "SMOKE FAIL: $1" >&2; exit 1; }

if ! "$BIN" -v "$VAULT" INIT --open >/dev/null; then fail "INIT"; fi
if ! "$BIN" -v "$VAULT" ADD \
        "otpauth://totp/GitHub:alice?secret=$SECRET&digits=8&period=30" >/dev/null
then fail "ADD (totp)"; fi
if ! "$BIN" -v "$VAULT" ADD \
        "otpauth://totp/Acme:bob?secret=$SECRET&digits=6&period=30" >/dev/null
then fail "ADD (second)"; fi

LIST="$("$BIN" -v "$VAULT" LIST)"
if ! echo "$LIST" | grep -q '^GitHub:alice$'; then fail "LIST missing GitHub:alice"; fi
if ! echo "$LIST" | grep -q '^Acme:bob$';    then fail "LIST missing Acme:bob"; fi

CODE="$("$BIN" -v "$VAULT" GET github 2>/dev/null)"
if ! echo "$CODE" | grep -Eq '^[0-9]{8}$'; then fail "GET github not 8 digits: '$CODE'"; fi

# The vault-backed code must equal the direct code for the same secret/params,
# proving the secret round-tripped through the vault. Retry once in case the two
# invocations straddle a 30-second step boundary.
match() {
    a="$("$BIN" -v "$VAULT" GET github 2>/dev/null)"
    b="$("$BIN" CODE "$SECRET" 8 30 2>/dev/null)"
    [ "$a" = "$b" ]
}
if ! match && ! match; then fail "GET != CODE for the same secret"; fi

if ! "$BIN" -v "$VAULT" REMOVE github >/dev/null; then fail "REMOVE"; fi
LIST="$("$BIN" -v "$VAULT" LIST)"
if echo "$LIST" | grep -qi 'github';      then fail "REMOVE did not remove the account"; fi
if ! echo "$LIST" | grep -q '^Acme:bob$'; then fail "REMOVE removed the wrong account"; fi

# Prefs persistence: a manual offset set in one run is read back in another.
if ! "$BIN" OFFSET 3600 >/dev/null;                     then fail "OFFSET"; fi
if ! "$BIN" CLOCK | grep -q '^UTC offset : +3600 seconds'; then fail "CLOCK did not read the persisted offset"; fi

# INIT records the vault path in the prefs - but only when the path was not
# explicitly overridden (-v / AMIAUTH_VAULT), so scratch vaults (like every
# INIT above, all -v) never hijack the pref.
ABS_BIN=$(cd "$(dirname "$BIN")" && pwd)/$(basename "$BIN")
if [ -e "$TMP/prefs/vault" ]; then fail "explicit -v INIT recorded a vault pref"; fi
( cd "$TMP" && "$ABS_BIN" INIT --open >/dev/null ) || fail "default-path INIT"
if [ ! -e "$TMP/prefs/vault" ]; then fail "default-path INIT did not record the vault pref"; fi
case "$(cat "$TMP/prefs/vault")" in
    /*) ;;                              # absolute, as documented
    *)  fail "recorded vault pref is not an absolute path" ;;
esac
# ...and the recorded pref is picked up when no override is given
( cd "$TMP" && "$ABS_BIN" ADD "otpauth://totp/Pref:test?secret=$SECRET" >/dev/null ) \
    || fail "ADD via recorded pref"
( cd / && "$ABS_BIN" LIST | grep -q '^Pref:test$' ) \
    || fail "LIST from another cwd did not follow the recorded vault pref"

# NUDGE adjusts the existing offset by a relative delta, unlike OFFSET's
# absolute set — verify it composes (100 -> +30 -> -50 = 80) and persists.
if ! "$BIN" OFFSET 100 >/dev/null; then fail "OFFSET (nudge baseline)"; fi
if ! "$BIN" NUDGE 30 | grep -q '^UTC offset : +130 seconds'; then fail "NUDGE +30"; fi
if ! "$BIN" NUDGE -50 | grep -q '^UTC offset : +80 seconds'; then fail "NUDGE -50"; fi
if ! "$BIN" CLOCK | grep -q '^UTC offset : +80 seconds'; then fail "NUDGE did not persist"; fi

echo "CLI smoke: OK"
