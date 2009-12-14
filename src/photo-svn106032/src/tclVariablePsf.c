#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dervish.h"
#include "phConsts.h"
#include "phVariablePsf.h"

/*
 * Utility function to convert e.g. "U16" to TYPE_U16
 */
static PIXDATATYPE
get_type_from_str(const char *typeStr)
{
   if(strcmp(typeStr, "U8") == 0) {
      return(TYPE_U8);
   } else if(strcmp(typeStr, "S8") == 0) {
      return(TYPE_S8);
   } else if(strcmp(typeStr, "U16") == 0) {
      return(TYPE_U16);
   } else if(strcmp(typeStr, "S16") == 0) {
      return(TYPE_S16);
   } else if(strcmp(typeStr, "S32") == 0) {
      return(TYPE_S32);
   } else if(strcmp(typeStr, "FL32") == 0) {
      return(TYPE_FL32);
   } else if(strcmp(typeStr, "PIX") == 0) {
      return(TYPE_PIX);
   }

   shErrStackPush("Type %s typeStr is not supported", typeStr);
   return((PIXDATATYPE)(-1));
}

/*****************************************************************************/
   
static char *tclPsfKernelNew_use =
  "USAGE: psfKernelNew <nsigma> <n_a> [n_b] -ncol_a -ncol_b";
#define tclPsfKernelNew_hlp \
  "Create a psfKernel to describe a spatially varying PSF"

