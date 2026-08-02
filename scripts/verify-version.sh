#!/bin/sh
# Verifies a release tag (e.g. v1.1) matches src/version.h and AmiAuth.readme
# before release.yml hands off to sidick/amiga-workflows' aminet-release.yml
# - the calling repo's own version-file format is project-specific, so this
# check stays here rather than in the shared workflow (see that repo's own
# aminet-release.yml header comment for why).
#
# Usage: scripts/verify-version.sh vX.Y
set -eu

tag_ref="${1:?usage: verify-version.sh <tag, e.g. v1.1>}"
tag="${tag_ref#v}"

src=$(sed -n 's/^#define AMIAUTH_VERSION[[:space:]]*"\(.*\)"$/\1/p' src/version.h)
readme=$(sed -n 's/^Version:[[:space:]]*\(.*\)$/\1/p' AmiAuth.readme)

echo "tag=$tag src/version.h=$src AmiAuth.readme=$readme"
[ "$tag" = "$src" ]    || { echo "::error file=src/version.h::Tag v$tag does not match AMIAUTH_VERSION \"$src\""; exit 1; }
[ "$tag" = "$readme" ] || { echo "::error file=AmiAuth.readme::Tag v$tag does not match Version: \"$readme\""; exit 1; }
