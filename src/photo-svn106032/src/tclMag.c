/*
 * TCL support for MAG type in photo.
 *
 * ENTRY POINT		SCOPE
 * tclMagDeclare	public	Declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 *
 * Heidi Newberg
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phMag.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * return a handle to a new MAG
 */
static char *tclMagNew_use =
  "USAGE: magNew";
static char *tclMagNew_hlp =
  "create a new MAG";

static int
tclMagNew(
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
      Tcl_SetResult(interp,tclMagNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new MAG
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phMagNew();
   handle.type = shTypeGetFromName("MAG");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new mag handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a MAG
 */
static char *tclMagDel_use =
  "USAGE: magDel mag";
static char *tclMagDel_hlp =
  "Delete a MAG";

static int
tclMagDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *mag;
   char *opts = "mag";

   shErrStackClear();

   ftclParseSave("magDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      mag = ftclGetStr("mag");
      if(p_shTclHandleAddrGet(interp,mag,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclMagDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phMagDel(handle->ptr);
   (void) p_shTclHandleDel(interp,mag);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclMagDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"magNew",
                (Tcl_CmdProc *)tclMagNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclMagNew_hlp, tclMagNew_use);

   shTclDeclare(interp,"magDel",
                (Tcl_CmdProc *)tclMagDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclMagDel_hlp, tclMagDel_use);
}

