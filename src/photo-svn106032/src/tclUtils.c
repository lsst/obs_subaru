/*
 * Declare various words that should be in dervish; perhaps they will be
 * in the next release?
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/resource.h>
#include "dervish.h"
#include "phDervishUtils.h"		/* utilities which will one day be
					   in dervish */
#include "phSpanUtil.h"
#include "phRandom.h"
#include "phUtils.h"
#include "phChainDiff.h"
#include "phObjc.h"
#include "phObjectCenter.h"

#define UTIL_SIZE 50

static char *module = "phTclDervishFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * Diff two lists
 */
static char *tclListDiff_use =
  "USAGE: listDiff [-diffmax #] list1 list2 proc";
static char *tclListDiff_hlp =
  "Compare two lists, <list1> and <list2>, both specified as handles. The\n\
comparison procedure <proc> should expect to be called with two arguments,\n\
handles to two elements of the two lists. It should return 0 if they are\n\
equal. If <-diffmax> is specified it is the maximum number of differences\n\
that will be detected. The running time of listDiff is expected to be\n\
O(ND) where N is the length of a list, and D is the number of differences.";

/*
 * we need a function to call the TCL procedure to see if two list elements
 * are the same. As listDiff doesn't know about TCL we need to pass the
 * interpreter at global (well, file static) scope.
 *
 * We use p_name as well as name so as to allow the storage to be auto
 *
 * diffproc is the function passed to shChainDiff that calls tcl with our proc
 */

static Tcl_Interp *p_interp = NULL;
static HANDLE p_h[2];			/* handles to be rebound */
static char *p_name[2];			/* names of handles */
static char cmdstr[100];		/* command to be executed */

static int
diffproc(void *e1, void *e2)
{
   int i;
   
   p_h[0].ptr = e1;
   p_h[1].ptr = e2;

   for(i = 0;i < 2;i++) {
      (void)p_shTclHandleAddrBind(p_interp,p_h[i],p_name[i]);
   }
   if(Tcl_Eval(p_interp,cmdstr) != TCL_OK) {
      strcat(p_interp->result,"\n");
      return(-100);
   } else {
      return(atoi(p_interp->result));
   }
}

