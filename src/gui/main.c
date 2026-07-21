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
#include <intuition/screens.h>     /* DrawInfo, SHADOWPEN/SHINEPEN (LED bevel) */
#include <libraries/locale.h>
#include <libraries/iffparse.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>          /* struct ColorMap, OBP_*, PRECISION_* */

#include <libraries/gadtools.h>     /* struct NewMenu, NM_*, GTMENUITEM_USERDATA */
#include <libraries/asl.h>          /* ASL_FileRequest, ASLFR_* (QR file picker) */
#include <libraries/commodities.h>  /* NewBroker, CxMsg, CXCMD_*, NBU_* (Stage 3) */
#include <workbench/workbench.h>    /* struct AppMessage (QR drag-and-drop) */
#include <workbench/startup.h>      /* struct WBArg */

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <gadgets/fuelgauge.h>
#include <gadgets/string.h>         /* STRINGA_* (typed-URI requester) */

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
#include <proto/string.h>       /* STRING_GetClass */
#include <proto/asl.h>          /* AllocAslRequestTags, AslRequest (QR picker) */
#include <proto/commodities.h>  /* CxBroker, HotKey, CxMsgType/ID (Stage 3) */
#include <proto/wb.h>           /* AddAppWindowA/RemoveAppWindow (QR drag-and-drop);
                                  * note: workbench.library's proto header is
                                  * "wb.h", not "workbench.h" */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "otp.h"
#include "vault.h"
#include "clock.h"
#include "prefs.h"
#include "entropy.h"                /* amiga_random (m68k build) */
#include "qr.h"                     /* qr_decode_gray (portable QR decoder) */
#include "pbkdf2.h"                 /* first-run KDF calibration probe */
#include "../version.h"

AMIAUTH_VERSTAG("AmiAuthGUI")
#include "qrimage.h"                /* qrimage_load_gray (datatypes glue) */
#include "guiport.h"                /* CLI->GUI IPC (Stage 3b public port) */
#include "crypto_select.h"          /* select crypto hot-loop impl, #47 */

/* Request a larger stack (libnix): the QR decoder still needs more than the
 * few KB a shell hands a Run/WBench program. qr_decode_gray keeps its big
 * quirc_code/quirc_data off the stack (heap), but quirc_decode uses a ~9 KB
 * datastream internally; 32 KB leaves ample headroom. Harmless to the rest of
 * the GUI. */
unsigned long __stack = 32768;

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
struct Library       *StringBase      = NULL;   /* string.gadget (typed URI) */
struct Library       *GadToolsBase    = NULL;   /* gadtools.library (menus)  */
struct Library       *DataTypesBase   = NULL;   /* datatypes.library (QR import: image load) */
struct Library       *AslBase         = NULL;   /* asl.library (QR import: file requester)   */
struct Library       *WorkbenchBase   = NULL;   /* workbench.library (QR import: drag-and-drop) */
struct Library       *CxBase          = NULL;   /* commodities.library (Stage 3: broker/hotkey) */

/* AppWindow message port: image icons dropped on the main window (QR import).
 * Created only when datatypes + workbench are available. g_appwin is the
 * handle from AddAppWindowA(), live only while win is actually open (see
 * win_show/win_hide) — registering the window as a Workbench drop target is
 * a runtime call against the real struct Window*, not a window.class
 * creation tag (WINDOW_AppPort alone, tried first, only wires iconify
 * support and never actually registers a drop target — #37). */
static struct MsgPort   *g_appport = NULL;
static struct AppWindow *g_appwin  = NULL;

/* Last known window position/size, remembered across a hide/show cycle
 * (Exchange Hide/Show, idle auto-lock, the close gadget) so re-showing the
 * window doesn't snap back to WPOS_CENTERSCREEN. -1 = no size seen yet. */
static WORD g_winleft = -1, g_wintop = -1, g_winw = -1, g_winh = -1;

/* Named public screen to open on (PUBSCREEN tooltype/arg), read once at
 * startup; empty = the default public screen, as before. Falls back to the
 * default automatically (WA_PubScreenFallBack) if the named screen isn't
 * open when we try. */
static char g_pubscreen[MAXPUBSCREENNAME + 1] = "";

/* Explicit vault override for this launch (VAULT tooltype/arg), read once at
 * startup; same precedence and session-only scope as the CLI's VAULT/K. */
static char g_vault_arg[256] = "";

/* clock-status LED: red/amber/green pens indexed by clock_state (-1 = none),
 * drawn in a recessed bevel (shadow/shine pens from the screen's DrawInfo). */
static LONG g_ledpen[3] = { -1, -1, -1 };
static LONG g_shadowpen = 1, g_shinepen = 2;   /* fallback: black / white */

/* timer.device, for the once-a-second refresh */
static struct MsgPort     *g_tport;
static struct timerequest *g_treq;

/* clipboard (PRIMARY_CLIP) via iffparse.library, for "copy code" */
static struct ClipboardHandle *g_clip;
static struct IFFHandle        *g_iff;

#define ID_FTXT MAKE_ID('F','T','X','T')
#define ID_CHRS MAKE_ID('C','H','R','S')

#define CLIP_CLEAR_SECS 30      /* wipe our copied code off the clipboard after this */

enum { GID_LIST = 1, GID_CODE, GID_GAUGE, GID_COPY, GID_ADD, GID_EDIT, GID_REMOVE,
       GID_NUDGEDOWN, GID_NUDGEUP };

/* Step size for the manual clock nudge buttons (seconds). Small enough for
 * fine adjustment while watching the countdown against a known-good code
 * (see docs/CLOCK.md); a few clicks covers a typical offline drift. */
#define CLOCK_NUDGE_STEP 10
enum { PWID_OK = 1, PWID_CANCEL, PWID_STR };   /* modal-requester gadgets */
enum { EDID_ISSUER = 1, EDID_LABEL, EDID_DIGITS, EDID_PERIOD, EDID_OK, EDID_CANCEL };  /* edit form */

/* menu / button command ids */
enum { CMD_ADD_CLIP = 1, CMD_ADD_TYPE, CMD_ADD_QR, CMD_EDIT, CMD_COPY, CMD_REMOVE, CMD_QUIT };

/* Commodity hotkey id (CxMsg id for the CX_POPKEY input event). */
#define EVT_HOTKEY 1

#define VAULT_PATH_DEFAULT "PROGDIR:AmiAuth.vault"
#define DEFAULT_IDLE_LOCK  120      /* seconds of inactivity before an encrypted vault re-locks */

/* Multi-column account list: name | live code | seconds-left. The code/left
 * cells change every second, so the nodes point at these persistent buffers
 * (LBNCA_CopyText FALSE) and we rewrite them in place; kept off the Amiga
 * stack. Weighted column widths (CIF_RIGHT/CENTER are V47, below our baseline).*/
static char g_code[VAULT_MAX_ACCOUNTS][12];
static char g_left[VAULT_MAX_ACCOUNTS][8];

/* The main window's title, including the version/build hash (set at window
 * creation, win_show below). Single source of truth so the QR-decode busy
 * title (do_add_qr, "AmiAuth - Decoding QR image...") restores the real
 * title afterward instead of a separate hardcoded "AmiAuth" that drops the
 * version/hash. */
#ifdef AMIAUTH_BUILD_HASH
static const char g_main_title[] = "AmiAuth " AMIAUTH_VERSION " (" AMIAUTH_BUILD_HASH ")";
#else
static const char g_main_title[] = "AmiAuth " AMIAUTH_VERSION;
#endif
static struct ColumnInfo g_columns[] = {
    { 50, (STRPTR)"Account", CIF_WEIGHTED },
    { 34, (STRPTR)"Code",    CIF_WEIGHTED },
    { 16, (STRPTR)"Left",    CIF_WEIGHTED },
    { -1, NULL, 0 }
};

/* button.gadget labels for the account-list toolbar. These double as the
 * single source of truth for their WMHI_VANILLAKEY shortcuts below: window.class
 * doesn't auto-wire GA_Text's '_' mnemonic markers for button.gadget the way
 * GadTools' BUTTON_KIND does, so the VANILLAKEY handler reads the letter
 * straight out of each string (index 1, right after the '_') instead of
 * duplicating it as a separate literal - renaming a label's mnemonic can't
 * silently desync its keyboard shortcut. */
static const char LBL_ADD[]        = "_Add";
static const char LBL_EDIT[]       = "_Edit";
static const char LBL_REMOVE[]     = "_Remove";
static const char LBL_COPY[]       = "_Copy";
static const char LBL_NUDGE_DOWN[] = "_D -10s";
static const char LBL_NUDGE_UP[]   = "_U +10s";

/* Menu strip (window.class WINDOW_NewMenu). nm_UserData carries the command id. */
static struct NewMenu g_menu[] = {
    { NM_TITLE, (STRPTR)"Project", NULL,        0, 0, NULL },
    { NM_ITEM,  (STRPTR)"Quit",    (STRPTR)"Q", 0, 0, (APTR)CMD_QUIT },
    { NM_TITLE, (STRPTR)"Account", NULL,        0, 0, NULL },
    /* No RA shortcut: RA-V is the system-wide Edit->Paste convention, and this
     * item does something bigger than a paste (parses the clipboard as an OTP
     * URI and adds a new vault entry) - reusing that muscle memory here would
     * surprise users expecting an ordinary paste. */
    { NM_ITEM,  (STRPTR)"Add from clipboard",  NULL, 0, 0, (APTR)CMD_ADD_CLIP },
    { NM_ITEM,  (STRPTR)"Add (type URI)...",   (STRPTR)"A", 0, 0, (APTR)CMD_ADD_TYPE },
    { NM_ITEM,  (STRPTR)"Add from QR image...",(STRPTR)"I", 0, 0, (APTR)CMD_ADD_QR },
    { NM_ITEM,  (STRPTR)"Edit selected...",    (STRPTR)"E", 0, 0, (APTR)CMD_EDIT },
    { NM_ITEM,  (STRPTR)"Copy code",           (STRPTR)"C", 0, 0, (APTR)CMD_COPY },
    { NM_ITEM,  NM_BARLABEL,       NULL,        0, 0, NULL },
    { NM_ITEM,  (STRPTR)"Remove selected...",  (STRPTR)"R", 0, 0, (APTR)CMD_REMOVE },
    { NM_END,   NULL, NULL, 0, 0, NULL }
};

/* ------------------------------------------------------------------ */

