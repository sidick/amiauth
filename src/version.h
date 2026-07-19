#ifndef AMIAUTH_VERSION_H
#define AMIAUTH_VERSION_H

/* Single source of truth for the release version.
 *
 * A release PR bumps AMIAUTH_VERSION and AMIAUTH_VERSION_DATE here and the
 * matching `Version:` field in AmiAuth.readme; the tag-driven release workflow
 * (.github/workflows/release.yml) refuses a v<tag> that does not match both.
 *
 * AMIAUTH_VERSION_DATE is the AmigaOS $VER date, <dd>.<mm>.<yyyy> per
 * https://wiki.amigaos.net/wiki/Version_Strings */
#define AMIAUTH_VERSION      "1.0"
#define AMIAUTH_VERSION_DATE "19.07.2026"

/* Embedded AmigaOS version string, findable by the shell `Version` command.
 * The leading "\0" guards against an adjacent string in the binary running
 * into ours; `used` keeps the otherwise-unreferenced constant out of the
 * optimiser's reach. */
#define AMIAUTH_VERSTAG(name) \
    static const char verstag[] __attribute__((used)) = \
        "\0$VER: " name " " AMIAUTH_VERSION " (" AMIAUTH_VERSION_DATE ")";

#endif
