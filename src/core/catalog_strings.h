
/****************************************************************

   This file was created automatically by `FlexCat 2.18'
   from "locale/AmiAuth.cd"

   using CatComp.sd 1.2 (24.09.1999)

   Do NOT edit by hand!

****************************************************************/

#ifndef AmiAuth_STRINGS_H
#define AmiAuth_STRINGS_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifdef  AmiAuth_BASIC_CODE
#undef  AmiAuth_BASIC
#undef  AmiAuth_CODE
#define AmiAuth_BASIC
#define AmiAuth_CODE
#endif

#ifdef  AmiAuth_BASIC
#undef  AmiAuth_ARRAY
#undef  AmiAuth_BLOCK
#define AmiAuth_ARRAY
#define AmiAuth_BLOCK
#endif

#ifdef  AmiAuth_ARRAY
#undef  AmiAuth_NUMBERS
#undef  AmiAuth_STRINGS
#define AmiAuth_NUMBERS
#define AmiAuth_STRINGS
#endif

#ifdef  AmiAuth_BLOCK
#undef  AmiAuth_STRINGS
#define AmiAuth_STRINGS
#endif


#ifdef AmiAuth_CODE
#include <proto/locale.h>
extern struct Library *LocaleBase;
#endif

#ifdef AmiAuth_NUMBERS

#define MSG_CLI_ALREADY_EXISTS 1
#define MSG_CLI_ADDED 2
#define MSG_CLI_REMOVED 3
#define MSG_CLI_USAGE_RUNAS 4
#define MSG_GUI_COL_ACCOUNT 5
#define MSG_GUI_COL_CODE 6
#define MSG_GUI_COL_LEFT 7
#define MSG_GUI_CANNOT_OPEN_VAULT 8
#define MSG_GUI_BAD_URI 9
#define MSG_GUI_BAD_SECRET 10
#define MSG_GUI_NO_QR 11
#define MSG_GUI_SAVE_FAILED 12
#define MSG_GUI_REKEY_DONE 13
#define MSG_GUI_REKEY_FAILED 14
#define MSG_GUI_OK 15
#define MSG_GUI_CANCEL 16
#define MSG_ERR_IO 17
#define MSG_ERR_FORMAT 18
#define MSG_ERR_AUTH 19
#define MSG_ERR_LOCKED 20
#define MSG_ERR_FULL 21
#define MSG_ERR_RANGE 22
#define MSG_ERR_UNKNOWN 23
#define MSG_CLI_ERR 24
#define MSG_CLI_WARN_HOTP_PERSIST 25
#define MSG_CLI_PROMPT_PASSPHRASE 26
#define MSG_CLI_NO_TTY 27
#define MSG_CLI_NO_RNG_SAVE 28
#define MSG_CLI_REKEY_STRENGTHEN_PROMPT 29
#define MSG_CLI_REKEY_SLOW_NOTICE 30
#define MSG_CLI_REKEY_LOWER_PROMPT 31
#define MSG_CLI_CONFIRM_YES_PROMPT 32
#define MSG_CLI_REKEYED 33
#define MSG_CLI_REKEY_FAILED 34
#define MSG_CLI_BAD_SECRET 35
#define MSG_CLI_SECONDS_REMAINING 36
#define MSG_CLI_CLOCK_SYNCED 37
#define MSG_CLI_CLOCK_MANUAL 38
#define MSG_CLI_CLOCK_UNVERIFIED 39
#define MSG_CLI_CLOCK_OFFSET_LINE 40
#define MSG_CLI_CLOCK_STATUS_LINE 41
#define MSG_CLI_CLOCK_CORRECTED_LINE 42
#define MSG_CLI_QUERYING 43
#define MSG_CLI_SYNC_FAILED 44
#define MSG_CLI_OFFSET_SAVE_FAILED 45
#define MSG_CLI_INIT_PASS_PROMPT 46
#define MSG_CLI_INIT_NO_TTY 47
#define MSG_CONFIRM_PASSPHRASE 48
#define MSG_CLI_PASS_MISMATCH 49
#define MSG_CLI_INIT_NO_RNG 50
#define MSG_CLI_KDF_ITERATIONS 51
#define MSG_CLI_CREATED 52
#define MSG_CLI_ALWAYS_UNLOCKED 53
#define MSG_CLI_ENCRYPTED 54
#define MSG_CLI_GUI_LOCKED 55
#define MSG_CLI_NO_ACCOUNT 56
#define MSG_CLI_VAULT_FULL 57
#define MSG_CLI_BAD_OTPURI 58
#define MSG_CLI_GUI_SAVEFAIL 59
#define MSG_CLI_URI_TOO_LONG 60
#define MSG_CLI_GUI_UNKNOWN 61
#define MSG_CLI_PARSE_FAILED 62
#define MSG_CLI_ADDED_NOISSUER 63
#define MSG_CLI_BAD_BASE32 64
#define MSG_CLI_NO_GUI 65
#define MSG_CLI_TAGLINE 66
#define MSG_CLI_ENCRYPTED_NOTE 67
#define MSG_GUI_MENU_PROJECT 68
#define MSG_GUI_QUIT 69
#define MSG_GUI_MENU_ACCOUNT 70
#define MSG_GUI_MENU_ADD_CLIP 71
#define MSG_GUI_MENU_ADD_TYPE 72
#define MSG_GUI_MENU_ADD_QR 73
#define MSG_GUI_MENU_EDIT 74
#define MSG_GUI_MENU_COPY 75
#define MSG_GUI_MENU_SHOW_QR 76
#define MSG_GUI_MENU_REMOVE 77
#define MSG_GUI_BTN_ADD 78
#define MSG_GUI_BTN_EDIT 79
#define MSG_GUI_BTN_REMOVE 80
#define MSG_GUI_BTN_COPY 81
#define MSG_GUI_BTN_NUDGE_DOWN 82
#define MSG_GUI_BTN_NUDGE_UP 83
#define MSG_GUI_COPY_PLAIN 84
#define MSG_GUI_COPIED 85
#define MSG_GUI_CLOCK_SYNCED 86
#define MSG_GUI_CLOCK_MANUAL 87
#define MSG_GUI_CLOCK_UNVERIFIED 88
#define MSG_GUI_CLOCK_LINE_SHORT 89
#define MSG_GUI_CLOCK_LINE_FULL 90
#define MSG_GUI_REKEY_STRENGTHEN_BODY 91
#define MSG_GUI_REKEY_STRENGTHEN_BTNS 92
#define MSG_GUI_REKEY_SLOW_BODY 93
#define MSG_GUI_REKEY_LOWER_BTNS 94
#define MSG_GUI_REKEY_CONFIRM_BODY 95
#define MSG_GUI_REKEY_REDUCE_BTNS 96
#define MSG_GUI_VAULT_FULL 97
#define MSG_GUI_IMG_TOOBIG 98
#define MSG_GUI_QR_UNAVAIL 99
#define MSG_GUI_IMG_NOMEM 100
#define MSG_GUI_IMG_READ_FAILED 101
#define MSG_GUI_TITLE_DECODING 102
#define MSG_GUI_QR_FILE_TITLE 103
#define MSG_GUI_TITLE_UNLOCK 104
#define MSG_GUI_CHOOSE_LOCATION 105
#define MSG_GUI_WELCOME 106
#define MSG_GUI_CREATE_VAULT_BTNS_DEFER 107
#define MSG_GUI_CREATE_VAULT_BTNS_QUIT 108
#define MSG_GUI_NEW_PASS_PROMPT 109
#define MSG_GUI_STORE_UNENCRYPTED_BODY 110
#define MSG_GUI_STORE_UNENCRYPTED_BTNS 111
#define MSG_GUI_PASS_MISMATCH 112
#define MSG_GUI_TRY_AGAIN 113
#define MSG_GUI_NO_RNG_BODY 114
#define MSG_GUI_CANNOT_WRITE_BODY 115
#define MSG_GUI_CHOOSE_LOCATION_BTNS 116
#define MSG_GUI_CREATE_FAILED 117
#define MSG_GUI_ENTER_PASS 118
#define MSG_GUI_WRONG_PASS 119
#define MSG_GUI_CANNOT_OPEN_PRINTF 120
#define MSG_GUI_URI_PASTE_LABEL 121
#define MSG_GUI_TITLE_ADD_ACCOUNT 122
#define MSG_GUI_LABEL_ISSUER 123
#define MSG_GUI_LABEL_LABEL 124
#define MSG_GUI_LABEL_DIGITS 125
#define MSG_GUI_LABEL_PERIOD 126
#define MSG_GUI_TITLE_EDIT_ACCOUNT 127
#define MSG_GUI_LABEL_REQUIRED_STEAM 128
#define MSG_GUI_LABEL_REQUIRED_FULL 129
#define MSG_GUI_QR_BUILD_FAILED 130
#define MSG_GUI_QR_TOO_LONG 131
#define MSG_GUI_QR_DISPLAY_FAILED 132
#define MSG_GUI_QR_WINDOW_FAILED 133
#define MSG_GUI_TITLE_QR_ISSUER 134
#define MSG_GUI_TITLE_QR_NOISSUER 135
#define MSG_GUI_SECRET_META_HINT 136
#define MSG_GUI_ADD_BTN 137
#define MSG_GUI_ISSUER_LABEL_REQUIRED 138
#define MSG_GUI_CX_DESCR 139
#define MSG_GUI_REMOVE_CONFIRM 140
#define MSG_GUI_REMOVE_BTNS 141
#define MSG_GUI_NEEDS_INTUITION 142
#define MSG_GUI_NEEDS_REACTION 143
#define MSG_GUI_WINDOW_CREATE_FAILED 144
#define MSG_GUI_WINDOW_OPEN_FAILED 145