static void close_libs(void)
{
    if (CxBase)          CloseLibrary(CxBase);
    if (WorkbenchBase)   CloseLibrary(WorkbenchBase);
    if (AslBase)         CloseLibrary(AslBase);
    if (DataTypesBase)   CloseLibrary(DataTypesBase);
    if (GadToolsBase)    CloseLibrary(GadToolsBase);
    if (StringBase)      CloseLibrary(StringBase);
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
    StringBase      = OpenLibrary((STRPTR)"gadgets/string.gadget", 0);  /* optional: typed URI */
    GadToolsBase    = OpenLibrary((STRPTR)"gadtools.library", 37);      /* optional: menus */
    /* QR import — all optional and feature-detected; absence just disables it:
     *   datatypes = load the image (whole feature); asl = the file-picker menu;
     *   workbench = drag-and-drop an image icon onto the window. */
    DataTypesBase   = OpenLibrary((STRPTR)"datatypes.library", 39);
    AslBase         = OpenLibrary((STRPTR)"asl.library", 37);
    WorkbenchBase   = OpenLibrary((STRPTR)"workbench.library", 37);
    CxBase          = OpenLibrary((STRPTR)"commodities.library", 37);  /* optional: commodity */
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
    struct DrawInfo *dri;
    if (!GfxBase || !win || !win->WScreen) return;
    cm = win->WScreen->ViewPort.ColorMap;
    if (!cm) return;
    for (i = 0; i < 3; i++)
        g_ledpen[i] = ObtainBestPenA(cm, rgb[i][0], rgb[i][1], rgb[i][2], tags);
    dri = GetScreenDrawInfo(win->WScreen);          /* bevel pens for the LED well */
    if (dri) {
        g_shadowpen = dri->dri_Pens[SHADOWPEN];
        g_shinepen  = dri->dri_Pens[SHINEPEN];
        FreeScreenDrawInfo(win->WScreen, dri);
    }
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

/* Draw the clock-status LED in the left margin of the status label gadget (its
 * text is centred, so the left is clear). A recessed bevel well (dark top/left,
 * light bottom/right) frames a red/amber/green colour fill, so it reads as a
 * deliberate indicator even where the colour maps to grey on a limited palette.
 * Uses the gadget's laid-out bounds, so it needs no layout sizing of its own. */
static void led_draw(struct Window *win, Object *labelobj, int state)
{
    struct Gadget *g = (struct Gadget *)labelobj;
    struct RastPort *rp = win->RPort;
    LONG pen, x, y, s = 11, x2, y2;
    if (!GfxBase || !labelobj || state < 0 || state > 2) return;
    if (g->Width <= s + 6 || g->Height <= s) return;   /* not laid out yet */
    pen = g_ledpen[state] >= 0 ? g_ledpen[state] : g_shadowpen;
    x  = g->LeftEdge + 4;
    y  = g->TopEdge + (g->Height - s) / 2;
    x2 = x + s - 1;
    y2 = y + s - 1;
    SetAPen(rp, (ULONG)pen);                            /* colour fill inside */
    RectFill(rp, x + 1, y + 1, x2 - 1, y2 - 1);
    SetAPen(rp, (ULONG)g_shadowpen);                    /* recessed: dark top+left */
    Move(rp, x, y2); Draw(rp, x, y); Draw(rp, x2, y);
    SetAPen(rp, (ULONG)g_shinepen);                     /* light bottom+right */
    Move(rp, x2, y); Draw(rp, x2, y2); Draw(rp, x, y2);
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

/* Read the primary clipboard's first FTXT/CHRS text into buf (NUL-terminated).
 * Returns the length copied, or -1 if unavailable/empty. Mirrors the clipread
 * test tool: OpenIFF(READ) -> StopChunk(FTXT,CHRS) -> ReadChunkBytes. */
static int clip_read_text(char *buf, size_t cap)
{
    size_t len = 0;
    int got = 0;
    if (!g_iff || cap == 0) return -1;
    buf[0] = '\0';
    if (OpenIFF(g_iff, IFFF_READ) != 0) return -1;
    if (StopChunk(g_iff, ID_FTXT, ID_CHRS) == 0) {
        while (ParseIFF(g_iff, IFFPARSE_SCAN) == 0) {
            struct ContextNode *cn = CurrentChunk(g_iff);
            if (cn && cn->cn_ID == ID_CHRS) {
                LONG n;
                while (len < cap - 1 &&
                       (n = ReadChunkBytes(g_iff, buf + len, (LONG)(cap - 1 - len))) > 0) {
                    len += (size_t)n;
                }
                got = 1;
                break;
            }
        }
    }
    CloseIFF(g_iff);
    buf[len] = '\0';
    return got ? (int)len : -1;
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

/* --- vault path: VAULT arg, else env, else pref, else default ---
 * (mirrors the CLI's resolve_vault precedence) */
static const char *vault_path(void)
{
    static char buf[256];
    const char *env;
    if (g_vault_arg[0]) return g_vault_arg;
    env = getenv("AMIAUTH_VAULT");
    if (env && env[0]) return env;
    if (prefs_get("vault", buf, sizeof buf) == 0 && buf[0]) return buf;
    return VAULT_PATH_DEFAULT;
}

/* Save the vault, generating a fresh nonce for an encrypted one (mirrors the
 * CLI save_vault). amiga_random never fails on the m68k build. */
static vault_result gui_save(const vault *v, const char *path)
{
    if (v->cipher == VAULT_CIPHER_CHACHA20) {
        uint8_t nonce[VAULT_NONCE_SIZE];
        if (amiga_random(nonce, sizeof nonce) != 0) return VAULT_ERR_IO;
        return vault_save(v, path, nonce);
    }
    return vault_save(v, path, NULL);
}

/* A simple info/confirm requester. `gadgets` is EasyRequest gadget text, e.g.
 * "OK" or "Remove|Cancel"; returns EasyRequest's result (1 = first/leftmost). */
static LONG gui_requester(struct Window *win, const char *body, const char *gadgets, const char *arg)
{
    struct EasyStruct es;
    es.es_StructSize   = sizeof es;
    es.es_Flags        = 0;
    es.es_Title        = (STRPTR)"AmiAuth";
    es.es_TextFormat   = (STRPTR)body;
    es.es_GadgetFormat = (STRPTR)gadgets;
    return EasyRequestArgs(win, &es, NULL, (APTR)&arg);
}

/* (Re)build the listbrowser nodes from the vault's accounts. Frees any existing
 * nodes first. Columns 1/2 point at the persistent g_code[]/g_left[] buffers,
 * blanked here (fixed width so cells never shrink). Detach the gadget's labels
 * before calling this at runtime, and reattach after. */
static void build_nodes(struct List *lblist, const vault *v)
{
    struct Node *n;
    size_t i;
    while ((n = RemHead(lblist)) != NULL) FreeListBrowserNode(n);
    for (i = 0; i < v->count; i++) {
        const otp_account *a = &v->accounts[i];
        char label[OTP_MAX_ISSUER + OTP_MAX_LABEL + 2];
        struct Node *node;
        if (a->issuer[0]) { strcpy(label, a->issuer); strcat(label, ":"); strcat(label, a->label); }
        else              strcpy(label, a->label);
        strcpy(g_code[i], "------");
        strcpy(g_left[i], "   ");
        node = (struct Node *)AllocListBrowserNode(3,
            LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, (ULONG)label,
            LBNA_Column, 1, LBNCA_Text, (ULONG)g_code[i],
            LBNA_Column, 2, LBNCA_Text, (ULONG)g_left[i],
            TAG_END);
        if (node) AddTail(lblist, node);
    }
}

/* Adaptive re-key after a successful unlock (mirrors the CLI maybe_rekey): the
 * stored iteration count and how long the KDF took reveal whether this machine
 * is much faster or slower than the one that secured the vault, so we can offer
 * to recalibrate the PBKDF2 count to ~KDF_TARGET_MS and re-save. Encrypted
 * vaults only; the ENVARC:AmiAuth/rekey = off pref (shared with the CLI)
 * silences it. `win` may be NULL — this runs before the main window opens, so
 * the requesters go to the Workbench screen. The 8x thresholds are generous so
 * emulator/warp variance never nags, only a clear hardware-class jump. */
static void gui_maybe_rekey(struct Window *win, vault *v, const char *path,
                            const char *pass, uint32_t unlock_ms)
{
    uint32_t ideal;
    char prefbuf[8];

    if (v->cipher != VAULT_CIPHER_CHACHA20) return;   /* encrypted only */
    if (unlock_ms == 0) return;                       /* no usable timer */
    if (prefs_get("rekey", prefbuf, sizeof prefbuf) == 0 &&
        Stricmp((STRPTR)prefbuf, (STRPTR)"off") == 0)
        return;

    ideal = vault_calibrate_iterations(v->iterations, unlock_ms);

    if (unlock_ms < KDF_TARGET_MS / 8 && ideal > v->iterations) {
        /* Much faster machine -> offer to strengthen (safe; one confirm). */
        LONG r = gui_requester(win,
            "This machine is much faster than the one that secured this vault.\n"
            "Strengthen it now (re-key to more PBKDF2 iterations)?",
            "Strengthen|Not now|Never here", NULL);
        if (r == 0) { prefs_set("rekey", "off"); return; }   /* Never (rightmost) */
        if (r != 1) return;                                  /* Not now */
    } else if (unlock_ms > KDF_TARGET_MS * 8) {
        /* Much slower machine -> offer to speed up, but this weakens it: two
         * confirms, mirroring the CLI's typed "yes". */
        char msg[136];
        sprintf(msg, "Unlock took about %lu seconds; this vault was tuned for "
                "faster hardware.\nRe-key LOWER for quicker unlocks here? "
                "This REDUCES security.", (unsigned long)((unlock_ms + 500) / 1000));
        if (gui_requester(win, msg, "Re-key lower|Cancel", NULL) != 1) return;
        if (gui_requester(win, "Really reduce this vault's security?",
                          "Reduce|Cancel", NULL) != 1) return;
    } else {
        return;                                       /* within range; nothing to do */
    }

    {   /* Re-key: fresh salt + calibrated count, same passphrase, then save. */
        uint8_t salt[VAULT_SALT_SIZE];
        if (amiga_random(salt, sizeof salt) == 0 &&
            vault_set_passphrase(v, pass, ideal, salt) == VAULT_OK &&
            gui_save(v, path) == VAULT_OK)
            gui_requester(win, "Vault re-keyed for this machine.", "OK", NULL);
        else
            gui_requester(win, "Re-key failed; the vault is unchanged.", "OK", NULL);
        memset(salt, 0, sizeof salt);
    }
}

/* Parse an otpauth:// URI, add it to the vault and save. Returns 1 if the vault
 * changed, else 0 (reporting the reason). Shared by the clipboard/typed add and
 * the QR-image add so all three behave identically. */
static int gui_add_uri(struct Window *win, vault *v, const char *path, const char *uri)
{
    otp_account acct;
    int changed = 0;
    vault_result rc;
    if (otpauth_parse(uri, &acct) != 0) {
        gui_requester(win, "That is not a valid otpauth:// URI.", "OK", NULL);
    } else {
        rc = vault_add(v, &acct);
        if (rc == VAULT_OK) { changed = 1; rc = gui_save(v, path); }
        if (rc != VAULT_OK)
            gui_requester(win, rc == VAULT_ERR_FULL
                          ? "The vault is full (max 64 accounts)."
                          : "Could not save the vault.", "OK", NULL);
    }
    memset(&acct, 0, sizeof acct);
    return changed;
}

/* Decode the QR in an image file into its otpauth:// URI and add it. Returns 1
 * if the vault changed. Reports load and decode failures. Runs under
 * do_add_qr()'s swapped stack - see there for why. */
static int do_add_qr_impl(struct Window *win, vault *v, const char *vpath, const char *img)
{
    static char uri[300];                 /* off the stack */
    unsigned char *gray = NULL;
    int iw = 0, ih = 0, changed = 0, lr, dr;

    lr = qrimage_load_gray(img, &gray, &iw, &ih);
    if (lr != QRIMAGE_OK) {
        gui_requester(win,
            lr == QRIMAGE_ERR_TOOBIG  ? "That image is too large to decode."
          : lr == QRIMAGE_ERR_UNAVAIL ? "QR import needs datatypes.library."
          : lr == QRIMAGE_ERR_NOMEM   ? "Not enough memory to load that image."
          :                             "Could not read that image "
                                        "(needs picture.datatype).", "OK", NULL);
        return 0;
    }
    /* Decoding is slow on the FPU-less CPU (quirc's soft-float geometry can take
     * tens of seconds), and it blocks the event loop — show a busy pointer and a
     * "Decoding" title so the wait is expected. SetWindowPointer is v39+, which
     * we already have wherever datatypes.library (v39) opened for QR import. */
    if (IntuitionBase->LibNode.lib_Version >= 39) {
        SetWindowPointer(win, WA_BusyPointer, TRUE, WA_PointerDelay, TRUE, TAG_END);
        SetWindowTitles(win, (STRPTR)"AmiAuth - Decoding QR image...", (STRPTR)~0UL);
    }
    dr = qr_decode_gray(gray, iw, ih, uri, sizeof uri);
    if (IntuitionBase->LibNode.lib_Version >= 39) {
        SetWindowPointer(win, TAG_END);           /* restore the default pointer */
        SetWindowTitles(win, (STRPTR)g_main_title, (STRPTR)~0UL);
    }
    if (dr == QR_OK)
        changed = gui_add_uri(win, v, vpath, uri);
    else
        gui_requester(win, "No otpauth QR code was found in that image.", "OK", NULL);
    qrimage_free(gray);
    memset(uri, 0, sizeof uri);
    return changed;
}

/* do_add_qr_impl's chain - picture.datatype's own PNG decompression (closed-
 * source, stack usage unknown to us) plus quirc's fixed ~9 KB quirc_decode()
 * datastream - is the deepest stack user in the GUI by far. __stack=32768
 * above should already cover it, but a crash was seen in the field on this
 * exact path with a real-world QR image (#76), so borrow dos.library's
 * StackSwap() for extra, guaranteed headroom on just this one risky call
 * rather than trusting the process's baseline stack alone. 64 KB is heap
 * memory, trivial on anything this app targets. */
#define QR_DECODE_STACK_SIZE (64UL * 1024)

static int do_add_qr(struct Window *win, vault *v, const char *vpath, const char *img)
{
    struct StackSwapStruct sss;
    APTR newstack;
    int result;

    newstack = AllocMem(QR_DECODE_STACK_SIZE, MEMF_ANY);
    if (!newstack)                        /* fall back to the current stack */
        return do_add_qr_impl(win, v, vpath, img);

    sss.stk_Lower  = newstack;
    sss.stk_Upper  = (ULONG)newstack + QR_DECODE_STACK_SIZE;
    sss.stk_Pointer = (APTR)sss.stk_Upper;

    StackSwap(&sss);
    result = do_add_qr_impl(win, v, vpath, img);
    StackSwap(&sss);                      /* back to the original stack */

    FreeMem(newstack, QR_DECODE_STACK_SIZE);
    return result;
}

/* Ask the user for an image file (asl.library). Returns 1 and fills `path`
 * (`cap` bytes) with a full path on OK. */
static int qr_file_request(struct Window *win, char *path, size_t cap)
{
    struct FileRequester *fr;
    int ok = 0;
    if (!AslBase) return 0;
    fr = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_Window,         (ULONG)win,
        ASLFR_TitleText,      (ULONG)"Select a QR image",
        ASLFR_DoPatterns,     TRUE,
        ASLFR_InitialPattern, (ULONG)"#?.(png|jpg|jpeg|gif|iff|ilbm|lbm|bmp)",
        TAG_END);
    if (!fr) return 0;
    if (AslRequest(fr, NULL) && fr->fr_File[0]) {
        path[0] = '\0';
        strncpy(path, (const char *)fr->fr_Drawer, cap - 1);
        path[cap - 1] = '\0';
        AddPart((STRPTR)path, (STRPTR)fr->fr_File, (ULONG)cap);
        ok = 1;
    }
    FreeAslRequest(fr);
    return ok;
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
    /* trailing '|' is a static cursor: the field is a read-only display
     * (real keys are captured at window level, see below), so there's no
     * gadget-drawn cursor to show it has focus without this (#37). A
     * vertical bar, not '_', so it isn't a descender easily clipped/missed
     * at the bottom of a tight text box. */
    stars[0] = '|'; stars[1] = '\0';

    maskobj   = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ReadOnly, TRUE, GA_Text, (ULONG)"|", TAG_END);
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
                            buf[--len] = '\0';
                            stars[len] = '|'; stars[len + 1] = '\0';
                            SetGadgetAttrs((struct Gadget *)maskobj, w, NULL,
                                           GA_Text, (ULONG)stars, TAG_END);
                        }
                    } else if (code >= 0x20 && code < 0x7F) {        /* printable */
                        if (len < cap - 1 && len < sizeof stars - 2) {
                            buf[len] = (char)code; stars[len] = '*';
                            len++;
                            buf[len] = '\0';
                            stars[len] = '|'; stars[len + 1] = '\0';
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

/* Record the vault's location in the prefs as an absolute path (resolved via
 * Lock/NameFromLock), so later launches find it even when PROGDIR: has moved -
 * the WBStartup gotcha in docs/STORAGE.md. Best-effort: falls back to the
 * path as given. */
static void record_vault_path(const char *path)
{
    char abs[256];
    BPTR lock = Lock((CONST_STRPTR)path, ACCESS_READ);
    strncpy(abs, path, sizeof abs - 1);
    abs[sizeof abs - 1] = '\0';
    if (lock) {
        if (!NameFromLock(lock, (STRPTR)abs, sizeof abs)) {
            strncpy(abs, path, sizeof abs - 1);
            abs[sizeof abs - 1] = '\0';
        }
        UnLock(lock);
    }
    prefs_set("vault", abs);
}

/* Probe PBKDF2 speed and pick the iteration count for a new vault - aim ~1s
 * to unlock on this machine (mirrors cli_calibrate; policy in SECURITY.md). */
static uint32_t gui_calibrate(void)
{
    static const uint8_t salt[16] = { 0 };
    uint8_t dk[64];
    uint32_t probe = 4, t0, ms;

    for (;;) {
        t0 = amiga_millis();
        pbkdf2_hmac_sha1((const uint8_t *)"calibration", 11, salt, sizeof salt,
                         probe, dk, sizeof dk);
        ms = amiga_millis() - t0;
        if (ms >= 50) break;                       /* enough to extrapolate */
        if (probe >= KDF_MAX_ITERATIONS) {
            if (ms == 0) { memset(dk, 0, sizeof dk); return VAULT_DEFAULT_ITERATIONS; }
            break;                                 /* just very fast; use it */
        }
        probe *= 4;
    }
    memset(dk, 0, sizeof dk);
    return vault_calibrate_iterations(probe, ms);
}

/* Pick a different home for the vault when the default is not writable
 * (read-only or protected install; docs/STORAGE.md). Returns 1 with `path`
 * (cap bytes) replaced by the chosen file. */
static int vault_location_request(char *path, size_t cap)
{
    struct FileRequester *fr;
    int ok = 0;
    if (!AslBase) return 0;
    fr = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText,   (ULONG)"Choose where to keep the vault",
        ASLFR_DoSaveMode,  TRUE,
        ASLFR_InitialFile, (ULONG)"AmiAuth.vault",
        TAG_END);
    if (!fr) return 0;
    if (AslRequest(fr, NULL) && fr->fr_File[0]) {
        path[0] = '\0';
        strncpy(path, (const char *)fr->fr_Drawer, cap - 1);
        path[cap - 1] = '\0';
        AddPart((STRPTR)path, (STRPTR)fr->fr_File, (ULONG)cap);
        ok = 1;
    }
    FreeAslRequest(fr);
    return ok;
}

/* First-launch setup: no vault exists, so create one from Workbench - the CLI
 * must not be a requirement for setup. Mirrors CLI INIT: empty passphrase =
 * always-unlocked behind an explicit warning, otherwise encrypted with a
 * freshly calibrated PBKDF2 count. On success *v is the saved, unlocked
 * vault, `path` (cap bytes) may have been redirected to a writable location,
 * the absolute path is recorded in the prefs, and *encrypted is set.
 * Returns 1 to continue into the app, 0 to decline (nothing written) - in
 * `deferred` mode (hidden start, prompt on first show) declining keeps the
 * commodity resident, so the cancel gadget says so. */
static int gui_first_run(vault *v, char *path, size_t cap, int *encrypted,
                         int deferred)
{
    char pass[128], confirm[128];
    vault_result rc = VAULT_ERR_IO;

    if (gui_requester(NULL,
            "Welcome to AmiAuth!\n\nThere is no vault yet - it will be created at\n%s",
            deferred ? "Create a vault...|Not now" : "Create a vault...|Quit",
            path) != 1)
        return 0;

    for (;;) {
        if (!passphrase_request("New passphrase (empty = always-unlocked):",
                                pass, sizeof pass))
            return 0;
        if (!pass[0]) {
            if (gui_requester(NULL,
                    "Store the vault UNENCRYPTED?\n\n"
                    "With no passphrase there is no at-rest protection:\n"
                    "anyone with access to the file can read your secrets.\n"
                    "(You can add a passphrase later.)",
                    "Store unencrypted|Go back", NULL) != 1)
                continue;
            rc = vault_create(v, NULL, 0, NULL);
            *encrypted = 0;
            break;
        }
        if (!passphrase_request("Confirm passphrase:", confirm, sizeof confirm)) {
            memset(pass, 0, sizeof pass);
            return 0;
        }
        if (strcmp(pass, confirm) != 0) {
            memset(confirm, 0, sizeof confirm);
            memset(pass, 0, sizeof pass);
            gui_requester(NULL, "The passphrases did not match.", "Try again", NULL);
            continue;
        }
        memset(confirm, 0, sizeof confirm);
        {
            uint8_t salt[VAULT_SALT_SIZE];
            if (amiga_random(salt, sizeof salt) != 0) {
                memset(pass, 0, sizeof pass);
                gui_requester(NULL,
                    "No secure random source is available, so an\n"
                    "encrypted vault cannot be created safely.",
                    "Cancel", NULL);
                return 0;
            }
            /* a second or two of PBKDF2 probing - same ~1s policy as the CLI */
            rc = vault_create(v, pass, gui_calibrate(), salt);
            memset(salt, 0, sizeof salt);
        }
        memset(pass, 0, sizeof pass);
        *encrypted = 1;
        break;
    }

    if (rc == VAULT_OK) rc = gui_save(v, path);
    while (rc == VAULT_ERR_IO) {
        /* the resolved spot is unwritable (read-only install?): offer another */
        if (gui_requester(NULL,
                "Cannot write the vault to\n%s\n\nChoose another location?",
                AslBase ? "Choose location...|Quit" : "Quit", path) != 1 ||
            !vault_location_request(path, cap)) {
            vault_lock(v);
            return 0;
        }
        rc = gui_save(v, path);
    }
    if (rc != VAULT_OK) {
        vault_lock(v);
        gui_requester(NULL, "Creating the vault failed.", "Quit", NULL);
        return 0;
    }
    {   /* a VAULT= or AMIAUTH_VAULT override is session-scoped: don't make
         * it sticky (mirrors the CLI, which only records a non-overridden
         * INIT path) */
        const char *env = getenv("AMIAUTH_VAULT");
        if (!g_vault_arg[0] && !(env && env[0])) record_vault_path(path);
    }
    return 1;
}

/* Open the vault, running whatever interaction that needs: first-run creation
 * when none exists, or the unlock prompt loop (with the adaptive re-key
 * offer) for an encrypted one. All requesters parent on NULL, so this works
 * before any window exists. Returns 1 unlocked, 0 user-declined (clean),
 * -1 unrecoverable (already reported to the user). */
static int gui_open_vault(vault *v, char *path, size_t cap, int *encrypted,
                          int deferred)
{
    char pass[128];
    vault_result rc = vault_load(v, path, NULL);

    if (rc == VAULT_ERR_IO) {
        BPTR l = Lock((CONST_STRPTR)path, ACCESS_READ);
        if (!l)                            /* no vault at all: first launch */
            return gui_first_run(v, path, cap, encrypted, deferred) ? 1 : 0;
        UnLock(l);                         /* exists but unreadable: below */
    }
    if (rc == VAULT_ERR_LOCKED) {
        const char *msg = "Enter passphrase:";
        *encrypted = 1;                    /* enables idle auto-lock */
        for (;;) {
            if (!passphrase_request(msg, pass, sizeof pass))
                return 0;                  /* cancelled */
            {   /* time the KDF so we can offer an adaptive re-key */
                uint32_t t0 = amiga_millis();
                rc = vault_load(v, path, pass);
                if (rc == VAULT_OK)
                    gui_maybe_rekey(NULL, v, path, pass, amiga_millis() - t0);
            }
            memset(pass, 0, sizeof pass);  /* scrub immediately */
            if (rc == VAULT_OK) return 1;
            if (rc == VAULT_ERR_AUTH) { msg = "Wrong passphrase - try again:"; continue; }
            break;                         /* IO/format error: reported below */
        }
    }
    if (rc != VAULT_OK) {
        /* a WB-launched process has no console, so a requester must carry
         * the error; keep the Printf for Shell launches */
        gui_requester(NULL, "Cannot open the vault at\n%s", "Quit", path);
        Printf((CONST_STRPTR)"AmiAuth: cannot open the vault (%ld)\n", (LONG)rc);
        return -1;
    }
    return 1;
}

/* Deferred-start open (#50): a hidden commodity boots with the vault locked
 * and runs the full open/create interaction on the first show instead. On
 * success the (empty) account list built at startup is rebuilt from the now
 * unlocked vault; the window is still closed, so the gadget takes NULL.
 * Returns 1 to proceed with opening the window; 0 to stay hidden+locked. */
static int deferred_open(vault *v, char *path, size_t cap, int *encrypted,
                         struct List *lblist, Object *listobj, size_t *naccounts)
{
    if (gui_open_vault(v, path, cap, encrypted, 1) <= 0)
        return 0;
    /* listobj only exists while the window is actually open (every gadget is
     * rebuilt fresh per show, see win_show) - while hidden there's nothing to
     * refresh here, build_nodes alone is enough: win_show picks lblist's
     * current contents straight up via LISTBROWSER_Labels at creation. */
    if (listobj)
        SetGadgetAttrs((struct Gadget *)listobj, NULL, NULL,
                       LISTBROWSER_Labels, ~0UL, TAG_END);
    build_nodes(lblist, v);
    if (listobj)
        SetGadgetAttrs((struct Gadget *)listobj, NULL, NULL,
                       LISTBROWSER_Labels, (ULONG)lblist, TAG_END);
    *naccounts = v->count;
    return 1;
}

/* --- modal requester with an editable string.gadget for an otpauth:// URI.
 * Returns 1 with the (NUL-terminated) URI in buf, 0 on cancel/empty. Needs
 * string.gadget; returns 0 if unavailable. The gadget supports native paste. */
static int uri_request(char *buf, size_t cap)
{
    Object *win, *layoutobj, *strobj, *okobj, *cancelobj, *labelobj, *buttons;
    struct Window *w;
    ULONG sig = 0;
    int done = -1;

    if (!StringBase || cap == 0) return 0;
    buf[0] = '\0';

    strobj = NewObject(STRING_GetClass(), NULL,
        GA_ID,            PWID_STR,
        GA_RelVerify,     TRUE,
        STRINGA_Buffer,   (ULONG)buf,          /* the gadget edits into buf */
        STRINGA_MaxChars, (ULONG)(cap - 1),
        TAG_END);
    okobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID, PWID_OK,     GA_RelVerify, TRUE, GA_Text, (ULONG)"OK",     TAG_END);
    cancelobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID, PWID_CANCEL, GA_RelVerify, TRUE, GA_Text, (ULONG)"Cancel", TAG_END);
    labelobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ReadOnly, TRUE, GA_Text, (ULONG)"Paste or type an otpauth:// URI:", TAG_END);
    buttons = NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
        LAYOUT_AddChild, (ULONG)okobj,
        LAYOUT_AddChild, (ULONG)cancelobj,
        TAG_END);
    layoutobj = NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_SpaceOuter,  TRUE,
        LAYOUT_AddChild, (ULONG)labelobj, CHILD_WeightedHeight, 0,
        LAYOUT_AddChild, (ULONG)strobj,   CHILD_WeightedHeight, 0, CHILD_MinWidth, 360,
        LAYOUT_AddChild, (ULONG)buttons,  CHILD_WeightedHeight, 0,
        TAG_END);
    win = NewObject(WINDOW_GetClass(), NULL,
        WA_Title,        (ULONG)"AmiAuth - Add account",
        WA_Activate,     TRUE,
        WA_CloseGadget,  TRUE,
        WA_DragBar,      TRUE,
        WA_DepthGadget,  TRUE,
        WA_IDCMP,        IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_VANILLAKEY,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WINDOW_Layout,   (ULONG)layoutobj,
        TAG_END);
    if (!win) return 0;

    w = (struct Window *)DoMethod(win, WM_OPEN, NULL);
    if (!w) { DisposeObject(win); return 0; }
    GetAttr(WINDOW_SigMask, win, &sig);
    ActivateLayoutGadget((struct Gadget *)layoutobj, w, NULL, PWID_STR);  /* type straight in */

    while (done < 0) {
        ULONG got = Wait(sig | SIGBREAKF_CTRL_C);
        ULONG r;
        UWORD code;
        if (got & SIGBREAKF_CTRL_C) { done = 0; break; }
        while ((r = DoMethod(win, WM_HANDLEINPUT, (ULONG)&code)) != WMHI_LASTMSG) {
            switch (r & WMHI_CLASSMASK) {
                case WMHI_CLOSEWINDOW: done = 0; break;
                case WMHI_GADGETUP:
                    if      ((r & WMHI_GADGETMASK) == PWID_OK)     done = 1;
                    else if ((r & WMHI_GADGETMASK) == PWID_STR)    done = 1;  /* Enter */
                    else if ((r & WMHI_GADGETMASK) == PWID_CANCEL) done = 0;
                    break;
                case WMHI_VANILLAKEY:
                    if (code == 0x1B) done = 0;                              /* Escape */
                    break;
            }
            if (done >= 0) break;
        }
    }

    if (done == 1) {
        STRPTR text = NULL;
        GetAttr(STRINGA_TextVal, strobj, (ULONG *)&text);
        if (text) { strncpy(buf, (char *)text, cap - 1); buf[cap - 1] = '\0'; }
        if (buf[0] == '\0') done = 0;                    /* empty == cancel */
    }
    DoMethod(win, WM_CLOSE, NULL);
    DisposeObject(win);
    return done == 1 ? 1 : 0;
}

