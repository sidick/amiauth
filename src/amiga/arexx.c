/* arexx.c -- see arexx.h. AmigaOS only (never built into the CLI or host). */
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/ports.h>
#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>

#include <stdio.h>
#include <string.h>

#include "arexx.h"

/* Defined here (the one file that actually calls rexxsyslib functions);
 * <proto/rexxsyslib.h>'s inline stubs reference this exact global by name,
 * same convention as SysBase/IntuitionBase/GfxBase elsewhere in this
 * project - it must NOT be static. gui/main.c's open_libs()/close_libs()
 * own its OpenLibrary()/CloseLibrary() lifecycle, matching every other
 * library base in this app. */
struct RxsLib *RexxSysBase = NULL;

/* Try slots 1..99 (a generous, arbitrary cap - realistically 1-2 instances
 * ever run) under one Forbid() so two AmiAuth processes launched at once
 * can't race onto the same name. */
#define AREXX_MAX_SLOT 99

struct MsgPort *arexx_open(const char *portname_override,
                           char *out_name, size_t out_name_cap)
{
    static char name[32];   /* "AMIAUTH.NN" - static: AddPort() keeps a
                              * reference, must outlive the port itself */
    struct MsgPort *port = NULL;

    if (!RexxSysBase) return NULL;

    Forbid();
    if (portname_override && portname_override[0]) {
        strncpy(name, portname_override, sizeof name - 1);
        name[sizeof name - 1] = '\0';
        if (!FindPort((CONST_STRPTR)name)) {
            port = CreateMsgPort();
            if (port) { port->mp_Node.ln_Name = name; AddPort(port); }
        }
    } else {
        int n;
        for (n = 1; n <= AREXX_MAX_SLOT; n++) {
            sprintf(name, "AMIAUTH.%d", n);
            if (!FindPort((CONST_STRPTR)name)) {
                port = CreateMsgPort();
                if (port) { port->mp_Node.ln_Name = name; AddPort(port); }
                break;
            }
        }
    }
    Permit();

    if (port && out_name && out_name_cap) {
        strncpy(out_name, name, out_name_cap - 1);
        out_name[out_name_cap - 1] = '\0';
    }
    return port;
}

void arexx_close(struct MsgPort *port)
{
    struct RexxMsg *msg;
    if (!port) return;
    Forbid();
    RemPort(port);
    Permit();
    while ((msg = (struct RexxMsg *)GetMsg(port)) != NULL) {
        /* Same caution as arexx_receive(): only touch RexxMsg-specific
         * fields (via arexx_reply) once IsRexxMsg confirms the layout. */
        if (IsRexxMsg(msg)) arexx_reply(msg, AREXX_RC_FAIL, NULL);
        else                ReplyMsg((struct Message *)msg);
    }
    DeleteMsgPort(port);
}

void *arexx_receive(struct MsgPort *port, arexx_parsed *out)
{
    struct RexxMsg *msg;
    while ((msg = (struct RexxMsg *)GetMsg(port)) != NULL) {
        if (!IsRexxMsg(msg)) {
            /* Not ours; shouldn't happen on a dedicated ARexx port ("AMIAUTH.N"),
             * but every AmigaOS message must be replied by its receiver or the
             * sender leaks/hangs waiting on a reply that never comes - drop
             * only the assumption that it's a RexxMsg, not the reply itself.
             * Plain ReplyMsg() only touches the generic Message fields
             * (guaranteed by whoever sent it), unlike arexx_reply() which
             * writes RexxMsg-specific fields we can't assume are there. */
            ReplyMsg((struct Message *)msg);
            continue;
        }
        if (arexx_parse((const char *)ARG0(msg), out) != 0)
            out->type = AREXX_CMD_UNKNOWN;   /* still reply - RC 10, see caller */
        return (void *)msg;
    }
    return NULL;
}

void arexx_reply(void *handle, int rc, const char *result)
{
    struct RexxMsg *msg = (struct RexxMsg *)handle;
    if (!msg) return;
    msg->rm_Result1 = rc;
    msg->rm_Result2 = 0;
    if ((msg->rm_Action & RXFF_RESULT) && result && result[0])
        msg->rm_Result2 = (LONG)CreateArgstring((UBYTE *)result, (ULONG)strlen(result));
    ReplyMsg((struct Message *)msg);
    /* Do not DeleteArgstring(rm_Result2) here - ARexx frees it after
     * consuming the reply. See arexx.h's note on this. */
}
