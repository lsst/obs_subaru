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
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phConsts.h"
#include "phUtils.h"
#include "phCorrectFrames.h"
#include "phObjects.h"

static char *module = "phTclCorrectFrames";    /* name of this set of code */

/****************************************************************************
**
**<AUTO EXTRACT>
** TCL VERB: initCorrectFrames
**
**</AUTO>
*/
/*****************************************************************************/
/*
 * Init and fini functions for Correct Frames
 */
static char *tclInitCorrectFrames_use =
  "USAGE: initCorrectFrames nrowInCorrectedFrame";
#define tclInitCorrectFrames_hlp \
  "Initialise the Correct Frames module"

static ftclArgvInfo initCorrectFrames_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclInitCorrectFrames_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclInitCorrectFrames(
		     ClientData clientData,
		     Tcl_Interp *interp,
		     int argc,
		     char **argv
		     )
{
   if(get_FtclOpts(interp, &argc, argv, initCorrectFrames_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
   phInitCorrectFrames();

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFiniCorrectFrames_use =
"USAGE: finiCorrectFrames";
#define tclFiniCorrectFrames_hlp \
  "Cleanup after the Correct Frames module"

static ftclArgvInfo finiCorrectFrames_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFiniCorrectFrames_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFiniCorrectFrames(
		     ClientData clientData,
		     Tcl_Interp *interp,
		     int argc,
		     char **argv
		     )
{
   if(get_FtclOpts(interp, &argc, argv, finiCorrectFrames_opts) != TCL_OK) {
      return(TCL_ERROR);
   }

   phFiniCorrectFrames();

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclCorrectFrames_use =
  "USAGE: correctFrames <raw> <bias> <flat> <ccdpars> <bias_scale> -leftbias -rightbias -stamp -regout -fix_bias -minval <fparams>";
#define tclCorrectFrames_hlp \
"Correct the raw region given a bias vector, flat vector, ccdpars, "\
"the factor that the biases were scaled by. The bias "\
"drift corrections are given by leftbias and rightbias. If the -stamp flag "\
"is specified, the raw region is a postage stamp; otherwise, it is assumed "\
"to be an entire frame."

static ftclArgvInfo correctFrames_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCorrectFrames_hlp},
   {"<raw>", FTCL_ARGV_STRING, NULL, NULL, "Raw REGION to correct"},
   {"<bias>", FTCL_ARGV_STRING, NULL, NULL, "REGION giving bias vector"},
   {"<flat>", FTCL_ARGV_STRING, NULL, NULL,
					  "REGION with (inverted) flat field"},
   {"<ccdpars>", FTCL_ARGV_STRING, NULL, NULL, "CCD parameters"},
   {"<fparams>", FTCL_ARGV_STRING, NULL, NULL, "FRAMEPARAMS describing frame"},
   {"<bias_scale>", FTCL_ARGV_INT, NULL, NULL, "Scale of bias vector"},
   {"-leftbias", FTCL_ARGV_INT, NULL, NULL, "Left bias drift"},
   {"-rightbias", FTCL_ARGV_INT, NULL, NULL, "Right bias drift"},
   {"-stamp", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					   "Process <raw> as a postage stamp"},
   {"-regout", FTCL_ARGV_STRING, NULL, NULL, "Name of output REGION"},
   {"-fix_bias", FTCL_ARGV_CONSTANT, (void *)1, NULL,
		       "Fix pre-Sept-1998 bias problems due to a zener diode"},
   {"-minval", FTCL_ARGV_DOUBLE, NULL, NULL,
			   "Minimum acceptable value for interpolated pixels"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define correctFrames_name "correctFrames"

static int
tclCorrectFrames(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *rawStr = NULL;			/* Raw REGION to correct */
   REGION *raw;
   char *biasStr = NULL;		/* REGION giving bias vector */
   REGION *bias;
   char *flatStr = NULL;		/* REGION with (inverted) flat field */
   REGION *flat;
   char *ccdparsStr = NULL;		/* CCD parameters */
   CCDPARS *ccdpars;
   int bias_scale = 0;			/* Scale of bias vector */
   int leftbias = 0;			/* Left bias drift */
   int rightbias = 0;			/* Right bias drift */
   int stamp = 0;			/* Process <raw> as a postage stamp */
   char *regoutStr = NULL;		/* Name of output REGION */
   REGION *regout;
   int fix_bias = 0;			/* Fix pre-Sept-1998 bias problems due
					   to a zener diode */
   double minval = 0;			/* Minimum acceptable value for
					   interpolated pixels */
   char *fparamsStr = NULL;		/* FRAMEPARAMS describing frame */
   FRAMEPARAMS *fparams;

   shErrStackClear();

   i = 1;
   correctFrames_opts[i++].dst = &rawStr;
   correctFrames_opts[i++].dst = &biasStr;
   correctFrames_opts[i++].dst = &flatStr;
   correctFrames_opts[i++].dst = &ccdparsStr;
   correctFrames_opts[i++].dst = &fparamsStr;
   correctFrames_opts[i++].dst = &bias_scale;
   correctFrames_opts[i++].dst = &leftbias;
   correctFrames_opts[i++].dst = &rightbias;
   correctFrames_opts[i++].dst = &stamp;
   correctFrames_opts[i++].dst = &regoutStr;
   correctFrames_opts[i++].dst = &fix_bias;
   correctFrames_opts[i++].dst = &minval;
   shAssert(correctFrames_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, correctFrames_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     correctFrames_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,rawStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"correctFrames: "
                       "argument \"", rawStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   raw = hand.ptr;
   if(raw->type != TYPE_PIX) {
      Tcl_AppendResult(interp, "REGION ", rawStr, " must be PIX",
		       (char *)NULL);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,biasStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"correctFrames: "
                       "argument \"", biasStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   bias = hand.ptr;
   if(bias->type != TYPE_U16) {
      Tcl_AppendResult(interp, "REGION ", biasStr, " must be U16",
		       (char *)NULL);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,flatStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"correctFrames: "
                       "argument \"", flatStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   flat = hand.ptr;
   if(flat->type != TYPE_U16) {
      Tcl_AppendResult(interp, "REGION ", flatStr, " must be U16",
		       (char *)NULL);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,ccdparsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CCDPARS")) {
      Tcl_AppendResult(interp,"correctFrames: "
                       "argument \"", ccdparsStr, "\" is not a CCDPARS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   ccdpars = hand.ptr;

   if(shTclHandleExprEval(interp,fparamsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FRAMEPARAMS")) {
      Tcl_AppendResult(interp,"correctFrames: "
                       "argument \"", fparamsStr, "\" is not a FRAMEPARAMS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   fparams = hand.ptr;

   if(regoutStr == NULL) {
      regout = shRegNew("corrected", raw->nrow, raw->ncol, raw->type);
      regout->mask = (MASK *)phSpanmaskNew(regout->nrow, regout->ncol);
      regout->row0 = raw->row0;
      regout->col0 = raw->col0;
   } else {
      if(shTclHandleExprEval(interp,regoutStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_AppendResult(interp,"correctFrames: "
			  "argument \"", regoutStr, "\" is not a REGION",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      regout = hand.ptr;
      if(regout->type != TYPE_PIX) {
	 Tcl_AppendResult(interp, "REGION ", regoutStr, " must be U16",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
   }
/*
 * work
 */
   if(phCorrectFrames(raw, bias, flat, ccdpars, fparams,
		      bias_scale, leftbias, rightbias,
		      stamp, regout, FLT2PIX(minval), fix_bias) != SH_SUCCESS){
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * Return the answer
 */
   if(regoutStr == NULL) {
      if(p_shTclHandleNew(interp,name) != TCL_OK) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
      hand.ptr = regout;
      hand.type = shTypeGetFromName("REGION");
      
      if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
	 Tcl_SetResult(interp,"can't bind \"regout\" to new REGION handle",
		       TCL_STATIC);
	 return(TCL_ERROR);
      }
      
      Tcl_SetResult(interp, name, TCL_VOLATILE);
   } else {
      Tcl_SetResult(interp, regoutStr, TCL_VOLATILE);
   }
   
   return(TCL_OK);
}


/***************************************************************************
 * Declare my new tcl verbs to tcl
 */
void
phTclCorrectFramesDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"initCorrectFrames",
                (Tcl_CmdProc *)tclInitCorrectFrames,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclInitCorrectFrames_hlp, tclInitCorrectFrames_use);

   shTclDeclare(interp,correctFrames_name,
		(Tcl_CmdProc *)tclCorrectFrames, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCorrectFrames_hlp,
		tclCorrectFrames_use);

   shTclDeclare(interp,"finiCorrectFrames",
                (Tcl_CmdProc *)tclFiniCorrectFrames,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFiniCorrectFrames_hlp, tclFiniCorrectFrames_use);
}