static int
tclListDiff(
	   ClientData clientData,
	   Tcl_Interp *interp,
	   int argc,
	   char **argv
	   )
{
   HANDLE l[2];				/* handles for lists */
   int diffmax;				/* argument */
   EDIT *diffs;				/* the diffs reported */
   int i;
   char list[2][UTIL_SIZE],proc[UTIL_SIZE];	/* arguments */
   char name[2][HANDLE_NAMELEN];	/* names of handles */
   char *opts = "list0 list1 proc -diffmax=-1";
   void *vptr;				/* used by shTclHandleExprEval */
   
   shErrStackClear();
   
   ftclParseSave("listDiff");
   if(ftclFullParseArg(opts,argc,argv) == 0) {
      Tcl_SetResult(interp,tclListDiff_use,TCL_STATIC);
      return(TCL_ERROR);
   }
   strncpy(list[0],ftclGetStr("list0"),UTIL_SIZE);
   strncpy(list[1],ftclGetStr("list1"),UTIL_SIZE);
   strncpy(proc,ftclGetStr("proc"),UTIL_SIZE);
   diffmax = ftclGetInt("diffmax");
/*
 * see if the specified proc exists and takes two arguments
 */
   sprintf(cmdstr,"info args %s",proc);
   if(Tcl_Eval(interp,cmdstr) != TCL_OK) {
      return(TCL_ERROR);
   } else {
      int nargs = 0;			/* number of arguments */
      char *ptr = interp->result;
      
      for(;;) {
	 if(*ptr == '\0' || isspace(*ptr)) {
	    break;
	 } else {
	    nargs++;
	 }
	 while(*ptr != '\0' && !isspace(*ptr)) ptr++;
	 while(isspace(*ptr)) ptr++;
      }
      if(nargs != 2) {
	 Tcl_SetResult(interp,"",TCL_STATIC);
	 Tcl_AppendResult(interp,"procedure ",proc,
			  " should take exactly two arguments",(char *)NULL);
	 return(TCL_ERROR);
      }
   }

   for(i = 0;i < 2;i++) {
      if(shTclHandleExprEval(interp,list[i],&l[i],&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }

      if(l[i].type != shTypeGetFromName("CHAIN")) {
	 Tcl_AppendResult(interp,"handle ",list[i]," is a ",
			  shNameGetFromType(l[i].type)," not a CHAIN",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
   }
/*
 * set up handles for the comparison proc that shChainDiff will call
 */
   for(i = 0;i < 2;i++) {
      p_name[i] = name[i];		/* make globally visible */
      if(p_shTclHandleNew(interp,name[i]) != TCL_OK) {
	 for(i--;i >= 0;i--) {
	    p_shTclHandleDel(interp,name[i]);
	 }
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }

      p_h[i].ptr = NULL; p_h[i].type = shTypeGetFromName("PTR");
      if(p_shTclHandleAddrBind(interp,p_h[i],name[i]) != TCL_OK) {
	 for(;i >= 0;i--) {
	    p_shTclHandleDel(interp,name[i]);
	 }
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
   }
   p_interp = interp;
   sprintf(cmdstr,"%s %s %s",proc,name[0],name[1]);

   Tcl_SetResult(interp,"",TCL_STATIC);
   diffs = shChainDiff(((CHAIN *)(l[0].ptr)),((CHAIN *)(l[1].ptr)),
		      diffproc,diffmax);

/*
 * delete the element handles
 */
   for(i = 0;i < 2;i++) {
      p_shTclHandleDel(interp,name[i]);
   }
   p_interp = NULL;			/* why not? it might trap some error */
   
   if(diffs == NULL) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   } else {
      if(shChainDiffAsList(interp,diffs) == SH_SUCCESS) {
	 shChainDiffFree();			/* clean up */
	 return(TCL_OK);
      } else {
	 shChainDiffFree();			/* clean up */
	 Tcl_SetResult(interp,"assembling return list for shChainDiff",
		       TCL_STATIC);
	 return(TCL_ERROR);
      }
	
   }
}


/******************************************************************************
 * Clear a region
 */

static char *tclRegClear_use = "USAGE: regClear region";
#define tclRegClear_hlp "set a REGION to zero"

static ftclArgvInfo regClear_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegClear_hlp},
   {"<region>", FTCL_ARGV_STRING, NULL, NULL, "Region to clear"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRegClear(
	    ClientData clientData,
	    Tcl_Interp *interp,
	    int argc,
	    char **argv
	   )
{
   int i;
   HANDLE inputHandle;
   REGION *input;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regionStr = NULL;		/* Region to clear */
   
   shErrStackClear();
   
   i = 1;
   regClear_opts[i++].dst = &regionStr;
   shAssert(regClear_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp, &argc, argv, regClear_opts) != TCL_OK) {
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp, regionStr, &inputHandle, &vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(inputHandle.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"tclRegClear: arg is not a REGION", TCL_STATIC);
      return(TCL_ERROR);
   }
   input = inputHandle.ptr;
/*
 * Hurrah! Do the work
 */
   shRegClear(input);

   Tcl_SetResult(interp, regionStr, TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
static char *tclMaskClear_use =
  "USAGE: MaskClear <mask>";
#define tclMaskClear_hlp \
  "Clear a mask"

static ftclArgvInfo maskClear_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMaskClear_hlp},
   {"<mask>", FTCL_ARGV_STRING, NULL, NULL, "Mask to clear"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclMaskClear(
	     ClientData clientDatag,
	     Tcl_Interp *interp,
	     int ac,
	     char **av
	     )
{
   int i;
   void *vptr;				/* used by shTclHandleExprEval */
   HANDLE hand;
   char *maskStr = NULL;		/* Mask to clear */
   
   shErrStackClear();

   i = 1;
   maskClear_opts[i++].dst = &maskStr;
   shAssert(maskClear_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,maskClear_opts) != TCL_OK) {
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,maskStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("MASK")) {
      Tcl_SetResult(interp,"maskClear: argument is not a MASK",TCL_STATIC);
      return(TCL_ERROR);
   }

   shMaskClear(hand.ptr);

   return(TCL_OK);
}


/******************************************************************************
 * Set an {8,16,32}-bit integral region to a constant value
 */

static char *tclRegIntSetVal_use = "USAGE: regIntSetVal region value";
#define tclRegIntSetVal_hlp "set a REGION to value"

static ftclArgvInfo regIntSetVal_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntSetVal_hlp},
   {"<region>", FTCL_ARGV_STRING, NULL, NULL, "Region to set"},
   {"<value>", FTCL_ARGV_DOUBLE, NULL, NULL, "Value to set region to"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};


static int
tclRegIntSetVal(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
   int i;
   HANDLE inputHandle;
   REGION *input;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regionStr = NULL;		/* Region to set */
   double value = 0.0;			/* Value to set region to */
   
   shErrStackClear();

   i = 1;
   regIntSetVal_opts[i++].dst = &regionStr;
   regIntSetVal_opts[i++].dst = &value;
   shAssert(regIntSetVal_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp, &argc, argv, regIntSetVal_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * Process the arguments
 */
   if(shTclHandleExprEval(interp, regionStr, &inputHandle, &vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(inputHandle.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"tclRegIntSetVal: arg is not a REGION", TCL_STATIC);
      return(TCL_ERROR);
   }
   input = inputHandle.ptr;
/*
 * Hurrah! Do the work
 */
   shRegIntSetVal(input,value);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Add a constant to an {8,16,32}-bit integral region
 */
static char *tclRegIntConstAdd_use = "USAGE: regIntConstAdd region value";
#define tclRegIntConstAdd_hlp "add a constant to a REGION"

static ftclArgvInfo regIntConstAdd_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntConstAdd_hlp},
   {"<region>", FTCL_ARGV_STRING, NULL, NULL, "Region to add to"},
   {"<value>", FTCL_ARGV_DOUBLE, NULL, NULL, "Value to add"},
   {"-dither", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Dither values"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};


static int
tclRegIntConstAdd(
		  ClientData clientData,
		  Tcl_Interp *interp,
		  int argc,
		  char **argv
		  )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regionStr = NULL;		/* Region to add to */
   REGION *reg;
   double value = 0;			/* Value to add */
   int dither = 0;			/* dither values? */
   
   shErrStackClear();

   i = 1;
   regIntConstAdd_opts[i++].dst = &regionStr;
   regIntConstAdd_opts[i++].dst = &value;
   regIntConstAdd_opts[i++].dst = &dither;
   shAssert(regIntConstAdd_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp, &argc, argv, regIntConstAdd_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * Process the arguments
 */
   if(shTclHandleExprEval(interp,regionStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regIntConstAdd: "
                    "argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(dither && reg->type != TYPE_U16) {
      Tcl_SetResult(interp,"regIntConstAdd: "
                    "I only know how to dither U16 regions",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * Hurrah! Do the work
 */
   if(shRegIntConstAdd(reg, value, dither) != SH_SUCCESS) {
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclRegIntGaussianAdd_use =
  "USAGE: RegIntGaussianAdd region rand mean sigma";
#define tclRegIntGaussianAdd_hlp \
  "Add an N(mean,sigma^2) variate to each pixel of an integral region"

static ftclArgvInfo regIntGaussianAdd_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntGaussianAdd_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "The region to add noise to"},
   {"<rand>", FTCL_ARGV_STRING, NULL, NULL, "a RANDOM for random numbers"},
   {"<mean>", FTCL_ARGV_DOUBLE, NULL, NULL, "The mean of the added variate"},
   {"<sigma>", FTCL_ARGV_DOUBLE, NULL, NULL,
				 "The standard deviation of the added variate"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRegIntGaussianAdd(
		     ClientData clientDatag,
		     Tcl_Interp *interp,
		     int ac,
		     char **av
		     )
{
   int i;
   HANDLE hand;
   REGION *reg;
   RANDOM *rand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* The region to add noise to */
   char *randStr = NULL;		/* a RANDOM for random numbers */
   double mean = 0.0;			/* The mean of the added variate */
   double sigma = 0.0;			/* The standard deviation of the added variate */

   shErrStackClear();

   i = 1;
   regIntGaussianAdd_opts[i++].dst = &regStr;
   regIntGaussianAdd_opts[i++].dst = &randStr;
   regIntGaussianAdd_opts[i++].dst = &mean;
   regIntGaussianAdd_opts[i++].dst = &sigma;
   shAssert(regIntGaussianAdd_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,regIntGaussianAdd_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regIntGaussianAdd: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,randStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("RANDOM")) {
      Tcl_SetResult(interp,"regIntGaussianAdd: "
                    "second argument is not a RANDOM",TCL_STATIC);
      return(TCL_ERROR);
   }
   rand = hand.ptr;
/*
 * work
 */
   if(phRegIntGaussianAdd(reg, rand, mean, sigma) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclRegIntConstMult_use =
  "USAGE: regIntConstMult <reg> <val>";
#define tclRegIntConstMult_hlp \
  ""

static ftclArgvInfo regIntConstMult_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntConstMult_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to multiply"},  
   {"<val>", FTCL_ARGV_DOUBLE, NULL, NULL, "Number to multiply by"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regIntConstMult_name "regIntConstMult"

static int
tclRegIntConstMult(ClientData clientData,
		   Tcl_Interp *interp,
		   int ac,
		   char **av)
{
     HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to multiply */
   REGION *reg;
   double val = 0.0;			/* Number to multiply by */

   shErrStackClear();

   a_i = 1;
   regIntConstMult_opts[a_i++].dst = &regStr;
   regIntConstMult_opts[a_i++].dst = &val;
   shAssert(regIntConstMult_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regIntConstMult_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regIntConstMult_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"regIntConstMult: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;
/*
 * work
 */
   shRegIntConstMult(reg, val);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * add noise to an image based on the value of the pixel
 */
static char *tclRegIntNoiseAdd_use =
  "USAGE: RegIntNoiseAdd <reg> <gain>";
#define tclRegIntNoiseAdd_hlp \
  "Add N(0,<reg>/gain) noise to a region <reg>. It is assumed that a softbias"\
" of <sbias> has been added to the region, so a pixel of <sbias> has no counts"

static ftclArgvInfo regIntNoiseAdd_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntNoiseAdd_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "The region in question"},
   {"<rand>", FTCL_ARGV_STRING, NULL, NULL, "a RANDOM for random numbers"},
   {"<sbias>", FTCL_ARGV_INT, NULL, NULL, "software bias"},
   {"<gain>", FTCL_ARGV_DOUBLE, NULL, NULL, "The gain of the image (e-/DN)"},
   {"-poisson", FTCL_ARGV_CONSTANT, (void *)1, NULL,
				   "Use Poisson (not Gaussian) random numbers"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRegIntNoiseAdd(
		  ClientData clientDatag,
		  Tcl_Interp *interp,
		  int ac,
		  char **av
		  )
{
   int i;
   HANDLE hand;
   REGION *reg;
   RANDOM *rand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* The region in question */
   char *randStr = NULL;		/* a RANDOM for random numbers */
   int sbias = 0;			/* software bias */
   double gain = 0.0;			/* The gain of the image (e-/DN) */
   int poisson = 0;			/* use Poisson deviates? */

   shErrStackClear();

   i = 1;
   regIntNoiseAdd_opts[i++].dst = &regStr;
   regIntNoiseAdd_opts[i++].dst = &randStr;
   regIntNoiseAdd_opts[i++].dst = &sbias;
   regIntNoiseAdd_opts[i++].dst = &gain;
   regIntNoiseAdd_opts[i++].dst = &poisson;
   shAssert(regIntNoiseAdd_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,regIntNoiseAdd_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regIntNoiseAdd: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,randStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("RANDOM")) {
      Tcl_SetResult(interp,"regIntNoiseAdd: "
                    "second argument is not a RANDOM",TCL_STATIC);
      return(TCL_ERROR);
   }
   rand = hand.ptr;
/*
 * work
 */
   if(phRegIntNoiseAdd(reg, rand, sbias, gain, poisson) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * return statistics of an image
 */
static char *tclRegStatsFromQuartiles_use =
  "USAGE: regStatsFromQuartiles region -coarse -clip -mean -median -sigma -iqr -q1 -q3 -cmed -csigma -min -max";
#define tclRegStatsFromQuartiles_hlp \
  "Estimate statistical properties of a region. The <region> is sampled "\
"every <coarse> pixels, and a histogram constructed. This is used to find "\
"the median and semi-interquartile range; the latter is converted to sigma "\
"on the assumption that the histogram is Gaussian; the histogram is clipped "\
"at <clip> sigma, and the median and SIQR recomputed. This SIQR is converted "\
"to <csigma> and returned. You can also get the raw <mean>, <median>, "\
"<sigma>, and <iqr>, and the clipped median, <cmed>. <counts> is the sum of "\
"all counts in the region"

static ftclArgvInfo regStatsFromQuartiles_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegStatsFromQuartiles_hlp},
   {"<region>", FTCL_ARGV_STRING, NULL, NULL, "Region to analyse"},
   {"-coarse", FTCL_ARGV_INT, NULL, NULL, "Coarseness of sampling"},
   {"-clip", FTCL_ARGV_DOUBLE, NULL, NULL, "How many sigma to clip at"},
   {"-mean", FTCL_ARGV_STRING, NULL, NULL, "variable for mean"},
   {"-median", FTCL_ARGV_STRING, NULL, NULL, "variable for median"},
   {"-sigma", FTCL_ARGV_STRING, NULL, NULL, "variable for sigma"},
   {"-iqr", FTCL_ARGV_STRING, NULL, NULL, "variable for iqr"},
   {"-cmedian", FTCL_ARGV_STRING, NULL, NULL, "variable for cmedian"},
   {"-csigma", FTCL_ARGV_STRING, NULL, NULL, "variable for csigma"},
   {"-minpix", FTCL_ARGV_STRING, NULL, NULL, "variable for minpix"},
   {"-maxpix", FTCL_ARGV_STRING, NULL, NULL, "variable for maxpix"},
   {"-counts", FTCL_ARGV_STRING, NULL, NULL, "variable for total counts"},
   {"-q1", FTCL_ARGV_STRING, NULL, NULL, "variable for lower quartile"},
   {"-q3", FTCL_ARGV_STRING, NULL, NULL, "variable for upper quartile"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRegStatsFromQuartiles(
	   ClientData clientData,
	   Tcl_Interp *interp,
	   int argc,
	   char **argv
	   )
{
   int i;
   HANDLE hand;
   REGION *reg;
   float counts = 0;
   PIX minpix = 0, maxpix = 0;
   PIX *pminpix, *pmaxpix;
   float mean = 0, median = 0, sigma = 0, iqr = 0, cmedian = 0, csigma = 0;
   float q1 = 0, q3 = 0;
   float *pmean, *pmedian, *psigma, *piqr, *pcmedian, *pcsigma, *pcounts;
   float *pq1, *pq3;
   char str[30];
   void *vptr;				/* used by shTclHandleExprEval */
   char *regionStr = NULL;		/* Region to analyse */
   int coarse = 10;			/* Coarseness of sampling */
   double clip = -1000;			/* How many sigma to clip at */
   char *meanStr = NULL;		/* variable for mean */
   char *medianStr = NULL;		/* variable for median */
   char *sigmaStr = NULL;		/* variable for sigma */
   char *iqrStr = NULL;			/* variable for iqr */
   char *cmedianStr = NULL;		/* variable for cmedian */
   char *csigmaStr = NULL;		/* variable for csigma */
   char *minpixStr = NULL;		/* variable for minpix */
   char *maxpixStr = NULL;		/* variable for maxpix */
   char *countsStr = NULL;		/* variable for total counts */
   char *q1Str = NULL;			/* variable for lower quartile */
   char *q3Str = NULL;			/* variable for upper quartile */
   
   shErrStackClear();
/*
 * Set default values
 */
   i = 1;
   regStatsFromQuartiles_opts[i++].dst = &regionStr;
   regStatsFromQuartiles_opts[i++].dst = &coarse;
   regStatsFromQuartiles_opts[i++].dst = &clip;
   regStatsFromQuartiles_opts[i++].dst = &meanStr;
   regStatsFromQuartiles_opts[i++].dst = &medianStr;
   regStatsFromQuartiles_opts[i++].dst = &sigmaStr;
   regStatsFromQuartiles_opts[i++].dst = &iqrStr;
   regStatsFromQuartiles_opts[i++].dst = &cmedianStr;
   regStatsFromQuartiles_opts[i++].dst = &csigmaStr;
   regStatsFromQuartiles_opts[i++].dst = &minpixStr;
   regStatsFromQuartiles_opts[i++].dst = &maxpixStr;
   regStatsFromQuartiles_opts[i++].dst = &countsStr;
   regStatsFromQuartiles_opts[i++].dst = &q1Str;
   regStatsFromQuartiles_opts[i++].dst = &q3Str;
   shAssert(regStatsFromQuartiles_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&argc,argv,regStatsFromQuartiles_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * Process the arguments
 */
   if(shTclHandleExprEval(interp,regionStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,
		    "tclRegStatsFromQuartiles: first arg is not a REGION",
		    						   TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(coarse <= 0) {
      Tcl_SetResult(interp,
		    "tclRegStatsFromQuartiles: coarse must be > 0",TCL_STATIC);
      return(TCL_ERROR);
   }
   
   clip = clip;
   pmean =    (meanStr != NULL) ? &mean : NULL;
   pmedian =  (medianStr != NULL) ? &median : NULL;
   psigma =   (sigmaStr != NULL) ? &sigma : NULL;
   piqr =     (iqrStr != NULL) ? &iqr : NULL;
   pcmedian = (cmedianStr != NULL) ? &cmedian : NULL;
   pcsigma =  (csigmaStr != NULL) ? &csigma : NULL;
   pminpix =  (minpixStr != NULL) ? &minpix : NULL;
   pmaxpix =  (maxpixStr != NULL) ? &maxpix : NULL;
   pcounts =  (countsStr != NULL) ? &counts : NULL;
   pq1 =      (q1Str != NULL) ? &q1 : NULL;
   pq3 =      (q3Str != NULL) ? &q3 : NULL;

   if(phRegStatsFromQuartiles(reg, coarse, clip, pmean, pmedian, psigma,
			      piqr, pq1, pq3, pcmedian, pcsigma,
			      pminpix, pmaxpix, pcounts) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   if(pmean != NULL) {
      sprintf(str,"%g",mean);
      if(Tcl_SetVar(interp,meanStr,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return mean",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 
   if(pmedian != NULL) {
      sprintf(str,"%g",median);
      if(Tcl_SetVar(interp,medianStr,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return median",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 
   if(psigma != NULL) {
      sprintf(str,"%g",sigma);
      if(Tcl_SetVar(interp,sigmaStr,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return sigma",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 
   if(piqr != NULL) {
      sprintf(str,"%g",iqr);
      if(Tcl_SetVar(interp,iqrStr,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return iqr",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 
   if(pcmedian != NULL) {
      sprintf(str,"%g",cmedian);
      if(Tcl_SetVar(interp,cmedianStr,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return cmedian",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 
   if(pcsigma != NULL) {
      sprintf(str,"%g",csigma);
      if(Tcl_SetVar(interp,csigmaStr,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return csigma",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 
   if(pminpix != NULL) {
      sprintf(str, PIXFMT, minpix);
      if(Tcl_SetVar(interp,minpixStr,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return minpix",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 
   if(pmaxpix != NULL) {
      sprintf(str, PIXFMT, maxpix);
      if(Tcl_SetVar(interp,maxpixStr,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return maxpix",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 
   if(pcounts != NULL) {
      sprintf(str,"%g",counts);
      if(Tcl_SetVar(interp,countsStr,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return counts",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 
   if(pq1 != NULL) {
      sprintf(str,"%g",q1);
      if(Tcl_SetVar(interp,q1Str,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return q1",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 
   if(pq3 != NULL) {
      sprintf(str,"%g",q3);
      if(Tcl_SetVar(interp,q3Str,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return q3",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 
   
   shTclInterpAppendWithErrStack(interp);
   
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Free all of the allocated memory in a header
 */
static char *tclHdrFreeAll_use =
  "USAGE: hdrFreeAll hdr";
#define tclHdrFreeAll_hlp \
  "Free all of the memory associated with a HDR, but not the HDR itself"\
"See hdrFree to only delete the lines of the header, and use "\
"handleDelFromType to free and then delete handles"

static ftclArgvInfo hdrFreeAll_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclHdrFreeAll_hlp},
   {"<header>", FTCL_ARGV_STRING, NULL, NULL, "Header to free"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclHdrFreeAll(
	      ClientData clientData,
	      Tcl_Interp *interp,
	      int argc,
	      char **argv
	      )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *headerStr = NULL;		/* Header to free */
   
   shErrStackClear();

   i = 1;
   hdrFreeAll_opts[i++].dst = &headerStr;
   shAssert(hdrFreeAll_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&argc,argv,hdrFreeAll_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * Process the arguments
 */
   if(shTclHandleExprEval(interp,headerStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }

   if(hand.type != shTypeGetFromName("HDR")) {
      Tcl_SetResult(interp,"hdrFreeAll: argument is not a HDR",TCL_STATIC);
      return(TCL_ERROR);
   }

   p_shHdrFreeForVec(hand.ptr);

   return(TCL_OK);
}

/*****************************************************************************/

/*
 * <AUTO EXTRACT>
 *
 * TCL VERB: phSqrt
 *
 * DESCRIPTION:
 * replace the value of each pixel in the given REGION with its square
 * root.
 *
 * input and output:
 *   UDATA **rows             pointer to head of data rows
 *
 * input:
 *   int nr                   number of rows in data area
 *   int nc                   number of columns in data area
 *   double rnoise            readout noise of chip
 *   double gain              gain of chip
 *   double quant             set to 0.5 to get normal result
 *
 * output:
 *   none
 *
 * returns:
 *   nothing
 *
 * </AUTO>
 */
/*============================================================================  
**============================================================================
**
** TCL VERB: tclSqrt
**
** DESCRIPTION:
**      Call routine to take sqrt of each pixel in an unsigned short int image.
**
** RETURN VALUES:
**      TCL_OK -   Successful completion.
**      TCL_ERROR -   Failed miserably.
**
** CALLS TO:
**
**============================================================================
*/
/*
 * Input is handle to image, read noise, gain, quantization value
 */

static char *tclSqrt_use =
  "USAGE: phSqrt <region-handle> rnoise gain quant";
static char *tclSqrt_hlp =
  "Take Square root of pixel values of unsigned short ints. This is a VERY " \
"SLOW function that should NOT be called from production code";

static int
tclSqrt(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   REGION *reg = NULL;
   PIX **rows;
   int nr, nc;
   double rnoise, gain, quant;
   double rval;
   int i,j;

   shErrStackClear();

   if(argc != 5) {
      Tcl_SetResult(interp,tclSqrt_use,TCL_STATIC);
      return(TCL_ERROR);
   }

/* Get the region pointer */

   if (shTclAddrGetFromName(interp, argv[1], (void **)&reg, "REGION") != TCL_OK)
        return (TCL_ERROR);


/* Get addr of unsigned int rows */

   if (reg -> type != TYPE_PIX) {
        Tcl_SetResult (interp, "Must be PIX.", TCL_STATIC);
        return (TCL_ERROR);
        }
   rows = reg->ROWS;
   nr = reg->nrow;
   nc = reg->ncol;

/* Get the remaining parameters */

   if (Tcl_GetDouble (interp, argv[2], &rnoise)  != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetDouble (interp, argv[3], &gain)  != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetDouble (interp, argv[4], &quant)  != TCL_OK) return(TCL_ERROR);

/* Done with the handles, etc. Now the action begins */

/* convert readout noise to rms photons */
   rnoise = rnoise * gain;

/* Compute square root of each pixel value */

   for (i=0; i<nr; i++) {
      for (j=0; j<nc; j++) {
           rval = rows[i][j];
           rval = 2. * sqrt (rnoise*rnoise + rval*gain) * quant;
           rows[i][j] = FLT2PIX(rval);
        }
   }

   return(TCL_OK);
}
static char *tclRegIntCopy_use =
  "USAGE: RegIntCopy <out> <in>";
#define tclRegIntCopy_hlp \
  "Copy region <in> to region <out>; the regions must be of identical size; some type conversions are performed.  See also regIntConvert"

static ftclArgvInfo regIntCopy_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntCopy_hlp},
   {"<out>", FTCL_ARGV_STRING, NULL, NULL, "Output region (or \"\" or NULL)"},
   {"<in>", FTCL_ARGV_STRING, NULL, NULL, "Input region"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRegIntCopy(
	      ClientData clientDatag,
	      Tcl_Interp *interp,
	      int ac,
	      char **av
	      )
{
   char name[HANDLE_NAMELEN];
   int i;
   HANDLE hand;
   REGION *in, *out;
   void *vptr;				/* used by shTclHandleExprEval */
   char *outStr = NULL;			/* Output region */
   char *inStr = NULL;			/* Input region */
   int made_out;			/* did we allocate out? */

   shErrStackClear();

   i = 1;
   regIntCopy_opts[i++].dst = &outStr;
   regIntCopy_opts[i++].dst = &inStr;
   shAssert(regIntCopy_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,regIntCopy_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,inStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regIntCopy: "
                    "argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   in = hand.ptr;

   made_out = (strcmp(outStr, "") == 0 || strcmp(outStr, "NULL") == 0) ? 1 : 0;
   if(made_out) {
      out = shRegNew(in->name, in->nrow, in->ncol, in->type);
   } else {
      *name = '\0';

      if(shTclHandleExprEval(interp,outStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_SetResult(interp,"regIntCopy: "
		       "argument is not a REGION",TCL_STATIC);
	 return(TCL_ERROR);
      }
      out = hand.ptr;

      if(in->nrow != out->nrow || in->ncol != out->ncol){
	 Tcl_SetResult(interp,"regIntCopy: "
		       "input and output REGIONs are not the same size",
		       TCL_STATIC);
	 return(TCL_ERROR);
      }
   }
/*
 * work
 */
   if(shRegIntCopy(out,in) != SH_SUCCESS) {
      if(made_out) {
	 shRegDel(out);
      }
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   if(made_out) {
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
   }
   
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * 
 */
static char *tclRegIntConvert_use =
  "USAGE: regIntConvert <reg> -type sval";
#define tclRegIntConvert_hlp \
  "Convert a region to a different type; not all types are supported"

static ftclArgvInfo regIntConvert_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntConvert_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to be converted"},
   {"-type", FTCL_ARGV_STRING, NULL, NULL, "Desired type"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regIntConvert_name "regIntConvert"

static int
tclRegIntConvert(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to be converted */
   REGION *reg;
   REGION *out = NULL;			/* region to return */
   char *typeStr = "U16";		/* Desired type */
   PIXDATATYPE type;			/* desired type of REGION */

   shErrStackClear();

   a_i = 1;
   regIntConvert_opts[a_i++].dst = &regStr;
   regIntConvert_opts[a_i++].dst = &typeStr;
   shAssert(regIntConvert_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regIntConvert_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regIntConvert_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"regIntConvert: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   type = phTypenameToType(typeStr);
   if (type == 0) {
      Tcl_SetResult(interp, "regIntConvert: Unsupported type ", TCL_STATIC);
      Tcl_AppendResult(interp, typeStr, (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   {
      char buff[80];
      sprintf(buff, "Copy of %s", reg->name);
      out = shRegNew(buff, reg->nrow, reg->ncol, type);
   }

   if(shRegIntCopy(out, reg) != SH_SUCCESS) {
      shRegDel(out);
      Tcl_SetResult(interp, "regIntConvert: ", TCL_STATIC);
      Tcl_AppendResult(interp, "Failed to convert region", (char *)NULL);
      return(TCL_ERROR);
   }
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
/*
 * export the centering code to tcl
 */
static char *tclObjectCenterFind_use =
"USAGE: ObjectCenterFind <reg> <row0> <col0> <sky> <sigma>\n"\
"-variance var -gain gain";
#define tclObjectCenterFind_hlp \
  "Find the centre of an object near (<row0>,<col0>) in the region <reg>; the"\
"position is derived by smoothing the region with a Gaussian of s.d. <sigma>"\
"(if >= 0). The variance and gain are only needed if the errors are desired."\
"The answers are returned as a keyed list"

static ftclArgvInfo objectCenterFind_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectCenterFind_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Input region"},
   {"<row0>", FTCL_ARGV_INT, NULL, NULL, "Approximate row centre of object"},
   {"<col0>", FTCL_ARGV_INT, NULL, NULL, "Approximate column centre"},
   {"<sky>", FTCL_ARGV_DOUBLE, NULL, NULL, "Sky level"},
   {"-sigma", FTCL_ARGV_DOUBLE, NULL, NULL,
		       "Standard deviation of smoothing Gaussian (if desired)"},
   {"-variance", FTCL_ARGV_DOUBLE, NULL, NULL, "Background variance"},
   {"-gain", FTCL_ARGV_DOUBLE, NULL, NULL, "CCD's gain"},
   {"-bkgd", FTCL_ARGV_DOUBLE, NULL, NULL,
	    "number of DN to subtract from image; softbias + unsubtracted sky"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjectCenterFind(
		    ClientData clientDatag,
		    Tcl_Interp *interp,
		    int ac,
		    char **av
		    )
{
   int i;
   HANDLE hand;
   float rowc, colc;			/* centre of object */
   float  drowc, dcolc;			/* errors in rowc, colc */
   float peak;				/* peak value */
   float sigsqx, sigsqy;		/* estimates of the width^2 */
   REGION *reg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Input region */
   int row0 = 0;			/* Approximate row centre of object */
   int col0 = 0;			/* Approximate column centre */
   double sky = 0.0;			/* Sky level */
   double sigma = 0.0;			/* Standard deviation of Gaussian */
   double variance = 0.0;		/* Background variance */
   double gain = 1.0;			/* CCD's gain */
   double bkgd = 0.0;			/* number of DN to subtract from image;
					   softbias + unsubtracted sky */

   shErrStackClear();

   i = 1;
   objectCenterFind_opts[i++].dst = &regStr;
   objectCenterFind_opts[i++].dst = &row0;
   objectCenterFind_opts[i++].dst = &col0;
   objectCenterFind_opts[i++].dst = &sky;
   objectCenterFind_opts[i++].dst = &sigma;
   objectCenterFind_opts[i++].dst = &variance;
   objectCenterFind_opts[i++].dst = &gain;
   objectCenterFind_opts[i++].dst = &bkgd;
   shAssert(objectCenterFind_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objectCenterFind_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"objectCenterFind: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;
   if(reg->type != TYPE_PIX) {
      Tcl_SetResult(interp,"objectCenterFind: "
                    "REGION is not of type U16",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   switch (i = phObjectCenterFind(reg,col0, row0, sky, sigma, bkgd,
			      variance, gain, &colc, &rowc, &dcolc, &drowc,
			      &peak, &sigsqx, &sigsqy)) {
    case 0:
      break;
    case -1:
      Tcl_SetResult(interp,"objectCenterFind: "
                    "Input centre isn't close to a maximum",TCL_STATIC);
      return(TCL_ERROR);
    case -2:
      Tcl_SetResult(interp,"objectCenterFind: "
                    "Object is too close to the edge of the frame",TCL_STATIC);
      return(TCL_ERROR);
    case -3:
      Tcl_SetResult(interp,"objectCenterFind: "
                    "object has vanishing second derivative",TCL_STATIC);
      return(TCL_ERROR);
    case -4:
      Tcl_SetResult(interp,"objectCenterFind: "
                    "Object's central pixel (after smoothing) is negative",TCL_STATIC);
      return(TCL_ERROR);
    case -5:
      Tcl_SetResult(interp,"objectCenterFind: "
                    "Object's central pixel (after smoothing) is zero",TCL_STATIC);
      return(TCL_ERROR);
    default:
      shFatal("objectCenterFind: Impossible return value: %d", i);
      break;				/* NOTREACHED */
   }
/*
 * create a keyed list to return values
 */
   {
      char str[100];
      sprintf(str, "rowc \"%.5f\"", rowc);
      Tcl_AppendElement(interp, str);
      sprintf(str, "colc \"%.5f\"", colc);
      Tcl_AppendElement(interp, str);
      sprintf(str, "drowc \"%.5f\"", drowc);
      Tcl_AppendElement(interp, str);
      sprintf(str, "dcolc \"%.5f\"", dcolc);
      Tcl_AppendElement(interp, str);
      sprintf(str, "peak \"%.5f\"", peak);
      Tcl_AppendElement(interp, str);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Find the highest pixel in a region
 */
static char *tclRegIntMaxPixelFind_use =
  "USAGE: regIntMaxPixelFind <reg> -row0 -col0 -row1 -col1 -bkgd dval -rowc sval -colc sval";
#define tclRegIntMaxPixelFind_hlp \
  "Find the highest pixel in a region"

static ftclArgvInfo regIntMaxPixelFind_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntMaxPixelFind_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "The region to be searched"},
   {"-row0", FTCL_ARGV_INT, NULL, NULL, "Starting row"},
   {"-col0", FTCL_ARGV_INT, NULL, NULL, "Starting column"},
   {"-row1", FTCL_ARGV_INT, NULL, NULL, "Ending row"},
   {"-col1", FTCL_ARGV_INT, NULL, NULL, "Ending column"},
   {"-bkgd", FTCL_ARGV_DOUBLE, NULL, NULL,
				    "Background level (needed for centroids)"},
   {"-rowc", FTCL_ARGV_STRING, NULL, NULL,
					  "Variable to hold the row-centroid"},
   {"-colc", FTCL_ARGV_STRING, NULL, NULL,
				       "Variable to hold the column-centroid"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regIntMaxPixelFind_name "regIntMaxPixelFind"

static int
tclRegIntMaxPixelFind(ClientData clientData,
		      Tcl_Interp *interp,
		      int ac,
		      char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* The region to be searched */
   REGION *reg;
   int row0 = 0;			/* Starting row */
   int col0 = 0;			/* Starting column */
   int row1 = 0;			/* Ending row */
   int col1 = 0;			/* Ending column */
   int rpeak, cpeak;			/* position of maximum pixel */
   double bkgd = 0.0;			/* Background level */
   char *rowcStr = NULL;		/* Variable to hold the row-centroid */
   char *colcStr = NULL;		/* Variable to hold the col-centroid */
   float rowc, colc;			/* row/column centroids */
   float *rowc_p, *colc_p;		/* pointers to {row,col}c, or NULL */

   shErrStackClear();

   i = 1;
   regIntMaxPixelFind_opts[i++].dst = &regStr;
   regIntMaxPixelFind_opts[i++].dst = &row0;
   regIntMaxPixelFind_opts[i++].dst = &col0;
   regIntMaxPixelFind_opts[i++].dst = &row1;
   regIntMaxPixelFind_opts[i++].dst = &col1;
   regIntMaxPixelFind_opts[i++].dst = &bkgd;
   regIntMaxPixelFind_opts[i++].dst = &rowcStr;
   regIntMaxPixelFind_opts[i++].dst = &colcStr;
   shAssert(regIntMaxPixelFind_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regIntMaxPixelFind_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regIntMaxPixelFind_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"regIntMaxPixelFind: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;   

   if(rowcStr == NULL && colcStr == NULL) {
      rowc_p = colc_p = NULL;
   } else {
      rowc_p = &rowc; colc_p = &colc;
   }
/*
 * work
 */
   if(phRegIntMaxPixelFind(reg, bkgd, row0, col0, row1, col1,
			   &rpeak, &cpeak, rowc_p, colc_p) < 0) {
      Tcl_AppendResult(interp, "regIntMaxPixelFind: unsupported region type",
		       (char *)NULL);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   if(rowcStr != NULL) {
      char buff[100];
      sprintf(buff, "%g", rowc);
      if(Tcl_SetVar(interp, rowcStr, buff, TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp, "Failed to return rowc", TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   }

   if(colcStr != NULL) {
      char buff[100];
      sprintf(buff, "%g", colc);
      if(Tcl_SetVar(interp, colcStr, buff, TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp, "Failed to return colc", TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   }

   {
      char buff[40];
      sprintf(buff, "%d %d", rpeak, cpeak);
      Tcl_AppendResult(interp, buff, (char *)NULL);
   }
   
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * return a CRC for a file
 */
static char *tclCrcCalcFromFile_use =
  "USAGE: CrcCalcFromFile filename [-nbyte n]";
#define tclCrcCalcFromFile_hlp \
  "Return a file's 16-bit CRC-CCITT using a type-3 Kermit block check.\n"\
"if <n> is specified, only use the first <n> bytes of the file"

static ftclArgvInfo crcCalcFromFile_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCrcCalcFromFile_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL,
					  "The file whose checksum is desired"},
   {"-nbyte", FTCL_ARGV_INT, NULL, NULL, "Maximum number of bytes to consider"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};
#define BUFSIZE 4096

static int
tclCrcCalcFromFile(
		   ClientData clientDatag,
		   Tcl_Interp *interp,
		   int ac,
		   char **av
		   )
{
   int i;
   char buff[BUFSIZE];			/* input buffer */
   long crc;				/* desired CRC */
   FILE *fil;				/* FILE pointer for file in question */
   int nread;				/* number of bytes read */
   char *fileStr = NULL;		/* The file whose checksum is desired*/
   int nbyte = 0;			/* Max. number of bytes to consider */

   shErrStackClear();

   i = 1;
   crcCalcFromFile_opts[i++].dst = &fileStr;
   crcCalcFromFile_opts[i++].dst = &nbyte;
   shAssert(crcCalcFromFile_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,crcCalcFromFile_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments. Note that if nbyte is initially <= 0,
 * we will read the whole file as the termination test is based on nbyte == 0.
 */
   nbyte = (nbyte > 0) ? nbyte : -1;
/*
 * work
 */
   if((fil = fopen(fileStr,"r")) == NULL) {
      shErrStackPushPerror("Can't open %s:",fileStr);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   crc = 0;
   while(nbyte != 0 && (nread = fread(buff,1,BUFSIZE,fil)) > 0) {
      if(nbyte > 0 && nbyte < nread) {
	 nread = nbyte;
      }

      crc = phCrcCalc(crc, buff, nread);

      nbyte -= nread;
   }
   fclose(fil);

   crc &= 0xFFFF;
/*
 * return answer
 */
   sprintf(buff,"0x%04lx",crc);
   Tcl_SetResult(interp,buff,TCL_VOLATILE);
   
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Return the local time as a string
 */
static char *tclCtime_use =
  "USAGE: Ctime [-utc]";
#define tclCtime_hlp \
  "Return the current time as a string (with the \n stripped); "\
"usually return the current time, but with the -utc flag, return UTC"

static ftclArgvInfo ctime_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCtime_hlp},
   {"-utc", FTCL_ARGV_CONSTANT, (void *)1, NULL, "return the UTC time"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclCtime(
	 ClientData clientDatag,
	 Tcl_Interp *interp,
	 int ac,
	 char **av
	 )
{
   int i;
   char buff[26 + 4];			/* ANSI says 26 chars are needed;
					   we may append " UTC" */
   time_t t;
   int utc = 0;				/* return the UTC time */
   
   shErrStackClear();

   i = 1;
   ctime_opts[i++].dst = &utc;
   shAssert(ctime_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,ctime_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * work
 */
   (void)time(&t);
   if(utc) {
      struct tm *tmp = gmtime(&t);
      if(tmp == NULL) {
	 Tcl_SetResult(interp, "UTC is not available", TCL_VOLATILE);
	 return(TCL_ERROR);
      }
      strcpy(buff,asctime(tmp));
   } else {
      strcpy(buff,ctime(&t));
   }
   shAssert(buff[24] == '\n');
   buff[24] = '\0';			/* strip newline */
   if(utc) {
      strcat(buff," UTC");
   }

   Tcl_SetResult(interp, buff, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclRegIntLincom_use =
  "USAGE: regIntLincom reg1 reg2 a b c -intersect -Union";
#define tclRegIntLincom_hlp \
  "Replace reg1 by a + b*reg1 + c*reg2"

static ftclArgvInfo regIntLincom_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntLincom_hlp},
   {"<reg1>", FTCL_ARGV_STRING, NULL, NULL,
		"Region to be replaced by linear combination of reg1 and reg2"},
   {"<reg2>", FTCL_ARGV_STRING, NULL, NULL, "Region to be added to reg2"},
   {"<a>", FTCL_ARGV_DOUBLE, NULL, NULL, "Constant term in linear combination"},
   {"<b>", FTCL_ARGV_DOUBLE, NULL, NULL,
				   "coefficient of reg1 in linear combination"},
   {"<c>", FTCL_ARGV_DOUBLE, NULL, NULL,
				   "coefficient of reg2 in linear combination"},
   {"-intersect", FTCL_ARGV_CONSTANT, (void *)1, NULL,
    "Only calculate linear combination where REGIONs intersect"},
   {"-union", FTCL_ARGV_CONSTANT, (void *)1, NULL,
    "Calculate union of input REGIONs (will rebind reg1's handle)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRegIntLincom(
		ClientData clientDatag,
		Tcl_Interp *interp,
		int ac,
		char **av
		)
{
   char name[HANDLE_NAMELEN];
   int a_i;
   HANDLE hand;
   REGION *reg1, *reg2;
   void *vptr;				/* used by shTclHandleExprEval */
   char *reg1Str = NULL;		/* Region to be replaced by linear combination of reg1 and reg2 */
   char *reg2Str = NULL;		/* Region to be added to reg2 */
   double a = 0.0;			/* Constant term in linear combination */
   double b = 0.0;			/* coefficient of reg1 in linear combination */
   double c = 0.0;			/* coefficient of reg2 in linear combination */
   int intersect = 0;			/* Only calculate linear combination where REGIONs intersect */
   int Union = 0;			/* Calculate union of input REGIONs (will rebind reg1's handle) */

   shErrStackClear();

   a_i = 1;
   regIntLincom_opts[a_i++].dst = &reg1Str;
   regIntLincom_opts[a_i++].dst = &reg2Str;
   regIntLincom_opts[a_i++].dst = &a;
   regIntLincom_opts[a_i++].dst = &b;
   regIntLincom_opts[a_i++].dst = &c;
   regIntLincom_opts[a_i++].dst = &intersect;
   regIntLincom_opts[a_i++].dst = &Union;
   shAssert(regIntLincom_opts[a_i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,regIntLincom_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,reg1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regIntLincom: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg1 = hand.ptr;

   if(*reg2Str == '\0') {
      reg2 = NULL;
      if(c != 0.0) {
	 Tcl_SetResult(interp,"regIntLincom: "
		       "reg2 may only be NULL if c == 0 ",TCL_STATIC);
	 return(TCL_ERROR);
      }
   } else {
      if(shTclHandleExprEval(interp,reg2Str,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_SetResult(interp,"regIntLincom: "
		       "second argument is not a REGION",TCL_STATIC);
	 return(TCL_ERROR);
      }
      reg2 = hand.ptr;
   }
/*
 * work
 */
   if(intersect) {
       if(Union) {
	   Tcl_SetResult(interp, "regionsUnion: Please don't choose -intersect _and_ -union", TCL_STATIC);

	   return(TCL_ERROR);
       }
       if(shRegIntLincom(reg1, reg2, a, b, c, 1) == NULL) {
	   shTclInterpAppendWithErrStack(interp);
	   
	   return(TCL_ERROR);
       }
   } else if(Union) {
       REGION *out = shRegIntLincom(reg1, reg2, a, b, c, 2);
       if(out == NULL) {
	   shTclInterpAppendWithErrStack(interp);
	   
	   return(TCL_ERROR);
       }

       if(out == reg1) {
	   Tcl_SetResult(interp, reg1Str, TCL_VOLATILE);
       } else {
	   /*
	    * We have to delete the handle, but if we simply call p_shTclHandleDel it won't call shRegDel
	    * correctly; so we need to do this ourselves, and set the address to NULL so that noone will
	    * think that we've freed it too many times. Sigh.
	    */
	   {
	       HANDLE *handPtr;
	       if(p_shTclHandleAddrGet(interp, reg1Str, &handPtr) != TCL_OK) {
		   return(TCL_ERROR);
	       }
	       shAssert(handPtr->ptr == reg1);
	       shRegDel(reg1); handPtr->ptr = NULL;
	       p_shTclHandleDel(interp, reg1Str);
	   }
	   /*
	    * Return out
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
       }
   } else {
       if(shRegIntLincom(reg1, reg2, a, b, c, LINCOM_EXACT) == NULL) {
	   shTclInterpAppendWithErrStack(interp);
	   
	   return(TCL_ERROR);
       }
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * set a circular region in a MASK
 */
static char *tclMaskSetFromCircle_use =
  "USAGE: MaskSetFromCircle <mask> <val> <rowc> <colc> <radius>";
#define tclMaskSetFromCircle_hlp \
  "Set the bits in a MASK to val within a circle of radius r, centred" \
"at (rowc, colc)"

static ftclArgvInfo maskSetFromCircle_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMaskSetFromCircle_hlp},
   {"<mask>", FTCL_ARGV_STRING, NULL, NULL, "Mask to set"},
   {"<val>", FTCL_ARGV_INT, NULL, NULL, "Value to set"},
   {"<rowc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Row centre of circle"},
   {"<colc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Column centre of circle"},
   {"<radius>", FTCL_ARGV_DOUBLE, NULL, NULL, "Radius of circle"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclMaskSetFromCircle(
		     ClientData clientDatag,
		     Tcl_Interp *interp,
		     int ac,
		     char **av
		     )
{
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *maskStr = NULL;		/* Mask to set */
   int val = 0;				/* Value to set */
   double rowc = 0.0;			/* Row centre of circle */
   double colc = 0.0;			/* Column centre of circle */
   double radius = 0.0;			/* Radius of circle */

   shErrStackClear();

   if(get_FtclOpts(interp,&ac,av,maskSetFromCircle_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,maskStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("MASK")) {
      Tcl_SetResult(interp,"maskSetFromCircle: "
                    "first argument is not a MASK",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phMaskSetFromCircle(hand.ptr, val, rowc, colc, radius);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Generate a longjmp when a signal is seen
 */
#include "phSignals.h"

void p_phSignalTrap(int sig);

static char *tclSignalTrap_use =
  "USAGE: signalTrap <SIGNAME>";
#define tclSignalTrap_hlp \
  "Setup a signal handler to trap <SIGNAME> (e.g. SIGABRT). When the signal" \
"is seen, setjmp will return the numerical value of the signal"

static ftclArgvInfo signalTrap_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSignalTrap_hlp},
   {"<signal>", FTCL_ARGV_STRING, NULL, NULL, "Signal to trap (e.g. SIGABRT)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclSignalTrap(
	      ClientData clientDatag,
	      Tcl_Interp *interp,
	      int ac,
	      char **av
	      )
{
   int i;
   int sig;				/* the signal to catch */
   char *signalStr = NULL;		/* Signal to trap (e.g. SIGABRT) */
   
   shErrStackClear();

   i = 1;
   signalTrap_opts[i++].dst = &signalStr;
   shAssert(signalTrap_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,signalTrap_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments. Convert signal name to a signal number
 */
   sig = -1;
   for(i = 0; i < 25; i++) {
      const char *signame = Tcl_SignalId(i);
      if(strcmp(signalStr,signame) == 0 ||
	 (strlen(signalStr) > 3 &&
			     strcmp(&signalStr[3],signame) == 0)) { /* got it */
	 sig = i;
	 break;
      }
   }
   if(sig < 0) {
      Tcl_SetResult(interp,"",TCL_STATIC);
      Tcl_AppendResult(interp,"signalTrap: ",signalStr,
		       " is not a valid signal name",(char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   signal(sig, p_phSignalTrap);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Ignore memory inconsistency errors from dervish. Only use this while
 * trying to recover from e.g. aborts. Yes, ONLY THEN!
 */
#if DERVISH_VERSION < 6 || (DERVISH_VERSION == 6 && DERVISH_MINOR_VERSION <= 9)
static void
MemInconsistency_callback(unsigned long long thresh, const SH_MEMORY *memBlock)
{
   volatile int i;

   shAssert(memBlock != NULL);		/* suppress compiler warnings */
   i = thresh;				/* something for a debugger to
					   set a break at */
   shFatal("DERVISH detected error in memory management");
}
#endif

static void
memory_inconsistency(unsigned long long thresh, const SH_MEMORY *memBlock)
{
   volatile int i;
   shAssert(memBlock != NULL);		/* suppress compiler warnings */
   i = thresh;				/* something for a debugger to
					   set a break at */
   shError("error in memory management ignored");
}

static char *tclMemoryErrorIgnore_use =
  "USAGE: memoryErrorIgnore [options]";
#define tclMemoryErrorIgnore_hlp \
  "Make dervish memory errors non-fatal "

static ftclArgvInfo memoryErrorIgnore_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMemoryErrorIgnore_hlp},
   {"-default", FTCL_ARGV_CONSTANT, 0, NULL, "Restore the default handler"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclMemoryErrorIgnore(ClientData clientDatag,
		     Tcl_Interp *interp,
		     int ac,
		     char **av)
{
   void (*callback)(unsigned long long, const SH_MEMORY *); /* callback function */
   int use_default = 0;			/* Restore the default handler */
   int i;

   shErrStackClear();

   i = 1;
   memoryErrorIgnore_opts[i++].dst = &use_default;
   shAssert(memoryErrorIgnore_opts[i].type == FTCL_ARGV_END);
   
   if(get_FtclOpts(interp,&ac,av,memoryErrorIgnore_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(use_default) {
#if DERVISH_VERSION <= 6 && DERVISH_MINOR_VERSION <= 9
      callback = MemInconsistency_callback;
#else
      callback = NULL;
#endif
   } else {
      callback = memory_inconsistency;
   }

   shMemInconsistencyCB(callback);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Set phTimeLeft to -1 after some time has passed. If p_phSignalTimer
 * is true, also raise SIGUSR1. Time runs out at p_phTimeLimit since the Epoch,
 * as defined by time()
 */
#include <sys/times.h>

time_t p_phTimeLimit = 0;		/* time when timer should go off */
volatile int phTimeLeft = 0;		/* time until timer should go off */
int p_phSignalTimer = 0;		/* should we raise a SIGUSR1 when
					   timer expires? */
int p_phTimeType = TIMER_USTIME;	/* what sort of time do we want? */


static char *tclTimerSet_use =
  "USAGE: timerSet [options] time";
#define tclTimerSet_hlp \
  "Set the volatile int phTimeLeft after <time> seconds. "\
"By default, use the sum of user and system CPU time. "\
"If <time> is zero, wait forever. "\
"The read-only TCL variable timeLeft is linked to phTimeLeft"

static ftclArgvInfo timerSet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclTimerSet_hlp},
   {"<time>", FTCL_ARGV_INT, NULL, NULL, "Number of seconds to wait"},
   {"-utime", FTCL_ARGV_CONSTANT, (void *)1, NULL,
						"Use user CPU time for <time>"},
   {"-stime", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					      "Use system CPU time for <time>"},
   {"-etime", FTCL_ARGV_CONSTANT, (void *)1, NULL,
						 "Use elapsed time for <time>"},
   {"-signal", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					 "Raise a SIGUSR1 when time runs out?"},
   {"-setvar", FTCL_ARGV_STRING, NULL, NULL,
					  "TCL variable to link to phTimeLeft"},
   {"-unsetvar", FTCL_ARGV_STRING, NULL, NULL,
					 "Unlink TCL variable from phTimeLeft"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},   
};

static void
handle_sigalrm(int sig)
{
   clock_t t1;				/* current time */
   static int clock_ticks_per_s = -1;	/* clock ticks per second */

   if(clock_ticks_per_s == -1) {
      clock_ticks_per_s = sysconf(_SC_CLK_TCK);
   }

   if(sig != SIGKILL) {			/* really raised by a SIGALRM */
      signal(sig, handle_sigalrm);	/* re-install signal handler */
   }

   if(p_phTimeType == TIMER_ETIME) {
      t1 = time(NULL);
   } else {
      struct tms tms;			/* returned by times() */

      (void)times(&tms);

      if(p_phTimeType == TIMER_STIME) {
	 t1 = tms.tms_stime;
      } else if(p_phTimeType == TIMER_UTIME) {
	 t1 = tms.tms_utime;
      } else {
	 shAssert(p_phTimeType == TIMER_USTIME);
	 t1 = tms.tms_stime + tms.tms_utime;
      }
      t1 = t1/clock_ticks_per_s + 0.5;
   }

   if(sig == SIGKILL) {			/* just setting things */
      p_phTimeLimit = t1 + phTimeLeft;
      return;
   }

   phTimeLeft = p_phTimeLimit - t1;
   if(phTimeLeft <= 0) {
      (void)alarm(0);			/* turn off timer */
      phTimeLeft = -1;			/* i.e. we've expired */
      
      if(p_phSignalTimer) {
	 kill(getpid(), SIGUSR1);
      }
   } else {
      (void)alarm(phTimeLeft);
   }
}

static int
tclTimerSet(ClientData clientDatag,
	    Tcl_Interp *interp,
	    int ac,
	    char **av)
{
   int i;
   int time = 0;			/* Number of seconds to wait */
   int utime = 0;			/* Use user CPU time for <time> */
   int stime = 0;			/* Use system CPU time for <time> */
   int etime = 0;			/* Use elapsed time for <time> */
   int send_signal = 0;			/* Send a SIGUSR1 when time runs out? */
   char *setvar = NULL;			/* TCL variable to link to phTimeLeft */
   char *unsetvar = NULL;		/* unlink TCL variable from phTimeLeft*/

   shErrStackClear();

   i = 1;
   timerSet_opts[i++].dst = &time;
   timerSet_opts[i++].dst = &utime;
   timerSet_opts[i++].dst = &stime;
   timerSet_opts[i++].dst = &etime;
   timerSet_opts[i++].dst = &send_signal;
   timerSet_opts[i++].dst = &setvar;
   timerSet_opts[i++].dst = &unsetvar;
   if(get_FtclOpts(interp,&ac,av,timerSet_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   p_phSignalTimer = send_signal;

   if(etime) {
      p_phTimeType = TIMER_ETIME;
   } else if(stime) {
      p_phTimeType = TIMER_STIME;
   } else if(utime) {
      p_phTimeType = TIMER_UTIME;
   } else {
      p_phTimeType = TIMER_USTIME;
   }
/*
 * Work. This consists of setting a timer and a signal handler, which
 * will in turn set phTimeLeft as desired. Additionally, we may link
 * phTimeLeft to the TCL variable $setvar
 */
   if(setvar != NULL) {
      Tcl_LinkVar(interp, setvar, (char *)&phTimeLeft,
					     TCL_LINK_INT | TCL_LINK_READ_ONLY);
   }
   if(unsetvar != NULL) {
      Tcl_UnlinkVar(interp, unsetvar);
   }

   if(time == 0) {
      p_phTimeLimit = (time_t)0;
      phTimeLeft = 0;
      alarm(0);				/* disable timer */
      return(TCL_OK);
   }

   phTimeLeft = time;
   handle_sigalrm(SIGKILL);		/* set p_phTimeLimit */
   signal(SIGALRM, handle_sigalrm);

   i = alarm(time);
   {
      char buff[20];
      sprintf(buff, "%d", i);
      Tcl_SetResult(interp, buff, TCL_VOLATILE);
   }

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclRegIntShift_use =
  "USAGE: regIntShift <in> <dr> <dc> -out -scr -filtsize";
#define tclRegIntShift_hlp \
  "Shift a region using sinc shifting. Return handle to new region unless -out is specified"

static ftclArgvInfo regIntShift_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntShift_hlp},
   {"<in>", FTCL_ARGV_STRING, NULL, NULL, "Region to shift"},
   {"<dr>", FTCL_ARGV_DOUBLE, NULL, NULL, "Shift in row direction"},
   {"<dc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Shift in column direction"},
   {"-out", FTCL_ARGV_STRING, NULL, NULL, "Output region"},
   {"-scr", FTCL_ARGV_STRING, NULL, NULL, "Scratch region; same size as <in>"},
   {"-filtsize", FTCL_ARGV_INT, NULL, NULL, "Size of sinc filter to use"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regIntShift_name "regIntShift"

static int
tclRegIntShift(ClientData clientData,
	       Tcl_Interp *interp,
	       int ac,
	       char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *inStr = NULL;			/* Region to shift */
   REGION *in;
   double dr = 0.0;			/* Shift in row direction */
   double dc = 0.0;			/* Shift in column direction */
   char *outStr = NULL;			/* Output region */
   REGION *out;
   char *scrStr = NULL;			/* Scratch region; same size as <in> */
   REGION *scr;
   int filtsize = 7;			/* Size of sinc filter to use */

   shErrStackClear();

   i = 1;
   regIntShift_opts[i++].dst = &inStr;
   regIntShift_opts[i++].dst = &dr;
   regIntShift_opts[i++].dst = &dc;
   regIntShift_opts[i++].dst = &outStr;
   regIntShift_opts[i++].dst = &scrStr;
   regIntShift_opts[i++].dst = &filtsize;
   shAssert(regIntShift_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regIntShift_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regIntShift_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,inStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regIntShift: "
                    "argument \"in\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   in = hand.ptr;

   if(in->type != TYPE_PIX) {
      Tcl_SetResult(interp,"regIntShift: "
                    "REGION \"in\" is not of type U16",TCL_STATIC);
      return(TCL_ERROR);
   }

   if(outStr == NULL) {
      out = NULL;
   } else {
      if(shTclHandleExprEval(interp,outStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_SetResult(interp,"regIntShift: "
		       "argument \"out\" is not a REGION",TCL_STATIC);
	 return(TCL_ERROR);
      }
      out = hand.ptr;

      if(out->type != TYPE_PIX) {
	 Tcl_SetResult(interp,"regIntShift: "
		       "REGION \"out\" is not of type U16",TCL_STATIC);
	 return(TCL_ERROR);
      }
      if(out->nrow != in->nrow || out->ncol != in->ncol) {
	 Tcl_SetResult(interp,"regIntShift: "
		       "REGION \"out\" is not same size as \"in\"",TCL_STATIC);
	 return(TCL_ERROR);
      }
   }
   
   if(scrStr == NULL) {
      scr = NULL;
   } else {
      if(shTclHandleExprEval(interp,scrStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_SetResult(interp,"regIntShift: "
		       "argument \"scr\" is not a REGION",TCL_STATIC);
	 return(TCL_ERROR);
      }
      scr = hand.ptr;

      if(scr->type != TYPE_PIX) {
	 Tcl_SetResult(interp,"regIntShift: "
		       "REGION \"scr\" is not of type U16",TCL_STATIC);
	 return(TCL_ERROR);
      }
      if(scr->nrow != in->nrow || scr->ncol != in->ncol) {
	 Tcl_SetResult(interp,"regIntShift: "
		       "REGION \"scr\" is not same size as \"in\"",TCL_STATIC);
	 return(TCL_ERROR);
      }
   }
/*
 * work
 */
   out = phRegIntShift(out, in, scr, filtsize, dr, dc);
/*
 * Return the answer
 */
   if(out == NULL) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   if(outStr == NULL) {
      if(p_shTclHandleNew(interp,name) != TCL_OK) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
      hand.ptr = out;
      hand.type = shTypeGetFromName("REGION");
      
      if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
	 Tcl_SetResult(interp,"can't bind \"name\" to new REGION handle",TCL_STATIC);
	 return(TCL_ERROR);
      }
      
      Tcl_SetResult(interp, name, TCL_VOLATILE);
   }
   
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclPhotoName_use =
  "USAGE: photoName";
#define tclPhotoName_hlp \
  "Return the CVS Name string, as compiled into the binary"

static ftclArgvInfo photoName_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPhotoName_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define photoName_name "photoName"

static int
tclPhotoName(ClientData clientData,
	     Tcl_Interp *interp,
	     int ac,
	     char **av)
{
   int a_i;

   shErrStackClear();

   a_i = 1;
   shAssert(photoName_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, photoName_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     photoName_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   Tcl_SetResult(interp, (char *)phPhotoName(), TCL_STATIC);

   return(TCL_OK);
}


/*****************************************************************************/
static char *tclTchebyFit_use =
  "USAGE: tchebyFit <x> <y> <w> <Ntcheby> -fitx sval";
#define tclTchebyFit_hlp \
  "Fit a Ntcheby-th order Tchebychev polynomial to y(x) and\n" \
   "evaluate at positions given by fitx (x by default)"

static ftclArgvInfo tchebyFit_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclTchebyFit_hlp},
   {"<x>", FTCL_ARGV_STRING, NULL, NULL, "grid of abscissae"},
   {"<y>", FTCL_ARGV_STRING, NULL, NULL, "ordinate array"},
   {"<w>", FTCL_ARGV_STRING, NULL, NULL, "weight array"},
   {"<Ntcheby>", FTCL_ARGV_INT, NULL, NULL, "the order of the Tchebychev fit"},
   {"-fitx", FTCL_ARGV_STRING, NULL, NULL, "evaluate the fit for these abscissae (default: x)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define tchebyFit_name "tchebyFit"

static int
tclTchebyFit(ClientData clientData,
	     Tcl_Interp *interp,
	     int ac,
	     char **av)
{
   char name[HANDLE_NAMELEN];
  HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
  char *xStr = NULL;		/* grid of abscissae */
   VECTOR *x;
  char *yStr = NULL;		/* ordinate array */
   VECTOR *y;
  char *wStr = NULL;		/* weight array */
   VECTOR *w;
  int Ntcheby = 0;		/* the order of the Tchebychev fit */
  char *fitxStr = NULL;		/* evaluate the fit for these abscissae */
  VECTOR *fitx;
  VECTOR_TYPE *fitx_vec;
  int fitx_dimen;
  VECTOR *fity;			/* return results here */

   shErrStackClear();

   a_i = 1;
  tchebyFit_opts[a_i++].dst = &xStr;
  tchebyFit_opts[a_i++].dst = &yStr;
  tchebyFit_opts[a_i++].dst = &wStr;
  tchebyFit_opts[a_i++].dst = &Ntcheby;
  tchebyFit_opts[a_i++].dst = &fitxStr;
   shAssert(tchebyFit_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, tchebyFit_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     tchebyFit_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
 if(shTclHandleExprEval(interp,xStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("VECTOR")) {
      Tcl_AppendResult(interp,"tchebyFit: "
                       "argument \"", xStr, "\" is not a VECTOR",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   x = hand.ptr;

  if(shTclHandleExprEval(interp,yStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("VECTOR")) {
      Tcl_AppendResult(interp,"tchebyFit: "
                       "argument \"", yStr, "\" is not a VECTOR",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   y = hand.ptr;

  if(shTclHandleExprEval(interp,wStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("VECTOR")) {
      Tcl_AppendResult(interp,"tchebyFit: "
                       "argument \"", wStr, "\" is not a VECTOR",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   w = hand.ptr;

  if(fitxStr != NULL) {
    if(shTclHandleExprEval(interp,fitxStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
    }
    if(hand.type != shTypeGetFromName("VECTOR")) {
      Tcl_AppendResult(interp,"tchebyFit: "
                       "argument \"", fitxStr, "\" is not a VECTOR",
                       (char *)NULL);
      return(TCL_ERROR);
    }
    fitx = hand.ptr;
    fitx_vec = fitx->vec; fitx_dimen = fitx->dimen;
  } else {
    fitx_vec = x->vec; fitx_dimen = x->dimen;
  }
/*
 * work
 */
  if(x->dimen != y->dimen || x->dimen != w->dimen) {
    Tcl_SetResult(interp, "tchebyFit: "
		  "all of x, y, w must have the same dimension ", TCL_STATIC);
      return(TCL_ERROR);
  }

  fity = shVectorNew(fitx_dimen);

  phTchebyfit(x->vec, y->vec, w->vec, x->dimen,Ntcheby,
	    fitx_vec, fity->vec, fitx_dimen);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = fity;
   hand.type = shTypeGetFromName("VECTOR");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"fity\" to new VECTOR handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
  
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclPhGenericChainDestroy_use =
  "USAGE: phGenericChainDestroy <chain>";
#define tclPhGenericChainDestroy_hlp \
  "Destroy a generic chain.  Assumes that shFree frees complete element"

static ftclArgvInfo phGenericChainDestroy_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPhGenericChainDestroy_hlp},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "Chain to destroy"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define phGenericChainDestroy_name "phGenericChainDestroy"

static int
tclPhGenericChainDestroy(ClientData clientData,
			 Tcl_Interp *interp,
			 int ac,
			 char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *chainStr = NULL;		/* Chain to destroy */
   CHAIN *chain;

   shErrStackClear();

   a_i = 1;
   phGenericChainDestroy_opts[a_i++].dst = &chainStr;
   shAssert(phGenericChainDestroy_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, phGenericChainDestroy_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     phGenericChainDestroy_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,chainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"phGenericChainDestroy: "
                       "argument \"", chainStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   chain = hand.ptr;
/*
 * work
 */
   chain->type = shTypeGetFromName("INT");
   if(shChainDestroy(chain, shFree) != 0) {
      Tcl_SetResult(interp, "phGenericChainDestroy: failed to destroy chain",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   p_shTclHandleDel(interp,chainStr);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclRegIntLogand_use =
  "USAGE: regIntLogand <reg> <mask>";
#define tclRegIntLogand_hlp \
  "Return the logical AND of a region with a mask"

static ftclArgvInfo regIntLogand_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntLogand_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to be ANDed"},
   {"<mask>", FTCL_ARGV_INT, NULL, NULL, "Integer to be ANDed into region"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regIntLogand_name "regIntLogand"

static int
tclRegIntLogand(ClientData clientData,
		Tcl_Interp *interp,
		int ac,
		char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to be ANDed */
   REGION *reg;
   int mask = 0;			/* Integer to be ANDed into region */

   shErrStackClear();

   a_i = 1;
   regIntLogand_opts[a_i++].dst = &regStr;
   regIntLogand_opts[a_i++].dst = &mask;
   shAssert(regIntLogand_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regIntLogand_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regIntLogand_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"regIntLogand: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;
/*
 * work
 */
   if(shRegIntLogand(reg, mask) != SH_SUCCESS) {
      return(TCL_ERROR);
   }

   Tcl_AppendResult(interp, regStr, (char *)NULL);
   return(TCL_OK);
}

/************************************************************************************************************/
static char *tclAtFilternamesReset_use =
  "USAGE: atFilternamesReset <filternames>";
#define tclAtFilternamesReset_hlp \
  "Change the value of atFilternames (not for regular SDSS processing!)"

static ftclArgvInfo atFilternamesReset_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclAtFilternamesReset_hlp},
   {"<filternames>", FTCL_ARGV_STRING, NULL, NULL, "New list of filternames"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define atFilternamesReset_name "atFilternamesReset"

static int
tclAtFilternamesReset(ClientData clientData,
		      Tcl_Interp *interp,
		      int ac,
		      char **av)
{
   int a_i;
   static char filternames_s[AT_NBAND + 4 + 1]; /* static storage for new value */
   char *filternames = NULL;		/* New list of filternames */
   int i, j;

   shErrStackClear();

   a_i = 1;
   atFilternamesReset_opts[a_i++].dst = &filternames;
   shAssert(atFilternamesReset_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, atFilternamesReset_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     atFilternamesReset_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   for(i = j = 0; filternames[i] != '\0'; i++) {
       if(filternames[i] != ' ') {
	   filternames[j++] = filternames[i];
       }
   }
   while(j < AT_NBAND + 4) {
       filternames[j++] = '\a';
   }
   filternames[j] = '\0';

   if(strlen(filternames) > AT_NBAND + 4) {
      Tcl_SetResult(interp, "atFilternamesReset: new value is too long", TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_AppendResult(interp, atFilternames, (char *)NULL); /* return old value */

   strncpy(filternames_s, filternames, AT_NBAND + 4); /* save non-volatile copy */
   filternames_s[AT_NBAND + 4] = '\0';
   
   atFilternames = filternames_s;

   return(TCL_OK);
}

/************************************************************************************************************/

static char *tclRegIntClipValInObjmask_use =
  "USAGE: regIntClipValInObjmask <reg> <objmask> <val> -drow ival -dcol ival";
#define tclRegIntClipValInObjmask_hlp \
  "Clip reg's pixels that lie in objmask to be <= val"

static ftclArgvInfo regIntClipValInObjmask_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntClipValInObjmask_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region whose values should be clipped"},
   {"<objmask>", FTCL_ARGV_STRING, NULL, NULL, "Objmask defining clipregion"},
   {"<val>", FTCL_ARGV_INT, NULL, NULL, "Clip pixels to lie at or below this value"},
   {"-drow", FTCL_ARGV_INT, NULL, NULL, "Shift objmask's origin by (drow, dcol)"},
   {"-dcol", FTCL_ARGV_INT, NULL, NULL, "Shift objmask's origin by (drow, dcol)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regIntClipValInObjmask_name "regIntClipValInObjmask"

static int
tclRegIntClipValInObjmask(ClientData clientData,
			  Tcl_Interp *interp,
			  int ac,
			  char **av)
{
    HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region whose values should be clipped */
   REGION *reg;
   char *objmaskStr = NULL;		/* Objmask defining clipregion */
   OBJMASK *objmask;
   int val = 0;				/* Clip pixels to lie at or below this value */
   int drow = 0;			/* Shift objmask's origin by (drow, dcol) */
   int dcol = 0;			/* Shift objmask's origin by (drow, dcol) */

   shErrStackClear();

   a_i = 1;
   regIntClipValInObjmask_opts[a_i++].dst = &regStr;
   regIntClipValInObjmask_opts[a_i++].dst = &objmaskStr;
   regIntClipValInObjmask_opts[a_i++].dst = &val;
   regIntClipValInObjmask_opts[a_i++].dst = &drow;
   regIntClipValInObjmask_opts[a_i++].dst = &dcol;
   shAssert(regIntClipValInObjmask_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regIntClipValInObjmask_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regIntClipValInObjmask_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"regIntClipValInObjmask: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,objmaskStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_AppendResult(interp,"regIntClipValInObjmask: "
                       "argument \"", objmaskStr, "\" is not an OBJMASK",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objmask = hand.ptr;
/*
 * work
 */
   phRegIntClipValInObjmask(reg, objmask, drow, dcol, val);

   return(TCL_OK);
}

/************************************************************************************************************/

static char *tclRegExtrapolate_use =
  "USAGE: regExtrapolate <out> <in>";
#define tclRegExtrapolate_hlp \
  "Extrapolate a region, returning a region with a row/column added to the right/top"

static ftclArgvInfo regExtrapolate_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegExtrapolate_hlp},
   {"<out>", FTCL_ARGV_STRING, NULL, NULL, "Output REGION (or \"\")"},
   {"<in>", FTCL_ARGV_STRING, NULL, NULL, "Input region"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regExtrapolate_name "regExtrapolate"

static int
tclRegExtrapolate(ClientData clientData,
		  Tcl_Interp *interp,
		  int ac,
		  char **av)
{
   char name[HANDLE_NAMELEN];
    HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *outStr = NULL;			/* Output REGION (or \"\") */
   REGION *out;
   char *inStr = NULL;			/* Input region */
   REGION *in;

   shErrStackClear();

   a_i = 1;
   regExtrapolate_opts[a_i++].dst = &outStr;
   regExtrapolate_opts[a_i++].dst = &inStr;
   shAssert(regExtrapolate_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regExtrapolate_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regExtrapolate_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(strcmp(outStr, "") == 0 || strcmp(outStr, "NULL") == 0) {
       out = NULL;
   } else {
       if(shTclHandleExprEval(interp,outStr,&hand,&vptr) != TCL_OK) {
	   return(TCL_ERROR);
       }
       if(hand.type != shTypeGetFromName("REGION")) {
	   Tcl_AppendResult(interp,"regExtrapolate: "
			    "argument \"", outStr, "\" is not a REGION",
			    (char *)NULL);
	   return(TCL_ERROR);
       }
       out = hand.ptr;
   }

   if(shTclHandleExprEval(interp,inStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"regExtrapolate: "
                       "argument \"", inStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   in = hand.ptr;
/*
 * work
 */
   out = phRegionExtrapolate(out, in);
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
/*
 * Parse a string such as 123, 100k, 200M, or 3.5G, returning a status
 * and the value via the sizep argument
 */
int
parseSizeStr(const char *sizeStr,	/* string */
	     unsigned long long *sizep)	/* desired value */
{
    int status = 0;			/* OK */
    const int len = strlen(sizeStr);
    const char last_char = sizeStr[(len > 1) ? len - 1 : '\0'];

    if (strchr(sizeStr, '.')) {
	double size = atof(sizeStr);
	
	if (isdigit(last_char)) {
	    ;
	} else if (last_char == 'k') {
	    size *= 1024;
	} else if (last_char == 'M') {
	    size *= 1024*1024;
	} else if (last_char == 'G') {
	    size *= 1024*1024*1024;
	} else {
	    status = -1;		/* Bad */
	}

	*sizep = (unsigned long long)(size + 0.5);
	return status;			/* OK */
    } else {
	unsigned long long size = atoll(sizeStr);
	if (isdigit(last_char)) {
	    ;
	} else if (last_char == 'k') {
	    size *= 1024;
	} else if (last_char == 'M') {
	    size *= 1024*1024;
	} else if (last_char == 'G') {
	    size *= 1024*1024*1024;
	} else {
	    status = -1;		/* Bad */
	}

	*sizep = size;
	return status;			/* OK */
    }
}

static char *tclStrategicMemoryReserveSet_use =
  "USAGE: strategicMemoryReserveSet <size>";
#define tclStrategicMemoryReserveSet_hlp \
  "Reserve a piece of memory to be released when we run out"

static ftclArgvInfo strategicMemoryReserveSet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclStrategicMemoryReserveSet_hlp},
   {"<size>", FTCL_ARGV_STRING, NULL, NULL, "Number of bytes to be reserved (end in k or M if desired)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define strategicMemoryReserveSet_name "strategicMemoryReserveSet"

static int
tclStrategicMemoryReserveSet(ClientData clientData,
			     Tcl_Interp *interp,
			     int ac,
			     char **av)
{
   int a_i;
   char *sizeStr = NULL;		/* Number of bytes to be reserved (end in k or M if desired) */
   unsigned long long size = 0;			/* number of bytes to be reserved */

   shErrStackClear();

   a_i = 1;
   strategicMemoryReserveSet_opts[a_i++].dst = &sizeStr;
   shAssert(strategicMemoryReserveSet_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, strategicMemoryReserveSet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     strategicMemoryReserveSet_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if (parseSizeStr(sizeStr, &size) < 0) {
       Tcl_SetResult(interp, "strategicMemoryReserveSet: unrecognised size: ", TCL_STATIC);
       Tcl_AppendResult(interp, sizeStr, (char *)NULL);
       shTclInterpAppendWithErrStack(interp);
       return(TCL_ERROR);
   }
/*
 * work
 */
   if (!phStrategicMemoryReserveSet(size)) {
       static char str[] = "Unable to reserve XXXXXXXXXXXXXX bytes";

       Tcl_SetResult(interp, "strategicMemoryReserveSet: ", TCL_STATIC);
       sprintf(str, "Unable to reserve %llu bytes", size);
       Tcl_AppendResult(interp, str, (char *)NULL);
       shTclInterpAppendWithErrStack(interp);
       return(TCL_ERROR);
   }

   return(TCL_OK);
}


static char *tclStrategicMemoryReserveIsEmpty_use =
  "USAGE: strategicMemoryReserveIsEmpty";
#define tclStrategicMemoryReserveIsEmpty_hlp \
  "Return true iff we are using the emergency memory reserve"

static ftclArgvInfo strategicMemoryReserveIsEmpty_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclStrategicMemoryReserveIsEmpty_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define strategicMemoryReserveIsEmpty_name "strategicMemoryReserveIsEmpty"

static int
tclStrategicMemoryReserveIsEmpty(ClientData clientData,
	       Tcl_Interp *interp,
	       int ac,
	       char **av)
{
   int a_i;

   shErrStackClear();

   a_i = 1;
   shAssert(strategicMemoryReserveIsEmpty_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, strategicMemoryReserveIsEmpty_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     strategicMemoryReserveIsEmpty_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   {
       static char val[3];
       sprintf(val, "%1d", phStrategicMemoryReserveIsEmpty());
   
       Tcl_AppendResult(interp, val, (char *)NULL);
   }
   
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclMemoryMaxSet_use =
  "USAGE: memoryMaxSet <size>";
#define tclMemoryMaxSet_hlp \
  "Use setrlimit() to set photo's maximum allowed memory footprint"

static ftclArgvInfo memoryMaxSet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMemoryMaxSet_hlp},
   {"<size>", FTCL_ARGV_STRING, NULL, NULL, "Maximum allowable memory size; may end in k or M"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define memoryMaxSet_name "memoryMaxSet"

static int
tclMemoryMaxSet(ClientData clientData,
		Tcl_Interp *interp,
		int ac,
		char **av)
{
   int a_i;
   char *sizeStr = NULL;		/* Maximum allowable memory size; may end in k or M */
   unsigned long long size;

   shErrStackClear();

   a_i = 1;
   memoryMaxSet_opts[a_i++].dst = &sizeStr;
   shAssert(memoryMaxSet_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, memoryMaxSet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     memoryMaxSet_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if (parseSizeStr(sizeStr, &size) < 0) {
       Tcl_SetResult(interp, "maxMemorySet: unrecognised size: ", TCL_STATIC);
       Tcl_AppendResult(interp, sizeStr, (char *)NULL);
       shTclInterpAppendWithErrStack(interp);
       return(TCL_ERROR);
   }
/*
 * work
 */
   {
       struct rlimit rlp;

       if (getrlimit(RLIMIT_AS, &rlp) < 0) {
	   Tcl_SetResult(interp, "maxMemorySet: getrlimit failed: ", TCL_STATIC);
	   Tcl_AppendResult(interp, strerror(errno), (char *)NULL);
	   shTclInterpAppendWithErrStack(interp);
	   return(TCL_ERROR);
       }

       if (size == 0) {			/* remove such restrictions as we can */
	   rlp.rlim_cur = rlp.rlim_max;
       } else {
	   if (size > rlp.rlim_max) {
	       Tcl_SetResult(interp, "maxMemorySet: attempt to set RLIMIT_AS to ", TCL_STATIC);
	       Tcl_AppendResult(interp, sizeStr, " (> hard limit)", (char *)NULL);
	       shTclInterpAppendWithErrStack(interp);
	       return(TCL_ERROR);
	   }
	   rlp.rlim_cur = size;		/* Set the current (soft) limit */
       }

       if (setrlimit(RLIMIT_AS, &rlp) < 0) {
	   Tcl_SetResult(interp, "maxMemorySet: setrlimit failed: ", TCL_STATIC);
	   Tcl_AppendResult(interp, strerror(errno), (char *)NULL);
	   shTclInterpAppendWithErrStack(interp);
	   return(TCL_ERROR);
       }
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclDervishDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"listDiff",
		(Tcl_CmdProc *)tclListDiff, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclListDiff_hlp, tclListDiff_use);

   shTclDeclare(interp,"regClear",
		(Tcl_CmdProc *)tclRegClear, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegClear_hlp, tclRegClear_use);

   shTclDeclare(interp,"regIntSetVal",
		(Tcl_CmdProc *)tclRegIntSetVal, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntSetVal_hlp, tclRegIntSetVal_use);

   shTclDeclare(interp,"regIntConstAdd",
		(Tcl_CmdProc *)tclRegIntConstAdd, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntConstAdd_hlp, tclRegIntConstAdd_use);

   shTclDeclare(interp,"regStatsFromQuartiles",
		(Tcl_CmdProc *)tclRegStatsFromQuartiles, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegStatsFromQuartiles_hlp,
		tclRegStatsFromQuartiles_use);

   shTclDeclare(interp,"hdrFreeAll",
		(Tcl_CmdProc *)tclHdrFreeAll, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclHdrFreeAll_hlp,
		tclHdrFreeAll_use);

   shTclDeclare(interp,"phSqrt", 
                (Tcl_CmdProc *)tclSqrt,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL, 
                module, tclSqrt_hlp, tclSqrt_use);

   shTclDeclare(interp,"maskClear",
		(Tcl_CmdProc *)tclMaskClear, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMaskClear_hlp,
		tclMaskClear_use);

   shTclDeclare(interp,"regIntGaussianAdd",
		(Tcl_CmdProc *)tclRegIntGaussianAdd, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntGaussianAdd_hlp,
		tclRegIntGaussianAdd_use);

   shTclDeclare(interp,"regIntCopy",
		(Tcl_CmdProc *)tclRegIntCopy, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntCopy_hlp,
		tclRegIntCopy_use);

   shTclDeclare(interp,"objectCenterFind",
		(Tcl_CmdProc *)tclObjectCenterFind, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectCenterFind_hlp,
		tclObjectCenterFind_use);

   shTclDeclare(interp,"regIntNoiseAdd",
		(Tcl_CmdProc *)tclRegIntNoiseAdd, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntNoiseAdd_hlp,
		tclRegIntNoiseAdd_use);

   shTclDeclare(interp,"crcCalcFromFile",
		(Tcl_CmdProc *)tclCrcCalcFromFile, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCrcCalcFromFile_hlp,
		tclCrcCalcFromFile_use);

   shTclDeclare(interp,"ctime",
		(Tcl_CmdProc *)tclCtime, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCtime_hlp,
		tclCtime_use);

   shTclDeclare(interp,"regIntLincom",
		(Tcl_CmdProc *)tclRegIntLincom, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntLincom_hlp,
		tclRegIntLincom_use);

   shTclDeclare(interp,"maskSetFromCircle",
		(Tcl_CmdProc *)tclMaskSetFromCircle, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMaskSetFromCircle_hlp,
		tclMaskSetFromCircle_use);

   shTclDeclare(interp,"signalTrap",
		(Tcl_CmdProc *)tclSignalTrap, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSignalTrap_hlp,
		tclSignalTrap_use);

   shTclDeclare(interp,"timerSet",
		(Tcl_CmdProc *)tclTimerSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclTimerSet_hlp,
		tclTimerSet_use);

   shTclDeclare(interp,"memoryErrorIgnore",
		(Tcl_CmdProc *)tclMemoryErrorIgnore, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMemoryErrorIgnore_hlp,
		tclMemoryErrorIgnore_use);

   shTclDeclare(interp,regIntShift_name,
		(Tcl_CmdProc *)tclRegIntShift, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntShift_hlp,
		tclRegIntShift_use);

   shTclDeclare(interp,regIntMaxPixelFind_name,
		(Tcl_CmdProc *)tclRegIntMaxPixelFind, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntMaxPixelFind_hlp,
		tclRegIntMaxPixelFind_use);

   shTclDeclare(interp,photoName_name,
		(Tcl_CmdProc *)tclPhotoName, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPhotoName_hlp,
		tclPhotoName_use);

   shTclDeclare(interp,regIntConvert_name,
		(Tcl_CmdProc *)tclRegIntConvert, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntConvert_hlp,
		tclRegIntConvert_use);

   shTclDeclare(interp,tchebyFit_name,
		(Tcl_CmdProc *)tclTchebyFit, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclTchebyFit_hlp,
		tclTchebyFit_use);


   shTclDeclare(interp,phGenericChainDestroy_name,
		(Tcl_CmdProc *)tclPhGenericChainDestroy, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPhGenericChainDestroy_hlp,
		tclPhGenericChainDestroy_use);

   shTclDeclare(interp,regIntLogand_name,
		(Tcl_CmdProc *)tclRegIntLogand, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntLogand_hlp,
		tclRegIntLogand_use);

   shTclDeclare(interp,atFilternamesReset_name,
		(Tcl_CmdProc *)tclAtFilternamesReset, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclAtFilternamesReset_hlp,
		tclAtFilternamesReset_use);

   shTclDeclare(interp,regIntClipValInObjmask_name,
		(Tcl_CmdProc *)tclRegIntClipValInObjmask, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntClipValInObjmask_hlp,
		tclRegIntClipValInObjmask_use);

   shTclDeclare(interp,regExtrapolate_name,
		(Tcl_CmdProc *)tclRegExtrapolate, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegExtrapolate_hlp,
		tclRegExtrapolate_use);

   shTclDeclare(interp,strategicMemoryReserveIsEmpty_name,
		(Tcl_CmdProc *)tclStrategicMemoryReserveIsEmpty, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclStrategicMemoryReserveIsEmpty_hlp,
		tclStrategicMemoryReserveIsEmpty_use);

   shTclDeclare(interp,strategicMemoryReserveSet_name,
		(Tcl_CmdProc *)tclStrategicMemoryReserveSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclStrategicMemoryReserveSet_hlp,
		tclStrategicMemoryReserveSet_use);

   shTclDeclare(interp,memoryMaxSet_name,
		(Tcl_CmdProc *)tclMemoryMaxSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMemoryMaxSet_hlp,
		tclMemoryMaxSet_use);

   shTclDeclare(interp,regIntConstMult_name,
		(Tcl_CmdProc *)tclRegIntConstMult, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntConstMult_hlp,
		tclRegIntConstMult_use);
}
