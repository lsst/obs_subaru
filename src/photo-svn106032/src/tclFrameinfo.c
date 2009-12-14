/*
 * TCL support for FRAMEINFO type in photo.
 *
 * ENTRY POINT		SCOPE
 * tclFrameinfoDeclare	public	Declare all the verbs defined in this module
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
#include "phFrameinfo.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * return a handle to a new FRAMEINFO
 */
static char *tclFrameinfoNew_use =
  "USAGE: frameinfoNew";
static char *tclFrameinfoNew_hlp =
  "create a new FRAMEINFO";

static int
tclFrameinfoNew(
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
      Tcl_SetResult(interp,tclFrameinfoNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new FRAMEINFO
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phFrameinfoNew();
   handle.type = shTypeGetFromName("FRAMEINFO");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new frameinfo handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a FRAMEINFO
 */
static char *tclFrameinfoDel_use =
  "USAGE: frameinfoDel frameinfo";
static char *tclFrameinfoDel_hlp =
  "Delete a FRAMEINFO";

static int
tclFrameinfoDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *frameinfo;
   char *opts = "frameinfo";

   shErrStackClear();

   ftclParseSave("frameinfoDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      frameinfo = ftclGetStr("frameinfo");
      if(p_shTclHandleAddrGet(interp,frameinfo,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclFrameinfoDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phFrameinfoDel(handle->ptr);
   (void) p_shTclHandleDel(interp,frameinfo);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclFrameinfoDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"frameinfoNew",
                (Tcl_CmdProc *)tclFrameinfoNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFrameinfoNew_hlp, tclFrameinfoNew_use);

   shTclDeclare(interp,"frameinfoDel",
                (Tcl_CmdProc *)tclFrameinfoDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFrameinfoDel_hlp, tclFrameinfoDel_use);
}