#endif /* AmiAuth_NUMBERS */


/****************************************************************************/


#ifdef AmiAuth_STRINGS

#define MSG_CLI_ALREADY_EXISTS_STR "%s: %s already exists\n"
#define MSG_CLI_ADDED_STR "Added %s:%s\n"
#define MSG_CLI_REMOVED_STR "Removed '%s'\n"
#define MSG_CLI_USAGE_RUNAS_STR "Run as '%s <COMMAND> ...'  ('%s ?' shows the arg template):\n"
#define MSG_GUI_COL_ACCOUNT_STR "Account"
#define MSG_GUI_COL_CODE_STR "Code"
#define MSG_GUI_COL_LEFT_STR "Left"
#define MSG_GUI_CANNOT_OPEN_VAULT_STR "Cannot open the vault at\n%s"
#define MSG_GUI_BAD_URI_STR "That is not a valid otpauth:// URI."
#define MSG_GUI_BAD_SECRET_STR "That is not an otpauth:// URI or a Base32 secret."
#define MSG_GUI_NO_QR_STR "No otpauth QR code was found in that image."
#define MSG_GUI_SAVE_FAILED_STR "Could not save the vault."
#define MSG_GUI_REKEY_DONE_STR "Vault re-keyed for this machine."
#define MSG_GUI_REKEY_FAILED_STR "Re-key failed; the vault is unchanged."
#define MSG_GUI_OK_STR "OK"
#define MSG_GUI_CANCEL_STR "Cancel"
#define MSG_ERR_IO_STR "cannot read/write the vault file"
#define MSG_ERR_FORMAT_STR "not a valid vault file"
#define MSG_ERR_AUTH_STR "wrong passphrase or the file has been tampered with"
#define MSG_ERR_LOCKED_STR "vault is locked"
#define MSG_ERR_FULL_STR "vault is full"
#define MSG_ERR_RANGE_STR "no such account"
#define MSG_ERR_UNKNOWN_STR "error"
#define MSG_CLI_ERR_STR "%s: %s\n"
#define MSG_CLI_WARN_HOTP_PERSIST_STR "%s: warning: could not persist HOTP counter (%s)\n"
#define MSG_CLI_PROMPT_PASSPHRASE_STR "Passphrase:"
#define MSG_CLI_NO_TTY_STR "%s: this vault is encrypted; run from an interactive terminal, or use an always-unlocked vault for scripting\n"
#define MSG_CLI_NO_RNG_SAVE_STR "%s: no secure random source to save an encrypted vault (Phase 4)\n"
#define MSG_CLI_REKEY_STRENGTHEN_PROMPT_STR "This machine is much faster than the one that secured this vault.\nStrengthen it now? [(y)es/(N)o/ne(v)er ask here]"
#define MSG_CLI_REKEY_SLOW_NOTICE_STR "%s: unlock took ~%lus; this vault was tuned for faster hardware.\n"
#define MSG_CLI_REKEY_LOWER_PROMPT_STR "Re-key LOWER for quicker unlocks here? This REDUCES security. [(y)es/(N)o]"
#define MSG_CLI_CONFIRM_YES_PROMPT_STR "Type 'yes' to confirm:"
#define MSG_CLI_REKEYED_STR "%s: re-keyed to %lu iterations\n"
#define MSG_CLI_REKEY_FAILED_STR "%s: re-key failed; vault left unchanged\n"
#define MSG_CLI_BAD_SECRET_STR "%s: invalid or empty Base32 secret\n"
#define MSG_CLI_SECONDS_REMAINING_STR "(%u seconds remaining)\n"
#define MSG_CLI_CLOCK_SYNCED_STR "synced (green)"
#define MSG_CLI_CLOCK_MANUAL_STR "offset applied (amber)"
#define MSG_CLI_CLOCK_UNVERIFIED_STR "unverified (red)"
#define MSG_CLI_CLOCK_OFFSET_LINE_STR "UTC offset : %+ld seconds (%+ld min)\n"
#define MSG_CLI_CLOCK_STATUS_LINE_STR "status     : %s\n"
#define MSG_CLI_CLOCK_CORRECTED_LINE_STR "corrected  : %04d-%02d-%02d %02d:%02d:%02d UTC\n"
#define MSG_CLI_QUERYING_STR "Querying %s ...\n"
#define MSG_CLI_SYNC_FAILED_STR "%s: SNTP sync failed (no TCP/IP stack, or no response from %s)\n"
#define MSG_CLI_OFFSET_SAVE_FAILED_STR "%s: could not save the offset\n"
#define MSG_CLI_INIT_PASS_PROMPT_STR "New passphrase (empty for an always-unlocked vault):"
#define MSG_CLI_INIT_NO_TTY_STR "%s: INIT needs an interactive terminal (use 'INIT --open' for a non-interactive always-unlocked vault)\n"
#define MSG_CONFIRM_PASSPHRASE_STR "Confirm passphrase:"
#define MSG_CLI_PASS_MISMATCH_STR "%s: passphrases did not match\n"
#define MSG_CLI_INIT_NO_RNG_STR "%s: no secure random source for an encrypted vault (Phase 4); create an always-unlocked vault with 'INIT --open'\n"
#define MSG_CLI_KDF_ITERATIONS_STR "%s: KDF iterations = %lu\n"
#define MSG_CLI_CREATED_STR "Created %s vault at %s\n"
#define MSG_CLI_ALWAYS_UNLOCKED_STR "always-unlocked"
#define MSG_CLI_ENCRYPTED_STR "encrypted"
#define MSG_CLI_GUI_LOCKED_STR "%s: the running GUI's vault is locked; unlock it there first\n"
#define MSG_CLI_NO_ACCOUNT_STR "%s: no account matching '%s'\n"
#define MSG_CLI_VAULT_FULL_STR "%s: the vault is full (max 64 accounts)\n"
#define MSG_CLI_BAD_OTPURI_STR "%s: not a valid otpauth:// URI\n"
#define MSG_CLI_GUI_SAVEFAIL_STR "%s: applied in the GUI but the re-save failed\n"
#define MSG_CLI_URI_TOO_LONG_STR "%s: this account's URI is too long to encode as a QR code\n"
#define MSG_CLI_GUI_UNKNOWN_STR "%s: the running GUI could not handle that\n"
#define MSG_CLI_PARSE_FAILED_STR "%s: could not parse otpauth:// URI\n"
#define MSG_CLI_ADDED_NOISSUER_STR "Added %s\n"
#define MSG_CLI_BAD_BASE32_STR "%s: that does not look like a Base32 secret\n"
#define MSG_CLI_NO_GUI_STR "%s: no running GUI to show\n"
#define MSG_CLI_TAGLINE_STR "%s - TOTP/HOTP authenticator for AmigaOS\n\n"
#define MSG_CLI_ENCRYPTED_NOTE_STR "Encrypted vaults prompt for the passphrase on the terminal.\n"
#define MSG_GUI_MENU_PROJECT_STR "Project"
#define MSG_GUI_QUIT_STR "Quit"
#define MSG_GUI_MENU_ACCOUNT_STR "Account"
#define MSG_GUI_MENU_ADD_CLIP_STR "Add from clipboard"
#define MSG_GUI_MENU_ADD_TYPE_STR "Add (type URI/secret)..."
#define MSG_GUI_MENU_ADD_QR_STR "Add from QR image..."
#define MSG_GUI_MENU_EDIT_STR "Edit selected..."
#define MSG_GUI_MENU_COPY_STR "Copy code"
#define MSG_GUI_MENU_SHOW_QR_STR "Show QR code..."
#define MSG_GUI_MENU_REMOVE_STR "Remove selected..."
#define MSG_GUI_BTN_ADD_STR "_Add"
#define MSG_GUI_BTN_EDIT_STR "_Edit"
#define MSG_GUI_BTN_REMOVE_STR "_Remove"
#define MSG_GUI_BTN_COPY_STR "_Copy"
#define MSG_GUI_BTN_NUDGE_DOWN_STR "_D -10s"
#define MSG_GUI_BTN_NUDGE_UP_STR "_U +10s"
#define MSG_GUI_COPY_PLAIN_STR "Copy"
#define MSG_GUI_COPIED_STR "Copied"
#define MSG_GUI_CLOCK_SYNCED_STR "synced"
#define MSG_GUI_CLOCK_MANUAL_STR "manual"
#define MSG_GUI_CLOCK_UNVERIFIED_STR "unverified"
#define MSG_GUI_CLOCK_LINE_SHORT_STR "Clock: %s"
#define MSG_GUI_CLOCK_LINE_FULL_STR "Clock: %s %s%lu:%02lu"
#define MSG_GUI_REKEY_STRENGTHEN_BODY_STR "This machine is much faster than the one that secured this vault.\nStrengthen it now (re-key to more PBKDF2 iterations)?"
#define MSG_GUI_REKEY_STRENGTHEN_BTNS_STR "Strengthen|Not now|Never here"
#define MSG_GUI_REKEY_SLOW_BODY_STR "Unlock took about %lu seconds; this vault was tuned for faster hardware.\nRe-key LOWER for quicker unlocks here? This REDUCES security."
#define MSG_GUI_REKEY_LOWER_BTNS_STR "Re-key lower|Cancel"
#define MSG_GUI_REKEY_CONFIRM_BODY_STR "Really reduce this vault's security?"
#define MSG_GUI_REKEY_REDUCE_BTNS_STR "Reduce|Cancel"
#define MSG_GUI_VAULT_FULL_STR "The vault is full (max 64 accounts)."
#define MSG_GUI_IMG_TOOBIG_STR "That image is too large to decode."
#define MSG_GUI_QR_UNAVAIL_STR "QR import needs datatypes.library."
#define MSG_GUI_IMG_NOMEM_STR "Not enough memory to load that image."
#define MSG_GUI_IMG_READ_FAILED_STR "Could not read that image (needs picture.datatype)."
#define MSG_GUI_TITLE_DECODING_STR "%s - Decoding QR image..."
#define MSG_GUI_QR_FILE_TITLE_STR "Select a QR image"
#define MSG_GUI_TITLE_UNLOCK_STR "%s - Unlock"
#define MSG_GUI_CHOOSE_LOCATION_STR "Choose where to keep the vault"
#define MSG_GUI_WELCOME_STR "Welcome to %s!\n\nThere is no vault yet - it will be created at\n%s"
#define MSG_GUI_CREATE_VAULT_BTNS_DEFER_STR "Create a vault...|Not now"
#define MSG_GUI_CREATE_VAULT_BTNS_QUIT_STR "Create a vault...|Quit"
#define MSG_GUI_NEW_PASS_PROMPT_STR "New passphrase (empty = always-unlocked):"
#define MSG_GUI_STORE_UNENCRYPTED_BODY_STR "Store the vault UNENCRYPTED?\n\nWith no passphrase there is no at-rest protection:\nanyone with access to the file can read your secrets.\n(You can add a passphrase later.)"
#define MSG_GUI_STORE_UNENCRYPTED_BTNS_STR "Store unencrypted|Go back"
#define MSG_GUI_PASS_MISMATCH_STR "The passphrases did not match."
#define MSG_GUI_TRY_AGAIN_STR "Try again"
#define MSG_GUI_NO_RNG_BODY_STR "No secure random source is available, so an\nencrypted vault cannot be created safely."
#define MSG_GUI_CANNOT_WRITE_BODY_STR "Cannot write the vault to\n%s\n\nChoose another location?"
#define MSG_GUI_CHOOSE_LOCATION_BTNS_STR "Choose location...|Quit"
#define MSG_GUI_CREATE_FAILED_STR "Creating the vault failed."
#define MSG_GUI_ENTER_PASS_STR "Enter passphrase:"
#define MSG_GUI_WRONG_PASS_STR "Wrong passphrase - try again:"
#define MSG_GUI_CANNOT_OPEN_PRINTF_STR "%s: cannot open the vault (%ld)\n"
#define MSG_GUI_URI_PASTE_LABEL_STR "Paste or type an otpauth:// URI or Base32 secret:"
#define MSG_GUI_TITLE_ADD_ACCOUNT_STR "%s - Add account"
#define MSG_GUI_LABEL_ISSUER_STR "Issuer:"
#define MSG_GUI_LABEL_LABEL_STR "Label:"
#define MSG_GUI_LABEL_DIGITS_STR "Digits:"
#define MSG_GUI_LABEL_PERIOD_STR "Period:"
#define MSG_GUI_TITLE_EDIT_ACCOUNT_STR "%s - Edit account"
#define MSG_GUI_LABEL_REQUIRED_STEAM_STR "Label is required."
#define MSG_GUI_LABEL_REQUIRED_FULL_STR "Label is required; digits must be 6-8, period 1-86400."
#define MSG_GUI_QR_BUILD_FAILED_STR "Could not build a QR code for this account."
#define MSG_GUI_QR_TOO_LONG_STR "This account's URI is too long to encode as a QR code."
#define MSG_GUI_QR_DISPLAY_FAILED_STR "Could not build the QR display."
#define MSG_GUI_QR_WINDOW_FAILED_STR "Could not open the QR window."
#define MSG_GUI_TITLE_QR_ISSUER_STR "%s - QR: %s:%s"
#define MSG_GUI_TITLE_QR_NOISSUER_STR "%s - QR: %s"
#define MSG_GUI_SECRET_META_HINT_STR "A bare secret carries no name - who is this account for?"
#define MSG_GUI_ADD_BTN_STR "Add"
#define MSG_GUI_ISSUER_LABEL_REQUIRED_STR "Issuer and Label are both required."
#define MSG_GUI_CX_DESCR_STR "TOTP/HOTP authenticator"
#define MSG_GUI_REMOVE_CONFIRM_STR "Remove %s?"
#define MSG_GUI_REMOVE_BTNS_STR "Remove|Cancel"
#define MSG_GUI_NEEDS_INTUITION_STR "needs intuition.library / utility.library v37+ (OS 2.04)"
#define MSG_GUI_NEEDS_REACTION_STR "needs ReAction/ClassAct classes (window/layout/listbrowser/fuelgauge/button)"
#define MSG_GUI_WINDOW_CREATE_FAILED_STR "%s: could not create the window\n"
#define MSG_GUI_WINDOW_OPEN_FAILED_STR "%s: could not open the window\n"

