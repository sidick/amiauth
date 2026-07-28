/* catalog.c — host stub. The real locale.library backend is in
 * src/amiga/catalog.c; exactly one is compiled per platform. */
#include "catalog.h"

#ifndef __amigaos__

void catalog_open(void) {}
void catalog_close(void) {}

const char *catalog_get(long id, const char *def)
{
    (void)id;
    return def;
}

#endif /* !__amigaos__ */
