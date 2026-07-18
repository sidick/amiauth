/* main.c — AmiAuth GUI: a ReAction viewer for the account vault.
 *
 * Opens the vault (prompting for the passphrase if it is encrypted) and shows
 * its accounts in a listbrowser; the selected account's live TOTP/HOTP code and
 * a countdown to the next code update once a second, with a "Copy" button (and
 * double-click) that puts the code on the clipboard. m68k/AmigaOS only (needs
 * intuition + ReAction/ClassAct). The commodity shell comes in a later stage.
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <devices/clipboard.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <libraries/locale.h>
#include <libraries/iffparse.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>          /* struct ColorMap, OBP_*, PRECISION_* */

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
#include <proto/iffparse.h>
#include <proto/graphics.h>
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
struct Library       *IFFParseBase    = NULL;   /* iffparse.library (optional) */
struct GfxBase       *GfxBase         = NULL;   /* graphics.library (LED)  */

/* clock-status LED: red/amber/green pens indexed by clock_state (-1 = none) */
static LONG g_ledpen[3] = { -1, -1, -1 };

/* timer.device, for the once-a-second refresh */
static struct MsgPort     *g_tport;
static struct timerequest *g_treq;

/* clipboard (PRIMARY_CLIP) via iffparse.library, for "copy code" */
static struct ClipboardHandle *g_clip;
static struct IFFHandle        *g_iff;

#define ID_FTXT MAKE_ID('F','T','X','T')
#define ID_CHRS MAKE_ID('C','H','R','S')

#define CLIP_CLEAR_SECS 30      /* wipe our copied code off the clipboard after this */

enum { GID_LIST = 1, GID_CODE, GID_GAUGE, GID_COPY };
enum { PWID_OK = 1, PWID_CANCEL };      /* passphrase requester gadgets */

#define VAULT_PATH_DEFAULT "PROGDIR:AmiAuth.vault"

/* Multi-column account list: name | live code | seconds-left. The code/left
 * cells change every second, so the nodes point at these persistent buffers
 * (LBNCA_CopyText FALSE) and we rewrite them in place; kept off the Amiga
 * stack. Weighted column widths (CIF_RIGHT/CENTER are V47, below our baseline).*/
static char g_code[VAULT_MAX_ACCOUNTS][12];
static char g_left[VAULT_MAX_ACCOUNTS][8];
static struct ColumnInfo g_columns[] = {
    { 50, (STRPTR)"Account", CIF_WEIGHTED },
    { 34, (STRPTR)"Code",    CIF_WEIGHTED },
    { 16, (STRPTR)"Left",    CIF_WEIGHTED },
    { -1, NULL, 0 }
};

/* ------------------------------------------------------------------ */

