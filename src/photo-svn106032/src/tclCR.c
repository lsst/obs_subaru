/*
 * TCL support for LIBRARY routines in photo.
 *
 * ENTRY POINT		SCOPE
 * tclPhotparDeclare	public	Declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 *
 * 
 * The following TCL verbs are declared herein:
 *       
 *      findCR 
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phLibrary.h"
#include "phCosmic.h"
#include "phUtils.h"

static char *module = "phTclCR";	/* name of this set of code */

/*****************************************************************************/
/*
 * call the phFindCR function
 */
static char *regstr = NULL;
static double sky = 0.0;
static double sigma_b = 0.0;
static char *psfstr = NULL;
static char *psfBasisStr = NULL;
static double e_per_dn = 0.0;
static int keep = 1;
static double min_sigma;
static int min_e;
static double cond3, cond32;
static double bkgd;
static char *adoptedCond32Var = NULL;

static char *tclFindCR_use =
  "USAGE: findCR region sky sigma_b seeing e_per_dn [-keep]";
static char *tclFindCR_hlp =
  "find cosmic rays in the given REGION";

static ftclArgvInfo findCR_argTable[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL, "find cosmic rays" },
   { "<region>", FTCL_ARGV_STRING, NULL, &regstr, "region" },
   { "<sky>", FTCL_ARGV_DOUBLE, NULL, &sky, "sky" },
   { "<sigma_b>", FTCL_ARGV_DOUBLE, NULL, &sigma_b, "sky sigma (in DN)" },
   { "<psf>", FTCL_ARGV_STRING, NULL, &psfstr, "psf for the frame" },
   { "<e_per_dn>", FTCL_ARGV_DOUBLE, NULL, &e_per_dn, "electrons per DN" },
   { "-keep", FTCL_ARGV_CONSTANT, (void*)1, &keep, "don't remove CRs"},
   { "-min_sigma", FTCL_ARGV_DOUBLE, NULL, &min_sigma,
       			     "min sigma above sky in pixel for CR candidates"},
   { "-min_e", FTCL_ARGV_INT, NULL, &min_e,
       			      "min number of e^- above sky for CR candidates"},
   { "-cond3", FTCL_ARGV_DOUBLE, NULL, &cond3,
       			     		      "fiddle factor for condition 3"},
   { "-cond32", FTCL_ARGV_DOUBLE, NULL, &cond32,
				       "second fiddle factor for condition 3"},
   { "-bkgd", FTCL_ARGV_DOUBLE, NULL, &bkgd,
			    "unsubtracted sky level, in excess of SOFT_BIAS" },
   { "-psfBasis", FTCL_ARGV_STRING, NULL, &psfBasisStr,
		    "PSF_BASIS describing PSF; used to tune algorithm not to flag the PSF as a CR" },
   { "-adopted_cond32", FTCL_ARGV_STRING, NULL, &adoptedCond32Var,
				"Variable to return adopted value of cond32" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};

static int
tclFindCR(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   float adopted_cond32 = 0;
   HANDLE hand;
   DGPSF *psf;
   REGION *reg;
   PSF_BASIS *psfBasis;
   int n;				/* number of CRs found */
   void *vptr;				/* used by shTclHandleExprEval */

   shErrStackClear();

   keep = 0;
   min_sigma = 6.0;
   min_e = 150;
   cond3 = 3.0;
   cond32 = 1.0;
   bkgd = 0;
   psfBasisStr = NULL;
   adoptedCond32Var = NULL;

   if(get_FtclOpts(interp, &argc, argv, findCR_argTable) != TCL_OK){
      return(TCL_ERROR);
   }

   /* get the input region */
   if(shTclHandleExprEval(interp,regstr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp, "findCR: first arg is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,psfstr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("DGPSF")) {
      Tcl_SetResult(interp,"findCR: fourth argument is not a DGPSF",
		    						   TCL_STATIC);
      return(TCL_ERROR);
   }
   psf = hand.ptr;

   if(psfBasisStr == NULL ||
      *psfBasisStr == '\0' || strcmp(psfBasisStr, "NULL") == 0) {
      psfBasis = NULL;
   } else {
      if(shTclHandleExprEval(interp,psfBasisStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("PSF_BASIS")) {
	 Tcl_AppendResult(interp,"findCR: "
			  "argument \"", psfBasisStr, "\" is not a PSF_BASIS",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      psfBasis = hand.ptr;
   }
/*
 * do the work
 */
   if((n = phFindCR(reg, sky, bkgd, sigma_b, psf,
		    e_per_dn, min_sigma, min_e, cond3, cond32, keep,
		    psfBasis, &adopted_cond32)) >= 0 ) {
      char tmp[10];

      if(adoptedCond32Var != NULL) {
	 char buff[100];
	 sprintf(buff, "%g", adopted_cond32);
	 if(Tcl_SetVar(interp, adoptedCond32Var, buff, TCL_LEAVE_ERR_MSG)
								     == NULL) {
	    Tcl_SetResult(interp, "Failed to return adopted_cond32",
			  TCL_VOLATILE);
	    return(TCL_ERROR);
	 }
      }

      sprintf(tmp,"%d",n);
      Tcl_SetResult(interp, tmp, TCL_VOLATILE);
      return(TCL_OK);
   } else {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   } 
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclLibraryCRDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"findCR",
                (Tcl_CmdProc *)tclFindCR,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFindCR_hlp, tclFindCR_use);
}
