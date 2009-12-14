#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "phMeschach.h"
#include "dervish.h"

static char *tclMeschachInit_use =
  "USAGE: meschachInit";
#define tclMeschachInit_hlp \
  "Initialise meschach data structures"

static ftclArgvInfo meschachInit_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMeschachInit_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define meschachInit_name "meschachInit"

static int
tclMeschachInit(ClientData clientData,
		Tcl_Interp *interp,
		int ac,
		char **av)
{
   int a_i;

   shErrStackClear();

   a_i = 1;
   shAssert(meschachInit_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, meschachInit_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     meschachInit_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phMeschachInit();

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclMeschachFini_use =
  "USAGE: meschachFini";
#define tclMeschachFini_hlp \
  "Clean up meschach data structures (and free its freelists)"

static ftclArgvInfo meschachFini_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMeschachFini_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define meschachFini_name "meschachFini"

static int
tclMeschachFini(ClientData clientData,
		Tcl_Interp *interp,
		int ac,
		char **av)
{
   int a_i;

   shErrStackClear();

   a_i = 1;
   shAssert(meschachFini_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, meschachFini_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     meschachFini_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phMeschachFini();

   return(TCL_OK);
}


/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclMeschachDeclare(Tcl_Interp *interp)
{
   char *module = "phTclMeschach";	/* name of this set of code */

   shTclDeclare(interp,meschachInit_name,
		(Tcl_CmdProc *)tclMeschachInit, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMeschachInit_hlp,
		tclMeschachInit_use);

   shTclDeclare(interp,meschachFini_name,
		(Tcl_CmdProc *)tclMeschachFini, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMeschachFini_hlp,
		tclMeschachFini_use);
}
