
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

#define MSG_CLI_ERR 0
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

#endif /* AmiAuth_NUMBERS */


/****************************************************************************/


#ifdef AmiAuth_STRINGS

#define MSG_CLI_ERR_STR "AmiAuth: %s\n"
#define MSG_CLI_ALREADY_EXISTS_STR "AmiAuth: %s already exists\n"
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
    { MSG_CLI_ERR, (STRPTR)MSG_CLI_ERR_STR },
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
};


#endif /* AmiAuth_ARRAY */


/****************************************************************************/


#ifdef AmiAuth_BLOCK

static const char AmiAuth_Block[] =
{

     "\x00\x00\x00\x00" "\x00\x0c"
    MSG_CLI_ERR_STR ""
     "\x00\x00\x00\x01" "\x00\x1c"
    MSG_CLI_ALREADY_EXISTS_STR "\x00"
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
