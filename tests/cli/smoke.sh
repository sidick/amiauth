#!/bin/sh
# CLI smoke test — exercises the always-unlocked vault round-trip end to end
# (INIT/ADD/LIST/GET/REMOVE). Dependency-free: it cross-checks the vault-backed
# GET against the vault-less CODE command for the same secret, rather than
# needing an external reference. The encrypted path needs a TTY and is covered
# by the vault unit tests, not here.
set -eu

BIN="${AMIAUTH_BIN:-build/amiauth}"
SECRET="GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"
TMP="$(mktemp -d)"
VAULT="$TMP/smoke.vault"
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

echo "CLI smoke: OK"