/* A "<label>  [gadget]" row for the edit form (fixed-width label so they align). */
static Object *labeled_row(const char *lbl, Object *gad)
{
    Object *l = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ReadOnly, TRUE, GA_Text, (ULONG)lbl, TAG_END);
    return NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
        LAYOUT_AddChild, (ULONG)l,
        CHILD_WeightedWidth, 0,
        CHILD_MinWidth, 64,
        LAYOUT_AddChild, (ULONG)gad,
        TAG_END);
}

/* Modal form to edit the selected account's issuer/label/digits/period. Returns
 * 1 if applied (written back into *acct), 0 on cancel. The secret/type/algorithm
 * are preserved. Needs string.gadget; returns 0 if unavailable. */
static int edit_request(otp_account *acct)
{
    Object *win, *layoutobj, *issuerg, *labelg, *digitsg, *periodg, *okobj, *cancelobj, *buttons;
    struct Window *w;
    ULONG sig = 0;
    int done = -1;
    /* Writable buffers the string gadgets edit into (a string.gadget needs a
     * STRINGA_Buffer or OM_NEW fails). Off the stack; prefilled with the field. */
    static char ibuf[OTP_MAX_ISSUER], lbuf[OTP_MAX_LABEL], dbuf[8], pbuf[12];

    if (!StringBase) return 0;
    strcpy(ibuf, acct->issuer);
    strcpy(lbuf, acct->label);
    sprintf(dbuf, "%lu", (unsigned long)acct->digits);   /* libnix mishandles %d */
    sprintf(pbuf, "%lu", (unsigned long)acct->period);

    issuerg = NewObject(STRING_GetClass(), NULL, GA_ID, EDID_ISSUER, GA_RelVerify, TRUE,
        STRINGA_Buffer, (ULONG)ibuf, STRINGA_MaxChars, OTP_MAX_ISSUER - 1, TAG_END);
    labelg  = NewObject(STRING_GetClass(), NULL, GA_ID, EDID_LABEL, GA_RelVerify, TRUE,
        STRINGA_Buffer, (ULONG)lbuf, STRINGA_MaxChars, OTP_MAX_LABEL - 1, TAG_END);
    digitsg = NewObject(STRING_GetClass(), NULL, GA_ID, EDID_DIGITS, GA_RelVerify, TRUE,
        STRINGA_Buffer, (ULONG)dbuf, STRINGA_MaxChars, 2, TAG_END);
    periodg = NewObject(STRING_GetClass(), NULL, GA_ID, EDID_PERIOD, GA_RelVerify, TRUE,
        STRINGA_Buffer, (ULONG)pbuf, STRINGA_MaxChars, 6, TAG_END);
    okobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID, EDID_OK,     GA_RelVerify, TRUE, GA_Text, (ULONG)"OK",     TAG_END);
    cancelobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID, EDID_CANCEL, GA_RelVerify, TRUE, GA_Text, (ULONG)"Cancel", TAG_END);
    buttons = NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
        LAYOUT_AddChild, (ULONG)okobj,
        LAYOUT_AddChild, (ULONG)cancelobj,
        TAG_END);
    layoutobj = NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_SpaceOuter, TRUE,
        LAYOUT_AddChild, (ULONG)labeled_row("Issuer:", issuerg), CHILD_WeightedHeight, 0, CHILD_MinWidth, 300,
        LAYOUT_AddChild, (ULONG)labeled_row("Label:",  labelg),  CHILD_WeightedHeight, 0,
        LAYOUT_AddChild, (ULONG)labeled_row("Digits:", digitsg), CHILD_WeightedHeight, 0,
        LAYOUT_AddChild, (ULONG)labeled_row("Period:", periodg), CHILD_WeightedHeight, 0,
        LAYOUT_AddChild, (ULONG)buttons,                         CHILD_WeightedHeight, 0,
        TAG_END);
    win = NewObject(WINDOW_GetClass(), NULL,
        WA_Title,        (ULONG)"AmiAuth - Edit account",
        WA_Activate,     TRUE,
        WA_CloseGadget,  TRUE,
        WA_DragBar,      TRUE,
        WA_DepthGadget,  TRUE,
        WA_IDCMP,        IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_VANILLAKEY,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WINDOW_Layout,   (ULONG)layoutobj,
        TAG_END);
    if (!win) return 0;

    w = (struct Window *)DoMethod(win, WM_OPEN, NULL);
    if (!w) { DisposeObject(win); return 0; }
    GetAttr(WINDOW_SigMask, win, &sig);
    ActivateLayoutGadget((struct Gadget *)layoutobj, w, NULL, EDID_ISSUER);

    while (done < 0) {
        ULONG got = Wait(sig | SIGBREAKF_CTRL_C);
        ULONG r;
        UWORD code;
        if (got & SIGBREAKF_CTRL_C) { done = 0; break; }
        while ((r = DoMethod(win, WM_HANDLEINPUT, (ULONG)&code)) != WMHI_LASTMSG) {
            ULONG gid = r & WMHI_GADGETMASK;
            switch (r & WMHI_CLASSMASK) {
                case WMHI_CLOSEWINDOW: done = 0; break;
                case WMHI_VANILLAKEY:  if (code == 0x1B) done = 0; break;
                case WMHI_GADGETUP:
                    if (gid == EDID_CANCEL) done = 0;
                    else if (gid == EDID_OK || gid == EDID_ISSUER || gid == EDID_LABEL ||
                             gid == EDID_DIGITS || gid == EDID_PERIOD) {   /* OK, or Enter in any field */
                        STRPTR t = NULL;
                        char issuer[OTP_MAX_ISSUER], label[OTP_MAX_LABEL];
                        int d; long p;
                        issuer[0] = label[0] = '\0';
                        GetAttr(STRINGA_TextVal, issuerg, (ULONG *)&t);
                        if (t) { strncpy(issuer, (char *)t, OTP_MAX_ISSUER - 1); issuer[OTP_MAX_ISSUER - 1] = '\0'; }
                        GetAttr(STRINGA_TextVal, labelg, (ULONG *)&t);
                        if (t) { strncpy(label, (char *)t, OTP_MAX_LABEL - 1); label[OTP_MAX_LABEL - 1] = '\0'; }
                        GetAttr(STRINGA_TextVal, digitsg, (ULONG *)&t); d = t ? atoi((char *)t) : 0;
                        GetAttr(STRINGA_TextVal, periodg, (ULONG *)&t); p = t ? atol((char *)t) : 0;
                        if (label[0] && d >= 6 && d <= 8 && p > 0 && p <= 86400) {
                            strcpy(acct->issuer, issuer);
                            strcpy(acct->label, label);
                            acct->digits = d;
                            acct->period = (uint32_t)p;
                            done = 1;
                        } else {
                            gui_requester(w, "Label is required; digits must be 6-8, period 1-86400.",
                                          "OK", NULL);           /* stay open to fix */
                        }
                    }
                    break;
            }
            if (done >= 0) break;
        }
    }

    DoMethod(win, WM_CLOSE, NULL);
    DisposeObject(win);
    return done == 1 ? 1 : 0;
}

