/*****************************************************************************
 ******************************************************************************
 **
 ** FILE:
 **	dervish_template.c
 **
 ** ABSTRACT:
 **	This file contains a template for users writing TCL main programs which
 **	utilize the command line editing and FSAOimage interface features of
 **	Dervish.
 **
 **	The routines invoked by this main program were written to encapsulate
 **	the lower layers of the Dervish, in order to minimize chances of the
 **	template needing changes in the future.
 **
 ** ENVIRONMENT:
 **	ANSI C.
 **
 ******************************************************************************
 ******************************************************************************
 */
#include <stdlib.h>
#include <stdio.h>
#include <termio.h>
#include <sys/types.h>
#include "dervish.h"

#define	L_NUMINTEXIT	1	    /* Number of Ctrl C or Ctrl Z interrupts
				       before main program exits */
#define	L_SHPROMPT	"MYVERB> "   /* Command line prompt for Dervish */

void privateTclDeclare(Tcl_Interp *interp);

int main()
{
   SHMAIN_HANDLE	sh_handle;
   Tcl_Interp	*interp;
   
   /*
    ** INITIALIZE TO BEGIN ACCEPTING TCL COMMANDS FROM TERMINAL AND FSAOIMAGE 
    */
   if (shMainTcl_Init(&sh_handle, L_NUMINTEXIT) != SH_SUCCESS)
     {exit(1);}
   
   /* Saving a local copy of TCL interp - in case you need it */
   interp = sh_handle.interp;
   
   /*
    ** DECLARE TCL COMMANDS
    */

   shMainTcl_Declare(&sh_handle);	/* Required Dervish TCL commands */
   ftclCreateCommands(interp);          /* Fermi enhancements to TCL   */
   ftclCreateHelp (interp);		/* Define help command         */
   ftclMsgDefine (interp);		/* Define help for tcl verbs   */
   /* 
     NOTE: shMainTcl_Declare is a comprehensive declaration routine for
     Dervish.  Included are TCL command declarations for region, peek, and
     FSAOimage.  If all commands are not desired, one can customize by
     replacing the call to this routine with individual calls to only the
     desired TCL command declaration routines.
     */
   
   /* --- PLACE YOUR FAVORITE TCL DECLARATIONS HERE --- */
   
   shTclPgplotDeclare(interp);         /* Pgplot Tcl commands */
   privateTclDeclare(interp);

   /*
    ** OTHER INITIALIZATION 
    */
      
   /* --- PLACE YOUR FAVORITE INITIALIZATIONS HERE --- */
   ftclParseInit(interp);		/* Ftcl command parsing */
 
   /*
    ** LOOP TO RETRIEVE AND PROCESS TCL COMMANDS
    */
   while(1)
     {
	/*
	 ** GO GET A COMPLETE TCL COMMAND FROM TERMINAL OR FSAOIMAGE
	 */
	if (shMainTcl_Get(&sh_handle, L_SHPROMPT) != SH_SUCCESS)
	  {break;}  /* Get out and clean-up if not successful */
	
	/*
	 ** PROCESS THE TCL COMMAND
	 */
	if (shMainTcl_Eval(&sh_handle) == 0)
	  {break;}  /* Get out and clean-up if "exit" or "quit" were entered */
     }
   
   /*
    ** CLEAN-UP BEFORE EXITING
    */
   shMainTcl_Clean(&sh_handle);
   exit(0);
}
 