static void close_libs(void)
{
    if (GfxBase)         CloseLibrary((struct Library *)GfxBase);
    if (IFFParseBase)    CloseLibrary(IFFParseBase);
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
    IFFParseBase    = OpenLibrary((STRPTR)"iffparse.library", 37);  /* optional: clipboard copy */
    GfxBase         = (struct GfxBase *)OpenLibrary((STRPTR)"graphics.library", 37);
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

/* --- clock-status LED + text ------------------------------------------- */
static void clock_status_text(const clock_ctx *c, char *buf)
{
    long off = c->offset_seconds;
    const char *sign = off < 0 ? "-" : "+";     /* %s, not %c (libnix sprintf) */
    unsigned long a = (unsigned long)(off < 0 ? -off : off);
    const char *word = c->state == CLOCK_SYNCED ? "synced"
                     : c->state == CLOCK_MANUAL ? "manual"
                     : "unverified";
    if (c->state == CLOCK_UNVERIFIED)
        sprintf(buf, "Clock: %s", word);
    else
        sprintf(buf, "Clock: %s %s%lu:%02lu", word, sign, a / 3600, (a % 3600) / 60);
}

/* Allocate the red/amber/green pens from the window's screen (best-effort). */
static void led_pens_alloc(struct Window *win)
{
    static const ULONG rgb[3][3] = {
        { 0xFFFFFFFFUL, 0x00000000UL, 0x00000000UL },   /* red   - unverified */
        { 0xFFFFFFFFUL, 0xAAAAAAAAUL, 0x00000000UL },   /* amber - manual     */
        { 0x00000000UL, 0xCCCCCCCCUL, 0x00000000UL }    /* green - synced     */
    };
    struct TagItem tags[] = { { OBP_Precision, PRECISION_GUI }, { TAG_END, 0 } };
    struct ColorMap *cm;
    int i;
    if (!GfxBase || !win || !win->WScreen) return;
    cm = win->WScreen->ViewPort.ColorMap;
    if (!cm) return;
    for (i = 0; i < 3; i++)
        g_ledpen[i] = ObtainBestPenA(cm, rgb[i][0], rgb[i][1], rgb[i][2], tags);
}

static void led_pens_free(struct Window *win)
{
    struct ColorMap *cm;
    int i;
    if (!GfxBase || !win || !win->WScreen) return;
    cm = win->WScreen->ViewPort.ColorMap;
    for (i = 0; i < 3; i++)
        if (g_ledpen[i] != -1) { ReleasePen(cm, (ULONG)g_ledpen[i]); g_ledpen[i] = -1; }
}

/* Draw a small red/amber/green LED square in the left margin of the status
 * label gadget (its text is centred, so the left is clear). Uses the gadget's
 * laid-out bounds, so it needs no layout sizing of its own. */
static void led_draw(struct Window *win, Object *labelobj, int state)
{
    struct Gadget *g = (struct Gadget *)labelobj;
    LONG pen, x, y, s = 10;
    if (!GfxBase || !labelobj || state < 0 || state > 2) return;
    if (g->Width <= s + 6 || g->Height <= s) return;   /* not laid out yet */
    pen = g_ledpen[state] >= 0 ? g_ledpen[state] : 1;   /* fallback: visible */
    x = g->LeftEdge + 4;
    y = g->TopEdge + (g->Height - s) / 2;
    SetAPen(win->RPort, 1);                             /* black bezel */
    RectFill(win->RPort, x, y, x + s - 1, y + s - 1);
    SetAPen(win->RPort, (ULONG)pen);                    /* colour */
    RectFill(win->RPort, x + 1, y + 1, x + s - 2, y + s - 2);
}

/* --- clipboard: put the current code on the primary clipboard as IFF FTXT,
 * via iffparse.library (the OS-standard IFF path). All best-effort: if
 * iffparse or the clipboard is unavailable, clip_open() returns 0 and the copy
 * feature is simply disabled. --- */
static int clip_open(void)
{
    if (!IFFParseBase) return 0;
    g_clip = OpenClipboard(PRIMARY_CLIP);
    if (!g_clip) return 0;
    g_iff = AllocIFF();
    if (!g_iff) return 0;
    g_iff->iff_Stream = (ULONG)g_clip;
    InitIFFasClip(g_iff);
    return 1;
}

static void clip_close(void)
{
    if (g_iff)  FreeIFF(g_iff);
    if (g_clip) CloseClipboard(g_clip);
}

/* The clip ID of the most recent clipboard write (0 if unavailable). Advances
 * whenever anyone posts, so it tells us whether our clip is still the current
 * one. Reuses the IOClipReq embedded in the iffparse ClipboardHandle. */
static ULONG clip_write_id(void)
{
    if (!g_clip) return 0;
    g_clip->cbh_Req.io_Command = CBD_CURRENTWRITEID;
    DoIO((struct IORequest *)&g_clip->cbh_Req);
    return (ULONG)g_clip->cbh_Req.io_ClipID;
}

/* Put `text` on the clipboard as an IFF FORM FTXT with one CHRS chunk (empty
 * text -> an empty FTXT, which clears the clip). Returns the resulting write ID
 * so the caller can later tell whether the clip is still ours; 0 on failure. */
static ULONG clip_write_text(const char *text)
{
    ULONG len = (ULONG)strlen(text);
    if (!g_iff) return 0;
    if (OpenIFF(g_iff, IFFF_WRITE) != 0) return 0;
    if (PushChunk(g_iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN) == 0) {
        if (len && PushChunk(g_iff, 0, ID_CHRS, IFFSIZE_UNKNOWN) == 0) {
            WriteChunkBytes(g_iff, (APTR)text, (LONG)len);  /* iffparse pads/sizes */
            PopChunk(g_iff);                                /* CHRS */
        }
        PopChunk(g_iff);                                    /* FORM */
    }
    CloseIFF(g_iff);
    return clip_write_id();
}

/* Overwrite the clipboard with an empty FTXT, wiping our copied code. Called
 * only when the clip is still ours (see the auto-clear countdown). */
static void clip_clear(void)
{
    clip_write_text("");
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
    Object *winobj = NULL, *codeobj = NULL, *gaugeobj = NULL, *listobj = NULL, *copyobj = NULL;
    Object *statobj = NULL;
    struct Window *win = NULL;
    clock_ctx clk;
    const char *err, *path;
    ULONG winsig = 0, timersig, sel = 0, lastsec = 0, lastmic = 0, our_clipid = 0;
    int have_timer = 0, have_clip = 0, running = 1, copied = 0, clear_secs = 0;
    char curcode[16], statbuf[48];
    size_t i;
    vault_result rc;

    curcode[0] = '\0';

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

    /* Build the account list: [name | code | left]. Column 0 (name) is copied
     * into the node; columns 1/2 point at g_code[i]/g_left[i], refreshed live. */
    for (i = 0; i < v.count; i++) {
        const otp_account *a = &v.accounts[i];
        char label[OTP_MAX_ISSUER + OTP_MAX_LABEL + 2];
        struct Node *node;
        if (a->issuer[0]) { strcpy(label, a->issuer); strcat(label, ":"); strcat(label, a->label); }
        else              strcpy(label, a->label);
        strcpy(g_code[i], "------");
        g_left[i][0] = '\0';
        node = (struct Node *)AllocListBrowserNode(3,
            LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, (ULONG)label,
            LBNA_Column, 1, LBNCA_Text, (ULONG)g_code[i],
            LBNA_Column, 2, LBNCA_Text, (ULONG)g_left[i],
            TAG_END);
        if (node) AddTail(&lblist, node);
    }

    have_timer = timer_open();
    have_clip  = clip_open();

    /* Build the gadgets explicitly (the ReAction *Object/End builder macros rely
     * on NewObject not being a function-like macro, which it is here). */
    listobj = NewObject(LISTBROWSER_GetClass(), NULL,
        GA_ID,                    GID_LIST,
        GA_RelVerify,             TRUE,
        LISTBROWSER_ColumnInfo,   (ULONG)g_columns,
        LISTBROWSER_ColumnTitles, TRUE,
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
    copyobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID,        GID_COPY,
        GA_RelVerify, TRUE,
        GA_Disabled,  !have_clip,           /* no clipboard.device -> greyed out */
        GA_Text,      (ULONG)"Copy",
        TAG_END);
    /* Clock-status label (a full-width read-only field). It states the trust
     * level + offset in words; we draw a small red/amber/green LED into its
     * left margin ourselves (led_draw) — the centred text clears the far left. */
    clock_status_text(&clk, statbuf);
    statobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ReadOnly, TRUE,
        GA_Text,     (ULONG)statbuf,
        TAG_END);
    {
        Object *layoutobj = NewObject(LAYOUT_GetClass(), NULL,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_SpaceOuter,  TRUE,
            LAYOUT_AddChild,    (ULONG)listobj,
            CHILD_MinWidth,     320,            /* room for three columns + titles */
            CHILD_MinHeight,    100,            /* show several accounts at once */
            LAYOUT_AddChild,    (ULONG)codeobj,
            CHILD_WeightedHeight, 0,
            LAYOUT_AddChild,    (ULONG)gaugeobj,
            CHILD_WeightedHeight, 0,
            LAYOUT_AddChild,    (ULONG)copyobj,
            CHILD_WeightedHeight, 0,
            LAYOUT_AddChild,    (ULONG)statobj,
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

    led_pens_alloc(win);                      /* red/amber/green from the screen */
    led_draw(win, statobj, clk.state);        /* initial LED */

    while (running) {
        ULONG sigs = Wait(winsig | timersig | SIGBREAKF_CTRL_C);

        if (sigs & SIGBREAKF_CTRL_C) running = 0;

        if (have_timer && (sigs & timersig)) {
            WaitIO((struct IORequest *)g_treq);   /* consume the request */
            if (copied > 0 && --copied == 0)      /* revert the "Copied" flash */
                SetGadgetAttrs((struct Gadget *)copyobj, win, NULL,
                               GA_Text, (ULONG)"Copy", TAG_END);
            /* auto-clear our copied code once it has sat ~30s, but only if the
             * clipboard is still ours (don't clobber something copied since). */
            if (clear_secs > 0 && --clear_secs == 0 && clip_write_id() == our_clipid)
                clip_clear();
            /* recompute + refresh below, then re-arm */
            timer_arm(1);
        }

        if (sigs & winsig) {
            ULONG result;
            UWORD code;
            int docopy = 0;
            while ((result = DoMethod(winobj, WM_HANDLEINPUT, (ULONG)&code)) != WMHI_LASTMSG) {
                switch (result & WMHI_CLASSMASK) {
                    case WMHI_CLOSEWINDOW:
                        running = 0;
                        break;
                    case WMHI_GADGETUP:
                        switch (result & WMHI_GADGETMASK) {
                            case GID_LIST: {
                                ULONG cs = 0, cm = 0;
                                GetAttr(LISTBROWSER_Selected, listobj, &sel);
                                CurrentTime(&cs, &cm);       /* a quick second click = copy */
                                if (DoubleClick(lastsec, lastmic, cs, cm)) docopy = 1;
                                lastsec = cs; lastmic = cm;
                                break;
                            }
                            case GID_COPY:
                                docopy = 1;
                                break;
                        }
                        break;
                }
            }
            if (docopy && have_clip && curcode[0]) {
                our_clipid = clip_write_text(curcode);
                clear_secs = CLIP_CLEAR_SECS;         /* start/reset auto-clear */
                SetGadgetAttrs((struct Gadget *)copyobj, win, NULL,
                               GA_Text, (ULONG)"Copied", TAG_END);
                copied = 2;                           /* revert after ~2 ticks */
            }
        }

        /* Refresh every account's code + countdown in the list, and drive the
         * detail pane (big code + gauge) from the selected row. */
        if (running && v.count > 0) {
            uint64_t now = clock_now_utc(&clk);
            uint32_t sel_period = OTP_DEFAULT_PERIOD, sel_rem = 0;
            char fmt[8];
            if (sel >= v.count) sel = 0;
            for (i = 0; i < v.count; i++) {
                const otp_account *a = &v.accounts[i];
                uint32_t period = a->period ? a->period : OTP_DEFAULT_PERIOD;
                uint32_t code = totp_sha1(a->secret, a->secret_len, now, 0, period, a->digits);
                uint32_t rem  = totp_seconds_remaining(now, 0, period);
                /* libnix sprintf lacks '*' width, so build "%06lu"/"%08lu". */
                sprintf(fmt, "%%0%dlu", (int)a->digits);
                sprintf(g_code[i], fmt, (unsigned long)code);
                /* fixed 2-digit field so the text never shrinks (a shrinking
                 * cell leaves stale pixels the listbrowser doesn't clear). */
                sprintf(g_left[i], "%2lus", (unsigned long)rem);
                if (i == sel) { sel_period = period; sel_rem = rem; }
            }
            /* The nodes point at g_code[]/g_left[]; re-set Labels to repaint.
             * Cell text is fixed-width (codes and the "NNs" countdown), so no
             * stale pixels are left behind. */
            SetGadgetAttrs((struct Gadget *)listobj, win, NULL,
                           LISTBROWSER_Labels, (ULONG)&lblist, TAG_END);
            strcpy(curcode, g_code[sel]);      /* selected row -> detail + copy */
            SetGadgetAttrs((struct Gadget *)codeobj, win, NULL,
                           GA_Text, (ULONG)curcode, TAG_END);
            SetGadgetAttrs((struct Gadget *)gaugeobj, win, NULL,
                           FUELGAUGE_Max, sel_period, FUELGAUGE_Level, sel_rem, TAG_END);
        }

        if (running) led_draw(win, statobj, clk.state);  /* keep the LED lit */
    }

cleanup:
    led_pens_free(win);                       /* while the screen is still valid */
    if (win) DoMethod(winobj, WM_CLOSE, NULL);
    if (winobj) DisposeObject(winobj);
    /* free the listbrowser nodes */
    {
        struct Node *n;
        while ((n = RemHead(&lblist)) != NULL) FreeListBrowserNode(n);
    }
    if (have_clip) clip_close();
    if (have_timer) timer_close();
    vault_lock(&v);
    close_libs();
    return 0;
}