/* Match an account by label, issuer, or "issuer:label" (case-insensitive) —
 * mirrors the CLI find_account so `AmiAuth GET <name>` behaves the same whether
 * it runs locally or is forwarded to this GUI (Stage 3b). */
static int gui_find_account(const vault *v, const char *q)
{
    size_t i;
    if (!q) return -1;
    for (i = 0; i < v->count; i++) {
        const otp_account *a = &v->accounts[i];
        char combo[OTP_MAX_ISSUER + OTP_MAX_LABEL + 2];
        strcpy(combo, a->issuer); strcat(combo, ":"); strcat(combo, a->label);
        if (Stricmp((STRPTR)q, (STRPTR)a->label)  == 0 ||
            Stricmp((STRPTR)q, (STRPTR)a->issuer) == 0 ||
            Stricmp((STRPTR)q, (STRPTR)combo)     == 0)
            return (int)i;
    }
    return -1;
}

/* Every gadget + the window itself is rebuilt from scratch on every hide/show
 * cycle - see the win_show/win_hide comment below for why nothing is reused
 * across a cycle any more. */
struct gui_widgets {
    Object *winobj, *listobj, *codeobj, *gaugeobj, *copyobj, *statobj,
           *addobj, *editobj, *removeobj, *nudgednobj, *nudgeupobj;
};