#endif /* AmiAuth_STRINGS */


/****************************************************************************/


#ifdef AmiAuth_ARRAY

struct AmiAuth_ArrayType
{
    LONG   cca_ID;
    STRPTR cca_Str;
};

static const struct AmiAuth_ArrayType AmiAuth_Array[] =
{
    { MSG_CLI_ALREADY_EXISTS, (STRPTR)MSG_CLI_ALREADY_EXISTS_STR },
    { MSG_CLI_ADDED, (STRPTR)MSG_CLI_ADDED_STR },
    { MSG_CLI_REMOVED, (STRPTR)MSG_CLI_REMOVED_STR },
    { MSG_CLI_USAGE_RUNAS, (STRPTR)MSG_CLI_USAGE_RUNAS_STR },
    { MSG_GUI_COL_ACCOUNT, (STRPTR)MSG_GUI_COL_ACCOUNT_STR },
    { MSG_GUI_COL_CODE, (STRPTR)MSG_GUI_COL_CODE_STR },
    { MSG_GUI_COL_LEFT, (STRPTR)MSG_GUI_COL_LEFT_STR },
    { MSG_GUI_CANNOT_OPEN_VAULT, (STRPTR)MSG_GUI_CANNOT_OPEN_VAULT_STR },
    { MSG_GUI_BAD_URI, (STRPTR)MSG_GUI_BAD_URI_STR },
    { MSG_GUI_BAD_SECRET, (STRPTR)MSG_GUI_BAD_SECRET_STR },
    { MSG_GUI_NO_QR, (STRPTR)MSG_GUI_NO_QR_STR },
    { MSG_GUI_SAVE_FAILED, (STRPTR)MSG_GUI_SAVE_FAILED_STR },
    { MSG_GUI_REKEY_DONE, (STRPTR)MSG_GUI_REKEY_DONE_STR },
    { MSG_GUI_REKEY_FAILED, (STRPTR)MSG_GUI_REKEY_FAILED_STR },
    { MSG_GUI_OK, (STRPTR)MSG_GUI_OK_STR },
    { MSG_GUI_CANCEL, (STRPTR)MSG_GUI_CANCEL_STR },
    { MSG_ERR_IO, (STRPTR)MSG_ERR_IO_STR },
    { MSG_ERR_FORMAT, (STRPTR)MSG_ERR_FORMAT_STR },
    { MSG_ERR_AUTH, (STRPTR)MSG_ERR_AUTH_STR },
    { MSG_ERR_LOCKED, (STRPTR)MSG_ERR_LOCKED_STR },
    { MSG_ERR_FULL, (STRPTR)MSG_ERR_FULL_STR },
    { MSG_ERR_RANGE, (STRPTR)MSG_ERR_RANGE_STR },
    { MSG_ERR_UNKNOWN, (STRPTR)MSG_ERR_UNKNOWN_STR },
    { MSG_CLI_ERR, (STRPTR)MSG_CLI_ERR_STR },
    { MSG_CLI_WARN_HOTP_PERSIST, (STRPTR)MSG_CLI_WARN_HOTP_PERSIST_STR },
    { MSG_CLI_PROMPT_PASSPHRASE, (STRPTR)MSG_CLI_PROMPT_PASSPHRASE_STR },
    { MSG_CLI_NO_TTY, (STRPTR)MSG_CLI_NO_TTY_STR },
    { MSG_CLI_NO_RNG_SAVE, (STRPTR)MSG_CLI_NO_RNG_SAVE_STR },
    { MSG_CLI_REKEY_STRENGTHEN_PROMPT, (STRPTR)MSG_CLI_REKEY_STRENGTHEN_PROMPT_STR },
    { MSG_CLI_REKEY_SLOW_NOTICE, (STRPTR)MSG_CLI_REKEY_SLOW_NOTICE_STR },
    { MSG_CLI_REKEY_LOWER_PROMPT, (STRPTR)MSG_CLI_REKEY_LOWER_PROMPT_STR },
    { MSG_CLI_CONFIRM_YES_PROMPT, (STRPTR)MSG_CLI_CONFIRM_YES_PROMPT_STR },
    { MSG_CLI_REKEYED, (STRPTR)MSG_CLI_REKEYED_STR },
    { MSG_CLI_REKEY_FAILED, (STRPTR)MSG_CLI_REKEY_FAILED_STR },
    { MSG_CLI_BAD_SECRET, (STRPTR)MSG_CLI_BAD_SECRET_STR },
    { MSG_CLI_SECONDS_REMAINING, (STRPTR)MSG_CLI_SECONDS_REMAINING_STR },
    { MSG_CLI_CLOCK_SYNCED, (STRPTR)MSG_CLI_CLOCK_SYNCED_STR },
    { MSG_CLI_CLOCK_MANUAL, (STRPTR)MSG_CLI_CLOCK_MANUAL_STR },
    { MSG_CLI_CLOCK_UNVERIFIED, (STRPTR)MSG_CLI_CLOCK_UNVERIFIED_STR },
    { MSG_CLI_CLOCK_OFFSET_LINE, (STRPTR)MSG_CLI_CLOCK_OFFSET_LINE_STR },
    { MSG_CLI_CLOCK_STATUS_LINE, (STRPTR)MSG_CLI_CLOCK_STATUS_LINE_STR },
    { MSG_CLI_CLOCK_CORRECTED_LINE, (STRPTR)MSG_CLI_CLOCK_CORRECTED_LINE_STR },
    { MSG_CLI_QUERYING, (STRPTR)MSG_CLI_QUERYING_STR },
    { MSG_CLI_SYNC_FAILED, (STRPTR)MSG_CLI_SYNC_FAILED_STR },
    { MSG_CLI_OFFSET_SAVE_FAILED, (STRPTR)MSG_CLI_OFFSET_SAVE_FAILED_STR },
    { MSG_CLI_INIT_PASS_PROMPT, (STRPTR)MSG_CLI_INIT_PASS_PROMPT_STR },
    { MSG_CLI_INIT_NO_TTY, (STRPTR)MSG_CLI_INIT_NO_TTY_STR },
    { MSG_CONFIRM_PASSPHRASE, (STRPTR)MSG_CONFIRM_PASSPHRASE_STR },
    { MSG_CLI_PASS_MISMATCH, (STRPTR)MSG_CLI_PASS_MISMATCH_STR },
    { MSG_CLI_INIT_NO_RNG, (STRPTR)MSG_CLI_INIT_NO_RNG_STR },
    { MSG_CLI_KDF_ITERATIONS, (STRPTR)MSG_CLI_KDF_ITERATIONS_STR },
    { MSG_CLI_CREATED, (STRPTR)MSG_CLI_CREATED_STR },
    { MSG_CLI_ALWAYS_UNLOCKED, (STRPTR)MSG_CLI_ALWAYS_UNLOCKED_STR },
    { MSG_CLI_ENCRYPTED, (STRPTR)MSG_CLI_ENCRYPTED_STR },
    { MSG_CLI_GUI_LOCKED, (STRPTR)MSG_CLI_GUI_LOCKED_STR },
    { MSG_CLI_NO_ACCOUNT, (STRPTR)MSG_CLI_NO_ACCOUNT_STR },
    { MSG_CLI_VAULT_FULL, (STRPTR)MSG_CLI_VAULT_FULL_STR },
    { MSG_CLI_BAD_OTPURI, (STRPTR)MSG_CLI_BAD_OTPURI_STR },
    { MSG_CLI_GUI_SAVEFAIL, (STRPTR)MSG_CLI_GUI_SAVEFAIL_STR },
    { MSG_CLI_URI_TOO_LONG, (STRPTR)MSG_CLI_URI_TOO_LONG_STR },
    { MSG_CLI_GUI_UNKNOWN, (STRPTR)MSG_CLI_GUI_UNKNOWN_STR },
    { MSG_CLI_PARSE_FAILED, (STRPTR)MSG_CLI_PARSE_FAILED_STR },
    { MSG_CLI_ADDED_NOISSUER, (STRPTR)MSG_CLI_ADDED_NOISSUER_STR },
    { MSG_CLI_BAD_BASE32, (STRPTR)MSG_CLI_BAD_BASE32_STR },
    { MSG_CLI_NO_GUI, (STRPTR)MSG_CLI_NO_GUI_STR },
    { MSG_CLI_TAGLINE, (STRPTR)MSG_CLI_TAGLINE_STR },
    { MSG_CLI_ENCRYPTED_NOTE, (STRPTR)MSG_CLI_ENCRYPTED_NOTE_STR },
    { MSG_GUI_MENU_PROJECT, (STRPTR)MSG_GUI_MENU_PROJECT_STR },
    { MSG_GUI_QUIT, (STRPTR)MSG_GUI_QUIT_STR },
    { MSG_GUI_MENU_ACCOUNT, (STRPTR)MSG_GUI_MENU_ACCOUNT_STR },
    { MSG_GUI_MENU_ADD_CLIP, (STRPTR)MSG_GUI_MENU_ADD_CLIP_STR },
    { MSG_GUI_MENU_ADD_TYPE, (STRPTR)MSG_GUI_MENU_ADD_TYPE_STR },
    { MSG_GUI_MENU_ADD_QR, (STRPTR)MSG_GUI_MENU_ADD_QR_STR },
    { MSG_GUI_MENU_EDIT, (STRPTR)MSG_GUI_MENU_EDIT_STR },
    { MSG_GUI_MENU_COPY, (STRPTR)MSG_GUI_MENU_COPY_STR },
    { MSG_GUI_MENU_SHOW_QR, (STRPTR)MSG_GUI_MENU_SHOW_QR_STR },
    { MSG_GUI_MENU_REMOVE, (STRPTR)MSG_GUI_MENU_REMOVE_STR },
    { MSG_GUI_BTN_ADD, (STRPTR)MSG_GUI_BTN_ADD_STR },
    { MSG_GUI_BTN_EDIT, (STRPTR)MSG_GUI_BTN_EDIT_STR },
    { MSG_GUI_BTN_REMOVE, (STRPTR)MSG_GUI_BTN_REMOVE_STR },
    { MSG_GUI_BTN_COPY, (STRPTR)MSG_GUI_BTN_COPY_STR },
    { MSG_GUI_BTN_NUDGE_DOWN, (STRPTR)MSG_GUI_BTN_NUDGE_DOWN_STR },
    { MSG_GUI_BTN_NUDGE_UP, (STRPTR)MSG_GUI_BTN_NUDGE_UP_STR },
    { MSG_GUI_COPY_PLAIN, (STRPTR)MSG_GUI_COPY_PLAIN_STR },
    { MSG_GUI_COPIED, (STRPTR)MSG_GUI_COPIED_STR },
    { MSG_GUI_CLOCK_SYNCED, (STRPTR)MSG_GUI_CLOCK_SYNCED_STR },
    { MSG_GUI_CLOCK_MANUAL, (STRPTR)MSG_GUI_CLOCK_MANUAL_STR },
    { MSG_GUI_CLOCK_UNVERIFIED, (STRPTR)MSG_GUI_CLOCK_UNVERIFIED_STR },
    { MSG_GUI_CLOCK_LINE_SHORT, (STRPTR)MSG_GUI_CLOCK_LINE_SHORT_STR },
    { MSG_GUI_CLOCK_LINE_FULL, (STRPTR)MSG_GUI_CLOCK_LINE_FULL_STR },
    { MSG_GUI_REKEY_STRENGTHEN_BODY, (STRPTR)MSG_GUI_REKEY_STRENGTHEN_BODY_STR },
    { MSG_GUI_REKEY_STRENGTHEN_BTNS, (STRPTR)MSG_GUI_REKEY_STRENGTHEN_BTNS_STR },
    { MSG_GUI_REKEY_SLOW_BODY, (STRPTR)MSG_GUI_REKEY_SLOW_BODY_STR },
    { MSG_GUI_REKEY_LOWER_BTNS, (STRPTR)MSG_GUI_REKEY_LOWER_BTNS_STR },
    { MSG_GUI_REKEY_CONFIRM_BODY, (STRPTR)MSG_GUI_REKEY_CONFIRM_BODY_STR },
    { MSG_GUI_REKEY_REDUCE_BTNS, (STRPTR)MSG_GUI_REKEY_REDUCE_BTNS_STR },
    { MSG_GUI_VAULT_FULL, (STRPTR)MSG_GUI_VAULT_FULL_STR },
    { MSG_GUI_IMG_TOOBIG, (STRPTR)MSG_GUI_IMG_TOOBIG_STR },
    { MSG_GUI_QR_UNAVAIL, (STRPTR)MSG_GUI_QR_UNAVAIL_STR },
    { MSG_GUI_IMG_NOMEM, (STRPTR)MSG_GUI_IMG_NOMEM_STR },
    { MSG_GUI_IMG_READ_FAILED, (STRPTR)MSG_GUI_IMG_READ_FAILED_STR },
    { MSG_GUI_TITLE_DECODING, (STRPTR)MSG_GUI_TITLE_DECODING_STR },
    { MSG_GUI_QR_FILE_TITLE, (STRPTR)MSG_GUI_QR_FILE_TITLE_STR },
    { MSG_GUI_TITLE_UNLOCK, (STRPTR)MSG_GUI_TITLE_UNLOCK_STR },
    { MSG_GUI_CHOOSE_LOCATION, (STRPTR)MSG_GUI_CHOOSE_LOCATION_STR },
    { MSG_GUI_WELCOME, (STRPTR)MSG_GUI_WELCOME_STR },
    { MSG_GUI_CREATE_VAULT_BTNS_DEFER, (STRPTR)MSG_GUI_CREATE_VAULT_BTNS_DEFER_STR },
    { MSG_GUI_CREATE_VAULT_BTNS_QUIT, (STRPTR)MSG_GUI_CREATE_VAULT_BTNS_QUIT_STR },
    { MSG_GUI_NEW_PASS_PROMPT, (STRPTR)MSG_GUI_NEW_PASS_PROMPT_STR },
    { MSG_GUI_STORE_UNENCRYPTED_BODY, (STRPTR)MSG_GUI_STORE_UNENCRYPTED_BODY_STR },
    { MSG_GUI_STORE_UNENCRYPTED_BTNS, (STRPTR)MSG_GUI_STORE_UNENCRYPTED_BTNS_STR },
    { MSG_GUI_PASS_MISMATCH, (STRPTR)MSG_GUI_PASS_MISMATCH_STR },
    { MSG_GUI_TRY_AGAIN, (STRPTR)MSG_GUI_TRY_AGAIN_STR },
    { MSG_GUI_NO_RNG_BODY, (STRPTR)MSG_GUI_NO_RNG_BODY_STR },
    { MSG_GUI_CANNOT_WRITE_BODY, (STRPTR)MSG_GUI_CANNOT_WRITE_BODY_STR },
    { MSG_GUI_CHOOSE_LOCATION_BTNS, (STRPTR)MSG_GUI_CHOOSE_LOCATION_BTNS_STR },
    { MSG_GUI_CREATE_FAILED, (STRPTR)MSG_GUI_CREATE_FAILED_STR },
    { MSG_GUI_ENTER_PASS, (STRPTR)MSG_GUI_ENTER_PASS_STR },
    { MSG_GUI_WRONG_PASS, (STRPTR)MSG_GUI_WRONG_PASS_STR },
    { MSG_GUI_CANNOT_OPEN_PRINTF, (STRPTR)MSG_GUI_CANNOT_OPEN_PRINTF_STR },
    { MSG_GUI_URI_PASTE_LABEL, (STRPTR)MSG_GUI_URI_PASTE_LABEL_STR },
    { MSG_GUI_TITLE_ADD_ACCOUNT, (STRPTR)MSG_GUI_TITLE_ADD_ACCOUNT_STR },
    { MSG_GUI_LABEL_ISSUER, (STRPTR)MSG_GUI_LABEL_ISSUER_STR },
    { MSG_GUI_LABEL_LABEL, (STRPTR)MSG_GUI_LABEL_LABEL_STR },
    { MSG_GUI_LABEL_DIGITS, (STRPTR)MSG_GUI_LABEL_DIGITS_STR },
    { MSG_GUI_LABEL_PERIOD, (STRPTR)MSG_GUI_LABEL_PERIOD_STR },
    { MSG_GUI_TITLE_EDIT_ACCOUNT, (STRPTR)MSG_GUI_TITLE_EDIT_ACCOUNT_STR },
    { MSG_GUI_LABEL_REQUIRED_STEAM, (STRPTR)MSG_GUI_LABEL_REQUIRED_STEAM_STR },
    { MSG_GUI_LABEL_REQUIRED_FULL, (STRPTR)MSG_GUI_LABEL_REQUIRED_FULL_STR },
    { MSG_GUI_QR_BUILD_FAILED, (STRPTR)MSG_GUI_QR_BUILD_FAILED_STR },
    { MSG_GUI_QR_TOO_LONG, (STRPTR)MSG_GUI_QR_TOO_LONG_STR },
    { MSG_GUI_QR_DISPLAY_FAILED, (STRPTR)MSG_GUI_QR_DISPLAY_FAILED_STR },
    { MSG_GUI_QR_WINDOW_FAILED, (STRPTR)MSG_GUI_QR_WINDOW_FAILED_STR },
    { MSG_GUI_TITLE_QR_ISSUER, (STRPTR)MSG_GUI_TITLE_QR_ISSUER_STR },
    { MSG_GUI_TITLE_QR_NOISSUER, (STRPTR)MSG_GUI_TITLE_QR_NOISSUER_STR },
    { MSG_GUI_SECRET_META_HINT, (STRPTR)MSG_GUI_SECRET_META_HINT_STR },
    { MSG_GUI_ADD_BTN, (STRPTR)MSG_GUI_ADD_BTN_STR },
    { MSG_GUI_ISSUER_LABEL_REQUIRED, (STRPTR)MSG_GUI_ISSUER_LABEL_REQUIRED_STR },
    { MSG_GUI_CX_DESCR, (STRPTR)MSG_GUI_CX_DESCR_STR },
    { MSG_GUI_REMOVE_CONFIRM, (STRPTR)MSG_GUI_REMOVE_CONFIRM_STR },
    { MSG_GUI_REMOVE_BTNS, (STRPTR)MSG_GUI_REMOVE_BTNS_STR },
    { MSG_GUI_NEEDS_INTUITION, (STRPTR)MSG_GUI_NEEDS_INTUITION_STR },
    { MSG_GUI_NEEDS_REACTION, (STRPTR)MSG_GUI_NEEDS_REACTION_STR },
    { MSG_GUI_WINDOW_CREATE_FAILED, (STRPTR)MSG_GUI_WINDOW_CREATE_FAILED_STR },
    { MSG_GUI_WINDOW_OPEN_FAILED, (STRPTR)MSG_GUI_WINDOW_OPEN_FAILED_STR },
};


