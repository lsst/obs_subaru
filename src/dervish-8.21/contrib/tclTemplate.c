#include <stdio.h>
#include "dervish.h"
#include "template.h"
  
static char *module = "contrib";	/* name of this set of code */

/*****************************************************************************/

static char *tclAdd_use =
  "USAGE: contrib_add #1 #2";
static char *tclAdd_hlp =
  "Add two numbers together\n";

static int
tclAdd(
       ClientData clientData,
       Tcl_Interp *interp,
       int argc,
       char **argv
       )
{
   char answer[40];			/* Please pray that 40 is sufficent */
   float x1,x2;
   
   if(argc != 3) {
      Tcl_SetResult(interp,tclAdd_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(Tcl_GetFloat(interp,argv[1],&x1) != TCL_OK ||
      Tcl_GetFloat(interp,argv[2],&x2) != TCL_OK) {
      return(TCL_ERROR);
   }
   
   sprintf(answer,"%g",shFloatAdd(x1,x2));
   Tcl_SetResult(interp,answer,TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
shTclTemplateDeclare(Tcl_Interp *interp) 
{
   shTclDeclare(interp,"contrib_add", 
		(Tcl_CmdProc *)tclAdd, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclAdd_hlp, tclAdd_use);
} 