/* --- window show/hide (Stage 3 commodity) ------------------------------- *
 * A fresh window (and every gadget in it) is built and disposed every
 * hide/show cycle. Two smaller approaches were tried and abandoned first:
 *
 * 1. A single persistent window.class object, cycled via WM_CLOSE/WM_OPEN
 *    across a hide. Per the window.class autodoc, WM_OPEN "locks default
 *    public screen if needed" and WM_CLOSE's documented behaviour never
 *    mentions releasing it - so that lock, taken once at the very first
 *    open, was held for as long as AmiAuth stayed resident, hidden or not
 *    (blocking a Workbench screen-mode change the whole time), and left a
 *    stale reference if the screen changed anyway (window wouldn't reopen,
 *    but Exchange still saw the process as alive).
 * 2. Disposing just the window wrapper each hide, detaching and reattaching
 *    the same layout/gadget tree to a fresh one each show. Fixed the lock,
 *    but the reused gadgets didn't reliably redraw against the new window -
 *    the account list, code, gauge and button contents came back blank.
 *
 * Rebuilding everything avoids both: nothing is reused across a cycle, so
 * there's nothing that can hold a stale screen reference or stale render
 * state (#37). led pens are screen-bound too, so (re)allocated per open. */
static struct Window *win_show(struct gui_widgets *gw, struct List *lblist,
                               const vault *v, int have_clip,
                               const clock_ctx *clk, char *statbuf,
                               ULONG *winsig, int ledstate)
{
    struct Window *w;
    Object *layoutobj, *btnrow, *statrow;

    gw->listobj = NewObject(LISTBROWSER_GetClass(), NULL,
        GA_ID,                    GID_LIST,
        GA_RelVerify,             TRUE,
        LISTBROWSER_ColumnInfo,   (ULONG)g_columns,
        LISTBROWSER_ColumnTitles, TRUE,
        LISTBROWSER_Labels,       (ULONG)lblist,
        LISTBROWSER_ShowSelected, TRUE,
        LISTBROWSER_Selected,     0,
        TAG_END);
    gw->codeobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID,       GID_CODE,
        GA_ReadOnly, TRUE,
        GA_Text,     (ULONG)"------",
        TAG_END);
    gw->gaugeobj = NewObject(FUELGAUGE_GetClass(), NULL,
        GA_ID,             GID_GAUGE,
        FUELGAUGE_Min,     0,
        FUELGAUGE_Max,     30,
        FUELGAUGE_Level,   0,
        FUELGAUGE_Percent, FALSE,
        FUELGAUGE_Ticks,   0,
        TAG_END);
    gw->addobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID,        GID_ADD,
        GA_RelVerify, TRUE,
        GA_Disabled,  !StringBase,          /* typed-URI requester needs string.gadget */
        GA_Text,      (ULONG)LBL_ADD,
        TAG_END);
    gw->editobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID,        GID_EDIT,
        GA_RelVerify, TRUE,
        GA_Disabled,  (!StringBase || v->count == 0),
        GA_Text,      (ULONG)LBL_EDIT,
        TAG_END);
    gw->removeobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID,        GID_REMOVE,
        GA_RelVerify, TRUE,
        GA_Disabled,  (v->count == 0),
        GA_Text,      (ULONG)LBL_REMOVE,
        TAG_END);
    gw->copyobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID,        GID_COPY,
        GA_RelVerify, TRUE,
        GA_Disabled,  (!have_clip || v->count == 0),  /* no clipboard / nothing to copy */
        GA_Text,      (ULONG)LBL_COPY,
        TAG_END);
    clock_status_text(clk, statbuf);
    gw->statobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ReadOnly, TRUE,
        GA_Text,     (ULONG)statbuf,
        TAG_END);
    gw->nudgednobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID,        GID_NUDGEDOWN,
        GA_RelVerify, TRUE,
        GA_Text,      (ULONG)LBL_NUDGE_DOWN,
        TAG_END);
    gw->nudgeupobj = NewObject(NULL, (STRPTR)"button.gadget",
        GA_ID,        GID_NUDGEUP,
        GA_RelVerify, TRUE,
        GA_Text,      (ULONG)LBL_NUDGE_UP,
        TAG_END);
    btnrow = NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
        LAYOUT_AddChild, (ULONG)gw->addobj,
        LAYOUT_AddChild, (ULONG)gw->editobj,
        LAYOUT_AddChild, (ULONG)gw->removeobj,
        LAYOUT_AddChild, (ULONG)gw->copyobj,
        TAG_END);
    statrow = NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
        LAYOUT_AddChild,      (ULONG)gw->nudgednobj,
        CHILD_WeightedWidth,  0,
        LAYOUT_AddChild,      (ULONG)gw->statobj,
        LAYOUT_AddChild,      (ULONG)gw->nudgeupobj,
        CHILD_WeightedWidth,  0,
        TAG_END);
    layoutobj = NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_SpaceOuter,  TRUE,
        LAYOUT_AddChild,    (ULONG)gw->listobj,
        CHILD_MinWidth,     320,            /* room for three columns + titles */
        CHILD_MinHeight,    100,            /* show several accounts at once */
        LAYOUT_AddChild,    (ULONG)gw->codeobj,
        CHILD_WeightedHeight, 0,
        LAYOUT_AddChild,    (ULONG)gw->gaugeobj,
        CHILD_WeightedHeight, 0,
        LAYOUT_AddChild,    (ULONG)btnrow,
        CHILD_WeightedHeight, 0,
        LAYOUT_AddChild,    (ULONG)statrow,
        CHILD_WeightedHeight, 0,
        TAG_END);

    gw->winobj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title,        (ULONG)g_main_title,
        WA_Activate,     TRUE,
        WA_CloseGadget,  TRUE,
        WA_DragBar,      TRUE,
        WA_DepthGadget,  TRUE,
        WA_SizeGadget,   TRUE,
        WA_IDCMP,        IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_MENUPICK
                        | IDCMP_VANILLAKEY    /* plain-letter gadget shortcuts, #55 */
                        | IDCMP_RAWKEY,        /* cursor up/down list navigation, #55 */
        (g_winleft < 0)  ? WINDOW_Position : TAG_IGNORE, (ULONG)WPOS_CENTERSCREEN,
        (g_winleft >= 0) ? WA_Left         : TAG_IGNORE, (ULONG)g_winleft,
        (g_winleft >= 0) ? WA_Top          : TAG_IGNORE, (ULONG)g_wintop,
        (g_winleft >= 0) ? WA_Width        : TAG_IGNORE, (ULONG)g_winw,
        (g_winleft >= 0) ? WA_Height       : TAG_IGNORE, (ULONG)g_winh,
        GadToolsBase ? WINDOW_NewMenu : TAG_IGNORE, (ULONG)g_menu,
        /* AppWindow (QR drag-and-drop) registration happens below via
         * AddAppWindowA against the real struct Window*, not here - see the
         * g_appport/g_appwin comment above. */
        WINDOW_Layout,   (ULONG)layoutobj,
        TAG_END);
    if (!gw->winobj) {
        Printf((CONST_STRPTR)"AmiAuth: could not create the window\n");
        return NULL;
    }
    /* PUBSCREEN=<name>: set as a separate SetAttrs() only when actually in
     * use, rather than folding WA_PubScreenName/WA_PubScreenFallBack into
     * the NewObject() tag list above (conditioned on TAG_IGNORE the same
     * way as the geometry/menu tags just above). That version prevented the
     * window from ever opening even with no PUBSCREEN set (i.e. with both
     * tags evaluating to TAG_IGNORE) - the exact mechanism wasn't pinned
     * down (the source preprocesses correctly, no tag-value collision), but
     * empirically their mere presence in that tag list was the trigger, so
     * they're kept out of it entirely unless actually needed (#62 - found
     * while investigating that issue, not the same bug). */
    if (g_pubscreen[0])
        SetAttrs(gw->winobj, WA_PubScreenName, (ULONG)g_pubscreen,
                            WA_PubScreenFallBack, (ULONG)TRUE, TAG_END);
    w = (struct Window *)DoMethod(gw->winobj, WM_OPEN, NULL);
    if (w) {
        GetAttr(WINDOW_SigMask, gw->winobj, winsig);
        led_pens_alloc(w);
        led_draw(w, gw->statobj, ledstate);
        if (g_appport)
            g_appwin = AddAppWindowA(0, 0, w, g_appport, NULL);
    }
    return w;
}

