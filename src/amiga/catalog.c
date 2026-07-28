/* catalog.c — locale.library catalog backend (#67). See catalog.h. AmigaOS
 * only; the host stub is in src/core/catalog.c.
 *
 * Opened once at startup (CLI main(); GUI open_libs()), closed once at exit
 * — persistent app-lifetime scope, unlike the existing transient
 * OpenLibrary/CloseLibrary pairs elsewhere in this codebase that only want
 * loc_GMTOffset for a moment (src/cli/main.c's cli_utc_offset(),
 * src/gui/main.c's GMT-offset probe). Absence of locale.library, or of an
 * installed AmiAuth.catalog for the user's language, is not an error - same
 * optional-feature pattern as every other library in this codebase; the
 * caller always gets English (the built-in default) either way.
 *
 * OC_BuiltInLanguage note: when it matches the system's current locale
 * language (the common case for most users - AmiAuth's built-ins are
 * English, most installs are English), OpenCatalog() intentionally skips
 * loading any on-disk catalog at all and GetCatalogStr() just returns the
 * default - this is documented AmigaOS behaviour, not a bug (confirmed
 * on-target: a deliberately mismatched OC_BuiltInLanguage does load the
 * catalog and GetCatalogStr() correctly returns the translated string with
 * substitution parameters intact - see tests/gui/catalog-onhw.sh). */
#ifdef __amigaos__

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include <libraries/locale.h>

#include "catalog.h"

/* <proto/locale.h>'s inline stubs reference this exact global by name and
 * type (struct LocaleBase *, not struct Library *) - same convention as
 * RexxSysBase/GfxBase elsewhere in this project; must not be static. */
struct LocaleBase *LocaleBase = NULL;
static struct Catalog *g_catalog = NULL;

/* Overridable at build time (-DAMIAUTH_LOCALE_LIBNAME='"nonexistent"') so an
 * on-target test can force a genuine OpenLibrary() failure and exercise the
 * LocaleBase==NULL fallback below, without touching the real locale.library
 * a WB clone's own boot (IPrefs etc.) still needs - see
 * `make catalog-nolib-onhw` / tests/gui/catalog-onhw.sh. Never set for a
 * real build; the Makefile only ever defines it for that one test binary. */
#ifndef AMIAUTH_LOCALE_LIBNAME
#define AMIAUTH_LOCALE_LIBNAME "locale.library"
#endif

void catalog_open(void)
{
    LocaleBase = (struct LocaleBase *)OpenLibrary((STRPTR)AMIAUTH_LOCALE_LIBNAME, 38);  /* OS 2.1+ */
    if (LocaleBase)
        g_catalog = OpenCatalog(NULL, (STRPTR)"AmiAuth.catalog",
                                OC_BuiltInLanguage, (ULONG)(STRPTR)"english",
                                TAG_DONE);
}

void catalog_close(void)
{
    if (LocaleBase) {
        if (g_catalog) CloseCatalog(g_catalog);
        CloseLibrary((struct Library *)LocaleBase);
    }
    g_catalog = NULL;
    LocaleBase = NULL;
}

const char *catalog_get(long id, const char *def)
{
    /* GetCatalogStr() is documented safe with a NULL catalog (just returns
     * def) - but not with a NULL LocaleBase, since that means the library
     * itself never opened and there is no function to call at all. */
    if (!LocaleBase) return def;
    return (const char *)GetCatalogStr(g_catalog, id, (STRPTR)def);
}

#endif /* __amigaos__ */
