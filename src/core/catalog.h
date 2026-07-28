/* catalog.h — locale.library catalog lookup for user-visible strings (#67).
 *
 * `locale/AmiAuth.cd` is the source of truth for every translatable string;
 * `make catalog-strings` (FlexCat + its standard CatComp_h.sd template)
 * regenerates catalog_strings.h from it — a normal checked-in source file,
 * not a build-time-generated artifact, so an ordinary `make test`/`cli`/
 * `m68k`/`gui` build needs neither FlexCat nor network access. Re-run
 * `make catalog-strings` (or `catalog-strings-docker`) after editing the
 * .cd and commit the result.
 *
 * Call sites use one macro everywhere, host or Amiga:
 *
 *     printf(MSG(MSG_CLI_ERR), vault_err(rc));
 *
 * catalog_get() is deliberately NOT the FlexCat-generated GetAmiAuthString()
 * — that helper (and the packed string block/array it walks) needs
 * <exec/types.h>, which doesn't exist off-Amiga. Since MSG() already has
 * the compile-time default string in hand (MSG_FOO_STR, pasted in via the
 * id##_STR trick below), the real lookup only needs the plain two-argument
 * GetCatalogStr(catalog, id, default) shape — trivial to reimplement per
 * platform (src/amiga/catalog.c; src/core/catalog.c's host stub) without
 * needing the generated code/array/block sections at all. */
#ifndef AMIAUTH_CATALOG_H
#define AMIAUTH_CATALOG_H

#ifndef __amigaos__
/* catalog_strings.h unconditionally #includes <exec/types.h>, even in
 * NUMBERS+STRINGS-only mode - though nothing in that mode actually
 * references any exec/types.h type (it's plain #define ID n / #define
 * ID_STR "..." text). Pre-satisfy its include guard to skip it on hosts
 * that don't have the header, rather than hand-duplicating the ID list. */
#define EXEC_TYPES_H
#endif
#define AmiAuth_NUMBERS
#define AmiAuth_STRINGS
#include "catalog_strings.h"
#undef AmiAuth_STRINGS
#undef AmiAuth_NUMBERS
#ifndef __amigaos__
#undef EXEC_TYPES_H
#endif

/* Open/close the catalog for this process's lifetime (app startup/exit) —
 * no-ops on the host. Absence of locale.library, or of an installed
 * AmiAuth.catalog for the user's language, is not an error: catalog_get()
 * always falls back to `def`. */
void catalog_open(void);
void catalog_close(void);

/* Look up `id`'s translated string if a catalog is open and has one, else
 * `def`. Called via MSG() below, never directly. */
const char *catalog_get(long id, const char *def);

#define MSG(id) catalog_get(id, id##_STR)

#endif /* AMIAUTH_CATALOG_H */
