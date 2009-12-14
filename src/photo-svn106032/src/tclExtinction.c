/*
 * TCL support for EXTINCTION type in photo.
 *
 * ENTRY POINT		SCOPE
 * tclExtinctionDeclare	public	Declare all the verbs defined in this module
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
#include "phUtils.h"
#include "phExtinction.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * return a handle to a new EXTINCTION
 */
static char *tclExtinctionNew_use =
  "USAGE: extinctionNew ncolors";
static char *tclExtinctionNew_hlp =
  "create a new EXTINCTION";

static int
tclExtinctionNew(
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
      Tcl_SetResult(interp,tclExtinctionNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if (Tcl_GetInt(interp, argv[1], &ncolors)!=TCL_OK) return (TCL_ERROR);
/*
 * ok, get a handle for our new EXTINCTION
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phExtinctionNew(ncolors);
   handle.type = shTypeGetFromName("EXTINCTION");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new extinction handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete an EXTINCTION 
 */
static char *tclExtinctionDel_use =
  "USAGE: extinctionDel extinction ";
static char *tclExtinctionDel_hlp =
  "Delete an EXTINCTION";

static int
tclExtinctionDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *extinction;
   char *opts = "extinction";

   shErrStackClear();

   ftclParseSave("extinctionDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      extinction = ftclGetStr("extinction");
      if(p_shTclHandleAddrGet(interp,extinction,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclExtinctionDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phExtinctionDel(handle->ptr);
   (void) p_shTclHandleDel(interp,extinction);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}



/**************************************************************************
 * given a handle to an EXTINCTION structure, set the 'color'th MAG
 * sub-structure's fields to the given filtername, mag, and magErr.
 */

static char *extMagExtstring = NULL;
static int extMagIndex = 0;
static char *extMagFilter = NULL;
static double extMagMagval = 0;
static double extMagMagerrval = 0;

static char *tclExtinctionSetMag_use =
  "USAGE: extinctionSetMag extinction magindex filter mag magerr";
#define tclExtinctionSetMag_hlp "Set the filtername, mag, and magErr fields of the 'magindex'th MAG structure in the given EXTINCTION structure."

static ftclArgvInfo extinctionMagSet_opts[] = {
   { NULL, FTCL_ARGV_HELP, NULL, NULL, tclExtinctionSetMag_hlp },
   { "<extinction>", FTCL_ARGV_STRING, NULL, &extMagExtstring, 
	 "Set magnitudes inside this EXTINCTION struct" },
   { "<magindex>", FTCL_ARGV_INT, NULL, &extMagIndex, 
         "set this element of MAG array" },
   { "<filter>", FTCL_ARGV_STRING, NULL, &extMagFilter, 
         "set the passBand field of the appropriate MAG to this string" },
   { "<mag>", FTCL_ARGV_DOUBLE, NULL, &extMagMagval, 
         "set the mag field of the appropriate MAG to this value" },
   { "<magerr>", FTCL_ARGV_DOUBLE, NULL, &extMagMagerrval,
         "set the magErr field of the appropriate MAG to this value" },
   { NULL, FTCL_ARGV_END, NULL, NULL, NULL }
};

static int
tclExtinctionSetMag(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   int magindex;
   char filtername[FILTER_MAXNAME];
   float mag, magerr;
   EXTINCTION *ext_ptr;
   MAG *mag_ptr;
   HANDLE *extHandle;

   shErrStackClear();

   if (get_FtclOpts(interp, &argc, argv, extinctionMagSet_opts) != TCL_OK) {
      return(TCL_ERROR);
   }

   if (p_shTclHandleAddrGet(interp, extMagExtstring, &extHandle) != TCL_OK) {
      return(TCL_ERROR);
   }
   if (extHandle->type != shTypeGetFromName("EXTINCTION")) {
      Tcl_SetResult(interp, "first arg is not an EXTINCTION", TCL_STATIC);
      return(TCL_ERROR);
   }
   ext_ptr = (EXTINCTION *) extHandle->ptr;
   magindex = extMagIndex;
   strncpy(filtername, extMagFilter, FILTER_MAXNAME - 1);
   filtername[FILTER_MAXNAME - 1] = '\0';
   mag = (float) extMagMagval;
   magerr = (float) extMagMagerrval;

   shAssert(ext_ptr->k != NULL);
   shAssert((magindex >= 0) && (magindex < ext_ptr->ncolors));
   mag_ptr = ext_ptr->k[magindex];

   /* copy the filter name, and make sure we terminate with a NULL char */
   strncpy(mag_ptr->passBand, filtername, FILTER_MAXNAME - 1);
   mag_ptr->mag = mag;
   mag_ptr->magErr = magerr;

   return(TCL_OK);
}




/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclExtinctionDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"extinctionNew",
                (Tcl_CmdProc *)tclExtinctionNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclExtinctionNew_hlp, tclExtinctionNew_use);

   shTclDeclare(interp,"extinctionDel",
                (Tcl_CmdProc *)tclExtinctionDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclExtinctionDel_hlp, tclExtinctionDel_use);

   shTclDeclare(interp,"extinctionSetMag",
                (Tcl_CmdProc *)tclExtinctionSetMag,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclExtinctionSetMag_hlp, tclExtinctionSetMag_use);
}

