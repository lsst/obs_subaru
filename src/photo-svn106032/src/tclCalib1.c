/*
 * TCL support for CALIB1 type in photo.
 *
 * ENTRY POINT		SCOPE
 * tclCalib1Declare	public	Declare all the verbs defined in this module
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
#include "phCalib1.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * return a handle to a new CALIB1
 */
static char *tclCalib1New_use =
  "USAGE: calib1New <filtername>";
#define tclCalib1New_hlp \
  "Make a new CALIB1"

static ftclArgvInfo calib1New_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCalib1New_hlp},
   {"<filtername>", FTCL_ARGV_STRING, NULL, NULL,
					    "Single-character name of filter"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define calib1New_name "calib1New"

static int
tclCalib1New(ClientData clientData,
	     Tcl_Interp *interp,
	     int ac,
	     char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   char *filternameStr = NULL;		/* Single-character name of filter */
   CALIB1 *calib1;

   shErrStackClear();

   i = 1;
   calib1New_opts[i++].dst = &filternameStr;
   shAssert(calib1New_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, calib1New_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     calib1New_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(strlen(filternameStr) != 1) {
      Tcl_AppendResult(interp, filternameStr, " has more than one character",
		       (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   calib1 = phCalib1New(*filternameStr);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = calib1;
   hand.type = shTypeGetFromName("CALIB1");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"calib1\" to new CALIB1 handle",
		    TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}



/*****************************************************************************/
/*
 * Delete a CALIB1
 */
static char *tclCalib1Del_use =
  "USAGE: calib1Del calib1";
static char *tclCalib1Del_hlp =
  "Delete a CALIB1";

static int
tclCalib1Del(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *calib1;
   char *opts = "calib1";

   shErrStackClear();

   ftclParseSave("calib1Del");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      calib1 = ftclGetStr("calib1");
      if(p_shTclHandleAddrGet(interp,calib1,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclCalib1Del_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phCalib1Del(handle->ptr);
   (void) p_shTclHandleDel(interp,calib1);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}
/*****************************************************************************/
/*
 * return a handle to a new CALIBIO
 */
static char *tclCalibIoNew_use =
  "USAGE: calibIoNew ncolors";
static char *tclCalibIoNew_hlp =
  "create a new CALIBIO";

static int
tclCalibIoNew(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];
   int ncolors;

   shErrStackClear();

   if(argc != 2) {
      Tcl_SetResult(interp,tclCalibIoNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if (Tcl_GetInt(interp, argv[1], &ncolors)!=TCL_OK) return (TCL_ERROR);
/*
 * ok, get a handle for our new CALIBIO
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phCalibIoNew(ncolors);
   handle.type = shTypeGetFromName("CALIB_IO");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new CalibIo handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}
/*****************************************************************************/
/*
 * Delete an CALIBIO
 */
static char *tclCalibIoDel_use =
  "USAGE: calibIoDel calibIo ";
static char *tclCalibIoDel_hlp =
  "Delete an CALIBIO";

static int
tclCalibIoDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *calibIo;
   char *opts = "calibIo";

   shErrStackClear();

   ftclParseSave("calibIoDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      calibIo = ftclGetStr("calibIo");
      if(p_shTclHandleAddrGet(interp,calibIo,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclCalibIoDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phCalibIoDel(handle->ptr);
   (void) p_shTclHandleDel(interp,calibIo);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}



/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclCalib1Declare(Tcl_Interp *interp)
{
   shTclDeclare(interp,calib1New_name,
		(Tcl_CmdProc *)tclCalib1New, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCalib1New_hlp,
		tclCalib1New_use);

   shTclDeclare(interp,"calib1Del",
                (Tcl_CmdProc *)tclCalib1Del,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCalib1Del_hlp, tclCalib1Del_use);

   shTclDeclare(interp,"calibIoNew",
                (Tcl_CmdProc *)tclCalibIoNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCalibIoNew_hlp, tclCalibIoNew_use);

   shTclDeclare(interp,"calibIoDel",
                (Tcl_CmdProc *)tclCalibIoDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCalibIoDel_hlp, tclCalibIoDel_use);
}

