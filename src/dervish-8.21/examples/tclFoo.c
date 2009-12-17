/*
 * TCL support for dervish utility functions
 *
 * ENTRY POINT		SCOPE
 * tclFooDeclare	public	declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL	 	TCL + Fermi extension
 *
 * Robert Lupton
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "tcl.h"
#include "shCUtils.h"
#include "foo.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "shTclHandle.h"
#include "shTclParseArgv.h"
#include "shTclRegSupport.h"		/* needed for shTclDeclare prototype */

static char *module = "tsFoo";		/* name of this set of code */

/*****************************************************************************/
/*
 * return a handle to a new BAR
 */
static char *g_barNew = "barNew";
static ftclArgvInfo g_barNewTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Create a new BAR\n" },
   {"<nfoo>", FTCL_ARGV_INT, NULL, NULL,
    "Number of FOO's" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclBarNew(
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];
   int nfoo;
   int retStatus;

   /*
    * parse arguments
    */
   g_barNewTbl[1].dst = &nfoo;   
   if ((retStatus = shTclParseArgv(interp, &argc, argv, g_barNewTbl,
        FTCL_ARGV_NO_LEFTOVERS, g_barNew)) != FTCL_ARGV_SUCCESS) {
      return(retStatus);
   }

   /*
    * ok, get a handle for our new bar
    */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   
   handle.ptr = tsBarNew(nfoo);
   handle.type = shTypeGetFromName("BAR");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new bar handle",TCL_STATIC);
      return(TCL_ERROR);
   }
   
   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a BAR
 */
static char *g_barDel = "barDel";
static ftclArgvInfo g_barDelTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Delete a BAR\n" },
   {"<bar>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle to BAR" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclBarDel(
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   HANDLE *handle;
   char *bar;
   int retStatus;
   
   /*
    * parse arguments
    */
   g_barDelTbl[1].dst = &bar;   
   if ((retStatus = shTclParseArgv(interp, &argc, argv, g_barDelTbl,
        FTCL_ARGV_NO_LEFTOVERS, g_barDel)) != FTCL_ARGV_SUCCESS) {
      return(retStatus);
   }

   if(p_shTclHandleAddrGet(interp,bar,&handle) != TCL_OK) {
      return(TCL_ERROR);
   }

   if(handle->type != shTypeGetFromName("BAR")) {
      Tcl_AppendResult(interp,"handle ",bar," is a ",
                       shNameGetFromType(handle->type)," not a BAR",
                       (char *)NULL);
      return(TCL_ERROR);
   }

   tsBarDel(handle->ptr);

   (void) p_shTclHandleDel(interp,bar);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
tsTclFooDeclare(Tcl_Interp *interp) 
{
   int flags = FTCL_ARGV_NO_LEFTOVERS;

   shTclDeclare(interp,"barNew", 
		(Tcl_CmdProc *)tclBarNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, module,
                shTclGetArgInfo(interp, g_barNewTbl, flags, g_barNew),
                shTclGetUsage(interp, g_barNewTbl, flags, g_barNew));

   shTclDeclare(interp,"barDel", 
		(Tcl_CmdProc *)tclBarDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, module, 
                shTclGetArgInfo(interp, g_barDelTbl, flags, g_barDel),
                shTclGetUsage(interp, g_barDelTbl, flags, g_barDel));
} 