static ftclArgvInfo psfKernelNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfKernelNew_hlp},
   {"<nsigma>", FTCL_ARGV_INT, NULL, NULL, "Number of Gaussian widths to use"},
   {"<n_a>", FTCL_ARGV_INT, NULL, NULL, "Max order of variation at a point"},
   {"[n_b]", FTCL_ARGV_INT, NULL, NULL, "Max order of spatial variation"},
   {"-ncol_a", FTCL_ARGV_INT, NULL, NULL,
				   "Max order of column variation at a point"},
   {"-ncol_b", FTCL_ARGV_INT, NULL, NULL,
				      "Max order of column spatial variation"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfKernelNew_name "psfKernelNew"

static int
tclPsfKernelNew(ClientData clientData,
		  Tcl_Interp *interp,
		  int ac,
		  char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   int nsigma = 0;			/* Number of Gaussian widths to use */
   int n_a = 0;				/* Max order of variation at a point */
   int n_b = 1;				/* Max order spatial variation */
   int ncol_a = -1;			/* Max order of col var. at a point */
   int ncol_b = -1;			/* Max order of col spatial variation*/
   int nrow_a, nrow_b;			/* Max order of row var. at a point */
   PSF_KERNEL *kern;			/* the desired kernel coeffs */

   shErrStackClear();

   i = 1;
   psfKernelNew_opts[i++].dst = &nsigma;
   psfKernelNew_opts[i++].dst = &n_a;
   psfKernelNew_opts[i++].dst = &n_b;
   psfKernelNew_opts[i++].dst = &ncol_a;
   psfKernelNew_opts[i++].dst = &ncol_b;
   shAssert(psfKernelNew_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfKernelNew_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfKernelNew_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   nrow_a = n_a;
   if(ncol_a < 0) {
      ncol_a = n_a;
   }

   nrow_b = n_b;
   if(ncol_b < 0) {
      ncol_b = n_b;
   }
/*
 * work
 */
   kern = phPsfKernelNew(nsigma, nrow_a, ncol_a, nrow_b, ncol_b);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = kern;
   hand.type = shTypeGetFromName("PSF_KERNEL");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"kern\" to new PSF_KERNEL handle",
								   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclPsfKernelDel_use =
  "USAGE: psfKernelDel <psfKernel>";
#define tclPsfKernelDel_hlp \
  "Delete a psfKernel"

static ftclArgvInfo psfKernelDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfKernelDel_hlp},
   {"<psfKernel>", FTCL_ARGV_STRING, NULL, NULL, "The PSF_KERNEL to delete"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfKernelDel_name "psfKernelDel"

static int
tclPsfKernelDel(ClientData clientData,
		  Tcl_Interp *interp,
		  int ac,
		  char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *psfKernelStr = NULL;		/* The PSF_KERNEL to delete */
   PSF_KERNEL *psfKernel;

   shErrStackClear();

   i = 1;
   psfKernelDel_opts[i++].dst = &psfKernelStr;
   shAssert(psfKernelDel_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfKernelDel_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfKernelDel_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,psfKernelStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_KERNEL")) {
      Tcl_AppendResult(interp,"psfKernelDel: "
                       "argument \"", psfKernelStr, "\" is not a PSF_KERNEL",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   psfKernel = hand.ptr;
/*
 * work
 */
   phPsfKernelDel(psfKernel);
   p_shTclHandleDel(interp,psfKernelStr);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclPsfBasisNew_use =
  "USAGE: psfBasisNew <psfKernel> <nrow> <ncol> -regType sval -type sval";
#define tclPsfBasisNew_hlp \
  "Create a new set of basis regions for estimating/fitting a variable PSF"

static ftclArgvInfo psfBasisNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfBasisNew_hlp},
   {"<psfKernel>", FTCL_ARGV_STRING, NULL, NULL,
				      "a PSF_KERNEL describing PSF variation"},
   {"<nrow>", FTCL_ARGV_INT, NULL, NULL, "Number of rows in each REGION"},
   {"<ncol>", FTCL_ARGV_INT, NULL, NULL, "Number of columns in each REGION"},
   {"-regType", FTCL_ARGV_STRING, NULL, NULL,
					 "Desired REGION type (default: U16)"},
   {"-type", FTCL_ARGV_STRING, NULL, NULL,
				       "Type of PSF_BASIS (Default: UNKNOWN)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfBasisNew_name "psfBasisNew"

static int
tclPsfBasisNew(ClientData clientData,
	       Tcl_Interp *interp,
	       int ac,
	       char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *psfKernelStr = NULL;		/* PSF_KERNEL describing variation */
   PSF_KERNEL *psfKernel;
   int nrow = 0;			/* Number of rows in each REGION */
   int ncol = 0;			/* Number of columns in each REGION */
   char *regTypeStr = "U16";		/* Desired type (default: U16) */
   PIXDATATYPE regType;			/* type of REGIONs */
   PSF_BASIS *basis;			/* desired PSF_BASIS */
   char *typeStr = "UNKNOWN";		/* Type of PSF_BASIS */
   BASIS_TYPE basisType;
   
   shErrStackClear();

   i = 1;
   psfBasisNew_opts[i++].dst = &psfKernelStr;
   psfBasisNew_opts[i++].dst = &nrow;
   psfBasisNew_opts[i++].dst = &ncol;
   psfBasisNew_opts[i++].dst = &regTypeStr;
   psfBasisNew_opts[i++].dst = &typeStr;
   shAssert(psfBasisNew_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfBasisNew_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfBasisNew_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(strcmp(typeStr, "UNKNOWN") == 0) {
      basisType = UNKNOWN_BASIS;
   } else if(strcmp(typeStr, "GAUSSIAN") == 0) {
      basisType = GAUSSIAN_BASIS;
   } else if(strcmp(typeStr, "KL") == 0) {
      basisType = KL_BASIS;
   } else {
      Tcl_SetResult(interp, "psfBasisNew: Unknown basis type", TCL_STATIC);
      Tcl_AppendResult(interp, typeStr, (char *)NULL);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   regType = get_type_from_str(regTypeStr);

   if(shTclHandleExprEval(interp,psfKernelStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_KERNEL")) {
      Tcl_AppendResult(interp,"psfBasisNew: "
                       "argument \"", psfKernelStr, "\" is not a PSF_KERNEL",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   psfKernel = hand.ptr;   
/*
 * work
 */
   basis = phPsfBasisNew(psfKernel, basisType, nrow, ncol, regType);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = basis;
   hand.type = shTypeGetFromName("PSF_BASIS");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"basis\" to new PSF_BASIS handle",
								   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

static char *tclPsfBasisDel_use =
  "USAGE: psfBasisDel <psfBasis>";
#define tclPsfBasisDel_hlp \
  "Delete a PSF_BASIS and all its regions"

static ftclArgvInfo psfBasisDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfBasisDel_hlp},
   {"<psfBasis>", FTCL_ARGV_STRING, NULL, NULL, "The PSF_BASIS to delete"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfBasisDel_name "psfBasisDel"

static int
tclPsfBasisDel(ClientData clientData,
	       Tcl_Interp *interp,
	       int ac,
	       char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *psfBasisStr = NULL;		/* The PSF_BASIS to delete */
   PSF_BASIS *psfBasis;

   shErrStackClear();

   i = 1;
   psfBasisDel_opts[i++].dst = &psfBasisStr;
   shAssert(psfBasisDel_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfBasisDel_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfBasisDel_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,psfBasisStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_BASIS")) {
      Tcl_AppendResult(interp,"psfBasisDel: "
                       "argument \"", psfBasisStr, "\" is not a PSF_BASIS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   psfBasis = hand.ptr;
/*
 * work
 */
   phPsfBasisDel(psfBasis);
   p_shTclHandleDel(interp,psfBasisStr);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclPsfBasisSet_use =
  "USAGE: psfBasisSet <psfBasis>";
#define tclPsfBasisSet_hlp \
  "Set the values of a PSF_BASIS's REGIONs"

static ftclArgvInfo psfBasisSet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfBasisSet_hlp},
   {"<psfBasis>", FTCL_ARGV_STRING, NULL, NULL, "The PSF_BASIS to set"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfBasisSet_name "psfBasisSet"

static int
tclPsfBasisSet(ClientData clientData,
	       Tcl_Interp *interp,
	       int ac,
	       char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *psfBasisStr = NULL;		/* The PSF_BASIS to set */
   PSF_BASIS *psfBasis;

   shErrStackClear();

   i = 1;
   psfBasisSet_opts[i++].dst = &psfBasisStr;
   shAssert(psfBasisSet_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfBasisSet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfBasisSet_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,psfBasisStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_BASIS")) {
      Tcl_AppendResult(interp,"psfBasisSet: "
                       "argument \"", psfBasisStr, "\" is not a PSF_BASIS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   psfBasis = hand.ptr;   
/*
 * work
 */
   phPsfBasisSet(psfBasis);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclRegConvolveWithPsfKernel_use =
  "USAGE: regConvolveWithPsfKernel <reg> <psfKernel> [psfBasis] -type";
#define tclRegConvolveWithPsfKernel_hlp \
   "Return a PSF_BASIS containing <region> convolved with the <psfKernel>.\n"\
"If [psfBasis] is provided and not equal to \"\", use it (and don't return\n"\
"another handle to it)"

static ftclArgvInfo regConvolveWithPsfKernel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegConvolveWithPsfKernel_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to convolve"},
   {"<psfKernel>", FTCL_ARGV_STRING, NULL, NULL,
				       "PSF_KERNEL describing desired kernel"},
   {"[psfBasis]", FTCL_ARGV_STRING, NULL, NULL,
				       "PSF_BASIS to contain answer; or \"\""},
   {"-type", FTCL_ARGV_STRING, NULL, NULL,
				 "Desired type for basis (default: reg's)"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regConvolveWithPsfKernel_name "regConvolveWithPsfKernel"

static int
tclRegConvolveWithPsfKernel(ClientData clientData,
			    Tcl_Interp *interp,
			    int ac,
			    char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to convolve */
   REGION *reg;
   char *psfKernelStr = NULL;		/* PSF_KERNEL describing kernel */
   PSF_KERNEL *psfKernel;
   char *psfBasisStr = NULL;		/* PSF_BASIS to contain answer; or ""*/
   PSF_BASIS *psfBasis = NULL;
   char *typeStr = NULL;		/* Desired basis type (default:reg's)*/
   PIXDATATYPE type;

   shErrStackClear();

   i = 1;
   regConvolveWithPsfKernel_opts[i++].dst = &regStr;
   regConvolveWithPsfKernel_opts[i++].dst = &psfKernelStr;
   regConvolveWithPsfKernel_opts[i++].dst = &psfBasisStr;
   regConvolveWithPsfKernel_opts[i++].dst = &typeStr;
   shAssert(regConvolveWithPsfKernel_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regConvolveWithPsfKernel_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regConvolveWithPsfKernel_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"regConvolveWithPsfKernel: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,psfKernelStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_KERNEL")) {
      Tcl_AppendResult(interp,"regConvolveWithPsfKernel: "
                       "argument \"", psfKernelStr, "\" is not a PSF_KERNEL",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   psfKernel = hand.ptr;

   if(psfBasisStr != NULL && *psfBasisStr == '\0') {
      psfBasisStr = NULL;
   }
   if(psfBasisStr != NULL) {
      if(shTclHandleExprEval(interp,psfBasisStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("PSF_BASIS")) {
	 Tcl_AppendResult(interp,"regConvolveWithPsfKernel: "
			  "argument \"", psfBasisStr, "\" is not a PSF_BASIS",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      psfBasis = hand.ptr;
   }

   if(typeStr == NULL) {
      type = reg->type;
   } else {
      type = get_type_from_str(typeStr);
   }
/*
 * work
 */
   psfBasis = phRegConvolveWithPsfKernel(reg, psfKernel, psfBasis, type);
/*
 * Return the answer
 */
   if(psfBasisStr == NULL) {
      if(p_shTclHandleNew(interp,name) != TCL_OK) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
      hand.ptr = psfBasis;
      hand.type = shTypeGetFromName("PSF_BASIS");
      
      if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
	 Tcl_SetResult(interp,
		       "can't bind \"psfBasis\" to new PSF_BASIS handle",
		       TCL_STATIC);
	 return(TCL_ERROR);
      }
      
      Tcl_SetResult(interp, name, TCL_VOLATILE);
   }
   
   return(TCL_OK);
}

/*****************************************************************************/
static char *tclRegionDotRegion_use =
  "USAGE: regionDotRegion <reg1> <reg2> -border";
#define tclRegionDotRegion_hlp \
  "Return the \"dot product\" of two regions (currently U16 and/or FL32)"

static ftclArgvInfo regionDotRegion_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegionDotRegion_hlp},
   {"<reg1>", FTCL_ARGV_STRING, NULL, NULL, "One region"},
   {"<reg2>", FTCL_ARGV_STRING, NULL, NULL, "Another region"},
   {"-border", FTCL_ARGV_INT, NULL, NULL,
			  "Number of pixels to ignore around edge of regions"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regionDotRegion_name "regionDotRegion"

static int
tclRegionDotRegion(ClientData clientData,
		   Tcl_Interp *interp,
		   int ac,
		   char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *reg1Str = NULL;		/* One region */
   REGION *reg1;
   char *reg2Str = NULL;		/* Another region */
   REGION *reg2;
   double dot;				/* desired answer */
   int border = 0;			/* Width of border to ignore at edges*/

   shErrStackClear();

   i = 1;
   regionDotRegion_opts[i++].dst = &reg1Str;
   regionDotRegion_opts[i++].dst = &reg2Str;
   regionDotRegion_opts[i++].dst = &border;
   shAssert(regionDotRegion_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regionDotRegion_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regionDotRegion_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,reg1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"regionDotRegion: "
                       "argument \"", reg1Str, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg1 = hand.ptr;

   if(shTclHandleExprEval(interp,reg2Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"regionDotRegion: "
                       "argument \"", reg2Str, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg2 = hand.ptr;

   if(reg1->nrow != reg2->nrow || reg1->ncol != reg2->ncol){
      Tcl_SetResult(interp,"regionDotRegion: "
                    "two REGIONs are not the same size", TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(phRegionDotRegion(&dot, reg1, reg2, border) < 0) {
      Tcl_AppendResult(interp, "regionDotRegion: "
		       "I cannot handle your mixture of types", (char *)NULL);
      return(TCL_ERROR);
   }

   {
      char buff[40];
      sprintf(buff, "%.8g", dot);
      Tcl_SetResult(interp, buff, TCL_VOLATILE);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Calculate a set of coefficients to match a PSF_BASIS to a region
 */
static char *tclPsfKernelSet_use =
  "USAGE: psfKernelSet <regs> <basis> [kern]";
#define tclPsfKernelSet_hlp \
  "Given a CHAIN of PSF_REGs and a PSF_BASIS with its regions\n"\
"initialised, (e.g. with regConvolveWithPsfKernel) fill out the\n"\
"coefficients in a PSF_KERNEL"

static ftclArgvInfo psfKernelSet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfKernelSet_hlp},
   {"<regs>", FTCL_ARGV_STRING, NULL, NULL, "Chain of PSF_REGs to be fit"},
   {"<basis>", FTCL_ARGV_STRING, NULL, NULL, "The PSF_BASIS to use"},
   {"[kern]", FTCL_ARGV_STRING, NULL, NULL, "The kernel to fill out"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfKernelSet_name "psfKernelSet"

static int
tclPsfKernelSet(ClientData clientData,
		Tcl_Interp *interp,
		int ac,
		char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regsStr = NULL;		/* The REGION to be fit */
   CHAIN *regs;
   char *basisStr = NULL;		/* The PSF_BASIS to use */
   PSF_BASIS *basis;
   char *kernStr = NULL;		/* The kernel to fill out */
   PSF_KERNEL *kern;

   shErrStackClear();

   i = 1;
   psfKernelSet_opts[i++].dst = &regsStr;
   psfKernelSet_opts[i++].dst = &basisStr;
   psfKernelSet_opts[i++].dst = &kernStr;
   shAssert(psfKernelSet_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfKernelSet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfKernelSet_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"psfKernelSet: "
                       "argument \"", regsStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   regs = hand.ptr;
   if(regs->type != shTypeGetFromName("PSF_REG")) {
      Tcl_AppendResult(interp,
                       "CHAIN \"", regsStr, "\" doesn't have type PSF_REG",
								 (char *)NULL);
      return(TCL_ERROR);
   }
   

   if(shTclHandleExprEval(interp,basisStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_BASIS")) {
      Tcl_AppendResult(interp,"psfKernelSet: "
                       "argument \"", basisStr, "\" is not a PSF_BASIS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   basis = hand.ptr;

   if(kernStr == NULL) {
      kern = NULL;
   } else {
      if(shTclHandleExprEval(interp,kernStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("PSF_KERNEL")) {
	 Tcl_AppendResult(interp,"psfKernelSet: "
			  "argument \"", kernStr, "\" is not a PSF_KERNEL",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      kern = hand.ptr;
   }
/*
 * work
 */
   kern = phPsfKernelSet(kern, regs, basis);
/*
 * Return the answer
 */
   if(kernStr == NULL) {
      if(p_shTclHandleNew(interp,name) != TCL_OK) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
      hand.ptr = kern;
      hand.type = shTypeGetFromName("PSF_KERNEL");
      
      if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
	 Tcl_SetResult(interp,"can't bind \"kern\" to new PSF_KERNEL handle",
		       TCL_STATIC);
	 return(TCL_ERROR);
      }
      
      Tcl_SetResult(interp, name, TCL_VOLATILE);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * apply a PSF_KERNEL to a given region
 */
static char *tclPsfKernelApply_use =
  "USAGE: psfKernelApply <psfReg> <kern> [scr1] [scr2] [scr3]";
#define tclPsfKernelApply_hlp \
  "Given a REGION and a PSF_KERNEL (with coefficients set as it were by\n"\
"psfKernelSet), apply that kernel to the PSF_REG, returning the result"

static ftclArgvInfo psfKernelApply_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfKernelApply_hlp},
   {"<psfReg>", FTCL_ARGV_STRING, NULL, NULL, "object to transform"},
   {"<kern>", FTCL_ARGV_STRING, NULL, NULL, "PSF_KERNEL to use"},
   {"[scr1]", FTCL_ARGV_STRING, NULL, NULL,
			   "Scratch FL32 region of same size as <psfReg.reg>"},
   {"[scr2]", FTCL_ARGV_STRING, NULL, NULL,
			   "Scratch FL32 region of same size as <psfReg.reg>"},
   {"[scr3]", FTCL_ARGV_STRING, NULL, NULL,
			   "Scratch FL32 region of same size as <psfReg.reg>"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfKernelApply_name "psfKernelApply"

static int
tclPsfKernelApply(ClientData clientData,
		  Tcl_Interp *interp,
		  int ac,
		  char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *psfRegStr = NULL;			/* Region to transform */
   PSF_REG *psfReg;
   REGION *reg;
   char *kernStr = NULL;		/* PSF_KERNEL to use */
   PSF_KERNEL *kern;
   char *scr1Str = NULL;		/* Scratch region of same size as reg*/
   REGION *scr1;
   char *scr2Str = NULL;		/* Scratch region of same size as reg*/
   REGION *scr2;
   char *scr3Str = NULL;		/* Scratch region of same size as reg*/
   REGION *scr3;
   REGION *out;				/* returned REGION */

   shErrStackClear();

   i = 1;
   psfKernelApply_opts[i++].dst = &psfRegStr;
   psfKernelApply_opts[i++].dst = &kernStr;
   psfKernelApply_opts[i++].dst = &scr1Str;
   psfKernelApply_opts[i++].dst = &scr2Str;
   psfKernelApply_opts[i++].dst = &scr3Str;
   shAssert(psfKernelApply_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfKernelApply_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfKernelApply_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,psfRegStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_REG")) {
      Tcl_AppendResult(interp,"psfKernelApply: "
                       "argument \"", psfRegStr, "\" is not a PSF_REG",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   psfReg = hand.ptr;
   
   reg = psfReg->reg;
   if(reg == NULL) {
      Tcl_SetResult(interp, "psfKernelApply: psfReg.reg is NULL", TCL_STATIC);
      return(TCL_ERROR);
   }
     
   if(shTclHandleExprEval(interp,kernStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_KERNEL")) {
      Tcl_AppendResult(interp,"psfKernelApply: "
                       "argument \"", kernStr, "\" is not a PSF_KERNEL",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   kern = hand.ptr;

   if(scr1Str == NULL) {
      scr1 = NULL;
   } else {
      if(shTclHandleExprEval(interp,scr1Str,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_AppendResult(interp,"psfKernelApply: "
			  "argument \"", scr1Str, "\" is not a REGION",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      scr1 = hand.ptr;
   }

   if(scr2Str == NULL) {
      scr2 = NULL;
   } else {
      if(shTclHandleExprEval(interp,scr2Str,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_AppendResult(interp,"psfKernelApply: "
			  "argument \"", scr2Str, "\" is not a REGION",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      scr2 = hand.ptr;
   }

   if(scr3Str == NULL) {
      scr3 = NULL;
   } else {
      if(shTclHandleExprEval(interp,scr3Str,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_AppendResult(interp,"psfKernelApply: "
			  "argument \"", scr3Str, "\" is not a REGION",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      scr3 = hand.ptr;
   }
/*
 * work
 */
   out = phPsfKernelApply(psfReg, kern, scr1, scr2, scr3);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = out;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"out\" to new REGION handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

/*****************************************************************************/
static char *tclPsfKLDecomp_use =
  "USAGE: psfKLDecomp <regs> <ncomp> -border [n_b] -ncol_b -constantWeight";
#define tclPsfKLDecomp_hlp \
  "KL decompose a set of images"

static ftclArgvInfo psfKLDecomp_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfKLDecomp_hlp},
   {"<regs>", FTCL_ARGV_STRING, NULL, NULL,
				"Chain of PSF_REGs containing observed stars"},
   {"<ncomp>", FTCL_ARGV_INT, NULL, NULL, "Number of KL components to keep"},
   {"[n_b]", FTCL_ARGV_INT, NULL, NULL, "Max order of spatial variation"},
   {"-border", FTCL_ARGV_INT, NULL, NULL,
			   "Number of pixels to ignore around region's edges"},
   {"-ncol_b", FTCL_ARGV_INT, NULL, NULL,
				   "Max order of column spatial variation"},
   {"-constantWeight", FTCL_ARGV_CONSTANT, (void *)1, NULL,
    "Each input should have the same weight in the KL decomp"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfKLDecomp_name "psfKLDecomp"

static int
tclPsfKLDecomp(ClientData clientData,
	       Tcl_Interp *interp,
	       int ac,
	       char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regsStr = NULL;		/* Chain of PSF_REGs containing stars*/
   CHAIN *regs;
   int ncomp = 0;			/* Number of KL components to keep */
   int border = 0;			/* width to ignore around reg's edges*/
   PSF_BASIS *basis;			/* returned basis */
   int n_b = 1;				/* Max order of spatial variation */
   int nrow_b, ncol_b = -1;		/* Max order of col spatial variation*/
   int constantWeight = 0;		/* Each input should have the same weight in the KL decomp */
  
   shErrStackClear();

   a_i = 1;
   psfKLDecomp_opts[a_i++].dst = &regsStr;
   psfKLDecomp_opts[a_i++].dst = &ncomp;
   psfKLDecomp_opts[a_i++].dst = &n_b;
   psfKLDecomp_opts[a_i++].dst = &border;
   psfKLDecomp_opts[a_i++].dst = &ncol_b;
   psfKLDecomp_opts[a_i++].dst = &constantWeight;
   shAssert(psfKLDecomp_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfKLDecomp_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfKLDecomp_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"psfKLDecomp: "
                       "argument \"", regsStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   regs = hand.ptr;

   if(regs->type != shTypeGetFromName("PSF_REG")) {
      Tcl_AppendResult(interp,
                       "CHAIN \"", regsStr, "\" doesn't have type PSF_REG",
								 (char *)NULL);
      return(TCL_ERROR);
   }

   nrow_b = n_b;
   if(ncol_b < 0) {
      ncol_b = n_b;
   }
/*
 * work
 */
   basis = phPsfKLDecomp(regs, border, ncomp, nrow_b, ncol_b, constantWeight);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = basis;
   hand.type = shTypeGetFromName("PSF_BASIS");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"basis\" to new PSF_BASIS handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclPsfKLReconstruct_use =
  "USAGE: psfKLReconstruct <basis> <rowc> <colc> -regType sval";
#define tclPsfKLReconstruct_hlp \
  "Reconstruct a PSF given a KL basis and a position"

static ftclArgvInfo psfKLReconstruct_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfKLReconstruct_hlp},
   {"<basis>", FTCL_ARGV_STRING, NULL, NULL, "PSF_BASIS to use"},
   {"<rowc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Row location of desired PSF"},
   {"<colc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Column location of desired PSF"},
   {"-regType", FTCL_ARGV_STRING, NULL, NULL,
				   "Type of desired region (default: U16)"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfKLReconstruct_name "psfKLReconstruct"

static int
tclPsfKLReconstruct(ClientData clientData,
		    Tcl_Interp *interp,
		    int ac,
		    char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *basisStr = NULL;		/* PSF_BASIS to use */
   PSF_BASIS *basis;
   double rowc = 0.0;			/* Row location of desired PSF */
   double colc = 0.0;			/* Column location of desired PSF */
   PSF_REG *psf_reg;
   char *regTypeStr = "U16";		/* Type of desired region */
   PIXDATATYPE regType;			/* type of desired REGION */

   shErrStackClear();

   i = 1;
   psfKLReconstruct_opts[i++].dst = &basisStr;
   psfKLReconstruct_opts[i++].dst = &rowc;
   psfKLReconstruct_opts[i++].dst = &colc;
   psfKLReconstruct_opts[i++].dst = &regTypeStr;
   shAssert(psfKLReconstruct_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfKLReconstruct_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfKLReconstruct_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,basisStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_BASIS")) {
      Tcl_AppendResult(interp,"psfKLReconstruct: "
                       "argument \"", basisStr, "\" is not a PSF_BASIS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   basis = hand.ptr;

   if(basis->type != KL_BASIS) {
      Tcl_SetResult(interp, "psfKLReconstruct: basis isn't KL", TCL_STATIC);
      return(TCL_ERROR);
   }

   regType = get_type_from_str(regTypeStr);
/*
 * work
 */
   psf_reg = phPsfKLReconstruct(basis, rowc, colc, regType);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = psf_reg;
   hand.type = shTypeGetFromName("PSF_REG");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"reg\" to new PSF_REG handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclPsfKLCompNew_use =
  "USAGE: psfKLCompNew -reg sval";
#define tclPsfKLCompNew_hlp \
  "Make a new PSF_KL_COMP"

static ftclArgvInfo psfKLCompNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfKLCompNew_hlp},
   {"-reg", FTCL_ARGV_STRING, NULL, NULL, "Region to include in PSF_KL_COMP"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfKLCompNew_name "psfKLCompNew"

static int
tclPsfKLCompNew(ClientData clientData,
		Tcl_Interp *interp,
		int ac,
		char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;
   PSF_KL_COMP *klc;
   char *regStr = NULL;			/* Region to include in PSF_KL_COMP */
   REGION *reg;

   shErrStackClear();

   i = 1;
   psfKLCompNew_opts[i++].dst = &regStr;
   shAssert(psfKLCompNew_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfKLCompNew_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfKLCompNew_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if(regStr == NULL) {
      reg = NULL;
   } else {
      if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_AppendResult(interp,"psfKLCompNew: "
			  "argument \"", regStr, "\" is not a REGION",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      reg = hand.ptr;
   }
/*
 * work
 */
   klc = phPsfKLCompNew(reg);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = klc;
   hand.type = shTypeGetFromName("PSF_KL_COMP");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"klc\" to new PSF_KL_COMP handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

static char *tclPsfKLCompDel_use =
  "USAGE: psfKLCompDel <klc>";
#define tclPsfKLCompDel_hlp \
  "Delete a PSF_KL_COMP"

static ftclArgvInfo psfKLCompDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfKLCompDel_hlp},
   {"<klc>", FTCL_ARGV_STRING, NULL, NULL, "PSF_KL_COMP to delete"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfKLCompDel_name "psfKLCompDel"

static int
tclPsfKLCompDel(ClientData clientData,
		Tcl_Interp *interp,
		int ac,
		char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *klcStr = NULL;			/* PSF_KL_COMP to delete */
   PSF_KL_COMP *klc;

   shErrStackClear();

   i = 1;
   psfKLCompDel_opts[i++].dst = &klcStr;
   shAssert(psfKLCompDel_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfKLCompDel_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfKLCompDel_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,klcStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_KL_COMP")) {
      Tcl_AppendResult(interp,"psfKLCompDel: "
                       "argument \"", klcStr, "\" is not a PSF_KL_COMP",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   klc = hand.ptr;
/*
 * work
 */
   phPsfKLCompDel(klc);
   p_shTclHandleDel(interp,klcStr);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclPsfKLCompSetFromBasis_use =
  "USAGE: psfKLCompSetFromBasis <basis> <c> -klc sval";
#define tclPsfKLCompSetFromBasis_hlp \
"Set the fields in a PSF_KL_COMP from the c'th component of a PSF_BASIS;\n"\
"if not PSF_KL_COMP is specified, one will be created and returned"

static ftclArgvInfo psfKLCompSetFromBasis_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfKLCompSetFromBasis_hlp},
   {"<basis>", FTCL_ARGV_STRING, NULL, NULL,
					 "PSF_BASIS with desired information"},
   {"<c>", FTCL_ARGV_INT, NULL, NULL, "Desired component"},
   {"-klc", FTCL_ARGV_STRING, NULL, NULL, "The PSF_KL_COMP to set, or \"\""},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfKLCompSetFromBasis_name "psfKLCompSetFromBasis"

static int
tclPsfKLCompSetFromBasis(ClientData clientData,
		Tcl_Interp *interp,
		int ac,
		char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *klcStr = NULL;			/* The PSF_KL_COMP to set, or "" */
   PSF_KL_COMP *klc;
   char *basisStr = NULL;		/* PSF_BASIS with desired information*/
   PSF_BASIS *basis;
   int c = 0;				/* Desired component */

   shErrStackClear();

   i = 1;
   psfKLCompSetFromBasis_opts[i++].dst = &basisStr;
   psfKLCompSetFromBasis_opts[i++].dst = &c;
   psfKLCompSetFromBasis_opts[i++].dst = &klcStr;
   shAssert(psfKLCompSetFromBasis_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfKLCompSetFromBasis_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfKLCompSetFromBasis_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(klcStr == NULL) {
      klc = NULL;
   } else {
      if(shTclHandleExprEval(interp,klcStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("PSF_KL_COMP")) {
	 Tcl_AppendResult(interp,"psfKLCompSetFromBasis: "
			  "argument \"", klcStr, "\" is not a PSF_KL_COMP",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      klc = hand.ptr;
   }

   if(shTclHandleExprEval(interp,basisStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_BASIS")) {
      Tcl_AppendResult(interp,"psfKLCompSetFromBasis: "
                       "argument \"", basisStr, "\" is not a PSF_BASIS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   basis = hand.ptr;
/*
 * work
 */
   klc = phPsfKLCompSetFromBasis(klc, basis, c);

   if(klcStr == NULL) {
/*
 * Return the answer
 */
      if(p_shTclHandleNew(interp,name) != TCL_OK) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
      hand.ptr = klc;
      hand.type = shTypeGetFromName("PSF_KL_COMP");
      
      if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
	 Tcl_SetResult(interp,"can't bind \"klc\" to new PSF_KL_COMP handle",
                                                                   TCL_STATIC);
	 return(TCL_ERROR);
      }
      
      Tcl_SetResult(interp, name, TCL_VOLATILE);
   }
   
   return(TCL_OK);
}

/*****************************************************************************/
static char *tclPsfBasisSetFromKLComp_use =
  "USAGE: psfBasisSetFromKLComp <basis> <c> <klc> -copy_reg";
#define tclPsfBasisSetFromKLComp_hlp \
  "Set component <c> of a PSF_BASIS from a PSF_KL_COMP"

static ftclArgvInfo psfBasisSetFromKLComp_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfBasisSetFromKLComp_hlp},
   {"<basis>", FTCL_ARGV_STRING, NULL, NULL, "PSF_BASIS to set"},
   {"<c>", FTCL_ARGV_INT, NULL, NULL, "Which component to set"},
   {"<klc>", FTCL_ARGV_STRING, NULL, NULL, "PSF_KL_COMP to set basis from"},
   {"-copy_reg", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					      "Copy region, not just pointer"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfBasisSetFromKLComp_name "psfBasisSetFromKLComp"

static int
tclPsfBasisSetFromKLComp(ClientData clientData,
			 Tcl_Interp *interp,
			 int ac,
			 char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *basisStr = NULL;		/* PSF_BASIS to set */
   PSF_BASIS *basis;
   int c = 0;				/* Which component to set */
   char *klcStr = NULL;			/* PSF_KL_COMP to set basis from */
   PSF_KL_COMP *klc;
   int copy_reg = 0;			/* Copy region, not just the pointer */

   shErrStackClear();

   i = 1;
   psfBasisSetFromKLComp_opts[i++].dst = &basisStr;
   psfBasisSetFromKLComp_opts[i++].dst = &c;
   psfBasisSetFromKLComp_opts[i++].dst = &klcStr;
   psfBasisSetFromKLComp_opts[i++].dst = &copy_reg;
   shAssert(psfBasisSetFromKLComp_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfBasisSetFromKLComp_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfBasisSetFromKLComp_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,basisStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_BASIS")) {
      Tcl_AppendResult(interp,"psfBasisSetFromKLComp: "
                       "argument \"", basisStr, "\" is not a PSF_BASIS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   basis = hand.ptr;

   if(shTclHandleExprEval(interp,klcStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_KL_COMP")) {
      Tcl_AppendResult(interp,"psfBasisSetFromKLComp: "
                       "argument \"", klcStr, "\" is not a PSF_KL_COMP",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   klc = hand.ptr;
/*
 * work
 */
   phPsfBasisSetFromKLComp(basis, c, klc, copy_reg);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * return a new PSF_BASIS that's the first or second derivative of the
 * one provided
 */
static char *tclPsfBasisDifferentiate_use =
  "USAGE: psfBasisDifferentiate <basis> <nderiv_row> <nderiv_col> -scr sval -cosbell ival";
#define tclPsfBasisDifferentiate_hlp \
"return a new PSF_BASIS that's the first or second derivative of the" \
"one provided"

static ftclArgvInfo psfBasisDifferentiate_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfBasisDifferentiate_hlp},
   {"<basis>", FTCL_ARGV_STRING, NULL, NULL, "Input PSF_BASIS"},
   {"<nderiv_row>", FTCL_ARGV_INT, NULL, NULL,
					"Order of differentiation along rows"},
   {"<nderiv_col>", FTCL_ARGV_INT, NULL, NULL,
				     "Order of differentiation along columns"},
   {"-scr", FTCL_ARGV_STRING, NULL, NULL,
		     "Scratch REGION of same size and type as those in basis"},
   {"-bell", FTCL_ARGV_INT, NULL, NULL,
		    "Half-width of cosbell (filter has 2*bell-1 elements)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfBasisDifferentiate_name "psfBasisDifferentiate"

static int
tclPsfBasisDifferentiate(ClientData clientData,
			 Tcl_Interp *interp,
			 int ac,
			 char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *basisStr = NULL;		/* Input PSF_BASIS */
   PSF_BASIS *basis;
   int nderiv_row = 0;			/* Order of differentiation along row*/
   int nderiv_col = 0;			/* Order of differentiation along col*/
   char *scrStr = NULL;			/* Scratch REGION */
   int bell = 5;			/* Half-width of cosbell */
   REGION *scr;
   PSF_BASIS *deriv;			/* desired derivative */

   shErrStackClear();

   a_i = 1;
   psfBasisDifferentiate_opts[a_i++].dst = &basisStr;
   psfBasisDifferentiate_opts[a_i++].dst = &nderiv_row;
   psfBasisDifferentiate_opts[a_i++].dst = &nderiv_col;
   psfBasisDifferentiate_opts[a_i++].dst = &scrStr;
   psfBasisDifferentiate_opts[a_i++].dst = &bell;
   shAssert(psfBasisDifferentiate_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfBasisDifferentiate_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfBasisDifferentiate_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,basisStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_BASIS")) {
      Tcl_AppendResult(interp,"psfBasisDifferentiate: "
                       "argument \"", basisStr, "\" is not a PSF_BASIS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   basis = hand.ptr;

   if(scrStr == NULL) {
      scr = NULL;
   } else {
      if(shTclHandleExprEval(interp,scrStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_AppendResult(interp,"psfBasisDifferentiate: "
			  "argument \"", scrStr, "\" is not a REGION",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      scr = hand.ptr;
   }

   if(nderiv_row < 0 || nderiv_row > 2) {
      Tcl_SetResult(interp, "psfBasisDifferentiate: ", TCL_STATIC);
      Tcl_AppendResult(interp, " nderiv_row must be in [0,2]", (char *)NULL);
      return(TCL_ERROR);
   }
   if(nderiv_col < 0 || nderiv_col > 2) {
      Tcl_SetResult(interp, "psfBasisDifferentiate: ", TCL_STATIC);
      Tcl_AppendResult(interp, " nderiv_col must be in [0,2]", (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   deriv = phPsfBasisDifferentiate(basis, scr, nderiv_row, nderiv_col, bell);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = deriv;
   hand.type = shTypeGetFromName("PSF_BASIS");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"deriv\" to new PSF_BASIS handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);   

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclVariablePsfDeclare(Tcl_Interp *interp)
{
   char *module = "phTclPsfKernel";	/* name of this set of code */

   shTclDeclare(interp,psfKernelNew_name,
		(Tcl_CmdProc *)tclPsfKernelNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfKernelNew_hlp,
		tclPsfKernelNew_use);

   shTclDeclare(interp,psfKernelDel_name,
		(Tcl_CmdProc *)tclPsfKernelDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfKernelDel_hlp,
		tclPsfKernelDel_use);

   shTclDeclare(interp,psfBasisNew_name,
		(Tcl_CmdProc *)tclPsfBasisNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfBasisNew_hlp,
		tclPsfBasisNew_use);

   shTclDeclare(interp,psfBasisDel_name,
		(Tcl_CmdProc *)tclPsfBasisDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfBasisDel_hlp,
		tclPsfBasisDel_use);

   shTclDeclare(interp,psfBasisSet_name,
		(Tcl_CmdProc *)tclPsfBasisSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfBasisSet_hlp,
		tclPsfBasisSet_use);

   shTclDeclare(interp,regConvolveWithPsfKernel_name,
		(Tcl_CmdProc *)tclRegConvolveWithPsfKernel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegConvolveWithPsfKernel_hlp,
		tclRegConvolveWithPsfKernel_use);

   shTclDeclare(interp,regionDotRegion_name,
		(Tcl_CmdProc *)tclRegionDotRegion, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegionDotRegion_hlp,
		tclRegionDotRegion_use);

   shTclDeclare(interp,psfKernelSet_name,
		(Tcl_CmdProc *)tclPsfKernelSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfKernelSet_hlp,
		tclPsfKernelSet_use);

   shTclDeclare(interp,psfKernelApply_name,
		(Tcl_CmdProc *)tclPsfKernelApply, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfKernelApply_hlp,
		tclPsfKernelApply_use);

   shTclDeclare(interp,psfKLDecomp_name,
		(Tcl_CmdProc *)tclPsfKLDecomp, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfKLDecomp_hlp,
		tclPsfKLDecomp_use);

   shTclDeclare(interp,psfKLReconstruct_name,
		(Tcl_CmdProc *)tclPsfKLReconstruct, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfKLReconstruct_hlp,
		tclPsfKLReconstruct_use);

   shTclDeclare(interp,psfKLCompNew_name,
		(Tcl_CmdProc *)tclPsfKLCompNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfKLCompNew_hlp,
		tclPsfKLCompNew_use);

   shTclDeclare(interp,psfKLCompDel_name,
		(Tcl_CmdProc *)tclPsfKLCompDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfKLCompDel_hlp,
		tclPsfKLCompDel_use);

   shTclDeclare(interp,psfKLCompSetFromBasis_name,
		(Tcl_CmdProc *)tclPsfKLCompSetFromBasis, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfKLCompSetFromBasis_hlp,
		tclPsfKLCompSetFromBasis_use);

   shTclDeclare(interp,psfBasisSetFromKLComp_name,
		(Tcl_CmdProc *)tclPsfBasisSetFromKLComp, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfBasisSetFromKLComp_hlp,
		tclPsfBasisSetFromKLComp_use);

   shTclDeclare(interp,psfBasisDifferentiate_name,
		(Tcl_CmdProc *)tclPsfBasisDifferentiate, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfBasisDifferentiate_hlp,
		tclPsfBasisDifferentiate_use);
}
