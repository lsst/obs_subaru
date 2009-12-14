/*
 * TCL support for PHOTPAR type in photo.
 *
 * ENTRY POINT		SCOPE
 * tclPhotparDeclare	public	Declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 *
 * Michael Richmond, based on model of Heidi Newberg.  7/29/1993
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phPhotpar.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * return a handle to a new PHOTPAR
 */
static char *tclPhotparNew_use =
  "USAGE: photparNew";
static char *tclPhotparNew_hlp =
  "create a new PHOTPAR";

static int
tclPhotparNew(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];

   shErrStackClear();

   if(argc != 1) {
      Tcl_SetResult(interp,tclPhotparNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new PHOTPAR
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phPhotparNew();
   handle.type = shTypeGetFromName("PHOTPAR");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new photpar handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a PHOTPAR
 */
static char *tclPhotparDel_use =
  "USAGE: photparDel photpar";
static char *tclPhotparDel_hlp =
  "Delete a PHOTPAR";

static int
tclPhotparDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *photpar;
   char *opts = "photpar";

   shErrStackClear();

   ftclParseSave("photparDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      photpar = ftclGetStr("photpar");
      if(p_shTclHandleAddrGet(interp,photpar,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      ftclParseRestore("photparDel");
      Tcl_SetResult(interp,tclPhotparDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phPhotparDel(handle->ptr);
   (void) p_shTclHandleDel(interp,photpar);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclPhotparDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"photparNew",
                (Tcl_CmdProc *)tclPhotparNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclPhotparNew_hlp, tclPhotparNew_use);

   shTclDeclare(interp,"photparDel",
                (Tcl_CmdProc *)tclPhotparDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclPhotparDel_hlp, tclPhotparDel_use);
}

