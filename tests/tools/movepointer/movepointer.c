
/*
    This little utility was written by the German Softcracking Group 9
    and intentionally left in the PUBLIC DOMAIN.

   Feel free to copy the program and this Lattice-C source file, but
   keep the name of the autor in it.

                     G.S.G. 9 (Cewy)  10-May-87

   If you'll do any changes (fix bugs or else) leave the new source
   (Lattice only !) at the BBS-BOX called 'Barrel-Burst-Box' and a
   message to me 'CEWY'. I'll check the box each week!

   Phone : (Germany) 06151/ 595240    (300 8N1 or 1200 8N1)

*/

/* executable (batch) file :
  .key name
  .def name MovePointer
  stack 30000
  lc -v <name>
  BLink FROM LIB:c.o+<name>.o TO <name> LIBRARY LIB:lc.lib+LIB:amiga.lib NODEBUG
*/

/* -------------------------- include section ------------------------ */

#include <stdio.h>
#include <intuition/intuition.h>
#include <devices/input.h>

/* -------------------- global variable section ---------------------- */

struct   IntuitionBase  *IntuitionBase = NULL; 
struct   InputEvent     MyEvent;
struct   IOStdReq       *MyRequest = NULL, *CreateStdIO();
struct   MsgPort        *MyPort = NULL,    *CreatePort ();

/* ---------------------- local function section --------------------- */
/* DESCRIPTION :
      print the usage of the program, so the user knows how to handle it.
   BUGS :
      none.
   COMMENT :
      uses puts instead of printf, 'cause puts is much shorter.
*/
void Usage()
{
   puts ("Usage: MovePointer NewXpos [NewYpos [R(elative) K(lick)]]");
   puts ("written by G.S.G. 9 in May '87");
   exit (20);
}

/* ======== */
/* DESCRIPTION :
      Close everything opened by me or the system
   BUGS :
      none.
   COMMENT :
      none.
*/
void CleanUp ()
{
   /* say good bye */
   if (MyRequest) {
      if (MyRequest-> io_Device) CloseDevice (MyRequest);
      DeleteStdIO (MyRequest);
   }
   if (MyPort) DeletePort (MyPort);

   /* close the opened library */
   if (IntuitionBase) CloseLibrary (IntuitionBase);

}

/* ======== */
/* DESCRIPTION :
      Die. Call Cleanup to leave the system in peace.
   BUGS :
      none.
   COMMENT :
      none.
*/
void abort (value)
int value;
{
   CleanUp ();
   exit (value);
}

/* ----------------------- main program section ----------------------- */
/* DESCRIPTION :
      Find the WorkbenchScreen and tell intuition that the mouse did move
   BUGS :
      none. (?)
   COMMENT :
      none.
*/

main (argc,argv)
int argc;
char *argv[];
{
   BOOL  Relative;
   int   Mouseklick;

   /* check the number of arguments */
   if (argc <3 || argc >5 || *argv[1] == '?') Usage ();

   Relative = FALSE;
   Mouseklick = 0;

   while (argc > 3 && *argv[3])
      switch (*argv[3]++) {
         case 'r' : case 'R' : Relative = TRUE; break;
         case 'k' : case 'K' : Mouseklick++; break;
         default  : Usage ();
      }


   /* open the intuition */
   if (!(IntuitionBase = (struct IntuitionBase *) OpenLibrary
         ("intuition.library",0))) abort (100);

   /* open the needed ports and IOs */
   if (!(MyPort    = CreatePort (0,0)))     abort (110);
   if (!(MyRequest = CreateStdIO (MyPort))) abort (120);

   if (OpenDevice ("input.device",0,MyRequest,0) ) abort (130);

   MyRequest -> io_Command = IND_WRITEEVENT;
   MyRequest -> io_Flags   = 0;
   MyRequest -> io_Length  = sizeof (struct InputEvent);
   MyRequest -> io_Data    = (APTR) &MyEvent;

   MyEvent.ie_NextEvent          = NULL;            /* no further event*/
   MyEvent.ie_Class              = IECLASS_RAWMOUSE;
   MyEvent.ie_TimeStamp.tv_secs  = 0;
   MyEvent.ie_TimeStamp.tv_micro = 0;
   MyEvent.ie_Code               = IECODE_NOBUTTON;
   MyEvent.ie_Qualifier          = IEQUALIFIER_RELATIVEMOUSE;

   if (!Relative) {
      MyEvent.ie_Y   = -574;  /* move to origin, biggest screen    */
      MyEvent.ie_X   = -704;  /* see morerows (FISH 54)           */
      DoIO (MyRequest);       /* tell intuition what you wanna do */
   }

   MyEvent.ie_Y   = (SHORT) atoi (argv[2]);
   MyEvent.ie_X   = (SHORT) atoi (argv[1]);
   DoIO (MyRequest);  /* tell intuition what you wanna do */

   for (; Mouseklick; Mouseklick--) {
      MyRequest -> io_Command = IND_WRITEEVENT;
      MyRequest -> io_Flags   = 0;
      MyRequest -> io_Length  = sizeof (struct InputEvent);
      MyRequest -> io_Data    = (APTR) &MyEvent;

      MyEvent.ie_NextEvent          = NULL;            /* no further event*/
      MyEvent.ie_Class              = IECLASS_RAWMOUSE;
      MyEvent.ie_TimeStamp.tv_secs  = 0;
      MyEvent.ie_TimeStamp.tv_micro = 0;
      MyEvent.ie_Code               = IECODE_LBUTTON;
      MyEvent.ie_X = 0;
      MyEvent.ie_Y = 0;

      DoIO (MyRequest);
      Delay (1);

      MyEvent.ie_Code =  IECODE_NOBUTTON;    /* hold down the button */
      DoIO (MyRequest);
      Delay (1);
   }

   CleanUp (); /* close all things */

   /* say 'well done' the the system */
   return (0);
}

