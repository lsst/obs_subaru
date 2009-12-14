/*
 * TCL support for CCDPARS type in photo.
 *
 * ENTRY POINT		SCOPE
 * tclCcdparsDeclare	public	Declare all the verbs defined in this module
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
#include "phCcdpars.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * return a handle to a new CCDDEFECT
 */
static char *tclCcddefectNew_use =
  "USAGE: ccddefectNew";
static char *tclCcddefectNew_hlp =
  "create a new CCDDEFECT";

static int
tclCcddefectNew(
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
      Tcl_SetResult(interp,tclCcddefectNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new CCDDEFECT
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phCcddefectNew();
   handle.type = shTypeGetFromName("CCDDEFECT");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new ccddefect handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a CCDDEFECT
 */
static char *tclCcddefectDel_use =
  "USAGE: ccddefectDel ccddefect";
static char *tclCcddefectDel_hlp =
  "Delete a CCDDEFECT";

static int
tclCcddefectDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *ccddefect;
   char *opts = "ccddefect";

   shErrStackClear();

   ftclParseSave("ccddefectDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      ccddefect = ftclGetStr("ccddefect");
      if(p_shTclHandleAddrGet(interp,ccddefect,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclCcddefectDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phCcddefectDel(handle->ptr);
   (void) p_shTclHandleDel(interp,ccddefect);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * return a handle to a new CCDPARS
 */
static char *tclCcdparsNew_use =
  "USAGE: ccdparsNew";
static char *tclCcdparsNew_hlp =
  "create a new CCDPARS";

static int
tclCcdparsNew(
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
      Tcl_SetResult(interp,tclCcdparsNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new CCDPARS
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phCcdparsNew();
   handle.type = shTypeGetFromName("CCDPARS");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new ccdpars handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/* 
 * Delete a CCDPARS
 */
static char *tclCcdparsDel_use =
  "USAGE: ccdparsDel ccdpars";
static char *tclCcdparsDel_hlp =
  "Delete a CCDPARS";

static int
tclCcdparsDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *ccdpars;
   char *opts = "ccdpars";

   shErrStackClear();

   ftclParseSave("ccdparsDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      ccdpars = ftclGetStr("ccdpars");
      if(p_shTclHandleAddrGet(interp,ccdpars,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclCcdparsDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phCcdparsDel(handle->ptr);
   (void) p_shTclHandleDel(interp,ccdpars);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

#if 0
/***************************************************************************
 * Given a chip (row, col) position, go into the CCD parameters file and
 * find all the relevant chip parameters.  Create a new CCDPARS structure
 * and return a handle to it.
 * 
 * Note that the user MUST call 'inipar' to set up the CCD parameters file
 * before calling this verb!
 * 
 * Input:  row, col of chip in focal plane
 * Output: a handle to a new CCDPARS structure
 *
 */

static char *tclCcdparsFetchById_use =
  "USAGE: ccdparsFetchById row col";
static char *tclCcdparsFetchById_hlp =
  "create a new CCDPARS, containing info specific to the CCD at the given position in the camera focal plane.  The user must call 'inipar' with the CCD parameter file as an argument before calling this function.";

static int
tclCcdparsFetchById(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   int row, col;
   char name[HANDLE_NAMELEN];
   char *opts = "row col";
   HANDLE handle;
   CCDPARS *ccd;

   shErrStackClear();

   ftclParseSave("ccdparsFetchById");
   if (ftclFullParseArg(opts, argc, argv) != 0) {
      row = ftclGetInt("row");
      col = ftclGetInt("col");
   }
   else {
      ftclParseRestore("ccdparsFetchById");
      Tcl_SetResult(interp, tclCcdparsFetchById_use, TCL_STATIC);
      return(TCL_ERROR);
   }

   /* try to look up and create the new CCDPARS structure */
   if ((ccd = phCcdparsFetchById(row, col)) == NULL) {
      Tcl_SetResult(interp, "tclCcdparsFetchById: phCcdparsFetchById fails",
	       TCL_STATIC);
      return(TCL_ERROR);
   }

   /* okay, assign a handle to this structure */
   if (p_shTclHandleNew(interp, name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   handle.ptr = ccd;
   handle.type = shTypeGetFromName("CCDPARS");
   if (p_shTclHandleAddrBind(interp, handle, name) != TCL_OK) {
      Tcl_SetResult(interp, "can't bind handle to new CCDPARS structure",
	    TCL_STATIC);
      return(TCL_ERROR);
   }

   /* all went well. */
   Tcl_SetResult(interp, name, TCL_VOLATILE);
   return(TCL_OK);
}

#endif

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclCcdparsDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"ccddefectNew",
                (Tcl_CmdProc *)tclCcddefectNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCcddefectNew_hlp, tclCcddefectNew_use);

   shTclDeclare(interp,"ccddefectDel",
                (Tcl_CmdProc *)tclCcddefectDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCcddefectDel_hlp, tclCcddefectDel_use);
   shTclDeclare(interp,"ccdparsNew",
                (Tcl_CmdProc *)tclCcdparsNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCcdparsNew_hlp, tclCcdparsNew_use);

   shTclDeclare(interp,"ccdparsDel",
                (Tcl_CmdProc *)tclCcdparsDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCcdparsDel_hlp, tclCcdparsDel_use);

#if 0
   shTclDeclare(interp,"ccdparsFetchById",
                (Tcl_CmdProc *)tclCcdparsFetchById,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCcdparsFetchById_hlp, tclCcdparsFetchById_use);
#endif
}

