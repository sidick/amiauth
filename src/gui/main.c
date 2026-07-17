/* main.c — AmiAuth GUI, Stage 1: a read-only ReAction viewer.
 *
 * Opens an always-unlocked vault and shows its accounts in a listbrowser; the
 * selected account's live TOTP/HOTP code and a countdown to the next code update
 * once a second. m68k/AmigaOS only (needs intuition + ReAction/ClassAct). The
 * passphrase flow, clipboard, and the commodity come in later stages.
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <libraries/locale.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <gadgets/fuelgauge.h>

#include <clib/alib_protos.h>       /* DoMethod, NewList */
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/fuelgauge.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "otp.h"
#include "vault.h"
#include "clock.h"
#include "prefs.h"

/* --- library / class bases. Initialised (strong definitions) on purpose: a
 * common/tentative definition lets the linker pull libnix's auto-open stub,
 * which tries to open these ReAction classes as "<name>.library" and aborts
 * before main ("window.library failed to load"). We open them all in
 * open_libs() instead. --- */
struct IntuitionBase *IntuitionBase   = NULL;
struct Library       *UtilityBase     = NULL;
struct Library       *WindowBase      = NULL;   /* window.class          */
struct Library       *LayoutBase      = NULL;   /* gadgets/layout.gadget */
struct Library       *ListBrowserBase = NULL;   /* listbrowser.gadget    */
struct Library       *FuelGaugeBase   = NULL;   /* fuelgauge.gadget      */
struct Library       *ButtonBase      = NULL;   /* button.gadget         */

/* timer.device, for the once-a-second refresh */
static struct MsgPort     *g_tport;
static struct timerequest *g_treq;

enum { GID_LIST = 1, GID_CODE, GID_GAUGE };
enum { PWID_OK = 1, PWID_CANCEL };      /* passphrase requester gadgets */

#define VAULT_PATH_DEFAULT "PROGDIR:AmiAuth.vault"

/* ------------------------------------------------------------------ */

static void close_libs(void)
{
    if (ButtonBase)      CloseLibrary(ButtonBase);
    if (FuelGaugeBase)   CloseLibrary(FuelGaugeBase);
    if (ListBrowserBase) CloseLibrary(ListBrowserBase);
    if (LayoutBase)      CloseLibrary(LayoutBase);
    if (WindowBase)      CloseLibrary(WindowBase);
    if (UtilityBase)     CloseLibrary(UtilityBase);
    if (IntuitionBase)   CloseLibrary((struct Library *)IntuitionBase);
}

