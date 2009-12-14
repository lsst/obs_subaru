/*
 * TCL support for DGPSF type in photo.
 *
 * ENTRY POINT		SCOPE
 * tclDgpsfDeclare	public	Declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "dervish.h"
#include "phDgpsf.h"
#include "phUtils.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

/**********************************************************************
**<AUTO EXTRACT>
** TCL VERB: dgpsfNew
**
** Description:
**
** Return a handle to a new DGPSF
**
**</AUTO>
**********************************************************************/

static char *tclDgpsfNew_use =
  "USAGE: dgpsfNew [options] [-help]";
#define tclDgpsfNew_hlp \
  "create a new DGPSF, optionally specifying parameter values."\
"If values are not specified sigmax2 defaults to 1e10,\n"\
"sigmay{1,2} default to sigmax{1,2}, sigmap to sigmax2, and all others to 0."

static ftclArgvInfo dgpsfNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclDgpsfNew_hlp},
   {"-sigma1_2G", FTCL_ARGV_DOUBLE, NULL, NULL,
				    "standard deviation of narrow Gaussian - 2G fit"},
   {"-sigma2_2G", FTCL_ARGV_DOUBLE, NULL, NULL,
				     "standard deviation of wide Gaussian - 2G fit"},
   {"-b_2G", FTCL_ARGV_DOUBLE, NULL, NULL, "fraction of power in wide Gaussian - 2G fit"},
   {"-sigmax1", FTCL_ARGV_DOUBLE, NULL, NULL,
				    "x- standard deviation of narrow Gaussian"},
   {"-sigmay1", FTCL_ARGV_DOUBLE, NULL, NULL,
				    "y- standard deviation of narrow Gaussian"},
   {"-sigmax2", FTCL_ARGV_DOUBLE, NULL, NULL,
				      "x- standard deviation of wide Gaussian"},
   {"-sigmay2", FTCL_ARGV_DOUBLE, NULL, NULL,
				      "y- standard deviation of wide Gaussian"},
   {"-b", FTCL_ARGV_DOUBLE, NULL, NULL, "fraction of power in wide Gaussian"},
   {"-p0", FTCL_ARGV_DOUBLE, NULL, NULL, "value of power-law at origin"},
   {"-beta", FTCL_ARGV_DOUBLE, NULL, NULL, "power law index"},
   {"-sigmap", FTCL_ARGV_DOUBLE, NULL, NULL, "\"sigma\" of power law part"},
   {"-a", FTCL_ARGV_DOUBLE, NULL, NULL, "Central amplitude of PSF"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclDgpsfNew(
	    ClientData clientDatag,
	    Tcl_Interp *interp,
	    int ac,
	    char **av
	    )
{
   int i;
   DGPSF *dgpsf;
   HANDLE hand;
   char name[HANDLE_NAMELEN];
   double sigma1_2G = -1.0;		/* s.d. of narrow Gaussian - 2G fit */
   double sigma2_2G = -1.0;		/* s.d. of wide Gaussian - 2G fit */
   double b_2G = -1.0;			/* fraction of power in wide Gaussian - 2G fit */
   double sigmax1 = -1.0;		/* x s.d. of narrow Gaussian */
   double sigmay1 = -1.0;		/* y s.d. of narrow Gaussian */
   double sigmax2 = -1.0;		/* x s.d. of wide Gaussian */
   double sigmay2 = -1.0;		/* y s.d. of wide Gaussian */
   double b = -1.0;			/* fraction of power in wide Gaussian*/
   double p0 = -1.0;			/* value of power-law at origin */
   double beta = -1.0;			/* power law index */
   double sigmap = -1.0;		/* "sigma" of power law part */
   double a = -1.0;			/* central amplitude of PSF */
   
   shErrStackClear();

   i = 1;
   dgpsfNew_opts[i++].dst = &sigma1_2G;
   dgpsfNew_opts[i++].dst = &sigma2_2G;
   dgpsfNew_opts[i++].dst = &b_2G;
   dgpsfNew_opts[i++].dst = &sigmax1;
   dgpsfNew_opts[i++].dst = &sigmay1;
   dgpsfNew_opts[i++].dst = &sigmax2;
   dgpsfNew_opts[i++].dst = &sigmay2;
   dgpsfNew_opts[i++].dst = &b;
   dgpsfNew_opts[i++].dst = &p0;
   dgpsfNew_opts[i++].dst = &beta;
   dgpsfNew_opts[i++].dst = &sigmap;
   dgpsfNew_opts[i++].dst = &a;
   shAssert(dgpsfNew_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,dgpsfNew_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(sigmay1 < 0) sigmay1 = sigmax1;
   if(sigmay2 < 0) sigmay2 = sigmax2;
   if(sigmap < 0) sigmap = sigmax2;
/*
 * work
 */
   dgpsf = phDgpsfNew();
   if(sigma1_2G >= 0) { dgpsf->sigma1_2G = sigma1_2G; }
   if(sigma2_2G >= 0) { dgpsf->sigma2_2G = sigma2_2G; }
   if(b_2G >= 0) { dgpsf->b_2G = b_2G; }

   if(sigmax1 >= 0) { dgpsf->sigmax1 = sigmax1; }
   if(sigmay1 >= 0) { dgpsf->sigmay1 = sigmay1; }
   if(sigmax2 >= 0) { dgpsf->sigmax2 = sigmax2; }
   if(sigmay2 >= 0) { dgpsf->sigmay2 = sigmay2; }
   if(b >= 0) { dgpsf->b = b; }
   if(p0 >= 0) { dgpsf->p0 = p0; }
   if(beta >= 0) { dgpsf->beta = beta; }
   if(sigmap >= 0) { dgpsf->sigmap = sigmap; }
   if(a >= 0) { dgpsf->a = a; }
/*
 * get the width (sigma) of a single Gaussian fit to the PSF
 */
   dgpsf->width = sqrt((pow(dgpsf->sigmax1,2) + pow(dgpsf->sigmay1,2))/2 +
		       pow(dgpsf->b,2)*
			     (pow(dgpsf->sigmax2,2) + pow(dgpsf->sigmay2,2))/2);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = dgpsf;
   hand.type = shTypeGetFromName("DGPSF");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new DGPSF handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/**********************************************************************
**<AUTO EXTRACT>
** TCL VERB: dgpsfDel
**
** Description:
**
** Delete a DGPSF
**
**</AUTO>
**********************************************************************/

static char *tclDgpsfDel_use =
  "USAGE: dgpsfDel dgpsf";
static char *tclDgpsfDel_hlp =
  "Delete a DGPSF";

static int
tclDgpsfDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *dgpsf;
   char *opts = "dgpsf";

   shErrStackClear();

   ftclParseSave("dgpsfDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      dgpsf = ftclGetStr("dgpsf");
      if(p_shTclHandleAddrGet(interp,dgpsf,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      ftclParseRestore("dgpsfDel");
      Tcl_SetResult(interp,tclDgpsfDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(handle->type != shTypeGetFromName("DGPSF")) {
      Tcl_SetResult(interp,"handle is not a DGPSF",TCL_STATIC);
      return(TCL_ERROR);
   }

   phDgpsfDel(handle->ptr);
   (void) p_shTclHandleDel(interp,dgpsf);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}


/**********************************************************************
**<AUTO EXTRACT>
** TCL VERB: dgpsfCopy
**
** Description:
**
** Copy the content of dgpsf2 to dgpsf1.
**
**</AUTO>
**********************************************************************/

static char *tclDgpsfCopy_use =
  "USAGE: dgpsfCopy dgpsf1 dgpsf2";
static char *tclDgpsfCopy_hlp =
  "copy the content of dgpsf2 to dgpsf1, returning dgpsf1";

static int
tclDgpsfCopy(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle1, *handle2;
   char *opts = "dgpsf1 dgpsf2";
   char *hname;

   shErrStackClear();

   ftclParseSave("dgpsfCopy");
   if(ftclFullParseArg(opts,argc,argv) == 0) {
      ftclParseRestore("dgpsfCopy");
      Tcl_SetResult(interp,tclDgpsfCopy_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   hname = ftclGetStr("dgpsf1");
   if(p_shTclHandleAddrGet(interp,hname,&handle1) != TCL_OK) {
      return(TCL_ERROR);
   }
   hname = ftclGetStr("dgpsf2");
   if(p_shTclHandleAddrGet(interp,hname,&handle2) != TCL_OK) {
      return(TCL_ERROR);
   }

   if(handle1->type != shTypeGetFromName("DGPSF")) {
      Tcl_SetResult(interp,"first handle is not a DGPSF",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(handle2->type != shTypeGetFromName("DGPSF")) {
      Tcl_SetResult(interp,"second handle is not a DGPSF",TCL_STATIC);
      return(TCL_ERROR);
   }

   phDgpsfCopy(handle1->ptr,handle2->ptr);

   Tcl_SetResult(interp,ftclGetStr("dgpsf1"),TCL_STATIC);
   return(TCL_OK);
}


/**************************************************************************/
/*
 * Given a REGION, a DGPSF structure and a position,
 *   make a copy of the DGPSF with the given amplitude and add it 
 *   to the REGION.
 */

static char *tclDgpsfAddToReg_use =
  "USAGE: dgpsfAddToReg <reg> <dgpsf> <row> <col> -outer -a -help";
#define tclDgpsfAddToReg_hlp \
"Add the DGPSF with given amplitude DGPSF.a, at the given position, " \
"to the REGION; touch only pixels within 'outer' radius."

static ftclArgvInfo dgpsfAddToReg_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclDgpsfAddToReg_hlp},
   {"<region>", FTCL_ARGV_STRING, NULL, NULL, "region to add PSF to"},
   {"<dgpsf>", FTCL_ARGV_STRING, NULL, NULL, "DGPSF to add"},
   {"<rowc>", FTCL_ARGV_DOUBLE, NULL, NULL, "row-centre of PSF"},
   {"<colc>", FTCL_ARGV_DOUBLE, NULL, NULL, "column-centre of PSF"},
   {"-outer", FTCL_ARGV_DOUBLE, NULL, NULL,
					  "Outer radius of area to operate on"},
   {"-a", FTCL_ARGV_DOUBLE, NULL, NULL,
				      "Amplitude of PSF (overrides <dgpsf>.a)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclDgpsfAddToReg(ClientData clientDatag,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   HANDLE hand;
   int i;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regionStr = NULL;		/* region to add PSF to */
   REGION *reg;
   char *dgpsfStr = NULL;		/* DGPSF to add */
   DGPSF *dgpsf;
   double rowc = 0.0;			/* row-centre of PSF */
   double colc = 0.0;			/* column-centre of PSF */
   double outer = 0.0;			/* Outer radius of area to operate on*/
   double a = 0.0;			/* PSF Amplitude (overrides dgpsf.a)*/

   shErrStackClear();

   i = 1;
   dgpsfAddToReg_opts[i++].dst = &regionStr;
   dgpsfAddToReg_opts[i++].dst = &dgpsfStr;
   dgpsfAddToReg_opts[i++].dst = &rowc;
   dgpsfAddToReg_opts[i++].dst = &colc;
   dgpsfAddToReg_opts[i++].dst = &outer;
   dgpsfAddToReg_opts[i++].dst = &a;
   shAssert(dgpsfAddToReg_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,dgpsfAddToReg_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regionStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"dgpsfAddToReg: "
                    "argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,dgpsfStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("DGPSF")) {
      Tcl_SetResult(interp,"dgpsfAddToReg: "
                    "argument is not a DGPSF",TCL_STATIC);
      return(TCL_ERROR);
   }
   dgpsf = hand.ptr;
/*
 * work
 */
   if(fabs(a) < 1e-10) {
      a = dgpsf->a;
   }

   phDgpsfAdd(reg, dgpsf, outer, rowc, colc, a);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclDgpsfAddInObjmask_use =
  "USAGE: dgpsfAddInObjmask <reg> <om> <dgpsf> <rowc> <colc> -outer dval -a dval -drow ival -dcol ival";
#define tclDgpsfAddInObjmask_hlp \
  "Add a PSF to a region, but only within the OBKMASK"

static ftclArgvInfo dgpsfAddInObjmask_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclDgpsfAddInObjmask_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "REGION to have PSF added"},
   {"<om>", FTCL_ARGV_STRING, NULL, NULL, "OBJMASK to add PSF within"},
   {"<dgpsf>", FTCL_ARGV_STRING, NULL, NULL, "DGPSF to add"},
   {"<rowc>", FTCL_ARGV_DOUBLE, NULL, NULL, "row-centre of PSF"},
   {"<colc>", FTCL_ARGV_DOUBLE, NULL, NULL, "column-centre of PSF"},
   {"-outer", FTCL_ARGV_DOUBLE, NULL, NULL, "Outer radius of area to operate on"},
   {"-a", FTCL_ARGV_DOUBLE, NULL, NULL, "Amplitude of PSF (overrides <dgpsf>.a)"},
   {"-drow", FTCL_ARGV_INT, NULL, NULL, "Offset om by this many rows"},
   {"-dcol", FTCL_ARGV_INT, NULL, NULL, "Offset om by this many columns"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define dgpsfAddInObjmask_name "dgpsfAddInObjmask"

static int
tclDgpsfAddInObjmask(ClientData clientData,
		     Tcl_Interp *interp,
		     int ac,
		     char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to have PSF added */
   REGION *reg;
   char *omStr = NULL;			/* OBJMASK to add PSF within */
   OBJMASK *om;
   char *dgpsfStr = NULL;		/* DGPSF to add */
   DGPSF *dgpsf;
   double rowc = 0.0;			/* row-centre of PSF */
   double colc = 0.0;			/* column-centre of PSF */
   double outer = 0.0;			/* Outer radius of area to operate on*/
   double a = 0.0;			/* PSF Amplitude (overrides dgpsf.a) */
   int drow = 0;			/* Offset om by this many rows */
   int dcol = 0;			/* Offset om by this many columns */

   shErrStackClear();

   a_i = 1;
   dgpsfAddInObjmask_opts[a_i++].dst = &regStr;
   dgpsfAddInObjmask_opts[a_i++].dst = &omStr;
   dgpsfAddInObjmask_opts[a_i++].dst = &dgpsfStr;
   dgpsfAddInObjmask_opts[a_i++].dst = &rowc;
   dgpsfAddInObjmask_opts[a_i++].dst = &colc;
   dgpsfAddInObjmask_opts[a_i++].dst = &outer;
   dgpsfAddInObjmask_opts[a_i++].dst = &a;
   dgpsfAddInObjmask_opts[a_i++].dst = &drow;
   dgpsfAddInObjmask_opts[a_i++].dst = &dcol;
   shAssert(dgpsfAddInObjmask_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, dgpsfAddInObjmask_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     dgpsfAddInObjmask_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"dgpsfAddInObjmask: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,omStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_AppendResult(interp,"dgpsfAddInObjmask: "
                       "argument \"", omStr, "\" is not an OBJMASK",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   om = hand.ptr;

   if(shTclHandleExprEval(interp,dgpsfStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("DGPSF")) {
      Tcl_AppendResult(interp,"dgpsfAddInObjmask: "
                       "argument \"", dgpsfStr, "\" is not a DGPSF",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   dgpsf = hand.ptr;
/*
 * work
 */
   if(fabs(a) < 1e-10) {
      a = dgpsf->a;
   }

   phDgpsfAddInObjmask(reg, om, drow, dcol, dgpsf, outer, rowc, colc, a);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclDgpsfDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"dgpsfNew",
		(Tcl_CmdProc *)tclDgpsfNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDgpsfNew_hlp,
		tclDgpsfNew_use);

   shTclDeclare(interp,"dgpsfDel",
                (Tcl_CmdProc *)tclDgpsfDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclDgpsfDel_hlp, tclDgpsfDel_use);

   shTclDeclare(interp,"dgpsfCopy",
                (Tcl_CmdProc *)tclDgpsfCopy,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclDgpsfCopy_hlp, tclDgpsfCopy_use);

   shTclDeclare(interp,"dgpsfAddToReg",
		(Tcl_CmdProc *)tclDgpsfAddToReg, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDgpsfAddToReg_hlp,
		tclDgpsfAddToReg_use);

   shTclDeclare(interp,dgpsfAddInObjmask_name,
		(Tcl_CmdProc *)tclDgpsfAddInObjmask, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDgpsfAddInObjmask_hlp,
		tclDgpsfAddInObjmask_use);
}
