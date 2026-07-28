#!/usr/bin/env bash
# fetch-flexcat.sh — fetch + build FlexCat (github.com/adtools/flexcat) as a
# HOST tool, for generating AmiAuth's catalog message-ID header and (for
# translators) .catalog files from locale/AmiAuth.cd (#67).
#
# FlexCat is a modern, portable, GPL-licensed replacement for the classic
# catcomp tool — it builds and runs natively on Linux (confirmed via its own
# `make OS=unix` and CI), so it never touches the m68k cross-compiler or the
# AmiAuth binary itself; it's purely a build-time tool, same status as
# tools/docs2guide.py. Not part of the shipped (zero-dependency) Amiga build.
#
# Pinned to a specific commit (not just a tag) for the same reproducibility
# fetch-amissl-sdk.sh gets from a SHA-256 pin on a downloaded archive — a git
# commit hash is already a strong content hash, so no separate checksum step
# is needed here.
#
# Idempotent: builds only when the pinned-commit binary is missing. Requires
# a working native (non-cross) C compiler on PATH — e.g. run this inside the
# amiga-gcc Docker image (`make flexcat-docker`), which bundles a real gcc
# for exactly this purpose, or any plain Linux/Unix host with gcc. FlexCat's
# own Makefile assumes GCC-flavoured flags (-gstabs) that macOS's clang
# rejects, so a native macOS build isn't supported here — use the *-docker
# Makefile target instead.
#
# Usage:
#   tools/fetch-flexcat.sh
#
# Environment:
#   FLEXCAT_COMMIT     override the pinned commit (default below)
#   FLEXCAT_CACHE_DIR  cache root (default ${TMPDIR:-/tmp}/flexcat-src)
#
# On success, prints the built binary's path to stdout.
# Exit codes: 0 ok, 1 fetch/build failure, 2 usage.
set -euo pipefail

# Tag 2.18 (27.04.2016), the latest release as of this writing.
FLEXCAT_COMMIT="${FLEXCAT_COMMIT:-b45d1e6ceb8647d8965d494bdddb617ca4927b4e}"
# Not $HOME/.cache: under this project's `--user UID:GID` Docker convention
# (numeric UID with no /etc/passwd entry, see Makefile's DOCKER_USER), $HOME
# resolves to "/" - unwritable. /tmp is reliably writable regardless.
FLEXCAT_CACHE_DIR="${FLEXCAT_CACHE_DIR:-${TMPDIR:-/tmp}/flexcat-src}"

src_dir="$FLEXCAT_CACHE_DIR/$FLEXCAT_COMMIT"
bin_path="$src_dir/src/bin_unix/flexcat"
stamp="$src_dir/.ok"

if [ -f "$stamp" ] && [ -x "$bin_path" ]; then
    echo "$bin_path"
    exit 0
fi

need() { command -v "$1" >/dev/null 2>&1 || { echo "fetch-flexcat.sh: missing tool '$1'" >&2; exit 1; }; }
need git
need make
need cc

mkdir -p "$FLEXCAT_CACHE_DIR"
rm -rf "$src_dir"
echo "fetch-flexcat.sh: cloning adtools/flexcat @ $FLEXCAT_COMMIT" >&2
git clone --quiet https://github.com/adtools/flexcat.git "$src_dir"
(cd "$src_dir" && git checkout --quiet "$FLEXCAT_COMMIT")

got_commit=$(cd "$src_dir" && git rev-parse HEAD)
if [ "$got_commit" != "$FLEXCAT_COMMIT" ]; then
    echo "fetch-flexcat.sh: checkout landed on $got_commit, expected $FLEXCAT_COMMIT" >&2
    exit 1
fi

# FlexCat's own build bootstraps a few generated headers (FlexCat_cat.h,
# locale.c, ...) using a *pre-existing* flexcat binary - a chicken-and-egg
# problem the upstream repo solves by checking those generated files in.
# `git checkout` doesn't preserve source timestamps, so the checked-in
# outputs can end up looking older than their .pot/.sd inputs, tricking
# `make` into trying (and failing) to regenerate them. Force the opposite
# ordering so make treats them as already up to date.
(cd "$src_dir/src" && touch locale/FlexCat.pot && sleep 1 && \
    touch FlexCat_cat.h locale.c locale_other.c FlexCat_cat_other.h)

echo "fetch-flexcat.sh: building (make OS=unix)" >&2
(cd "$src_dir/src" && make OS=unix >&2)

if [ ! -x "$bin_path" ]; then
    echo "fetch-flexcat.sh: build finished but $bin_path is missing" >&2
    exit 1
fi

touch "$stamp"
echo "$bin_path"