static const char *open_libs(void)
{
    IntuitionBase = (struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library", 37);
    UtilityBase   = OpenLibrary((STRPTR)"utility.library", 37);
    WindowBase      = OpenLibrary((STRPTR)"window.class", 0);
    LayoutBase      = OpenLibrary((STRPTR)"gadgets/layout.gadget", 0);
    ListBrowserBase = OpenLibrary((STRPTR)"gadgets/listbrowser.gadget", 0);
    FuelGaugeBase   = OpenLibrary((STRPTR)"gadgets/fuelgauge.gadget", 0);
    ButtonBase      = OpenLibrary((STRPTR)"gadgets/button.gadget", 0);
    if (!IntuitionBase || !UtilityBase)
        return "needs intuition.library / utility.library v37+ (OS 2.04)";
    if (!WindowBase || !LayoutBase || !ListBrowserBase || !FuelGaugeBase || !ButtonBase)
        return "needs ReAction/ClassAct classes (window/layout/listbrowser/"
               "fuelgauge/button)";
    return NULL;
}

static int timer_open(void)
{
    g_tport = CreateMsgPort();
    if (!g_tport) return 0;
    g_treq = (struct timerequest *)CreateIORequest(g_tport, sizeof *g_treq);
    if (!g_treq) return 0;
    return OpenDevice((STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)g_treq, 0) == 0;
}

static void timer_close(void)
{
    if (g_treq) {
        if (!CheckIO((struct IORequest *)g_treq)) { AbortIO((struct IORequest *)g_treq); }
        WaitIO((struct IORequest *)g_treq);
        CloseDevice((struct IORequest *)g_treq);
        DeleteIORequest((struct IORequest *)g_treq);
    }
    if (g_tport) DeleteMsgPort(g_tport);
}

static void timer_arm(ULONG secs)
{
    g_treq->tr_node.io_Command = TR_ADDREQUEST;
    g_treq->tr_time.tv_secs  = secs;
    g_treq->tr_time.tv_micro = 0;
    SendIO((struct IORequest *)g_treq);
}

/* --- clock: corrected UTC via the saved offset, else locale, mirroring the CLI --- */
static long locale_offset(void)
{
    struct Library *LocaleBase = OpenLibrary((STRPTR)"locale.library", 38);
    long off = 0;
    if (LocaleBase) {
        struct Locale *loc = OpenLocale(NULL);
        if (loc) { off = loc->loc_GMTOffset * 60; CloseLocale(loc); }
        CloseLibrary(LocaleBase);
    }
    return off;
}

static void clock_setup(clock_ctx *c)
{
    long off;
    clock_init(c);
    if (prefs_get_long("offset", &off) == 0) clock_set_offset(c, off);
    else if ((off = locale_offset()) != 0)   clock_set_offset(c, off);
}

/* --- vault path: VAULT pref, else env, else default --- */
static const char *vault_path(void)
{
    static char buf[256];
    const char *env = getenv("AMIAUTH_VAULT");
    if (env && env[0]) return env;
    if (prefs_get("vault", buf, sizeof buf) == 0 && buf[0]) return buf;
    return VAULT_PATH_DEFAULT;
}

/* --- modal masked passphrase requester ---------------------------------- *
 * Opens a small window.class window and reads a passphrase, showing one '*'
 * per character (the real keys are captured at window level via VANILLAKEY,
 * never held in a gadget buffer). Returns 1 with the passphrase in `buf`
 * (NUL-terminated), or 0 on cancel (buf left zeroed). `buf` is caller-owned
 * and must live off the stack. Reuses the classes open_libs() already opened.
 */
static int passphrase_request(const char *msg, char *buf, size_t cap)
{
    static char stars[130];                 /* mask display; kept off the stack */
    Object *win, *maskobj, *okobj, *cancelobj;
    struct Window *w;
    ULONG sig = 0;
    size_t len = 0;
    int done = -1;                          /* -1 running, 0 cancel, 1 accept */

    if (cap == 0) return 0;
    buf[0] = '\0';
    stars[0] = '\0';

    maskobj   = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ReadOnly, TRUE, GA_Text, (ULONG)"", TAG_END);
    okobj     = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID, PWID_OK,     GA_RelVerify, TRUE, GA_Text, (ULONG)"OK",     TAG_END);
    cancelobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID, PWID_CANCEL, GA_RelVerify, TRUE, GA_Text, (ULONG)"Cancel", TAG_END);
    {
        Object *labelobj = NewObject(NULL, (STRPTR)"button.gadget",
            GA_ReadOnly, TRUE, GA_Text, (ULONG)msg, TAG_END);
        Object *buttons = NewObject(LAYOUT_GetClass(), NULL,
            LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
            LAYOUT_AddChild,    (ULONG)okobj,
            LAYOUT_AddChild,    (ULONG)cancelobj,
            TAG_END);
        Object *layoutobj = NewObject(LAYOUT_GetClass(), NULL,
            LAYOUT_Orientation,   LAYOUT_ORIENT_VERT,
            LAYOUT_SpaceOuter,    TRUE,
            LAYOUT_AddChild,      (ULONG)labelobj,
            CHILD_WeightedHeight, 0,
            LAYOUT_AddChild,      (ULONG)maskobj,
            CHILD_WeightedHeight, 0,
            LAYOUT_AddChild,      (ULONG)buttons,
            CHILD_WeightedHeight, 0,
            TAG_END);
        win = NewObject(WINDOW_GetClass(), NULL,
            WA_Title,        (ULONG)"AmiAuth - Unlock",
            WA_Activate,     TRUE,
            WA_CloseGadget,  TRUE,
            WA_DragBar,      TRUE,
            WA_DepthGadget,  TRUE,
            WA_IDCMP,        IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_VANILLAKEY,
            WINDOW_Position, WPOS_CENTERSCREEN,
            WINDOW_Layout,   (ULONG)layoutobj,
            TAG_END);
    }
    if (!win) return 0;

    w = (struct Window *)DoMethod(win, WM_OPEN, NULL);
    if (!w) { DisposeObject(win); return 0; }
    GetAttr(WINDOW_SigMask, win, &sig);

    while (done < 0) {
        ULONG got = Wait(sig | SIGBREAKF_CTRL_C);
        ULONG r;
        UWORD code;
        if (got & SIGBREAKF_CTRL_C) { done = 0; break; }
        while ((r = DoMethod(win, WM_HANDLEINPUT, (ULONG)&code)) != WMHI_LASTMSG) {
            switch (r & WMHI_CLASSMASK) {
                case WMHI_CLOSEWINDOW:
                    done = 0;
                    break;
                case WMHI_GADGETUP:
                    if      ((r & WMHI_GADGETMASK) == PWID_OK)     done = 1;
                    else if ((r & WMHI_GADGETMASK) == PWID_CANCEL) done = 0;
                    break;
                case WMHI_VANILLAKEY:
                    if      (code == 0x0D) done = 1;                 /* Return */
                    else if (code == 0x1B) done = 0;                 /* Escape */
                    else if (code == 0x08 || code == 0x7F) {         /* Backspace/Del */
                        if (len > 0) {
                            buf[--len] = '\0'; stars[len] = '\0';
                            SetGadgetAttrs((struct Gadget *)maskobj, w, NULL,
                                           GA_Text, (ULONG)stars, TAG_END);
                        }
                    } else if (code >= 0x20 && code < 0x7F) {        /* printable */
                        if (len < cap - 1 && len < sizeof stars - 1) {
                            buf[len] = (char)code; stars[len] = '*';
                            len++;
                            buf[len] = '\0'; stars[len] = '\0';
                            SetGadgetAttrs((struct Gadget *)maskobj, w, NULL,
                                           GA_Text, (ULONG)stars, TAG_END);
                        }
                    }
                    break;
            }
            if (done >= 0) break;
        }
    }

    DoMethod(win, WM_CLOSE, NULL);
    DisposeObject(win);
    if (done != 1) memset(buf, 0, cap);
    memset(stars, 0, sizeof stars);
    return done == 1 ? 1 : 0;
}

