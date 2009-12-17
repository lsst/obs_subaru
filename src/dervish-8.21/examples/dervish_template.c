/*****************************************************************************
 ******************************************************************************
 **
 ** FILE:
 **     dervish_template.c
 **
 ** ABSTRACT:
 **     This file contains a template for users writing TCL main programs which
 **     utilize the FSAOimage interface features of Dervish.
 **
 **     The routines invoked by this main program were written to encapsulate
 **     the lower layers of the Dervish, in order to minimize chances of the
 **     template needing changes in the future.
 **
 ** ENVIRONMENT:
 **     ANSI C.
 ******************************************************************************
 ******************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>

#include "dervish.h"

#define L_SHPROMPT    "dervish> "   /* Command line prompt for dervish */

int main(int argc, char *argv[])
{
   CMD_HANDLE sh_handle;
   Tcl_Interp    *interp;

   if (shMainTcl_Init(&sh_handle, &argc, argv, L_SHPROMPT) != SH_SUCCESS)
       return 1;

   interp = sh_handle.interp;

   /*
    * Declare required TCL commands
    */
   ftclCreateCommands(interp);          /* Fermi enhancements to TCL   */
   ftclCreateHelp (interp);             /* Define help command         */
   ftclMsgDefine (interp);              /* Define help for tcl verbs   */
 
   /* --- PLACE YOUR FAVORITE TCL DECLARATIONS HERE --- */
 
   shTclPgplotDeclare(interp);         /* Pgplot Tcl commands */
 
   /* --- PLACE YOUR FAVORITE INITIALIZATIONS HERE --- */
   ftclParseInit(interp);               /* Ftcl command parsing */
 
   /*
    * NOTE: shMainTcl_Declare is a comprehensive declaration routine for
    * Dervish.  Included are TCL command declarations for region, peek, and
    * FSAOimage.  If all commands are not desired, one can customize by
    * replacing the call to this routine with individual calls to only the
    * desired TCL command declaration routines. It runs the initialization
    * scripts, so it must go after all of the initialization routines have
    * been called.
    */
   shMainTcl_Declare(&sh_handle);       /* Required Dervish TCL commands */

   /*
    *             DO NOT PUT ANYTHING BELOW THIS LINE.
    *
    * We will now enter the main loop of Tk. There is no successfull return
    * from this loop. Anything added below this line will never be reached,
    * NEVER!!
    */
   shTk_MainLoop(&sh_handle);

   return 0;
}
