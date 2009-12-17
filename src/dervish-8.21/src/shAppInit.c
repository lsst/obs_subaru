/* shAppInit.c --
 *	this version is originated from tkXAppInit.c of tclX7.3a
 *
 *	11/15/1994 -- Chih-Hao Huang
 *	Add warning if DERVISH_STARTUP is not properly set
 */

/* 
 * tkXAppInit.c --
 *
 *      Provides a default version of the Tcl_AppInit procedure for use with
 *      applications built with Extended Tcl and Tk.  This is based on the
 *      the UCB Tk file tkAppInit.c
 *
 *-----------------------------------------------------------------------------
 * Copyright 1991-1993 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: shAppInit.c,v 1.4 1994/11/15 17:02:34 huangch Exp $
 *-----------------------------------------------------------------------------
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#ifndef lint
static char rcsid[] = "$Header: /p/IRIX/sdssrcvs/v1_2/src/dervish/src/shAppInit.c,v 1.4 1994/11/15 17:02:34 huangch Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdlib.h>		/* prototype of getenv() */
#include <string.h>

#include "tclExtend.h"
#include "tk.h"
#include <math.h>

extern int TkX_Init (Tcl_Interp *interp);	/* prototype */

/*
 * The following variable is a special hack that allows applications
 * to be linked using the procedure "main" from the Tk library.  The
 * variable generates a reference to "main", which causes main to
 * be brought in from the library (and all of Tk and Tcl with it).

 * PCG: Dervish does not need this special hack since it has it's own
 *      main (in ../examples/dervish_templates.c for examples)
 *      So we comment this definition out.
EXTERN int main _ANSI_ARGS_((int     argc,
                             char  **argv));
int (*tclXDummyMainPtr)() = (int (*)()) main;
*/

/*
 * The following variable is a special hack that insures the tcl
 * version of matherr() is used when linking against shared libraries
 * Only define if matherr is used on this system.
 */

#if defined(DOMAIN) && defined(SING)
EXTERN int matherr _ANSI_ARGS_((struct exception *));
int (*tclDummyMathPtr)() = (int (*)()) matherr;
#endif

/*
 *  The following variable is defined in tkMain.c (ou of the tk product)
 *  Not defining it here would imply to link with tkMain.o and thus
 *  you would get a main function (and that is a problem if you already
 *  have one!)
 */
/*extern char* tcl_RcFileName = NULL;*/

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *	This procedure performs application-specific initialization.
 *	Most applications, especially those that incorporate additional
 *	packages, will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in interp->result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

extern int useTk;

int
Tcl_AppInit(Tcl_Interp *interp)
{

    Tk_Window main_win;

    main_win = Tk_MainWindow(interp);


    /*
     * Call the init procedures for included packages.  Each call should
     * look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module.
     */

    if (TclX_Init(interp) == TCL_ERROR) {
	fprintf(stderr, "Warning: %s\n", interp->result);
    }

    if ((main_win != NULL) && (useTk))
    {
	if (TkX_Init(interp) == TCL_ERROR)
	{
		fprintf(stderr, "Warning: %s\n", interp->result);
	}
	else if (Tk_Init(interp) == TCL_ERROR)
	{
		fprintf(stderr, "Warning: %s\n", interp->result);
	}
    }

    /*
     * Call Tcl_CreateCommand for application-specific commands, if
     * they weren't already created by the init procedures called above.
     */

    /*
     * Specify a user-specific startup file to invoke if the application
     * is run interactively.  Typically the startup file is "~/.apprc"
     * where "app" is the name of the application.  If this line is deleted
     * then no user-specific startup file will be run under any conditions.
     */


{
        char *script;
	char *this;
	char *end;
	char *inres;
        char cmd[132];
	int stop, strsize;

	script = (char *)getenv("DERVISH_STARTUP");
	if (script != (char *) NULL) {
	  sprintf(cmd, "source %s",script);
	  printf("Executing commands in %s: ",script);
	  Tcl_Eval (interp, cmd);
	  printf("%s\n", interp->result);
	}
	else
	{
		fprintf(stderr, "Warning: DERVISH_STARTUP was not set");
		fprintf(stderr, " and the standard startup script was");
		fprintf(stderr, " not\n         executed. Be prepared");
		fprintf(stderr, " to see different behaviors.\n");
	}

	script = (char *)getenv("DERVISH_USER");
	if (script != (char *) NULL) {
	  this = script;
	  end = script;
	  stop = 0;
	  do {
	    for (end=this; *end != ':' && *end != '\0'; end++);
	    if (*end == '\0') {
	      stop = 1;
	    } else {
	      *end = '\0';
	    }
	    if (end != this) {
	      sprintf(cmd, "source %s",this);
	      printf("Executing commands in %s: ",this);
	      Tcl_Eval (interp, cmd);
	      printf("%s\n", interp->result);
	      this = end+1;
	    }
	  } while (!stop);
	}
	
}

    tcl_RcFileName = "~/.tclrc";
    return TCL_OK;
}