#endif /* AmiAuth_ARRAY */


/****************************************************************************/


#ifdef AmiAuth_BLOCK

static const char AmiAuth_Block[] =
{

     "\x00\x00\x00\x01" "\x00\x16"
    MSG_CLI_ALREADY_EXISTS_STR ""
     "\x00\x00\x00\x02" "\x00\x0c"
    MSG_CLI_ADDED_STR ""
     "\x00\x00\x00\x03" "\x00\x0e"
    MSG_CLI_REMOVED_STR "\x00"
     "\x00\x00\x00\x04" "\x00\x3c"
    MSG_CLI_USAGE_RUNAS_STR ""
     "\x00\x00\x00\x05" "\x00\x08"
    MSG_GUI_COL_ACCOUNT_STR "\x00"
     "\x00\x00\x00\x06" "\x00\x04"
    MSG_GUI_COL_CODE_STR ""
     "\x00\x00\x00\x07" "\x00\x04"
    MSG_GUI_COL_LEFT_STR ""
     "\x00\x00\x00\x08" "\x00\x1c"
    MSG_GUI_CANNOT_OPEN_VAULT_STR "\x00"
     "\x00\x00\x00\x09" "\x00\x24"
    MSG_GUI_BAD_URI_STR "\x00"
     "\x00\x00\x00\x0a" "\x00\x32"
    MSG_GUI_BAD_SECRET_STR "\x00"
     "\x00\x00\x00\x0b" "\x00\x2c"
    MSG_GUI_NO_QR_STR "\x00"
     "\x00\x00\x00\x0c" "\x00\x1a"
    MSG_GUI_SAVE_FAILED_STR "\x00"
     "\x00\x00\x00\x0d" "\x00\x20"
    MSG_GUI_REKEY_DONE_STR ""
     "\x00\x00\x00\x0e" "\x00\x26"
    MSG_GUI_REKEY_FAILED_STR ""
     "\x00\x00\x00\x0f" "\x00\x02"
    MSG_GUI_OK_STR ""
     "\x00\x00\x00\x10" "\x00\x06"
    MSG_GUI_CANCEL_STR ""
     "\x00\x00\x00\x11" "\x00\x20"
    MSG_ERR_IO_STR ""
     "\x00\x00\x00\x12" "\x00\x16"
    MSG_ERR_FORMAT_STR ""
     "\x00\x00\x00\x13" "\x00\x34"
    MSG_ERR_AUTH_STR "\x00"
     "\x00\x00\x00\x14" "\x00\x10"
    MSG_ERR_LOCKED_STR "\x00"
     "\x00\x00\x00\x15" "\x00\x0e"
    MSG_ERR_FULL_STR "\x00"
     "\x00\x00\x00\x16" "\x00\x10"
    MSG_ERR_RANGE_STR "\x00"
     "\x00\x00\x00\x17" "\x00\x06"
    MSG_ERR_UNKNOWN_STR "\x00"
     "\x00\x00\x00\x18" "\x00\x08"
    MSG_CLI_ERR_STR "\x00"
     "\x00\x00\x00\x19" "\x00\x32"
    MSG_CLI_WARN_HOTP_PERSIST_STR "\x00"
     "\x00\x00\x00\x1a" "\x00\x0c"
    MSG_CLI_PROMPT_PASSPHRASE_STR "\x00"
     "\x00\x00\x00\x1b" "\x00\x6e"
    MSG_CLI_NO_TTY_STR "\x00"
     "\x00\x00\x00\x1c" "\x00\x42"
    MSG_CLI_NO_RNG_SAVE_STR "\x00"
     "\x00\x00\x00\x1d" "\x00\x72"
    MSG_CLI_REKEY_STRENGTHEN_PROMPT_STR ""
     "\x00\x00\x00\x1e" "\x00\x42"
    MSG_CLI_REKEY_SLOW_NOTICE_STR "\x00"
     "\x00\x00\x00\x1f" "\x00\x4a"
    MSG_CLI_REKEY_LOWER_PROMPT_STR ""
     "\x00\x00\x00\x20" "\x00\x16"
    MSG_CLI_CONFIRM_YES_PROMPT_STR ""
     "\x00\x00\x00\x21" "\x00\x20"
    MSG_CLI_REKEYED_STR "\x00"
     "\x00\x00\x00\x22" "\x00\x28"
    MSG_CLI_REKEY_FAILED_STR ""
     "\x00\x00\x00\x23" "\x00\x24"
    MSG_CLI_BAD_SECRET_STR "\x00"
     "\x00\x00\x00\x24" "\x00\x18"
    MSG_CLI_SECONDS_REMAINING_STR "\x00"
     "\x00\x00\x00\x25" "\x00\x0e"
    MSG_CLI_CLOCK_SYNCED_STR ""
     "\x00\x00\x00\x26" "\x00\x16"
    MSG_CLI_CLOCK_MANUAL_STR ""
     "\x00\x00\x00\x27" "\x00\x10"
    MSG_CLI_CLOCK_UNVERIFIED_STR ""
     "\x00\x00\x00\x28" "\x00\x26"
    MSG_CLI_CLOCK_OFFSET_LINE_STR "\x00"
     "\x00\x00\x00\x29" "\x00\x10"
    MSG_CLI_CLOCK_STATUS_LINE_STR ""
     "\x00\x00\x00\x2a" "\x00\x30"
    MSG_CLI_CLOCK_CORRECTED_LINE_STR "\x00"
     "\x00\x00\x00\x2b" "\x00\x10"
    MSG_CLI_QUERYING_STR ""
     "\x00\x00\x00\x2c" "\x00\x40"
    MSG_CLI_SYNC_FAILED_STR "\x00"
     "\x00\x00\x00\x2d" "\x00\x1e"
    MSG_CLI_OFFSET_SAVE_FAILED_STR ""
     "\x00\x00\x00\x2e" "\x00\x34"
    MSG_CLI_INIT_PASS_PROMPT_STR ""
     "\x00\x00\x00\x2f" "\x00\x68"
    MSG_CLI_INIT_NO_TTY_STR "\x00"
     "\x00\x00\x00\x30" "\x00\x14"
    MSG_CONFIRM_PASSPHRASE_STR "\x00"
     "\x00\x00\x00\x31" "\x00\x1e"
    MSG_CLI_PASS_MISMATCH_STR ""
     "\x00\x00\x00\x32" "\x00\x72"
    MSG_CLI_INIT_NO_RNG_STR "\x00"
     "\x00\x00\x00\x33" "\x00\x1a"
    MSG_CLI_KDF_ITERATIONS_STR "\x00"
     "\x00\x00\x00\x34" "\x00\x18"
    MSG_CLI_CREATED_STR "\x00"
     "\x00\x00\x00\x35" "\x00\x10"
    MSG_CLI_ALWAYS_UNLOCKED_STR "\x00"
     "\x00\x00\x00\x36" "\x00\x0a"
    MSG_CLI_ENCRYPTED_STR "\x00"
     "\x00\x00\x00\x37" "\x00\x3e"
    MSG_CLI_GUI_LOCKED_STR "\x00"
     "\x00\x00\x00\x38" "\x00\x1e"
    MSG_CLI_NO_ACCOUNT_STR "\x00"
     "\x00\x00\x00\x39" "\x00\x28"
    MSG_CLI_VAULT_FULL_STR ""
     "\x00\x00\x00\x3a" "\x00\x20"
    MSG_CLI_BAD_OTPURI_STR "\x00"
     "\x00\x00\x00\x3b" "\x00\x2e"
    MSG_CLI_GUI_SAVEFAIL_STR ""
     "\x00\x00\x00\x3c" "\x00\x3a"
    MSG_CLI_URI_TOO_LONG_STR ""
     "\x00\x00\x00\x3d" "\x00\x2a"
    MSG_CLI_GUI_UNKNOWN_STR ""
     "\x00\x00\x00\x3e" "\x00\x24"
    MSG_CLI_PARSE_FAILED_STR "\x00"
     "\x00\x00\x00\x3f" "\x00\x0a"
    MSG_CLI_ADDED_NOISSUER_STR "\x00"
     "\x00\x00\x00\x40" "\x00\x2c"
    MSG_CLI_BAD_BASE32_STR ""
     "\x00\x00\x00\x41" "\x00\x1c"
    MSG_CLI_NO_GUI_STR "\x00"
     "\x00\x00\x00\x42" "\x00\x2a"
    MSG_CLI_TAGLINE_STR ""
     "\x00\x00\x00\x43" "\x00\x3c"
    MSG_CLI_ENCRYPTED_NOTE_STR ""
     "\x00\x00\x00\x44" "\x00\x08"
    MSG_GUI_MENU_PROJECT_STR "\x00"
     "\x00\x00\x00\x45" "\x00\x04"
    MSG_GUI_QUIT_STR ""
     "\x00\x00\x00\x46" "\x00\x08"
    MSG_GUI_MENU_ACCOUNT_STR "\x00"
     "\x00\x00\x00\x47" "\x00\x12"
    MSG_GUI_MENU_ADD_CLIP_STR ""
     "\x00\x00\x00\x48" "\x00\x18"
    MSG_GUI_MENU_ADD_TYPE_STR ""
     "\x00\x00\x00\x49" "\x00\x14"
    MSG_GUI_MENU_ADD_QR_STR ""
     "\x00\x00\x00\x4a" "\x00\x10"
    MSG_GUI_MENU_EDIT_STR ""
     "\x00\x00\x00\x4b" "\x00\x0a"
    MSG_GUI_MENU_COPY_STR "\x00"
     "\x00\x00\x00\x4c" "\x00\x10"
    MSG_GUI_MENU_SHOW_QR_STR "\x00"
     "\x00\x00\x00\x4d" "\x00\x12"
    MSG_GUI_MENU_REMOVE_STR ""
     "\x00\x00\x00\x4e" "\x00\x04"
    MSG_GUI_BTN_ADD_STR ""
     "\x00\x00\x00\x4f" "\x00\x06"
    MSG_GUI_BTN_EDIT_STR "\x00"
     "\x00\x00\x00\x50" "\x00\x08"
    MSG_GUI_BTN_REMOVE_STR "\x00"
     "\x00\x00\x00\x51" "\x00\x06"
    MSG_GUI_BTN_COPY_STR "\x00"
     "\x00\x00\x00\x52" "\x00\x08"
    MSG_GUI_BTN_NUDGE_DOWN_STR "\x00"
     "\x00\x00\x00\x53" "\x00\x08"
    MSG_GUI_BTN_NUDGE_UP_STR "\x00"
     "\x00\x00\x00\x54" "\x00\x04"
    MSG_GUI_COPY_PLAIN_STR ""
     "\x00\x00\x00\x55" "\x00\x06"
    MSG_GUI_COPIED_STR ""
     "\x00\x00\x00\x56" "\x00\x06"
    MSG_GUI_CLOCK_SYNCED_STR ""
     "\x00\x00\x00\x57" "\x00\x06"
    MSG_GUI_CLOCK_MANUAL_STR ""
     "\x00\x00\x00\x58" "\x00\x0a"
    MSG_GUI_CLOCK_UNVERIFIED_STR ""
     "\x00\x00\x00\x59" "\x00\x0a"
    MSG_GUI_CLOCK_LINE_SHORT_STR "\x00"
     "\x00\x00\x00\x5a" "\x00\x16"
    MSG_GUI_CLOCK_LINE_FULL_STR "\x00"
     "\x00\x00\x00\x5b" "\x00\x78"
    MSG_GUI_REKEY_STRENGTHEN_BODY_STR "\x00"
     "\x00\x00\x00\x5c" "\x00\x1e"
    MSG_GUI_REKEY_STRENGTHEN_BTNS_STR "\x00"
     "\x00\x00\x00\x5d" "\x00\x86"
    MSG_GUI_REKEY_SLOW_BODY_STR ""
     "\x00\x00\x00\x5e" "\x00\x14"
    MSG_GUI_REKEY_LOWER_BTNS_STR "\x00"
     "\x00\x00\x00\x5f" "\x00\x24"
    MSG_GUI_REKEY_CONFIRM_BODY_STR ""
     "\x00\x00\x00\x60" "\x00\x0e"
    MSG_GUI_REKEY_REDUCE_BTNS_STR "\x00"
     "\x00\x00\x00\x61" "\x00\x24"
    MSG_GUI_VAULT_FULL_STR ""
     "\x00\x00\x00\x62" "\x00\x22"
    MSG_GUI_IMG_TOOBIG_STR ""
     "\x00\x00\x00\x63" "\x00\x22"
    MSG_GUI_QR_UNAVAIL_STR ""
     "\x00\x00\x00\x64" "\x00\x26"
    MSG_GUI_IMG_NOMEM_STR "\x00"
     "\x00\x00\x00\x65" "\x00\x34"
    MSG_GUI_IMG_READ_FAILED_STR "\x00"
     "\x00\x00\x00\x66" "\x00\x1a"
    MSG_GUI_TITLE_DECODING_STR "\x00"
     "\x00\x00\x00\x67" "\x00\x12"
    MSG_GUI_QR_FILE_TITLE_STR "\x00"
     "\x00\x00\x00\x68" "\x00\x0c"
    MSG_GUI_TITLE_UNLOCK_STR "\x00"
     "\x00\x00\x00\x69" "\x00\x1e"
    MSG_GUI_CHOOSE_LOCATION_STR ""
     "\x00\x00\x00\x6a" "\x00\x40"
    MSG_GUI_WELCOME_STR ""
     "\x00\x00\x00\x6b" "\x00\x1a"
    MSG_GUI_CREATE_VAULT_BTNS_DEFER_STR "\x00"
     "\x00\x00\x00\x6c" "\x00\x16"
    MSG_GUI_CREATE_VAULT_BTNS_QUIT_STR ""
     "\x00\x00\x00\x6d" "\x00\x2a"
    MSG_GUI_NEW_PASS_PROMPT_STR "\x00"
     "\x00\x00\x00\x6e" "\x00\xa8"
    MSG_GUI_STORE_UNENCRYPTED_BODY_STR ""
     "\x00\x00\x00\x6f" "\x00\x1a"
    MSG_GUI_STORE_UNENCRYPTED_BTNS_STR "\x00"
     "\x00\x00\x00\x70" "\x00\x1e"
    MSG_GUI_PASS_MISMATCH_STR ""
     "\x00\x00\x00\x71" "\x00\x0a"
    MSG_GUI_TRY_AGAIN_STR "\x00"
     "\x00\x00\x00\x72" "\x00\x56"
    MSG_GUI_NO_RNG_BODY_STR "\x00"
     "\x00\x00\x00\x73" "\x00\x36"
    MSG_GUI_CANNOT_WRITE_BODY_STR ""
     "\x00\x00\x00\x74" "\x00\x18"
    MSG_GUI_CHOOSE_LOCATION_BTNS_STR "\x00"
     "\x00\x00\x00\x75" "\x00\x1a"
    MSG_GUI_CREATE_FAILED_STR ""
     "\x00\x00\x00\x76" "\x00\x12"
    MSG_GUI_ENTER_PASS_STR "\x00"
     "\x00\x00\x00\x77" "\x00\x1e"
    MSG_GUI_WRONG_PASS_STR "\x00"
     "\x00\x00\x00\x78" "\x00\x20"
    MSG_GUI_CANNOT_OPEN_PRINTF_STR ""
     "\x00\x00\x00\x79" "\x00\x32"
    MSG_GUI_URI_PASTE_LABEL_STR "\x00"
     "\x00\x00\x00\x7a" "\x00\x10"
    MSG_GUI_TITLE_ADD_ACCOUNT_STR ""
     "\x00\x00\x00\x7b" "\x00\x08"
    MSG_GUI_LABEL_ISSUER_STR "\x00"
     "\x00\x00\x00\x7c" "\x00\x06"
    MSG_GUI_LABEL_LABEL_STR ""
     "\x00\x00\x00\x7d" "\x00\x08"
    MSG_GUI_LABEL_DIGITS_STR "\x00"
     "\x00\x00\x00\x7e" "\x00\x08"
    MSG_GUI_LABEL_PERIOD_STR "\x00"
     "\x00\x00\x00\x7f" "\x00\x12"
    MSG_GUI_TITLE_EDIT_ACCOUNT_STR "\x00"
     "\x00\x00\x00\x80" "\x00\x12"
    MSG_GUI_LABEL_REQUIRED_STEAM_STR ""
     "\x00\x00\x00\x81" "\x00\x36"
    MSG_GUI_LABEL_REQUIRED_FULL_STR ""
     "\x00\x00\x00\x82" "\x00\x2c"
    MSG_GUI_QR_BUILD_FAILED_STR "\x00"
     "\x00\x00\x00\x83" "\x00\x36"
    MSG_GUI_QR_TOO_LONG_STR ""
     "\x00\x00\x00\x84" "\x00\x20"
    MSG_GUI_QR_DISPLAY_FAILED_STR "\x00"
     "\x00\x00\x00\x85" "\x00\x1e"
    MSG_GUI_QR_WINDOW_FAILED_STR "\x00"
     "\x00\x00\x00\x86" "\x00\x0e"
    MSG_GUI_TITLE_QR_ISSUER_STR ""
     "\x00\x00\x00\x87" "\x00\x0c"
    MSG_GUI_TITLE_QR_NOISSUER_STR "\x00"
     "\x00\x00\x00\x88" "\x00\x38"
    MSG_GUI_SECRET_META_HINT_STR ""
     "\x00\x00\x00\x89" "\x00\x04"
    MSG_GUI_ADD_BTN_STR "\x00"
     "\x00\x00\x00\x8a" "\x00\x24"
    MSG_GUI_ISSUER_LABEL_REQUIRED_STR "\x00"
     "\x00\x00\x00\x8b" "\x00\x18"
    MSG_GUI_CX_DESCR_STR "\x00"
     "\x00\x00\x00\x8c" "\x00\x0a"
    MSG_GUI_REMOVE_CONFIRM_STR ""
     "\x00\x00\x00\x8d" "\x00\x0e"
    MSG_GUI_REMOVE_BTNS_STR "\x00"
     "\x00\x00\x00\x8e" "\x00\x38"
    MSG_GUI_NEEDS_INTUITION_STR ""
     "\x00\x00\x00\x8f" "\x00\x4c"
    MSG_GUI_NEEDS_REACTION_STR ""
     "\x00\x00\x00\x90" "\x00\x20"
    MSG_GUI_WINDOW_CREATE_FAILED_STR ""
     "\x00\x00\x00\x91" "\x00\x1e"
    MSG_GUI_WINDOW_OPEN_FAILED_STR ""

};

