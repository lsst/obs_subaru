/*
 *
 * ENTRY POINT			SCOPE	DESCRIPTION
 * tclCorrectFrames	public	Declare all the verbs defined in 
 *					this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 *	DERVISH		Survey Human Interface and Visualization
 *
 * Brad Holden (but stealing liberally from Heidi Newberg's code)
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phCorrectFrames.h"

static char *module = "phTclFlatfieldsInterp"; /* name of this set of code */

/************************************************************************
 * Call the flatFieldsInterp routine.  This function creates the new
 *   region "flatvec" which will contain the output of the flatfieldsInterp
 *   routine.
 * 
 * If all goes well, this TCL verb will print the name of a handle which
 *   contains the new REGION.  If not, it prints an error message and
 *   returns TCL_ERROR.
 */

static char *tclFlatfieldsInterp_use = 
  "USAGE: flatfieldsInterp flat2d";
static char *tclFlatfieldsInterp_hlp =
  "Create a flatfield vector for a night from a large set of flat vectors taken during the course of the night.";

static int
tclFlatfieldsInterp(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
  REGION *flat2d, *flatvec;
  HANDLE handle;
  char name[HANDLE_NAMELEN];
  char errors[200];
  char *flat2dName;
  char *formalCmd = "flat2d";

  shErrStackClear();

  ftclParseSave("tclFlatfieldsInterp");

  if(ftclFullParseArg(formalCmd, argc, argv)) {
    flat2dName = ftclGetStr("flat2d");
    if(shTclRegAddrGetFromName(interp, flat2dName, &flat2d) != TCL_OK) {
      sprintf(errors,"Cannot find region named: %s", flat2dName);
      Tcl_SetResult(interp,errors,TCL_VOLATILE);      
      return (TCL_ERROR);
    }
  }
  else {
    Tcl_SetResult(interp, tclFlatfieldsInterp_use, TCL_VOLATILE);
    return TCL_ERROR;
  }

  /* let's create a REGION for the output flatfield vector - of the same
     type as "flat2d", with same number of columns, and 3 rows */
  if ((flatvec = shRegNew("tclFlatfieldsInterp", 3, flat2d->ncol, 
           flat2d->type)) == NULL) {
    Tcl_SetResult(interp, "tclFlatfieldsInterp: can't allocate for new REGION",
           TCL_STATIC);
    return(TCL_ERROR);
  }

  /* call the C function */
  if (phFlatfieldsInterp(flat2d, flatvec) != SH_SUCCESS) {
     Tcl_SetResult(interp,"Error in phFlatfieldsInterp", TCL_STATIC);
     return(TCL_ERROR);
  }

  /* now bind the output REGION to a new handle */
  if (p_shTclHandleNew(interp, name) != TCL_OK) {
    shTclInterpAppendWithErrStack(interp);
    return(TCL_ERROR);
  }
  handle.ptr = flatvec;
  handle.type = shTypeGetFromName("REGION");
  if (p_shTclHandleAddrBind(interp, handle, name) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new REGION handle", TCL_STATIC);
    return(TCL_ERROR);
  }

  Tcl_SetResult(interp, name, TCL_VOLATILE);
  return(TCL_OK);
}



/************************************************************************
 * Call the SkyBiasInterp routine.   
 ************************************************************************/ 
static char *tclSkyBiasInterp_use = 
  "USAGE: skybiasInterp sky1d drift driftOdd driftEven row nadj sigrej mode ccdpars calib";
static char *tclSkyBiasInterp_hlp =
  "Interpolate sky and bias drift values.";

