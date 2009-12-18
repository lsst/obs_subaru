/*****************************************************************************
 ******************************************************************************
 **
 ** FILE:
 **	astrotoolsMain.c
 **
 ** ABSTRACT:
 **	This is the main program for the ASTROTOOLS product.
 **
 **	This product is built on top of DERVISH and FTCL.
 **
 ** ENVIRONMENT:
 **	ANSI C.
 **
 ******************************************************************************
 ******************************************************************************
 */
#define ATMAIN
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include "dervish.h"
#include "astrotools.h"
#include "sdssmath.h"
#undef ATMAIN

void smaVerbsDeclare (Tcl_Interp *interp);	/* prototype for sdssmath verbs */

#define	L_PHPROMPT	"astls> "   /* Command line prompt for ASTROTOOLS */

int main(int argc, char *argv[])
{
   SHMAIN_HANDLE	handle;
   /*Tcl_Interp	*interp;*/
   
   /*
    ** INITIALIZE TO BEGIN ACCEPTING TCL COMMANDS FROM TERMINAL AND FSAOIMAGE 
    */
   if (shMainTcl_Init(&handle, &argc, argv, L_PHPROMPT) != SH_SUCCESS)
     return 1;
   
   /* Saving a local copy of TCL interp - in case you need it */
   /*interp = handle.interp;*/
   
   /*
    ** DECLARE TCL COMMANDS
    */
   ftclCreateCommands(handle.interp);   /* Fermi enhancements to TCL   */
   ftclCreateHelp (handle.interp);	/* Define help command         */
   ftclMsgDefine (handle.interp);	/* Define help for tcl verbs   */

   /* 
     NOTE: shMainTcl_Declare is a comprehensive declaration routine for
     Dervish.  Included are TCL command declarations for region, peek, and
     FSAOimage.  If all commands are not desired, one can customize by
     replacing the call to this routine with individual calls to only the
     desired TCL command declaration routines. It runs the initialisation
     scripts, so must go after all of the initialisation routines have
     been called
     */
   shMainTcl_Declare(&handle);	/* Required Dervish TCL commands */ 

   /* --- PLACE YOUR FAVORITE INITIALIZATIONS HERE --- */
   /* declarations should be in order of dependence, i.e., dervish, sdssmath, astrotools */
   smaVerbsDeclare (handle.interp);	/* sdssmath verbs */
   atVerbsDeclare (handle.interp);	/* Verbs in this product */


   /*
    *             DO NOT PUT ANYTHING BELOW THIS LINE.
    *
    * We will now enter the main loop of Tk. There is no successfull return
    * from this loop. Anything added below this line will never be reached,
    * NEVER!!
    */
   shTk_MainLoop(&handle);

   return 0;

}