static void win_hide(struct gui_widgets *gw, struct Window *w)
{
    if (!w || !gw->winobj) return;
    g_winleft = w->LeftEdge; g_wintop = w->TopEdge;
    g_winw    = w->Width;    g_winh   = w->Height;
    if (g_appwin) { RemoveAppWindow(g_appwin); g_appwin = NULL; }
    led_pens_free(w);
    DoMethod(gw->winobj, WM_CLOSE, NULL);
    DisposeObject(gw->winobj);  /* cascades: also disposes every gadget in it */
    memset(gw, 0, sizeof *gw);
}

/* ------------------------------------------------------------------ */

int main(int argc, char **argv)
{
    static vault v;                     /* ~19 KB: keep it off the stack */
    struct List lblist;
    struct gui_widgets gw = { 0 };      /* every gadget - rebuilt each show, see win_show */
    struct Window *win = NULL;
    clock_ctx clk;
    const char *err, *path;
    ULONG winsig = 0, appsig = 0, timersig, cxsig = 0, sel = 0, lastsec = 0, lastmic = 0, our_clipid = 0, idle_secs = 0;
    int have_timer = 0, have_clip = 0, running = 1, copied = 0, clear_secs = 0, encrypted = 0, changed = 0, popup = 1, deferred = 0;
    static char vpath[512];             /* vault path; first-run may relocate it */
    long idle_limit = 0;
    size_t i, naccounts = 0;
    char curcode[16], statbuf[48];
    static char uribuf[300];            /* otpauth:// URI; off the stack */
    vault_result rc;
    /* Commodity (Stage 3): broker + hotkey, so AmiAuth runs as a background
     * authenticator poppable from Exchange / a hotkey. Optional — no CxBase =
     * plain window app (close = quit), as before. */
    struct MsgPort *cxport = NULL;
    CxObj *broker = NULL;
    STRPTR *tt = NULL;
    int retcode = 0;
    /* Public port (Stage 3b): the CLI forwards vault commands here so it doesn't
     * open the vault a second time. Created once we hold the (unlocked) vault. */
    struct MsgPort *pubport = NULL;
    ULONG pubsig = 0;

    curcode[0] = '\0';
    crypto_select_init();

    NewList(&lblist);

    if ((err = open_libs()) != NULL) {
        Printf((CONST_STRPTR)"AmiAuth: %s\n", (ULONG)err);
        close_libs();
        return 20;
    }

    /* Read CX_* tooltypes (WBStartup icon) or CLI args uniformly (amiga.lib). */
    tt = ArgArrayInit(argc, (CONST_STRPTR *)argv);

    /* PUBSCREEN=<name>: open on a named public screen instead of the default
     * one. Not commodity-specific (no CX_ prefix, matching TIMESERVER), and
     * independent of whether a broker exists - applies to the plain-window
     * fallback (no commodities.library) too. */
    {
        STRPTR ps = ArgString((CONST_STRPTR *)tt, (CONST_STRPTR)"PUBSCREEN", NULL);
        if (ps && ps[0]) {
            strncpy(g_pubscreen, (const char *)ps, sizeof g_pubscreen - 1);
            g_pubscreen[sizeof g_pubscreen - 1] = '\0';
        }
    }

    /* VAULT=<path>: explicit vault for this launch (also no CX_ prefix - not
     * commodity behaviour). Copied out because ArgArrayDone frees the
     * array's strings. */
    {
        STRPTR va = ArgString((CONST_STRPTR *)tt, (CONST_STRPTR)"VAULT", NULL);
        if (va && va[0]) {
            strncpy(g_vault_arg, (const char *)va, sizeof g_vault_arg - 1);
            g_vault_arg[sizeof g_vault_arg - 1] = '\0';
        }
    }

    /* Register the commodity broker BEFORE unlocking: a second launch must
     * detect the running instance and exit without prompting for a passphrase. */
    if (CxBase && (cxport = CreateMsgPort()) != NULL) {
        struct NewBroker nb;
        LONG cberr = 0;
        nb.nb_Version = NB_VERSION;
        nb.nb_Name    = (STRPTR)"AmiAuth";
        nb.nb_Title   = (STRPTR)"AmiAuth";
        nb.nb_Descr   = (STRPTR)"TOTP/HOTP authenticator";
        nb.nb_Unique  = NBU_UNIQUE | NBU_NOTIFY;   /* one instance; notify the elder */
        nb.nb_Flags   = COF_SHOW_HIDE;             /* Exchange Show/Hide */
        nb.nb_Pri     = (BYTE)ArgInt((CONST_STRPTR *)tt, (CONST_STRPTR)"CX_PRIORITY", 0);
        nb.nb_Port    = cxport;
        nb.nb_ReservedChannel = 0;
        broker = CxBroker(&nb, &cberr);
        if (!broker && cberr == CBERR_DUP) {
            /* Another AmiAuth is resident (now told to appear via NBU_NOTIFY);
             * leave its unlocked vault alone and exit quietly. */
            DeleteMsgPort(cxport);
            ArgArrayDone();
            close_libs();
            return 0;
        }
        if (broker) {
            STRPTR popkey = ArgString((CONST_STRPTR *)tt, (CONST_STRPTR)"CX_POPKEY", (CONST_STRPTR)"ctrl alt a");
            AttachCxObj(broker, HotKey((CONST_STRPTR)popkey, cxport, EVT_HOTKEY));
            ActivateCxObj(broker, 1);
            cxsig = 1UL << cxport->mp_SigBit;
            /* CX_POPUP=no starts hidden (window opens on the hotkey/Exchange). */
            { STRPTR pu = ArgString((CONST_STRPTR *)tt, (CONST_STRPTR)"CX_POPUP", (CONST_STRPTR)"yes");
              popup = !(pu && (pu[0] == 'n' || pu[0] == 'N')); }
        }
    }

    /* No commodities.library (or the broker above failed for some other
     * reason): the broker's CBERR_DUP is our only single-instance check, so
     * without one, two launches would race the same vault file with nothing
     * arbitrating between them (#64). Fall back to the same public port used
     * for CLI forwarding later - it exists independent of commodities.library
     * - checked here before any prompt, exactly like the broker path above.
     * Deliberately not forwarding an AAP_SHOW to wake the resident instance:
     * that would mean waiting on a reply from a port whose owning process
     * could itself be a stale registration left behind by an earlier abnormal
     * exit (#71), which would hang this launch rather than just quietly
     * declining to prompt - matching the broker path's own behaviour, which
     * has the same limitation via NBU_NOTIFY. */
    if (!broker) {
        struct MsgPort *resident;
        Forbid();
        resident = FindPort((CONST_STRPTR)AMIAUTH_PORT_NAME);
        Permit();
        if (resident) {
            ArgArrayDone();
            close_libs();
            return 0;
        }
    }

    strncpy(vpath, vault_path(), sizeof vpath - 1);
    vpath[sizeof vpath - 1] = '\0';
    path = vpath;
    /* CX_POPUP=no promises a silent boot, so a hidden commodity start defers
     * every prompt - unlock or first-run - to the first show (#50). An
     * always-unlocked vault needs no interaction and opens right away. */
    if (broker && !popup) {
        if (vault_load(&v, path, NULL) != VAULT_OK)
            deferred = 1;                 /* resident, locked, list empty */
    } else {
        int r = gui_open_vault(&v, vpath, sizeof vpath, &encrypted, 0);
        if (r <= 0) {
            if (r < 0) retcode = 10;
            goto cleanup;                  /* declined (clean) or reported error */
        }
    }

    clock_setup(&clk);
    /* One SNTP sync at startup so the resident instance has accurate (green) time
     * without a manual CLI SYNC. Server precedence mirrors the CLI: the TIMESERVER
     * tooltype, then the saved "server" pref, then the default pool. Fails
     * fast/quiet with no TCP/IP stack (or no response), leaving the persisted
     * offset in place (clock_sntp_sync only updates clk on success). */
    {
        STRPTR ts = ArgString((CONST_STRPTR *)tt, (CONST_STRPTR)"TIMESERVER", NULL);
        char cfg[128];
        const char *server;
        if (ts && ts[0])                                              server = (const char *)ts;
        else if (prefs_get("server", cfg, sizeof cfg) == 0 && cfg[0]) server = cfg;
        else                                                         server = "pool.ntp.org";
        if (clock_sntp_sync(&clk, server) == 0) {
            prefs_set("server", server);
            prefs_set_long("offset", clk.offset_seconds);
        }
    }
    naccounts = v.count;
    /* Idle auto-lock (encrypted vaults only): scrub + re-prompt after this many
     * idle seconds. Pref "idlelock" overrides; 0 disables. */
    if (prefs_get_long("idlelock", &idle_limit) != 0)
        idle_limit = DEFAULT_IDLE_LOCK;

    build_nodes(&lblist, &v);          /* account rows: [name | code | left] */

    have_timer = timer_open();
    have_clip  = clip_open();

    /* Every gadget is (re)built fresh in win_show on each show - see its
     * comment for why. QR import's menu item and the AppWindow port are
     * independent of the window's lifecycle, so still only set up once. */
    if (!(DataTypesBase && AslBase)) {
        int mi;
        for (mi = 0; g_menu[mi].nm_Type != NM_END; mi++)
            if (g_menu[mi].nm_Type == NM_ITEM &&
                (ULONG)g_menu[mi].nm_UserData == CMD_ADD_QR)
                g_menu[mi].nm_Flags |= NM_ITEMDISABLED;
    }
    if (DataTypesBase && WorkbenchBase)
        g_appport = CreateMsgPort();

    timersig = have_timer ? (1UL << g_tport->mp_SigBit) : 0;
    appsig   = g_appport ? (1UL << g_appport->mp_SigBit) : 0;
    if (have_timer) timer_arm(1);

    /* Open the window now, unless started as a hidden commodity (CX_POPUP=no):
     * then it stays dormant until the hotkey / Exchange "Show". win_show sets
     * winsig; it stays 0 while hidden so the event loop drops it from the mask. */
    if (popup) {
        win = win_show(&gw, &lblist, &v, have_clip, &clk, statbuf, &winsig, clk.state);
        if (!win && !broker) {                /* a plain window app needs its window */
            if (gw.winobj) Printf((CONST_STRPTR)"AmiAuth: could not open the window\n");
            goto cleanup;
        }
    }

    /* Public port for CLI forwarding (Stage 3b), only if we're the resident one.
     * Single-instance itself is handled earlier (the Stage 3a broker's CBERR_DUP,
     * or the early FindPort check above for the no-commodities case, #64) - this
     * guard is just to avoid a duplicate AddPort() if somehow reached anyway. */
    Forbid();
    if (!FindPort((CONST_STRPTR)AMIAUTH_PORT_NAME) &&
        (pubport = CreateMsgPort()) != NULL) {
        pubport->mp_Node.ln_Name = (char *)AMIAUTH_PORT_NAME;
        pubport->mp_Node.ln_Pri  = 0;
        AddPort(pubport);
        pubsig = 1UL << pubport->mp_SigBit;
    }
    Permit();

    while (running) {
        ULONG sigs = Wait((win ? winsig : 0) | timersig | (win ? appsig : 0) |
                          cxsig | pubsig | SIGBREAKF_CTRL_C);
        changed = 0;                            /* set by any add/remove/edit this pass */

        if (sigs & SIGBREAKF_CTRL_C) running = 0;

        /* --- commodity: hotkey + Exchange (Show/Hide/Enable/Disable/Kill) --- */
        if (cxsig && (sigs & cxsig)) {
            CxMsg *cxm;
            while ((cxm = (CxMsg *)GetMsg(cxport)) != NULL) {
                ULONG type = CxMsgType(cxm);
                LONG  id   = CxMsgID(cxm);
                ReplyMsg((struct Message *)cxm);
                if (type == CXM_IEVENT && id == EVT_HOTKEY) {
                    if (win) { WindowToFront(win); ActivateWindow(win); }
                    else if (!deferred ||
                             deferred_open(&v, vpath, sizeof vpath, &encrypted,
                                           &lblist, gw.listobj, &naccounts)) {
                        deferred = 0;   /* declined? stays hidden and locked */
                        win = win_show(&gw, &lblist, &v, have_clip, &clk, statbuf, &winsig, clk.state);
                    }
                } else if (type == CXM_COMMAND) {
                    switch (id) {
                        case CXCMD_APPEAR:
                        case CXCMD_UNIQUE:      /* a second launch asked us to show */
                            if (win) { WindowToFront(win); ActivateWindow(win); }
                            else if (!deferred ||
                                     deferred_open(&v, vpath, sizeof vpath, &encrypted,
                                                   &lblist, gw.listobj, &naccounts)) {
                                deferred = 0;
                                win = win_show(&gw, &lblist, &v, have_clip, &clk, statbuf, &winsig, clk.state);
                            }
                            break;
                        case CXCMD_DISAPPEAR:
                            win_hide(&gw, win); win = NULL;
                            break;
                        case CXCMD_ENABLE:  ActivateCxObj(broker, 1); break;
                        case CXCMD_DISABLE: ActivateCxObj(broker, 0); break;
                        case CXCMD_KILL:    running = 0; break;
                    }
                }
            }
        }

        /* --- CLI forwarding (Stage 3b): serve vault commands to the CLI, which
         * writes into the buffer it passed (same address space). Only while the
         * vault is unlocked; the passphrase never crosses the port. --- */
        if (pubsig && (sigs & pubsig)) {
            struct AmiAuthReq *req;
            while ((req = (struct AmiAuthReq *)GetMsg(pubport)) != NULL) {
                const char *arg = (const char *)req->aar_Arg;
                char  *rb    = (char *)req->aar_Buf;
                ULONG  rbcap = req->aar_BufLen;
                req->aar_Result = AAR_OK;
                switch (req->aar_Cmd) {
                    case AAP_SHOW:
                        if (win) { WindowToFront(win); ActivateWindow(win); }
                        else if (!deferred ||
                                 deferred_open(&v, vpath, sizeof vpath, &encrypted,
                                               &lblist, gw.listobj, &naccounts)) {
                            deferred = 0;
                            win = win_show(&gw, &lblist, &v, have_clip, &clk, statbuf, &winsig, clk.state);
                        } else {
                            req->aar_Result = AAR_LOCKED;   /* declined: still locked */
                        }
                        break;
                    case AAP_LIST:
                        if (!v.unlocked) { req->aar_Result = AAR_LOCKED; break; }
                        if (rb && rbcap) {
                            size_t k; rb[0] = '\0';
                            for (k = 0; k < v.count; k++) {
                                const otp_account *a = &v.accounts[k];
                                char line[OTP_MAX_ISSUER + OTP_MAX_LABEL + 3];
                                if (a->issuer[0]) { strcpy(line, a->issuer); strcat(line, ":"); strcat(line, a->label); }
                                else              strcpy(line, a->label);
                                strcat(line, "\n");
                                if (strlen(rb) + strlen(line) < rbcap) strcat(rb, line);
                            }
                        }
                        break;
                    case AAP_GET: {
                        int idx;
                        if (!v.unlocked) { req->aar_Result = AAR_LOCKED; break; }
                        idx = gui_find_account(&v, arg);
                        if (idx < 0) { req->aar_Result = AAR_NOTFOUND; break; }
                        {
                            otp_account *a = &v.accounts[idx];
                            char fmt[8];
                            unsigned long code;
                            if (strcmp(a->type, "hotp") == 0) {
                                code = (unsigned long)hotp_sha1(a->secret, a->secret_len, a->counter, a->digits);
                                a->counter++;                     /* stateful: persist */
                                if (gui_save(&v, path) != VAULT_OK) req->aar_Result = AAR_SAVEFAIL;
                            } else {
                                uint64_t now = clock_now_utc(&clk);
                                code = (unsigned long)totp_sha1(a->secret, a->secret_len, now, 0, a->period, a->digits);
                            }
                            if (rb && rbcap) { sprintf(fmt, "%%0%dlu\n", (int)a->digits); sprintf(rb, fmt, code); }
                        }
                        break;
                    }
                    case AAP_ADD: {
                        otp_account acct;
                        if (!v.unlocked)                     req->aar_Result = AAR_LOCKED;
                        else if (otpauth_parse(arg, &acct) != 0) req->aar_Result = AAR_BADARG;
                        else {
                            vault_result r = vault_add(&v, &acct);
                            if (r == VAULT_ERR_FULL) req->aar_Result = AAR_FULL;
                            else if (r != VAULT_OK)  req->aar_Result = AAR_SAVEFAIL;
                            else { if (gui_save(&v, path) != VAULT_OK) req->aar_Result = AAR_SAVEFAIL; changed = 1; }
                        }
                        memset(&acct, 0, sizeof acct);
                        break;
                    }
                    case AAP_REMOVE: {
                        int idx;
                        if (!v.unlocked) { req->aar_Result = AAR_LOCKED; break; }
                        idx = gui_find_account(&v, arg);
                        if (idx < 0) req->aar_Result = AAR_NOTFOUND;
                        else if (vault_remove(&v, (size_t)idx) != VAULT_OK) req->aar_Result = AAR_SAVEFAIL;
                        else { if (gui_save(&v, path) != VAULT_OK) req->aar_Result = AAR_SAVEFAIL; changed = 1; }
                        break;
                    }
                    default: req->aar_Result = AAR_BADARG; break;
                }
                ReplyMsg((struct Message *)req);
            }
        }

        if (have_timer && (sigs & timersig)) {
            WaitIO((struct IORequest *)g_treq);   /* consume the request */
            if (win && copied > 0 && --copied == 0)  /* revert the "Copied" flash */
                SetGadgetAttrs((struct Gadget *)gw.copyobj, win, NULL,
                               GA_Text, (ULONG)"Copy", TAG_END);
            /* auto-clear our copied code once it has sat ~30s, but only if the
             * clipboard is still ours (don't clobber something copied since). */
            if (clear_secs > 0 && --clear_secs == 0 && clip_write_id() == our_clipid)
                clip_clear();
            /* idle auto-lock: after enough inactivity, lock and hide an
             * encrypted vault's window; the next Show/hotkey/AAP_SHOW
             * re-prompts via the same deferred_open() path a hidden
             * commodity start uses. Blocking here with a nested
             * passphrase_request() modal instead would starve the
             * window/hotkey/CLI-port signals for as long as it's up (#37). */
            if (win && encrypted && idle_limit > 0 && (long)(++idle_secs) >= idle_limit) {
                vault_lock(&v);
                win_hide(&gw, win); win = NULL;
                deferred = 1;
                idle_secs = 0;
            }
            /* recompute + refresh below, then re-arm */
            timer_arm(1);
        }

        if (win && (sigs & winsig)) {
            ULONG result;
            UWORD code;
            int docopy = 0, doadd_clip = 0, doadd_type = 0, doadd_qr = 0, doedit = 0, doremove = 0;
            long donudge = 0;
            idle_secs = 0;                          /* any window input = activity */
            while ((result = DoMethod(gw.winobj, WM_HANDLEINPUT, (ULONG)&code)) != WMHI_LASTMSG) {
                switch (result & WMHI_CLASSMASK) {
                    case WMHI_CLOSEWINDOW:
                        /* As a commodity the close gadget hides (stay resident,
                         * vault unlocked); a plain window app quits. */
                        if (broker) { win_hide(&gw, win); win = NULL; }
                        else        { running = 0; }
                        break;
                    case WMHI_GADGETUP:
                        switch (result & WMHI_GADGETMASK) {
                            case GID_LIST: {
                                ULONG cs = 0, cm = 0;
                                GetAttr(LISTBROWSER_Selected, gw.listobj, &sel);
                                CurrentTime(&cs, &cm);       /* a quick second click = copy */
                                if (DoubleClick(lastsec, lastmic, cs, cm)) docopy = 1;
                                lastsec = cs; lastmic = cm;
                                break;
                            }
                            case GID_COPY:      docopy = 1;     break;
                            case GID_ADD:       doadd_type = 1; break;
                            case GID_EDIT:      doedit = 1;     break;
                            case GID_REMOVE:    doremove = 1;   break;
                            case GID_NUDGEDOWN: donudge = -CLOCK_NUDGE_STEP; break;
                            case GID_NUDGEUP:   donudge = CLOCK_NUDGE_STEP;  break;
                        }
                        break;
                    case WMHI_MENUPICK: {
                        struct Menu *ms = NULL;
                        UWORD mc = (UWORD)(result & WMHI_MENUMASK);
                        GetAttr(WINDOW_MenuStrip, gw.winobj, (ULONG *)&ms);
                        while (ms && mc != MENUNULL) {
                            struct MenuItem *it = ItemAddress(ms, mc);
                            if (!it) break;
                            switch ((ULONG)GTMENUITEM_USERDATA(it)) {
                                case CMD_ADD_CLIP: doadd_clip = 1; break;
                                case CMD_ADD_TYPE: doadd_type = 1; break;
                                case CMD_ADD_QR:   doadd_qr   = 1; break;
                                case CMD_EDIT:     doedit     = 1; break;
                                case CMD_COPY:     docopy     = 1; break;
                                case CMD_REMOVE:   doremove   = 1; break;
                                case CMD_QUIT:     running    = 0; break;
                            }
                            mc = it->NextSelect;
                        }
                        break;
                    }
                    case WMHI_VANILLAKEY: {
                        /* Plain-letter gadget shortcuts (#55): window.class only
                         * dispatches these on request (IDCMP_VANILLAKEY, added
                         * above) - it does not auto-wire the '_' markers in
                         * GA_Text the way GadTools' BUTTON_KIND does. The letter
                         * each case matches comes from the LBL_* string itself
                         * (see their declaration), not a separate literal, so a
                         * relabel can't silently desync its shortcut. Mirror each
                         * shortcut's own gadget's disabled condition so a key
                         * press can't do what the equivalent click can't. */
                        int ch = (int)(result & WMHI_KEYMASK);
                        if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';   /* fold case */
                        if      (ch == (LBL_ADD[1]        | 0x20)) { if (StringBase) doadd_type = 1; }
                        else if (ch == (LBL_EDIT[1]       | 0x20)) { if (StringBase && v.count > 0) doedit = 1; }
                        else if (ch == (LBL_REMOVE[1]     | 0x20)) { if (v.count > 0) doremove = 1; }
                        else if (ch == (LBL_COPY[1]       | 0x20)) { if (have_clip && v.count > 0) docopy = 1; }
                        else if (ch == (LBL_NUDGE_DOWN[1] | 0x20)) { donudge = -CLOCK_NUDGE_STEP; }
                        else if (ch == (LBL_NUDGE_UP[1]   | 0x20)) { donudge = CLOCK_NUDGE_STEP;  }
                        break;
                    }
                    case WMHI_RAWKEY: {
                        /* Cursor Up/Down move the account selection by one row
                         * (#55) - the listbrowser doesn't do this on its own.
                         * `code` carries the raw key for a WMHI_RAWKEY result
                         * (CURSORUP/CURSORDOWN = 0x4C/0x4D, intuition.h). Only
                         * the visible selection moves here; the detail pane
                         * (code/gauge) follows on the next per-second refresh,
                         * same as a mouse click on a row (GID_LIST above). */
                        if (v.count > 0 && (code == CURSORUP || code == CURSORDOWN)) {
                            if (code == CURSORDOWN) { if (sel + 1 < v.count) sel++; }
                            else                     { if (sel > 0) sel--; }
                            SetGadgetAttrs((struct Gadget *)gw.listobj, win, NULL,
                                           LISTBROWSER_Selected, sel, TAG_END);
                        }
                        break;
                    }
                }
            }
            if (docopy && have_clip && curcode[0]) {
                our_clipid = clip_write_text(curcode);
                clear_secs = CLIP_CLEAR_SECS;         /* start/reset auto-clear */
                SetGadgetAttrs((struct Gadget *)gw.copyobj, win, NULL,
                               GA_Text, (ULONG)"Copied", TAG_END);
                copied = 2;                           /* revert after ~2 ticks */
            }

            /* --- Add (clipboard or typed URI) --- */
            if (doadd_clip || doadd_type) {
                int ok = doadd_clip ? (clip_read_text(uribuf, sizeof uribuf) > 0)
                                    : uri_request(uribuf, sizeof uribuf);
                if (ok) changed |= gui_add_uri(win, &v, path, uribuf);
                memset(uribuf, 0, sizeof uribuf);
            }

            /* --- Add from a QR image (asl file requester) --- */
            if (doadd_qr) {
                char img[256];
                if (qr_file_request(win, img, sizeof img))
                    changed |= do_add_qr(win, &v, path, img);
            }

            /* --- Edit selected (issuer/label/digits/period; secret kept) --- */
            if (doedit && v.count > 0) {
                otp_account acct;
                if (sel >= v.count) sel = 0;
                acct = v.accounts[sel];               /* copy preserves the secret */
                if (edit_request(&acct)) {
                    v.accounts[sel] = acct;           /* apply the edits */
                    changed = 1;
                    rc = gui_save(&v, path);
                    if (rc != VAULT_OK)
                        gui_requester(win, "Could not save the vault.", "OK", NULL);
                }
                memset(&acct, 0, sizeof acct);
            }

            /* --- Remove selected (with confirmation) --- */
            if (doremove && v.count > 0) {
                char name[OTP_MAX_ISSUER + OTP_MAX_LABEL + 2];
                const otp_account *a;
                if (sel >= v.count) sel = 0;
                a = &v.accounts[sel];
                if (a->issuer[0]) { strcpy(name, a->issuer); strcat(name, ":"); strcat(name, a->label); }
                else              strcpy(name, a->label);
                if (gui_requester(win, "Remove %s?", "Remove|Cancel", name) == 1) {
                    rc = vault_remove(&v, sel);
                    changed = 1;
                    if (rc == VAULT_OK) rc = gui_save(&v, path);
                    if (rc != VAULT_OK)
                        gui_requester(win, "Could not save the vault.", "OK", NULL);
                }
            }

            /* --- Manual clock nudge: relative adjustment, saved immediately,
             * status text + LED refreshed right away rather than waiting for
             * the next timer tick. Independent of the vault (clock affects
             * every account's code, encrypted or not). --- */
            if (donudge) {
                clock_nudge(&clk, donudge);
                prefs_set_long("offset", clk.offset_seconds);
                clock_status_text(&clk, statbuf);
                SetGadgetAttrs((struct Gadget *)gw.statobj, win, NULL, GA_Text, (ULONG)statbuf, TAG_END);
                led_draw(win, gw.statobj, clk.state);
            }
        }

        /* --- drag-and-drop: image icons dropped on the window (QR import) --- */
        if (win && g_appport && (sigs & appsig)) {
            struct AppMessage *am;
            idle_secs = 0;                          /* a drop counts as activity */
            while ((am = (struct AppMessage *)GetMsg(g_appport)) != NULL) {
                LONG n;
                for (n = 0; n < am->am_NumArgs; n++) {
                    struct WBArg *wa = &am->am_ArgList[n];
                    char fp[256];
                    if (wa->wa_Lock && NameFromLock(wa->wa_Lock, (STRPTR)fp, sizeof fp)) {
                        if (wa->wa_Name && wa->wa_Name[0])
                            AddPart((STRPTR)fp, (STRPTR)wa->wa_Name, sizeof fp);
                        if (fp[0]) changed |= do_add_qr(win, &v, path, fp);
                    }
                }
                ReplyMsg((struct Message *)am);
            }
        }

        /* --- reflect an add/remove/edit in the list + button states. Rebuild the
         * nodes even while hidden (a CLI-forwarded add/remove can arrive then), so
         * the list is correct on the next show; only touch gadgets when open. --- */
        if (changed) {
            if (win)
                SetGadgetAttrs((struct Gadget *)gw.listobj, win, NULL, LISTBROWSER_Labels, ~0UL, TAG_END);
            build_nodes(&lblist, &v);
            naccounts = v.count;
            if (sel >= v.count) sel = v.count ? v.count - 1 : 0;
            curcode[0] = '\0';
            if (win) {
                SetGadgetAttrs((struct Gadget *)gw.listobj, win, NULL, LISTBROWSER_Labels, (ULONG)&lblist, TAG_END);
                SetGadgetAttrs((struct Gadget *)gw.removeobj, win, NULL, GA_Disabled, (v.count == 0), TAG_END);
                SetGadgetAttrs((struct Gadget *)gw.editobj,   win, NULL,
                               GA_Disabled, (!StringBase || v.count == 0), TAG_END);
                SetGadgetAttrs((struct Gadget *)gw.copyobj,   win, NULL,
                               GA_Disabled, (!have_clip || v.count == 0), TAG_END);
            }
        }

        /* Refresh every account's code + countdown in the list, and drive the
         * detail pane (big code + gauge) from the selected row. Only when
         * shown, and only on the actual once-a-second timer tick - this does
         * real HMAC-SHA1 work per account, so gate it strictly: an earlier
         * version ran this on every loop wakeup regardless of source (window
         * activity, CLI forwarding, commodity messages, AppWindow drag
         * notifications...), which with any accounts present was enough
         * uncounted crypto work on every wakeup to visibly stall unrelated
         * Workbench drag gestures system-wide, not just interaction with
         * this window (#37). */
        if (have_timer && (sigs & timersig) && win && running && v.count > 0) {
            uint64_t now = clock_now_utc(&clk);
            uint32_t sel_period = OTP_DEFAULT_PERIOD, sel_rem = 0;
            char fmt[8], newcode[sizeof g_code[0]];
            int list_changed = 0;
            if (sel >= v.count) sel = 0;
            for (i = 0; i < v.count; i++) {
                const otp_account *a = &v.accounts[i];
                uint32_t period = a->period ? a->period : OTP_DEFAULT_PERIOD;
                uint32_t code = totp_sha1(a->secret, a->secret_len, now, 0, period, a->digits);
                uint32_t rem  = totp_seconds_remaining(now, 0, period);
                /* libnix sprintf lacks '*' width, so build "%06lu"/"%08lu". */
                sprintf(fmt, "%%0%dlu", (int)a->digits);
                sprintf(newcode, fmt, (unsigned long)code);
                /* A full LISTBROWSER_Labels reset stalls unrelated Workbench
                 * drag gestures system-wide (#62 - closed-source ReAction
                 * cost, not fixable on our side), so only pay for it when a
                 * code actually rolled over, not on every second's tick. */
                if (strcmp(newcode, g_code[i]) != 0) {
                    strcpy(g_code[i], newcode);
                    list_changed = 1;
                }
                /* Only actually redrawn when list_changed triggers the relist
                 * below, so between rollovers this freezes rather than
                 * ticking down - the detail pane's gauge (right below) is the
                 * live per-second countdown for the selected account. */
                sprintf(g_left[i], "%2lus", (unsigned long)rem);
                if (i == sel) { sel_period = period; sel_rem = rem; }
            }
            /* The nodes point at g_code[]/g_left[]; re-set Labels to repaint.
             * Cell text is fixed-width (codes and the "NNs" countdown), so no
             * stale pixels are left behind. */
            if (list_changed)
                SetGadgetAttrs((struct Gadget *)gw.listobj, win, NULL,
                               LISTBROWSER_Labels, (ULONG)&lblist, TAG_END);
            strcpy(curcode, g_code[sel]);      /* selected row -> detail + copy */
            SetGadgetAttrs((struct Gadget *)gw.codeobj, win, NULL,
                           GA_Text, (ULONG)curcode, TAG_END);
            SetGadgetAttrs((struct Gadget *)gw.gaugeobj, win, NULL,
                           FUELGAUGE_Max, sel_period, FUELGAUGE_Level, sel_rem, TAG_END);
        }

        /* Same reasoning as the refresh block above: NUDGE (and anything else
         * that actually changes clk.state) already redraws the LED directly
         * and immediately where it happens, so this trailing call is only a
         * periodic safety-net repaint - gate it to the timer tick too rather
         * than every loop wakeup. */
        if (have_timer && (sigs & timersig) && win && running)
            led_draw(win, gw.statobj, clk.state);
    }

cleanup:
    if (pubport) {                            /* stop new forwards, drain pending */
        struct AmiAuthReq *req;
        RemPort(pubport);
        while ((req = (struct AmiAuthReq *)GetMsg(pubport)) != NULL) {
            req->aar_Result = AAR_LOCKED;     /* we're going away */
            ReplyMsg((struct Message *)req);
        }
        DeleteMsgPort(pubport);
    }
    if (win) { win_hide(&gw, win); win = NULL; }  /* also disposes gw.winobj */
    if (gw.winobj) DisposeObject(gw.winobj);  /* only fires if WM_OPEN itself failed */
    if (broker) DeleteCxObjAll(broker);       /* detach the hotkey + broker */
    if (cxport) {                             /* drain any late commodity msgs */
        struct Message *m;
        while ((m = GetMsg(cxport)) != NULL) ReplyMsg(m);
        DeleteMsgPort(cxport);
    }
    if (tt) ArgArrayDone();
    if (g_appport) {                          /* drain any late drops, then close */
        struct AppMessage *am;
        while ((am = (struct AppMessage *)GetMsg(g_appport)) != NULL)
            ReplyMsg((struct Message *)am);
        DeleteMsgPort(g_appport);
        g_appport = NULL;
    }
    /* free the listbrowser nodes */
    {
        struct Node *n;
        while ((n = RemHead(&lblist)) != NULL) FreeListBrowserNode(n);
    }
    if (have_clip) clip_close();
    if (have_timer) timer_close();
    vault_lock(&v);
    close_libs();
    return retcode;
}
