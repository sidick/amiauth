/* arexx.h -- ARexx port for AmiAuth automation (#46), GUI-only.
 *
 * A genuine public RexxMsg port (AMIAUTH.<n>), separate from the private
 * CLI-forwarding port in guiport.h - two ports, two purposes, coexisting.
 * Port creation/teardown and RexxMsg mechanics live here; the actual
 * per-command work (touching the vault/window) stays in src/gui/main.c's
 * event loop, same split guiport.h/main.c already uses for AAP_*. Command
 * parsing itself is portable (src/core/arexx_cmd.h), so only this file's
 * RexxMsg glue is Amiga-only.
 *
 * The passphrase never crosses this port - see docs/SECURITY.md.
 */
#ifndef AMIAUTH_AREXX_H
#define AMIAUTH_AREXX_H

#include <exec/ports.h>

#include "arexx_cmd.h"

/* Open the port. `portname_override` (NULL/empty for the default) is the
 * PORTNAME tooltype/arg; otherwise derives "AMIAUTH.<n>" (uppercase,
 * lowest free slot, the standard <BASENAME>.<slot#> convention). Copies
 * the actual name used into out_name (out_name_cap bytes, for display -
 * e.g. the window title) if out_name is non-NULL. Returns the port, or
 * NULL if rexxsyslib.library isn't open (the caller's own open_libs()
 * must set RexxSysBase first) or no port could be created - absence just
 * means no ARexx port, not a fatal error, matching this project's other
 * optional-library features. */
struct MsgPort *arexx_open(const char *portname_override,
                           char *out_name, size_t out_name_cap);

/* Reply any still-queued message with AREXX_RC_FAIL (mirrors pubport's own
 * teardown in main.c), then RemPort + DeleteMsgPort. */
void arexx_close(struct MsgPort *port);

/* Pull the next genuine ARexx message off `port` (validated via
 * IsRexxMsg(); anything else is silently dropped - nothing but the ARexx
 * interpreter should ever PutMsg() to a public ARexx-named port, but this
 * mirrors the standard caution) and parse it into `out` via arexx_parse().
 * Returns an opaque handle for arexx_reply(), or NULL once the port is
 * drained for this signal. */
void *arexx_receive(struct MsgPort *port, arexx_parsed *out);

/* Reply to the message `handle` identifies (from arexx_receive). Always
 * sets the RC; only builds a RESULT argstring if the caller actually asked
 * for one (RXFB_RESULT) - `result` may be NULL/empty either way. The
 * argstring (if any) is never freed here: ReplyMsg() hands ownership to
 * the ARexx interpreter, which frees it after consuming the reply
 * (confirmed against a canonical reference host implementation - do not
 * "fix" this into a leak by adding a DeleteArgstring call here). */
void arexx_reply(void *handle, int rc, const char *result);

#endif /* AMIAUTH_AREXX_H */
