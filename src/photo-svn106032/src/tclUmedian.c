/****************************************************************************
**
**<AUTO EXTRACT>
** TCL VERB: regMeanFitByCol
**
** DESCRIPTION:
**
** Estimate the median in a column of a region in which the 
** pixel values are severely undersampled. Do this by fitting
** to a Gaussian distribution.
**
**</AUTO>
*/

#include <stdlib.h>
#include <string.h>
#include "dervish.h"
#include "phUtils.h"
#include "phHist.h"
#include "phUmedian.h"


/**********************************************************************
**<AUTO EXTRACT>
** TCL VERB: initGaussFit
**
** Description:
**
** Call the "phInitGaussFit" function.  Must be done before any 
** PSF-fitting functions are called.
**
**</AUTO>
**********************************************************************/

static char *tclInitGaussFit_use =
  "USAGE: initGaussFit";
static char *tclInitGaussFit_hlp =
  "Initialize information used in GAUSSFIT-fitting";
static int
tclInitGaussFit(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   char *opts = "";

   shErrStackClear();

   ftclParseSave("initGaussFit");
   if(ftclFullParseArg(opts,argc,argv) == 0) {
      ftclParseRestore("initGaussFit");
      Tcl_SetResult(interp,tclInitGaussFit_use,TCL_STATIC);
      return(TCL_ERROR);
   }
   if (phInitGaussFit() != SH_SUCCESS) {      Tcl_AppendResult(interp, 
	    "phInitGaussFit returned with error", (char *)NULL);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/**********************************************************************
**<AUTO EXTRACT>
** TCL VERB: finiGaussFit
**
** Description:
**
** Call the "phFiniGaussFit" function.  Must be done after the pipeline
** is finished with computations, and about to terminate.
**
**</AUTO>
**********************************************************************/

static char *tclFiniGaussFit_use =
  "USAGE: finiGaussFit";
static char *tclFiniGaussFit_hlp =
  "De-allocate resources used in GAUSSFIT-fitting";

static int
tclFiniGaussFit(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   char *opts = "";

   shErrStackClear();

   ftclParseSave("finiGaussFit");
   if(ftclFullParseArg(opts,argc,argv) == 0) {
      ftclParseRestore("finiGaussFit");
      Tcl_SetResult(interp,tclFiniGaussFit_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if (phFiniGaussFit() != SH_SUCCESS) {
      Tcl_AppendResult(interp, 
	    "phFiniGaussFit returned with error", (char *)NULL);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*************************************************************************
 * given a PHHIST structure (which must already have been filled with
 * data values), estimate the quartiles by fitting a gaussian to the
 * central peak. Ignores bins in the histogram past where the values
 * first fall to zero around the peak.
 *
 * Print out the quartiles, mean, mode and sigma in a keyed list. */

static char *histstr = NULL;


static char *tclHistU16StatsFit_use =
  "USAGE: histU16StatsFit hist";
static char *tclHistU16StatsFit_hlp =
  "given a PHHIST structure already filled with data estimate the quartiles and sigma by fitting a gaussian to the central peak. Return as keyes list.";

static ftclArgvInfo histU16StatsFit_argTable[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
         "given a PHHIST structure already filled with data estimate the quartiles and sigma by fitting a gaussian to the central peak. Return as keyes list." },
   { "<hist>", FTCL_ARGV_STRING, NULL, &histstr,
         "histogram for which to estimate statistics" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};


static int
tclHistU16StatsFit(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *histHandle;
   PHHIST *hist;
   char err_msg[100];
   int error;
   float array[3], sigma;

   shErrStackClear();

   if(get_FtclOpts(interp, &argc, argv, histU16StatsFit_argTable) != TCL_OK) {
      return(TCL_ERROR);
   }

   if (!phGaussFitIsInitialised()) {
      Tcl_SetResult(interp, "tclHistU16StatsFit: you must initialize with initGaussFit", 
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   /* get the input PHHIST structure */
   if (p_shTclHandleAddrGet(interp, histstr, &histHandle) != TCL_OK) {
      return(TCL_ERROR);
   }
   if (histHandle->type != shTypeGetFromName("PHHIST")) {
      Tcl_SetResult(interp, "tclHistU16StatsFit: first arg is not a PHHIST ",
            TCL_STATIC);
      return(TCL_ERROR);
   }
   hist = histHandle->ptr;

   /* call the phHistU16StatsFit function to calculate the Ntiles */
   if (phHistU16StatsFit(hist, array, &sigma,&error) != 
	    SH_SUCCESS) {
      Tcl_SetResult(interp, 
	    "tclHistU16StatsFit: phHistU16StatsFit returns with error",
	    TCL_STATIC);
      return(TCL_ERROR);
   }

   /* 
    * now put the output -- quartiles and sigma --
    * into a keyed list 
    */
   sprintf(err_msg, "quartiles \"%.4f %.4f %.4f\"", 
	       array[0], array[1], array[2]);
   Tcl_AppendElement(interp, err_msg);
   sprintf(err_msg, "sigma \"%.4f\"", sigma);
   Tcl_AppendElement(interp, err_msg);
   sprintf(err_msg, "error \"%d\"", error);
   Tcl_AppendElement(interp, err_msg);

   return(TCL_OK);
}




static char *module = "phTclUmedian";    /* name of this set of code */

char g_regMeanFitByCol[] = "regMeanFitByCol";

static ftclArgvInfo g_regMeanFitByColTbl[] = {
    {NULL, FTCL_ARGV_HELP, NULL, NULL,
       "estimates the median of the input region, which presumably contains undersampled data\n" },
    {"<region>", FTCL_ARGV_STRING, NULL, NULL, 
       "Input region"},
    {"-biasScale", FTCL_ARGV_INT, NULL, NULL, 
       "Scaling factor for output (integer) vector"},
    {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclRegMeanFitByCol(ClientData clientData, Tcl_Interp *interp, int argc, 
		   char **argv)
{
  int rstat;
  char *a_region = NULL;
  char name[HANDLE_NAMELEN];
  int a_biasScale = 1;
  HANDLE outputHandle;
  REGION *inreg, *outreg;

  g_regMeanFitByColTbl[1].dst = &a_region;
  g_regMeanFitByColTbl[2].dst = &a_biasScale;

  /* Parse Arguments */

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_regMeanFitByColTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_regMeanFitByCol)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclRegAddrGetFromName(interp, a_region, &inreg) != TCL_OK) {
      return(TCL_ERROR);
  }


  if (!phGaussFitIsInitialised()) {
      Tcl_SetResult(interp, "tclRegMeanFitByCol: you must initialize with initGaussFit", 
		    TCL_STATIC);
      return(TCL_ERROR);
   }



   /* create a new region for the output */
  if ((outreg = shRegNew("tclRegMeanFitByCol", 1, inreg->ncol, inreg->type)) ==
            NULL) {
      Tcl_SetResult(interp, "tclRegMeanFitByCol: can't create new region", 
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   
  /* actually find the median */
  if (phRegMeanFitByCol(inreg, a_biasScale, outreg) != SH_SUCCESS) {
      shRegDel(outreg);
      Tcl_SetResult(interp, 
		    "tclRegMeanFitByCol: phRegMeanFitByCol returns with error",
		    TCL_STATIC);
      return(TCL_ERROR);
  }
   /* bind a new handle to this region and return */
   if (p_shTclHandleNew(interp, name) != TCL_OK) {
      shRegDel(outreg);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   outputHandle.ptr = outreg;
   outputHandle.type = shTypeGetFromName("REGION");
   if (p_shTclHandleAddrBind(interp, outputHandle, name) != TCL_OK) {
      Tcl_SetResult(interp, "can't bind to new REGION handle", TCL_STATIC);
      return(TCL_ERROR);
   }

  Tcl_SetResult(interp, name, TCL_VOLATILE);
  return(TCL_OK);
  
}



void
phTclUmedianDeclare(Tcl_Interp *interp)
{
    int  flags = FTCL_ARGV_NO_LEFTOVERS;
    shTclDeclare(interp, g_regMeanFitByCol, 
		 (Tcl_CmdProc *) tclRegMeanFitByCol,
		 (ClientData) 0, (Tcl_CmdDeleteProc *)NULL, module,
		 shTclGetArgInfo(interp, g_regMeanFitByColTbl, flags, 
				 g_regMeanFitByCol),
		 shTclGetUsage(interp, g_regMeanFitByColTbl, flags, 
			       g_regMeanFitByCol));
   shTclDeclare(interp,"histU16StatsFit",
                (Tcl_CmdProc *)tclHistU16StatsFit,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclHistU16StatsFit_hlp, tclHistU16StatsFit_use);

   shTclDeclare(interp,"initGaussFit",
                (Tcl_CmdProc *)tclInitGaussFit,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclInitGaussFit_hlp, tclInitGaussFit_use);

   shTclDeclare(interp,"finiGaussFit",
                (Tcl_CmdProc *)tclFiniGaussFit,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFiniGaussFit_hlp, tclFiniGaussFit_use);

}

