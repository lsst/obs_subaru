/* TCL access to #define types.  Hopefully, later versions of DERVISH
 * will do this for us
 *
 * ENTRY POINT          SCOPE
 * tclDefineTranslateDeclare public  Declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *      FTCL            TCL + XTCL + Fermilab extensions
 *
 * Heidi Newberg
 */
#include <stdio.h>
#include <string.h>
#include "dervish.h"
#include "phFilter.h"

static char *module = "phTclStructsFacil";	/* name of this set of code */

/*****************************************************************************/
/*
* get the values of #define types
*/
static char *tclDefineTranslate_use =
  "USAGE: defineTranslate string";
static char *tclDefineTranslate_hlp =
  "return the value of a #define number";

static int
tclDefineTranslate(
	ClientData clientData,
	Tcl_Interp *interp,
	int argc,
	char **argv
	)
{
   char result[25];
   double translation;

   shErrStackClear();

   if (argc != 2) {
      Tcl_SetResult(interp, tclDefineTranslate_use, TCL_STATIC);
      return (TCL_ERROR);
   }

   if (!strcmp(argv[1],"FILTER_MAXNAME")) {
      translation = FILTER_MAXNAME;
   } else {
      Tcl_SetResult(interp, "No translation available", TCL_STATIC);
      return TCL_ERROR;
   }

   sprintf(result, "%f", translation);

   Tcl_SetResult(interp, result, TCL_VOLATILE);
   return (TCL_OK);

}

/******************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 *
 */

void
phTclDefineTranslateDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"defineTranslate",
                (Tcl_CmdProc *)tclDefineTranslate,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclDefineTranslate_hlp, tclDefineTranslate_use);
}
