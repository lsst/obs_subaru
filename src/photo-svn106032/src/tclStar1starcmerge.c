/*
 * TCL support for STAR1STARCMERGE type in photo.
 *
 * ENTRY POINT		SCOPE
 * tclStar1starcmergeDeclare	public	Declare all the verbs defined in this module
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
#include "phStar1starcmerge.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * return a handle to a new STAR1STARCMERGE
 */
static char *tclStar1starcmergeNew_use =
  "USAGE: star1starcmergeNew";
static char *tclStar1starcmergeNew_hlp =
  "create a new STAR1STARCMERGE";

static int
tclStar1starcmergeNew(
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
      Tcl_SetResult(interp,tclStar1starcmergeNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new STAR1STARCMERGE
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phStar1starcmergeNew();
   handle.type = shTypeGetFromName("STAR1STARCMERGE");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new star1starcmerge handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a STAR1STARCMERGE
 */
static char *tclStar1starcmergeDel_use =
  "USAGE: star1starcmergeDel star1starcmerge";
static char *tclStar1starcmergeDel_hlp =
  "Delete a STAR1STARCMERGE";

static int
tclStar1starcmergeDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *star1starcmerge;
   char *opts = "star1starcmerge";

   shErrStackClear();

   ftclParseSave("star1starcmergeDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      star1starcmerge = ftclGetStr("star1starcmerge");
      if(p_shTclHandleAddrGet(interp,star1starcmerge,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclStar1starcmergeDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phStar1starcmergeDel(handle->ptr);
   (void) p_shTclHandleDel(interp,star1starcmerge);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclStar1starcmergeDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"star1starcmergeNew",
                (Tcl_CmdProc *)tclStar1starcmergeNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStar1starcmergeNew_hlp, tclStar1starcmergeNew_use);

   shTclDeclare(interp,"star1starcmergeDel",
                (Tcl_CmdProc *)tclStar1starcmergeDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStar1starcmergeDel_hlp, tclStar1starcmergeDel_use);
}

