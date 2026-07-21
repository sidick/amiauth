/* guiport.h — CLI <-> resident-GUI IPC protocol (Stage 3b).
 *
 * When an AmiAuthGUI is running it holds the unlocked vault and owns a public
 * message port (AMIAUTH_PORT_NAME). The CLI forwards vault commands to it rather
 * than opening the vault a second time (which would re-prompt for the passphrase
 * and, for writes, race the GUI's in-memory copy). If no GUI is running the CLI
 * falls back to its own local path.
 *
 * The passphrase NEVER crosses the port: the GUI serves requests only while its
 * vault is unlocked, otherwise it answers AAR_LOCKED. AmigaOS has one shared
 * address space, so the GUI writes its reply straight into a buffer the CLI
 * passes by pointer (aar_Buf).
 */
#ifndef AMIAUTH_GUIPORT_H
#define AMIAUTH_GUIPORT_H

#define AMIAUTH_PORT_NAME "AmiAuth"

/* Commands (aar_Cmd). */
enum {
    AAP_GET = 1,    /* arg = account name; reply buf = the code               */
    AAP_LIST,       /* reply buf = "issuer:label\n" per account               */
    AAP_ADD,        /* arg = otpauth:// URI                                   */
    AAP_REMOVE,     /* arg = account name                                     */
    AAP_SHOW,       /* pop the GUI window to the front                        */
    AAP_ADD_SECRET  /* arg = "issuer\nlabel\nbase32secret" (bare-secret ADD;
                     * issuer may be empty, the other two must not be)        */
};

/* Result codes (aar_Result / *result). */
enum {
    AAR_OK = 0,
    AAR_LOCKED,     /* the GUI vault is currently locked                      */
    AAR_NOTFOUND,   /* no account matched                                     */
    AAR_FULL,       /* vault is full                                          */
    AAR_BADARG,     /* e.g. not a valid otpauth:// URI                        */
    AAR_SAVEFAIL    /* mutation applied in memory but the re-save failed      */
};

/* Try to forward a command to a resident GUI. Returns 0 if a GUI handled it
 * (*result set to an AAR_* code, buf filled for GET/LIST), or -1 if no GUI is
 * running (the caller should fall back to opening the vault locally). Portable
 * signature; the real client is Amiga-only (guiport.c), the host build stubs it
 * to always return -1. */
int gui_forward(int cmd, const char *arg, char *buf, unsigned long buflen,
                int *result);

#if defined(__amigaos__) || defined(AMIGA) || defined(amiga)
#include <exec/ports.h>

/* The forwarded request. The CLI allocates it, sets aar_Msg.mn_ReplyPort to its
 * reply port, and PutMsg()s it to the GUI's public port; the GUI fills
 * aar_Result (+ aar_Buf for GET/LIST) and ReplyMsg()s it back. */
struct AmiAuthReq {
    struct Message aar_Msg;
    UWORD  aar_Cmd;
    UWORD  aar_Result;
    STRPTR aar_Arg;         /* command argument (account name / otpauth URI) */
    STRPTR aar_Buf;         /* CLI-owned reply buffer; the GUI writes here    */
    ULONG  aar_BufLen;      /* size of aar_Buf                                */
};
#endif

#endif /* AMIAUTH_GUIPORT_H */
