/* guiport.c — CLI-side client: forward a command to a resident AmiAuthGUI.
 *
 * AmigaOS only (public message port IPC). On the host the CLI stubs gui_forward
 * to return -1 (there is never a resident GUI). See guiport.h.
 */
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/ports.h>
#include <proto/exec.h>

#include <string.h>

#include "guiport.h"

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

    WaitPort(reply);
    GetMsg(reply);                        /* reclaim our own request */
    if (result) *result = (int)req.aar_Result;
    DeleteMsgPort(reply);
    return 0;
}
