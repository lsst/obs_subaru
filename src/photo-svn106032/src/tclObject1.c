/*
 * TCL support for OBJECT1 type in photo.
 *
 * ENTRY POINT		SCOPE
 * tclObject1Declare	public	Declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phObjc.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * return a handle to a new OBJECT1
 */
static char *tclObject1New_use =
  "USAGE: object1New";
static char *tclObject1New_hlp =
  "create a new OBJECT1";

static int
tclObject1New(
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
      Tcl_SetResult(interp,tclObject1New_use,TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new OBJECT1
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phObject1New();
   handle.type = shTypeGetFromName("OBJECT1");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new object1 handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a OBJECT1
 */
static char *tclObject1Del_use =
  "USAGE: object1Del object1";
static char *tclObject1Del_hlp =
  "Delete a OBJECT1";

static int
tclObject1Del(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *obj1;
   char *opts = "obj1";

   shErrStackClear();

   ftclParseSave("object1Del");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      obj1 = ftclGetStr("obj1");
      if(p_shTclHandleAddrGet(interp,obj1,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclObject1Del_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phObject1Del(handle->ptr);
   (void) p_shTclHandleDel(interp,obj1);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclObject1Declare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"object1New",
                (Tcl_CmdProc *)tclObject1New,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclObject1New_hlp, tclObject1New_use);

   shTclDeclare(interp,"object1Del",
                (Tcl_CmdProc *)tclObject1Del,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclObject1Del_hlp, tclObject1Del_use);
}




