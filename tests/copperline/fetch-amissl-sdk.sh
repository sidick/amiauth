#!/usr/bin/env bash
# fetch-amissl-sdk.sh — fetch + unpack the AmiSSL v5 SDK and OS3 runtime
# package, for the dev-only AmiSSL-vs-builtin PBKDF2 benchmark (issue #85
# groundwork). Not part of the shipped build — AmiAuth stays zero-dependency
# at runtime; this only feeds a local benchmark tool.
#
# Fetches two archives from the same pinned release:
#   - AmiSSL-<ver>-SDK.lha  — headers + libamisslstubs.a, for linking the
#     bench binary against m68k-amigaos-gcc.
#   - AmiSSL-<ver>-OS3.lha  — the actual runtime amisslmaster.library and the
#     CPU-specific amissl_v*.library, which must be staged onto the guest's
#     boot volume (LIBS:/LIBS:AmiSSL/) for OpenLibrary() to succeed. The SDK
#     alone does not include these.
#
# Idempotent: downloads + extracts only when the pinned-version directory is
# missing. Verifies SHA-256 on both archives. On success, prints two lines to
# stdout: the SDK's Developer dir, then the OS3 package's AmiSSL dir.
#
# Usage:
#   tests/copperline/fetch-amissl-sdk.sh
#
# Environment:
#   AMISSL_VERSION    override the pinned version (default 5.27)
#   AMISSL_CACHE_DIR  cache root (default $HOME/.cache/amissl-sdk)
#
# Exit codes: 0 ok, 1 download/verify/extract failure, 2 usage.
set -euo pipefail

AMISSL_VERSION="${AMISSL_VERSION:-5.27}"
AMISSL_CACHE_DIR="${AMISSL_CACHE_DIR:-$HOME/.cache/amissl-sdk}"

# Pinned SHA-256s for both archives. If AMISSL_VERSION is changed, matching
# hashes must be supplied via AMISSL_SDK_SHA256/AMISSL_OS3_SHA256 — refuse to
# silently fetch an unpinned version.
case "$AMISSL_VERSION" in
    5.27)
        PINNED_SDK_SHA256="5003bef8c5930354d16b0ce7196d71b2811891c42fad38a9238c5ce4098ad42a"
        PINNED_OS3_SHA256="722476cfbf21c0caf6ff4a77e32ee8baf67d3045db0e9ff2ccce6c782bed5847"
        ;;
    *)
        PINNED_SDK_SHA256="${AMISSL_SDK_SHA256:-}"
        PINNED_OS3_SHA256="${AMISSL_OS3_SHA256:-}"
        ;;
esac

if [ -z "$PINNED_SDK_SHA256" ] || [ -z "$PINNED_OS3_SHA256" ]; then
    echo "fetch-amissl-sdk.sh: AMISSL_VERSION=$AMISSL_VERSION has no pinned" >&2
    echo "  SHA-256s. Set AMISSL_SDK_SHA256 and AMISSL_OS3_SHA256 to proceed." >&2
    exit 2
fi

version_dir="$AMISSL_CACHE_DIR/amissl-${AMISSL_VERSION}"
sdk_root="$version_dir/sdk"
os3_root="$version_dir/os3"
stamp="$version_dir/.ok"

sdk_dev="$sdk_root/AmiSSL/Developer"
os3_dir="$os3_root/AmiSSL"

if [ -f "$stamp" ] && [ -d "$sdk_dev" ] && [ -d "$os3_dir" ]; then
    echo "$sdk_dev"
    echo "$os3_dir"
    exit 0
fi

need() { command -v "$1" >/dev/null 2>&1 || { echo "fetch-amissl-sdk.sh: missing tool '$1'" >&2; exit 1; }; }
need curl
need lha
if command -v sha256sum >/dev/null 2>&1; then
    sha_cmd="sha256sum"
elif command -v shasum >/dev/null 2>&1; then
    sha_cmd="shasum -a 256"
else
    echo "fetch-amissl-sdk.sh: need sha256sum or shasum on PATH" >&2
    exit 1
fi

fetch_verify_extract() {
    local name="$1" url="$2" pinned="$3" dest_dir="$4"
    local archive_path="$version_dir/$name"
    mkdir -p "$version_dir" "$dest_dir"

    echo "fetch-amissl-sdk.sh: fetching $name" >&2
    curl -fSL --retry 3 -o "$archive_path" "$url"

    local got
    got=$($sha_cmd "$archive_path" | awk '{print $1}')
    if [ "$got" != "$pinned" ]; then
        echo "fetch-amissl-sdk.sh: checksum mismatch for $name" >&2
        echo "  expected $pinned" >&2
        echo "  got      $got" >&2
        rm -f "$archive_path"
        exit 1
    fi

    (cd "$dest_dir" && lha xq "$archive_path")
}

fetch_verify_extract "AmiSSL-${AMISSL_VERSION}-SDK.lha" \
    "https://github.com/jens-maus/amissl/releases/download/${AMISSL_VERSION}/AmiSSL-${AMISSL_VERSION}-SDK.lha" \
    "$PINNED_SDK_SHA256" "$sdk_root"

fetch_verify_extract "AmiSSL-${AMISSL_VERSION}-OS3.lha" \
    "https://github.com/jens-maus/amissl/releases/download/${AMISSL_VERSION}/AmiSSL-${AMISSL_VERSION}-OS3.lha" \
    "$PINNED_OS3_SHA256" "$os3_root"

if [ ! -d "$sdk_dev/include/openssl" ] \
   || [ ! -f "$sdk_dev/lib/AmigaOS3/libamisslstubs.a" ]; then
    echo "fetch-amissl-sdk.sh: unexpected SDK layout under $sdk_root" >&2
    exit 1
fi
if [ ! -f "$os3_dir/Libs/AmigaOS3/amisslmaster.library" ] \
   || [ ! -f "$os3_dir/Libs/AmigaOS3/AmiSSL/68020-40/amissl_v362.library" ]; then
    echo "fetch-amissl-sdk.sh: unexpected OS3 package layout under $os3_root" >&2
    exit 1
fi

touch "$stamp"
echo "$sdk_dev"
echo "$os3_dir"