#endif /* AmiAuth_BLOCK */


/****************************************************************************/


#ifdef AmiAuth_CODE

#ifndef AmiAuth_CODE_EXISTS
 #define AmiAuth_CODE_EXISTS

 STRPTR GetAmiAuthString(struct AmiAuth_LocaleInfo *li, LONG stringNum)
 {
 LONG   *l;
 UWORD  *w;
 STRPTR  builtIn;

     l = (LONG *)AmiAuth_Block;

     while (*l != stringNum)
       {
       w = (UWORD *)((ULONG)l + 4);
       l = (LONG *)((ULONG)l + (ULONG)*w + 6);
       }
     builtIn = (STRPTR)((ULONG)l + 6);

// #define AmiAuth_XLocaleBase LocaleBase
// #define LocaleBase li->li_LocaleBase
    
     if(LocaleBase && li)
        return(GetCatalogStr(li->li_Catalog, stringNum, builtIn));

// #undef  LocaleBase
// #define LocaleBase XLocaleBase
// #undef  AmiAuth_XLocaleBase

     return(builtIn);
 }

#else

 STRPTR GetAmiAuthString(struct AmiAuth_LocaleInfo *li, LONG stringNum);

#endif /* AmiAuth_CODE_EXISTS */

#endif /* AmiAuth_CODE */


/****************************************************************************/


#endif /* AmiAuth_STRINGS_H */