/* ------------------------------------------------------------------ */

int main(void)
{
    static vault v;                     /* ~19 KB: keep it off the stack */
    static char pass[128];              /* passphrase buffer; off the stack, scrubbed after use */
    struct List lblist;
    Object *winobj = NULL, *codeobj = NULL, *gaugeobj = NULL, *listobj = NULL;
    struct Window *win = NULL;
    clock_ctx clk;
    const char *err, *path;
    ULONG winsig = 0, timersig, sel = 0;
    int have_timer = 0, running = 1;
    size_t i;
    vault_result rc;

    NewList(&lblist);

    if ((err = open_libs()) != NULL) {
        Printf((CONST_STRPTR)"AmiAuth: %s\n", (ULONG)err);
        close_libs();
        return 20;
    }

    path = vault_path();
    rc = vault_load(&v, path, NULL);
    if (rc == VAULT_ERR_LOCKED) {
        const char *msg = "Enter passphrase:";
        for (;;) {
            if (!passphrase_request(msg, pass, sizeof pass)) {
                close_libs();              /* user cancelled — a clean exit */
                return 0;
            }
            rc = vault_load(&v, path, pass);
            memset(pass, 0, sizeof pass);  /* scrub the passphrase immediately */
            if (rc == VAULT_OK) break;
            if (rc == VAULT_ERR_AUTH) { msg = "Wrong passphrase - try again:"; continue; }
            break;                         /* IO/format error: reported below */
        }
    }
    if (rc != VAULT_OK) {
        Printf((CONST_STRPTR)"AmiAuth: cannot open the vault (%ld)\n", (LONG)rc);
        close_libs();
        return 10;
    }

    clock_setup(&clk);

    /* Build the account list (issuer:label per row). */
    for (i = 0; i < v.count; i++) {
        const otp_account *a = &v.accounts[i];
        char label[OTP_MAX_ISSUER + OTP_MAX_LABEL + 2];
        struct Node *node;
        if (a->issuer[0]) { strcpy(label, a->issuer); strcat(label, ":"); strcat(label, a->label); }
        else              strcpy(label, a->label);
        node = (struct Node *)AllocListBrowserNode(1,
            LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, (ULONG)label,
            TAG_END);
        if (node) AddTail(&lblist, node);
    }

    have_timer = timer_open();

    /* Build the gadgets explicitly (the ReAction *Object/End builder macros rely
     * on NewObject not being a function-like macro, which it is here). */
    listobj = NewObject(LISTBROWSER_GetClass(), NULL,
        GA_ID,                    GID_LIST,
        GA_RelVerify,             TRUE,
        LISTBROWSER_Labels,       (ULONG)&lblist,
        LISTBROWSER_ShowSelected, TRUE,
        LISTBROWSER_Selected,     0,
        TAG_END);
    codeobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID,       GID_CODE,
        GA_ReadOnly, TRUE,
        GA_Text,     (ULONG)"------",
        TAG_END);
    gaugeobj = NewObject(FUELGAUGE_GetClass(), NULL,
        GA_ID,             GID_GAUGE,
        FUELGAUGE_Min,     0,
        FUELGAUGE_Max,     30,
        FUELGAUGE_Level,   0,
        FUELGAUGE_Percent, FALSE,
        FUELGAUGE_Ticks,   0,
        TAG_END);
    {
        Object *layoutobj = NewObject(LAYOUT_GetClass(), NULL,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_SpaceOuter,  TRUE,
            LAYOUT_AddChild,    (ULONG)listobj,
            LAYOUT_AddChild,    (ULONG)codeobj,
            CHILD_WeightedHeight, 0,
            LAYOUT_AddChild,    (ULONG)gaugeobj,
            CHILD_WeightedHeight, 0,
            TAG_END);
        winobj = NewObject(WINDOW_GetClass(), NULL,
            WA_Title,        (ULONG)"AmiAuth",
            WA_Activate,     TRUE,
            WA_CloseGadget,  TRUE,
            WA_DragBar,      TRUE,
            WA_DepthGadget,  TRUE,
            WA_SizeGadget,   TRUE,
            WA_IDCMP,        IDCMP_CLOSEWINDOW | IDCMP_GADGETUP,
            WINDOW_Position, WPOS_CENTERSCREEN,
            WINDOW_Layout,   (ULONG)layoutobj,
            TAG_END);
    }

    if (!winobj) {
        Printf((CONST_STRPTR)"AmiAuth: could not create the window\n");
        goto cleanup;
    }

    win = (struct Window *)DoMethod(winobj, WM_OPEN, NULL);
    if (!win) {
        Printf((CONST_STRPTR)"AmiAuth: could not open the window\n");
        goto cleanup;
    }

    GetAttr(WINDOW_SigMask, winobj, &winsig);
    timersig = have_timer ? (1UL << g_tport->mp_SigBit) : 0;
    if (have_timer) timer_arm(1);

    while (running) {
        ULONG sigs = Wait(winsig | timersig | SIGBREAKF_CTRL_C);

        if (sigs & SIGBREAKF_CTRL_C) running = 0;

        if (have_timer && (sigs & timersig)) {
            WaitIO((struct IORequest *)g_treq);   /* consume the request */
            /* recompute + refresh below, then re-arm */
            timer_arm(1);
        }

        if (sigs & winsig) {
            ULONG result;
            UWORD code;
            while ((result = DoMethod(winobj, WM_HANDLEINPUT, (ULONG)&code)) != WMHI_LASTMSG) {
                switch (result & WMHI_CLASSMASK) {
                    case WMHI_CLOSEWINDOW:
                        running = 0;
                        break;
                    case WMHI_GADGETUP:
                        if ((result & WMHI_GADGETMASK) == GID_LIST)
                            GetAttr(LISTBROWSER_Selected, listobj, &sel);
                        break;
                }
            }
        }

        /* Refresh the selected account's code + countdown. */
        if (running && v.count > 0) {
            const otp_account *a;
            uint64_t now = clock_now_utc(&clk);
            char buf[16], fmt[8];
            uint32_t code, remaining, period;
            if (sel >= v.count) sel = 0;
            a = &v.accounts[sel];
            period = a->period ? a->period : OTP_DEFAULT_PERIOD;
            code = totp_sha1(a->secret, a->secret_len, now, 0, period, a->digits);
            remaining = totp_seconds_remaining(now, 0, period);
            /* libnix sprintf lacks '*' width, so build "%06lu"/"%08lu". */
            sprintf(fmt, "%%0%dlu", (int)a->digits);
            sprintf(buf, fmt, (unsigned long)code);
            SetGadgetAttrs((struct Gadget *)codeobj, win, NULL,
                           GA_Text, (ULONG)buf, TAG_END);
            SetGadgetAttrs((struct Gadget *)gaugeobj, win, NULL,
                           FUELGAUGE_Max, period, FUELGAUGE_Level, remaining, TAG_END);
        }
    }

cleanup:
    if (win) DoMethod(winobj, WM_CLOSE, NULL);
    if (winobj) DisposeObject(winobj);
    /* free the listbrowser nodes */
    {
        struct Node *n;
        while ((n = RemHead(&lblist)) != NULL) FreeListBrowserNode(n);
    }
    if (have_timer) timer_close();
    vault_lock(&v);
    close_libs();
    return 0;
}
