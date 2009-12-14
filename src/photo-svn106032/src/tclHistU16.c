/*
 * TCL support for PHHIST type in photo.
 *
 * ENTRY POINT		SCOPE
 * tclHistDeclare   	public	Declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 *
 * Michael Richmond
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phUtils.h"
#include "phHist.h"

static char *module = "phTclHist";	/* name of this set of code */

/**************************************************************************
 * return a handle to a new PHHIST
 */

static int num_bin = -1;
static int min_bin = -1;

   /* this is somewhat redundant, but required for shTclDeclare */
static char *tclHistNew_use = 
   "USAGE: phHistNew numBin minBin";
static char *tclHistNew_hlp =
  "create a new PHHIST structure, with given number of bins and minimum bin";

static ftclArgvInfo phHistNew_argTable[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
         "create a new PHHIST structure, with given number of bins \nand minimum bin specified by the user. \n" },
   { "<numBin>", FTCL_ARGV_INT, NULL, &num_bin,
         "number of bins in histogram" },
   { "<minBin>", FTCL_ARGV_INT, NULL, &min_bin,
         "value of first bin in histogram" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};


static int
tclHistNew(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];
   PHHIST *hist;

   shErrStackClear();

   if(get_FtclOpts(interp, &argc, argv, phHistNew_argTable) != TCL_OK) {
      return(TCL_ERROR);
   }

   if ((hist = phPhhistNew(num_bin, (U16) min_bin)) == NULL) {
      Tcl_SetResult(interp, "tclHistNew: phHistu16New returns NULL",
	       TCL_STATIC);
      return(TCL_ERROR);
   }

   /* create a new handle and bind it to the new histogram structure */
   if (p_shTclHandleNew(interp, name) != TCL_OK) {
      phPhhistDel(hist);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   handle.ptr = hist;
   handle.type = shTypeGetFromName("PHHIST");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp, "tclHistu16New: Can't bind to new PHHIST handle",
	       TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/************************************************************************
 * Delete a PHHIST 
 */

static char *hist_name = NULL;

   /* again, this is redundant, but required by shTclDeclare */
static char *tclHistDel_use =
  "USAGE: phHistDel hist ";
static char *tclHistDel_hlp =
  "Delete a PHHIST structure";

static ftclArgvInfo phHistDel_argTable[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
         "delete the given PHHIST structure \n" },
   { "<hist>", FTCL_ARGV_STRING, NULL, &hist_name,
         "handle containing PHHIST structure to be deleted" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};


static int
tclHistDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;

   shErrStackClear();

   if(get_FtclOpts(interp, &argc, argv, phHistDel_argTable) != TCL_OK) {
      return(TCL_ERROR);
   }

   if (p_shTclHandleAddrGet(interp, hist_name, &handle) != TCL_OK) {
      return(TCL_ERROR);
   }

   /* make sure this is a PHHIST */
   if (handle->type != shTypeGetFromName("PHHIST")) {
      Tcl_SetResult(interp, "given handle doesn't point to a PHHIST",
	       TCL_STATIC);
      return(TCL_ERROR);
   }

   phPhhistDel(handle->ptr);
   (void) p_shTclHandleDel(interp, hist_name);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/**************************************************************************
 * Declare my new tcl verbs to tcl
 */

void
phTclHistDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"phHistNew",
                (Tcl_CmdProc *)tclHistNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclHistNew_hlp, tclHistNew_use);

   shTclDeclare(interp,"phHistDel",
                (Tcl_CmdProc *)tclHistDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclHistDel_hlp, tclHistDel_use);
}

