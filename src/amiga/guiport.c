/* guiport.c — CLI-side client: forward a command to a resident AmiAuthGUI.
 *
 * AmigaOS only (public message port IPC). On the host the CLI stubs gui_forward
 * to return -1 (there is never a resident GUI). See guiport.h.
 */
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/ports.h>
#include <dos/dos.h>            /* SIGBREAKF_CTRL_C */
#include <proto/exec.h>
#include <proto/dos.h>          /* PutStr (Ctrl-C notice) */

#include <string.h>

#include "guiport.h"
#include "catalog.h"            /* MSG(): localizable Ctrl-C notice */

int gui_forward(int cmd, const char *arg, char *buf, unsigned long buflen,
                int *result)
{
    struct MsgPort  *guiport, *reply;
    struct AmiAuthReq req;

    if (buf && buflen) buf[0] = '\0';

    reply = CreateMsgPort();
    if (!reply) return -1;

    memset(&req, 0, sizeof req);
    req.aar_Msg.mn_Node.ln_Type = NT_MESSAGE;
    req.aar_Msg.mn_Length       = sizeof req;
    req.aar_Msg.mn_ReplyPort    = reply;
    req.aar_Cmd    = (UWORD)cmd;
    req.aar_Arg    = (STRPTR)arg;
    req.aar_Buf    = (STRPTR)buf;
    req.aar_BufLen = buflen;

    /* Find the resident port and post under Forbid() so it can't disappear
     * between FindPort() and PutMsg(). */
    Forbid();
    guiport = FindPort((CONST_STRPTR)AMIAUTH_PORT_NAME);
    if (guiport)
        PutMsg(guiport, &req.aar_Msg);
    Permit();

    if (!guiport) {                       /* no GUI running -> caller goes local */
        DeleteMsgPort(reply);
        return -1;
    }

    /* Wait for the reply, but notice Ctrl-C. We must NOT abort the exchange:
     * the GUI holds pointers into this stack frame (req/arg/buf), so leaving
     * before the reply arrives would have it write into a dead frame. A
     * crashed GUI still means waiting forever, but now the user is told what
     * the CLI is stuck on instead of it being silently unkillable. */
    {
        ULONG portsig = 1UL << reply->mp_SigBit;
        int warned = 0;
        for (;;) {
            ULONG got = Wait(portsig | SIGBREAKF_CTRL_C);
            if (GetMsg(reply)) break;     /* reclaim our own request */
            if ((got & SIGBREAKF_CTRL_C) && !warned) {
                PutStr((CONST_STRPTR)MSG(MSG_CLI_GUI_WAIT));
                warned = 1;
            }
        }
    }
    if (result) *result = (int)req.aar_Result;
    DeleteMsgPort(reply);
    return 0;
}