static int
tclSkyBiasInterp(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
  REGION *sky1d, *drift, *Odrift, *Edrift;
  CALIB1 *calib=NULL;
  CCDPARS *ccdpars=NULL;
  char errors[200];
  char *sky1dName, *driftName, *OdriftName, *EdriftName, *calibName, *ccdparsName;
  char *formalCmd = "sky1d drift Odrift Edrift rowid nadj sigrej mode ccdpars calib";
  int rowid, nadj, mode;
  float sigrej;

  shErrStackClear();

  ftclParseSave("tclSkyBiasInterp");

  if(ftclFullParseArg(formalCmd, argc, argv)) {
    sky1dName = ftclGetStr("sky1d");
    if(shTclRegAddrGetFromName(interp, sky1dName, &sky1d) != TCL_OK) {
      sprintf(errors,"Cannot find region named: %s", sky1dName);
      Tcl_SetResult(interp,errors,TCL_VOLATILE);      
      return (TCL_ERROR);
    }
    driftName = ftclGetStr("drift");
    if(shTclRegAddrGetFromName(interp, driftName, &drift) != TCL_OK) {
      sprintf(errors,"Cannot find region named: %s", driftName);
      Tcl_SetResult(interp,errors,TCL_VOLATILE);
      return (TCL_ERROR);
    }
    OdriftName = ftclGetStr("Odrift");
    if(shTclRegAddrGetFromName(interp, OdriftName, &Odrift) != TCL_OK) {
      sprintf(errors,"Cannot find region named: %s", OdriftName);
      Tcl_SetResult(interp,errors,TCL_VOLATILE);
      return (TCL_ERROR);
    }
    EdriftName = ftclGetStr("Edrift");
    if(shTclRegAddrGetFromName(interp, EdriftName, &Edrift) != TCL_OK) {
      sprintf(errors,"Cannot find region named: %s", EdriftName);
      Tcl_SetResult(interp,errors,TCL_VOLATILE);
      return (TCL_ERROR);
    }
    rowid = ftclGetInt("rowid");
    nadj = ftclGetInt("nadj");
    sigrej = (float) ftclGetDouble("sigrej");
    mode = ftclGetInt("mode");

    ccdparsName = ftclGetStr("ccdpars");
      if(shTclAddrGetFromName(interp,ccdparsName,(void **)&ccdpars,"CCDPARS")
         != TCL_OK) {
         return(TCL_ERROR);
      }

    calibName = ftclGetStr("calib");
    if(shTclAddrGetFromName(interp, calibName, (void **)&calib, "CALIB1") 
       != TCL_OK) {
      sprintf(errors,"Cannot find calib1 named: %s", calibName);
      Tcl_SetResult(interp,errors,TCL_VOLATILE);
      return (TCL_ERROR);
    }
  }
  else {
    Tcl_SetResult(interp, tclSkyBiasInterp_use, TCL_VOLATILE);
    return TCL_ERROR;
  }

  /* call the C function */  
  if (phSkyBiasInterp(sky1d,drift,Odrift,Edrift,rowid,nadj,sigrej,mode,ccdpars,calib) != SH_SUCCESS) {
     Tcl_SetResult(interp,"Error in phSkyBiasInterp", TCL_STATIC);
     return(TCL_ERROR);
  }
 

  return(TCL_OK);
}

/*****************************************************************************/

static char *tclRegGetOddEven_use =
  "USAGE: regGetOddEven <reg> -row1 ival -row2 ival";
#define tclRegGetOddEven_hlp \
  "computes the median values of counts in odd and even columns for row1 <= row <= row2"

static ftclArgvInfo regGetOddEven_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegGetOddEven_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Input region"},
   {"-row1", FTCL_ARGV_INT, NULL, NULL, "Starting row in region"},
   {"-row2", FTCL_ARGV_INT, NULL, NULL, "Ending row in region"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regGetOddEven_name "regGetOddEven"

static int
tclRegGetOddEven(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Input region */
   REGION *reg;
   int row1 = 0;			/* Starting region */
   int row2 = -1;			/* Ending row in region */
   float oddMed, oddMedErr, evenMed, evenMedErr;

   shErrStackClear();

   a_i = 1;
   regGetOddEven_opts[a_i++].dst = &regStr;
   regGetOddEven_opts[a_i++].dst = &row1;
   regGetOddEven_opts[a_i++].dst = &row2;
   shAssert(regGetOddEven_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regGetOddEven_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regGetOddEven_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"regGetOddEven: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(reg->type != TYPE_U16) {
      Tcl_SetResult(interp, "regGetOddEven: region ", TCL_STATIC);
      Tcl_AppendResult(interp, regStr, "is not U16", (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(row2 < 0) {
      row2 = reg->nrow - 1;
   }
   
   if(phRegGetOddEven(reg, row1, row2,
		      &oddMed, &oddMedErr, &evenMed, &evenMedErr) !=
								  SH_SUCCESS) {
      Tcl_SetResult(interp,"Error in phRegGetOddEven", TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * Return values
 */
   {
      char buff[100];

      Tcl_ResetResult(interp);

      sprintf(buff, "%.10g", oddMed);
      Tcl_AppendElement(interp, buff);
      
      sprintf(buff, "%.10g", oddMedErr);
      Tcl_AppendElement(interp, buff);
      
      sprintf(buff, "%.10g", evenMed);
      Tcl_AppendElement(interp, buff);
      
      sprintf(buff, "%.10g", evenMedErr);
      Tcl_AppendElement(interp, buff);
   }
   
   return(TCL_OK);
}

/***************************************************************************
 * Declare my new tcl verbs to tcl
 */
void
phTclFlatfieldsInterpDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp, "flatfieldsInterp",
                (Tcl_CmdProc *)tclFlatfieldsInterp,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFlatfieldsInterp_hlp, tclFlatfieldsInterp_use);

   shTclDeclare(interp, "skybiasInterp",
                (Tcl_CmdProc *)tclSkyBiasInterp,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclSkyBiasInterp_hlp, tclSkyBiasInterp_use);

   shTclDeclare(interp,regGetOddEven_name,
		(Tcl_CmdProc *)tclRegGetOddEven, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegGetOddEven_hlp,
		tclRegGetOddEven_use);
}


