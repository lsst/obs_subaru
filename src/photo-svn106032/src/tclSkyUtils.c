/*
 * Export BINREGION, fast median finding, smoothing, and subtracting code
 * to TCL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phUtils.h"
#include "phSkyUtils.h"

static char *module = "phTclSkyUtils";	/* name of this set of code */

/*****************************************************************************/
/*
 * BINREGION utilities
 */
static char *tclBinregionNew_use =
  "USAGE: BinregionNew ";
#define tclBinregionNew_hlp \
  "Make a new BINREGION"

static ftclArgvInfo binregionNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclBinregionNew_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclBinregionNew(
		ClientData clientDatag,
		Tcl_Interp *interp,
		int ac,
		char **av
		)
{
   BINREGION *br;			/* the new BINREGION */
   char name[HANDLE_NAMELEN];
   HANDLE hand;

   shErrStackClear();

   if(get_FtclOpts(interp,&ac,av,binregionNew_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * work
 */
   br = phBinregionNew();
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = br;
   hand.type = shTypeGetFromName("BINREGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new BINREGION handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

static char *tclBinregionDel_use =
  "USAGE: BinregionDel <binregion>";
#define tclBinregionDel_hlp \
  "Delete a BINREGION"

static ftclArgvInfo binregionDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclBinregionDel_hlp},
   {"<binregion>", FTCL_ARGV_STRING, NULL, NULL, "The BINREGION to delete"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclBinregionDel(
		ClientData clientDatag,
		Tcl_Interp *interp,
		int ac,
		char **av
		)
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *binregionStr = NULL;		/* The BINREGION to delete */

   shErrStackClear();

   i = 1;
   binregionDel_opts[i++].dst = &binregionStr;
   shAssert(binregionDel_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,binregionDel_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,binregionStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("BINREGION")) {
      Tcl_SetResult(interp,"binregionDel: "
                    "argument is not a BINREGION",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   p_shTclHandleDel(interp,binregionStr);
   phBinregionDel(hand.ptr);

   return(TCL_OK);
}

static char *tclBinregionNewFromConst_use =
  "USAGE: BinregionNewFromConst val [options]";
#define tclBinregionNewFromConst_hlp \
  "Create a 1-point BINREGION and set it to <val>"

static ftclArgvInfo binregionNewFromConst_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclBinregionNewFromConst_hlp},
   {"<val>", FTCL_ARGV_DOUBLE, NULL, NULL, "The value for the BINREGION"},
   {"-nrow", FTCL_ARGV_INT, NULL, NULL, "number of rows (default: 1)"},
   {"-ncol", FTCL_ARGV_INT, NULL, NULL, "number of columns (default: 1)"},
   {"-bin_row", FTCL_ARGV_INT, NULL, NULL, "Amount to bin the rows (default : 1)"},
   {"-bin_col", FTCL_ARGV_INT, NULL, NULL, "Amount to bin the columns (default : 1)"},
   {"-shift", FTCL_ARGV_INT, NULL, NULL, "How much to shift values (default: 5)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclBinregionNewFromConst(
			 ClientData clientDatag,
			 Tcl_Interp *interp,
			 int ac,
			 char **av
			 )
{
   int i;
   BINREGION *br;
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   double val = 0.0;			/* The value for the BINREGION */
   int nrow = 1;			/* number of rows */
   int ncol = 1;			/* number of columns */
   int bin_row = 1;			/* Amount to bin the rows */
   int bin_col = 1;			/* Amount to bin the columns */
   int shift = 5;			/* How much to shift values */

   shErrStackClear();

   i = 1;
   binregionNewFromConst_opts[i++].dst = &val;
   binregionNewFromConst_opts[i++].dst = &nrow;
   binregionNewFromConst_opts[i++].dst = &ncol;
   binregionNewFromConst_opts[i++].dst = &bin_row;
   binregionNewFromConst_opts[i++].dst = &bin_col;
   binregionNewFromConst_opts[i++].dst = &shift;
   shAssert(binregionNewFromConst_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,binregionNewFromConst_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * work
 */
   br = phBinregionNewFromConst(val,nrow,ncol,bin_row,bin_col,shift);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = br;
   hand.type = shTypeGetFromName("BINREGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new BINREGION handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);


   return(TCL_OK);
}

/*****************************************************************************/

static char *tclMedianSky_use =
  "USAGE: MedianSky <region> <filt_c> <filt_r> [-coarse #] [-shift #]";
#define tclMedianSky_hlp \
  "Median filter a U16 <region> with a filter of size <filt_c>x<filt_r> \
The return value is a BINREGION containing the median values shifted up by \
<SHIFT> (default: 5); if <shift> == 0, it's a U16 region, otherwise it's \
S32. The image is first coarsely sampled on a <coarse>x<coarse> grid to \
determine the rough level and range of pixel values (default: 10x10) \
<filt_r> is the number of columns to be used in the row direction \
(i.e. it's the number of _columns_), and filt_c is the dimension of the \
filter along the columns (it's the number of _rows_). \
\
If min/maxval are supplied, the histograms used to find the sky will only \
contain values in the range [minval,maxval].  If minval or maxval is -ve \
it will be ignored; if they are both 0 then the full range of values present \
in the region will be used. \
"

static ftclArgvInfo medianSky_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMedianSky_hlp},
   {"<in>", FTCL_ARGV_STRING, NULL, NULL, "input region"},
   {"<filt_c>", FTCL_ARGV_INT, NULL, NULL, "column size of filter"},
   {"<filt_r>", FTCL_ARGV_INT, NULL, NULL, "row size of filter"},
   {"-coarse", FTCL_ARGV_INT, NULL, NULL, "How coarsly to sample region"},
   {"-shift", FTCL_ARGV_INT, NULL, NULL, "How much to shift output"},
   {"-nocenter", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					       "Don't shift to pixel centres?"},
   {"-skysig", FTCL_ARGV_STRING, NULL, NULL,
				  "A BINREGION to return the error in the sky"},
   {"-use_mean", FTCL_ARGV_CONSTANT, (void *)1, NULL, "use mean _NOT_ median"},
   {"-minval", FTCL_ARGV_INT, NULL, NULL, "minimum pixel value in histograms"},
   {"-maxval", FTCL_ARGV_INT, NULL, NULL, "maximum pixel value in histograms"},
   {"-S32", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Force output region to be S32"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclMedianSky(
	     ClientData clientDatag,
	     Tcl_Interp *interp,
	     int ac,
	     char **av
	     )
{
   int i;
   HANDLE hand;
   char name[HANDLE_NAMELEN];
   REGION *in;				/* input and ... */
   BINREGION *out;			/*           output regions */
   BINREGION *skysig;			/* BINREGION for sky sigma */
   void *vptr;				/* used by shTclHandleExprEval */
   char *inStr = NULL;			/* input region */
   int filt_c = 0;			/* column size of filter */
   int filt_r = 0;			/* row size of filter */
   int coarse = 10;			/* How coarsly to sample region */
   int shift = 5;			/* How much to shift output */
   int nocenter = 0;			/* Don't shift to pixel centres? */
   char *skysigStr = NULL;		/* A BINREGION to return the sky error*/
   int use_mean = 0;			/* use mean _NOT_ median */
   int minval = -1, maxval = -1;	/* min/max values for histograms */
   int useS32 = 0;			/* was -S32 specified? */
   PIXDATATYPE regType = 0;		/* Type of output REGION (0 => guess based on shift) */

   shErrStackClear();

   i = 1;
   medianSky_opts[i++].dst = &inStr;
   medianSky_opts[i++].dst = &filt_c;
   medianSky_opts[i++].dst = &filt_r;
   medianSky_opts[i++].dst = &coarse;
   medianSky_opts[i++].dst = &shift;
   medianSky_opts[i++].dst = &nocenter;
   medianSky_opts[i++].dst = &skysigStr;
   medianSky_opts[i++].dst = &use_mean;
   medianSky_opts[i++].dst = &minval;
   medianSky_opts[i++].dst = &maxval;
   medianSky_opts[i++].dst = &useS32;
   shAssert(medianSky_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,medianSky_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Process results of argument parsing
 */
   if(shTclHandleExprEval(interp,inStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }

   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"medianSky: argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   in = hand.ptr;
   if(in->type != TYPE_PIX) {
      Tcl_SetResult(interp,"medianSky: region is not U16",TCL_STATIC);
      return(TCL_ERROR);
   }

   if(skysigStr == NULL) {
      skysig = NULL;
   } else {
      if(shTclHandleExprEval(interp,skysigStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("BINREGION")) {
	 Tcl_SetResult(interp,"medianSky: "
		       "-skysig argument is not a BINREGION",TCL_STATIC);
	 return(TCL_ERROR);
      }
      skysig = hand.ptr;
   }

   if (useS32) {
       regType = TYPE_S32;
   }
/*
 * Work
 */
   out = phMedianSmooth(in,filt_c,filt_r,coarse, minval, maxval,
					      shift,!nocenter,skysig,use_mean,regType);
/*
 * And return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = out;
   hand.type = shTypeGetFromName("BINREGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new REGION handle",TCL_STATIC);
      phBinregionDel(out);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclSkyEstimateDebias_use =
  "USAGE: skyEstimateDebias <skyreg> <gain> [gain2] -clipped";
#define tclSkyEstimateDebias_hlp \
  "Debias an estimated sky value, allowing for the Poisson nature of the data, and the fact that we clip the median"

static ftclArgvInfo skyEstimateDebias_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSkyEstimateDebias_hlp},
   {"<skyreg>", FTCL_ARGV_STRING, NULL, NULL, "BINREGION containing skyvalues"},
   {"<gain>", FTCL_ARGV_DOUBLE, NULL, NULL,
      "Gain of amplifier (or left amp if gain2 is provided)"},
   {"[gain2]", FTCL_ARGV_DOUBLE, NULL, NULL, "Gain of right-hand amplifier"},
   {"-clipped", FTCL_ARGV_CONSTANT, (void *)1, NULL,"Sky estimate was clipped"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define skyEstimateDebias_name "skyEstimateDebias"

static int
tclSkyEstimateDebias(ClientData clientData,
		     Tcl_Interp *interp,
		     int ac,
		     char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *skyregStr = NULL;		/* BINREGION containing skyvalues */
   BINREGION *skyreg;
   double gain1 = 0.0;			/* Gain of amplifier (or left amp
					   if -gain2 is specified) */
   double gain2 = -1;			/* Gain of right-hand amplifier */
   int clipped = 0;			/* Sky estimate was clipped */

   shErrStackClear();

   a_i = 1;
   skyEstimateDebias_opts[a_i++].dst = &skyregStr;
   skyEstimateDebias_opts[a_i++].dst = &gain1;
   skyEstimateDebias_opts[a_i++].dst = &gain2;
   skyEstimateDebias_opts[a_i++].dst = &clipped;
   shAssert(skyEstimateDebias_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, skyEstimateDebias_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     skyEstimateDebias_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,skyregStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("BINREGION")) {
      Tcl_AppendResult(interp,"skyEstimateDebias: "
                       "argument \"", skyregStr, "\" is not a BINREGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   skyreg = hand.ptr;
/*
 * work
 */
   phSkyEstimateDebias(skyreg, clipped, gain1, gain2);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclSkySubtract_use =
  "USAGE: SkySubtract <out> <in> <smoothed> <rand> <filt_c> <filt_r> [-nbit #]";
#define tclSkySubtract_hlp \
  ""

static ftclArgvInfo skySubtract_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSkySubtract_hlp},
   {"<out>", FTCL_ARGV_STRING, NULL, NULL, "output region"},
   {"<in>", FTCL_ARGV_STRING, NULL, NULL, "input region"},
   {"<smoothed>", FTCL_ARGV_STRING, NULL, NULL, "smoothed region (input)"},
   {"<rand>", FTCL_ARGV_STRING, NULL, NULL, "Random number state"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclSkySubtract(
	       ClientData clientDatag,
	       Tcl_Interp *interp,
	       int ac,
	       char **av
	       )
{
   int i;
   HANDLE hand;
   REGION *in, *out;
   BINREGION *smoothed;
   RANDOM *random;
   void *vptr;				/* used by shTclHandleExprEval */
   char *outStr = NULL;			/* output region */
   char *inStr = NULL;			/* input region */
   char *smoothedStr = NULL;		/* smoothed region (input) */
   char *randStr = NULL;		/* Random number state */
   
   shErrStackClear();

   i = 1;
   skySubtract_opts[i++].dst = &outStr;
   skySubtract_opts[i++].dst = &inStr;
   skySubtract_opts[i++].dst = &smoothedStr;
   skySubtract_opts[i++].dst = &randStr;
   shAssert(skySubtract_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,skySubtract_opts) != TCL_OK) {

      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,outStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"skySubtract: first argument is not a REGION",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   out = hand.ptr;

   if(shTclHandleExprEval(interp,inStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"skySubtract: second argument is not a REGION",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   in = hand.ptr;

   if(shTclHandleExprEval(interp,smoothedStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("BINREGION")) {
      Tcl_SetResult(interp,"skySubtract: third argument is not a BINREGION",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   smoothed = hand.ptr;

   if(shTclHandleExprEval(interp,randStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("RANDOM")) {
      Tcl_SetResult(interp,"skySubtract: fourth argument is not a RANDOM",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   random = hand.ptr;
/*
 * We've got the arguments, now check that they won't fail any assertions
 */
   if(in->type != TYPE_PIX) {
      Tcl_SetResult(interp,"skySubtract: <in> must be U16",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(out->type != TYPE_PIX) {
      Tcl_SetResult(interp,"skySubtract: <out> must be U16",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(in->nrow != out->nrow || in->ncol != out->ncol) {
      Tcl_SetResult(interp,
		    "skySubtract: <in> and <out> must be the same size",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   if(smoothed->reg == NULL) {
      Tcl_SetResult(interp,"skySubtract: binned region must be non-NULL"
		    ,TCL_STATIC);
      return(TCL_ERROR);
   }
   if(smoothed->reg->type != TYPE_S32) {
      Tcl_SetResult(interp,"skySubtract: <smoothed> must be S32",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * Work
 */
   phSkySubtract(out,in,smoothed,random);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclBinregionInterpolate_use =
  "USAGE: BinregionInterpolate <binRegion> <row> <col>";
#define tclBinregionInterpolate_hlp \
  "Return the value of the pixel <row><col> by bilinear interpolation in the "\
"provided BINREGION"

static ftclArgvInfo binregionInterpolate_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclBinregionInterpolate_hlp},
   {"<binregion>", FTCL_ARGV_STRING, NULL, NULL, "Binned region (input)"},
   {"<row>", FTCL_ARGV_INT, NULL, NULL, "desired row"},
   {"<col>", FTCL_ARGV_INT, NULL, NULL, "desired column"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclBinregionInterpolate(
	       ClientData clientDatag,
	       Tcl_Interp *interp,
	       int ac,
	       char **av
	       )
{
   int i;
   HANDLE hand;
   BINREGION *bin;
   float val;
   void *vptr;				/* used by shTclHandleExprEval */
   char *binregionStr = NULL;		/* Binned region (input) */
   int row = 0;				/* desired row */
   int col = 0;				/* desired column */
   
   shErrStackClear();

   i = 1;
   binregionInterpolate_opts[i++].dst = &binregionStr;
   binregionInterpolate_opts[i++].dst = &row;
   binregionInterpolate_opts[i++].dst = &col;
   shAssert(binregionInterpolate_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,binregionInterpolate_opts) != TCL_OK) {

      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,binregionStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("BINREGION")) {
      Tcl_SetResult(interp,"binregionInterpolate: "
		    "first argument is not a BINREGION",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   bin = hand.ptr;
   if(bin->bin_col == 1 && bin->bin_row == 1) {
      ;					/* OK */
   } else if(bin->bin_col < 2 && bin->bin_row < 2) {
      Tcl_SetResult(interp,"binregionInterpolate: "
		    "BINREGION has too few rows or columns", TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * Work
 */
   val = phBinregionInterpolate(bin, row, col);
/*
 * return the answer
 */
   {
      char buff[40];
      sprintf(buff,"%.4f",val);
      Tcl_SetResult(interp,buff,TCL_VOLATILE);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclConvolveWithGaussian_use =
  "USAGE: convolveWithGaussian <out> <in> <scr> <filtsize> <sigma> -algorithm str -lshift n";
#define tclConvolveWithGaussian_hlp \
  "Convolve a U16 region <in> with a Gaussian (double-sided size of filter\n" \
"<filtsize>, s.d. <sigma>). The result is put in the region <out> which\n"\
"must be the same type and size as <in>; <scr> is a scratch buffer which\n"\
"must also agree with <in> in type and size.\n"\
"If -lshift is specified, normalise the filter to 2^n (default: 0)\n"\
"\n"\
"The algorithm may be specified as:\n"\
"   ANY    (allow the code to choose; default)\n"\
"   MULT   (use explicit multiples)\n"\
"   LUT    (calculate a look up table for the multiples)\n"\
"   PLUT   (use a packed LUT; only works for small filters. Not yet written)\n"

static ftclArgvInfo convolveWithGaussian_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclConvolveWithGaussian_hlp},
   {"<out>", FTCL_ARGV_STRING, NULL, NULL, "output region"},
   {"<in>", FTCL_ARGV_STRING, NULL, NULL, "input region"},
   {"<scr>", FTCL_ARGV_STRING, NULL, NULL, \
      "scratch region (input; or \"\" or \"NULL\")"},
   {"<filtsize>", FTCL_ARGV_INT, NULL, NULL, "size of filter"},
   {"<sigma>", FTCL_ARGV_DOUBLE, NULL, NULL, "sigma for filter"},
   {"-algorithm", FTCL_ARGV_STRING, NULL, NULL,
		     "Algorithm to use (ANY, MULT, LUT, or PLUT; default ANY)"},
   {"-lshift", FTCL_ARGV_INT, NULL, NULL,
				  "How much to left shift answer (default: 0)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclConvolveWithGaussian(
			ClientData clientDatag,
			Tcl_Interp *interp,
			int ac,
			char **av
			)
{
   int i;
   CONVOLVE_ALG alg = CONVOLVE_ANY;
   HANDLE hand;
   REGION *in, *out, *scr;
   void *vptr;				/* used by shTclHandleExprEval */
   char *outStr = NULL;			/* output region */
   char *inStr = NULL;			/* input region */
   char *scrStr = NULL;			/* scratch region (input) */
   int filtsize = 0;			/* size of filter */
   double sigma = 0.0;			/* sigma for filter */
   char *algorithmStr = NULL;		/* Algorithm to use (ANY, MULT, LUT,
					   or PLUT; default ANY) */
   int lshift = 0;			/* How much to left shift answer */
   int ret;				/* return code */

   shErrStackClear();

   i = 1;
   convolveWithGaussian_opts[i++].dst = &outStr;
   convolveWithGaussian_opts[i++].dst = &inStr;
   convolveWithGaussian_opts[i++].dst = &scrStr;
   convolveWithGaussian_opts[i++].dst = &filtsize;
   convolveWithGaussian_opts[i++].dst = &sigma;
   convolveWithGaussian_opts[i++].dst = &algorithmStr;
   convolveWithGaussian_opts[i++].dst = &lshift;
   shAssert(convolveWithGaussian_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,convolveWithGaussian_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * Check arguments
 */
   if(shTclHandleExprEval(interp,outStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"convolveWithGaussian: first argument is not a REGION",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   out = hand.ptr;

   if(shTclHandleExprEval(interp,inStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"convolveWithGaussian: second argument is not a REGION",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   in = hand.ptr;

   if(*scrStr == '\0' || strcmp(scrStr, "NULL") == 0) {
      *scrStr = '\0';
      scr = NULL;
   } else {
      if(shTclHandleExprEval(interp,scrStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_SetResult(interp,"convolveWithGaussian: third argument is not a REGION",
		       TCL_STATIC);
	 return(TCL_ERROR);
      }
      scr = hand.ptr;
   }

   if(lshift < 0) {     
      Tcl_SetResult(interp,"convolveWithGaussian: normalisation must be >= 1",
		    TCL_STATIC);
      return(TCL_ERROR);
   }

   if(algorithmStr != NULL) {
      if(strcmp(algorithmStr,"ANY") == 0) {
	 alg = CONVOLVE_ANY;
      } else if(strcmp(algorithmStr,"MULT") == 0) {
	 alg = CONVOLVE_MULT;
      } else if(strcmp(algorithmStr,"LUT") == 0) {
	 alg = CONVOLVE_LUT;
      } else if(strcmp(algorithmStr,"PLUT") == 0) {
	 alg = CONVOLVE_PLUT;
      } else {
	 shErrStackPush("convolveWithGaussian: unknown algorithm: %s",algorithmStr);
	 return(TCL_ERROR);
      }
   }
/*
 * Did they provide a scr region?
 */
   if(scr == NULL) {
      scr = shRegNew("scratch", in->nrow, in->ncol, in->type);
   }
/*
 * work
 */
   ret = phConvolveWithGaussian(out,in,scr,filtsize,sigma,lshift,alg);
   if(*scrStr == '\0') shRegDel(scr);

   if(ret != SH_SUCCESS) {
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclInitConvolveLUT_use =
  "USAGE: initConvolveLUT <nlut>";
#define tclInitConvolveLUT_hlp \
  "Initialise the LUTs used when convolving images"

static ftclArgvInfo initConvolveLUT_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclInitConvolveLUT_hlp},
   {"<nlut>", FTCL_ARGV_INT, NULL, NULL, "Desired number of LUTs"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define initConvolveLUT_name "initConvolveLUT"

static int
tclInitConvolveLUT(ClientData clientData,
		   Tcl_Interp *interp,
		   int ac,
		   char **av)
{
   int i;
   int nlut = 0;			/* Desired number of LUTs */

   shErrStackClear();

   i = 1;
   initConvolveLUT_opts[i++].dst = &nlut;
   shAssert(initConvolveLUT_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, initConvolveLUT_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     initConvolveLUT_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phInitConvolveLUT(nlut);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclFiniConvolveLUT_use =
  "USAGE: finiConvolveLUT";
#define tclFiniConvolveLUT_hlp \
  "Free any LUT buffers allocated while convolving images"

static ftclArgvInfo finiConvolveLUT_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFiniConvolveLUT_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define finiConvolveLUT_name "finiConvolveLUT"

static int
tclFiniConvolveLUT(ClientData clientData,
		   Tcl_Interp *interp,
		   int ac,
		   char **av)
{
   int i;

   shErrStackClear();

   i = 1;
   shAssert(finiConvolveLUT_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, finiConvolveLUT_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     finiConvolveLUT_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phFiniConvolveLUT();
  
   return(TCL_OK);
}


/*****************************************************************************/
static char *tclRegIntBin_use =
  "USAGE: RegIntBin <out> <in> <rfac> <cfac> \n"
"                   [-rshift #] [-addcon #] [-row0 #] [-col0 #] [-slow]";
#define tclRegIntBin_hlp \
  "Rebin REGION <in> into REGION <out>; both must be U16. The binning " \
"averages <cfac> columns and <rfac> rows. It is somewhat faster if <cfac> "\
"and <rfac> are chosen from 1x2, 2x1, 2x2, or 4x4, and if -slow is "\
"_not_ specified, rebinning will not be carried out. If the -rshift flag " \
"is omitted (or negative) the output will be the average of the input " \
"pixels; otherwise it'll be the sum of the input pixels right shifted " \
"<rshift> bits. If specified, <addcon> will be subtracted from the binned " \
"image. Finally, the result is truncated to the number of bits in the data"\
"If row0 or col0 is specified, the origin of the binned image will be at "\
"(row0,col0) in the original image (and pixels below row0, col0 will be "\
"ignored) (default: (0,0)"

static ftclArgvInfo regIntBin_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntBin_hlp},
   {"<breg>", FTCL_ARGV_STRING, NULL, NULL, "binned region (may be NULL)"},
   {"<in>", FTCL_ARGV_STRING, NULL, NULL, "Input region"},
   {"<rfac>", FTCL_ARGV_INT, NULL, NULL, "Row binning factor"},
   {"<cfac>", FTCL_ARGV_INT, NULL, NULL, "Column binning factor"},
   {"-rshift", FTCL_ARGV_INT, NULL, NULL, "number of bits to >> binned pixels"},
   {"-bkgd", FTCL_ARGV_INT, NULL, NULL, "Value for non-existent parts of <in>"},
   {"-addcon", FTCL_ARGV_INT, NULL, NULL, "Add this constant to binned image"},
   {"-row0", FTCL_ARGV_INT, NULL, NULL,"Row origin of region to be binned (0)"},
   {"-col0", FTCL_ARGV_INT, NULL, NULL,
				   "Column origin of region to be binned (0)"},
   {"-slow", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Allow slow algorithm?"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRegIntBin(
	     ClientData clientDatag,
	     Tcl_Interp *interp,
	     int ac,
	     char **av
	     )
{
   int addcon;
   int bkgd;
   BINREGION *breg; char *bregStr;
   int col0, row0;
   HANDLE hand;
   int i;
   REGION *in; char *inStr;
   char name[HANDLE_NAMELEN];
   BINREGION *out;			/* returned BINREGION; may == breg */
   int rfac, cfac;
   int rshift;
   int slow;
   void *vptr;				/* used by shTclHandleExprEval */

   shErrStackClear();

   i = 1;
   regIntBin_opts[i++].dst = &bregStr;
   regIntBin_opts[i++].dst = &inStr;
   regIntBin_opts[i++].dst = &rfac;
   regIntBin_opts[i++].dst = &cfac;
   regIntBin_opts[i++].dst = &rshift; rshift = -1;
   regIntBin_opts[i++].dst = &bkgd; bkgd = 0;
   regIntBin_opts[i++].dst = &addcon; addcon = 0;
   regIntBin_opts[i++].dst = &row0; row0 = 0;
   regIntBin_opts[i++].dst = &col0; col0 = 0;
   regIntBin_opts[i++].dst = &slow; slow = 0;
   shAssert(regIntBin_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,regIntBin_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Process arguments
 */
   if(bregStr[0] == '\0' ||
      strcmp(bregStr, "NULL") == 0 || strcmp(bregStr, "null") == 0) {
      breg = NULL;
   } else {
      if(shTclHandleExprEval(interp,bregStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("BINREGION")) {
	 Tcl_SetResult(interp,"regIntBin: "
		       "first argument is not a BINREGION",TCL_STATIC);
	 return(TCL_ERROR);
      }
      breg = hand.ptr;
   }

   if(shTclHandleExprEval(interp,inStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regIntBin: "
		    "second argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   in = hand.ptr;

   if(breg != NULL) {
      if(breg->reg == NULL) {
	 Tcl_SetResult(interp,"regIntBin: "
		       "bregput binned region has no region",TCL_STATIC);
      return(TCL_ERROR);
      }
#if 0
      if(in->nrow/rfac != breg->reg->nrow || in->ncol/cfac != breg->reg->ncol) {
	 Tcl_SetResult(interp,"regIntBin: "
		       "output region is wrong size",TCL_STATIC);
	 return(TCL_ERROR);
      }
#endif
   }

   if((out = phRegIntBin(breg, in, rfac, cfac, rshift,
			 bkgd, addcon, row0, col0, slow)) == NULL) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * Return the answer, if not equal to breg
 */
   if(breg == NULL) {
      if(p_shTclHandleNew(interp,name) != TCL_OK) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
      hand.ptr = out;
      hand.type = shTypeGetFromName("BINREGION");
      
      if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
	 Tcl_SetResult(interp,"can't bind to new BINREGION handle",TCL_STATIC);
	 return(TCL_ERROR);
      }
      
      Tcl_SetResult(interp, name, TCL_VOLATILE);
   } else {
      Tcl_SetResult(interp, bregStr, TCL_VOLATILE);
   }

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclBinregionUnbin_use =
  "USAGE: BinregionUnbin <binregion> <rand>";
#define tclBinregionUnbin_hlp \
  "Unbin a binregion returning a region. If the region is shifted it'll be"\
"unshifted and dithered; if no dithering's needed, <rand> may be \"\""

static ftclArgvInfo binregionUnbin_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclBinregionUnbin_hlp},
   {"<binregion>", FTCL_ARGV_STRING, NULL, NULL, "Binregion to expand"},
   {"<rand>", FTCL_ARGV_STRING, NULL, NULL, "RAND for dithering"},
   {"-bkgd", FTCL_ARGV_INT, NULL, NULL, "Value for non-existent parts of <in>"},
   {"-addcon", FTCL_ARGV_INT, NULL, NULL, "Add this constant to binned image"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclBinregionUnbin(
		  ClientData clientDatag,
		  Tcl_Interp *interp,
		  int ac,
		  char **av
		  )
{
   int addcon;
   int bkgd;
   BINREGION *br; char *brStr;
   HANDLE hand;
   int i;
   char name[HANDLE_NAMELEN];
   REGION *reg;
   RANDOM *rand; char *randStr;
   void *vptr;				/* used by shTclHandleExprEval */

   shErrStackClear();

   i = 1;
   binregionUnbin_opts[i++].dst = &brStr;
   binregionUnbin_opts[i++].dst = &randStr;
   binregionUnbin_opts[i++].dst = &bkgd; bkgd = 0;
   binregionUnbin_opts[i++].dst = &addcon; addcon = 0;

   if(get_FtclOpts(interp,&ac,av,binregionUnbin_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,brStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("BINREGION")) {
      Tcl_SetResult(interp,"binregionUnbin: "
                    "first argument is not a BINREGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   br = hand.ptr;

   if(*randStr == '\0') {
      if(br->shift != 0) {
	 Tcl_SetResult(interp,"binregionUnbin: "
		 "You must provide a rand as binregion is shifted",TCL_STATIC);
	 return(TCL_ERROR);
      }
      rand = NULL;
   } else {
      if(shTclHandleExprEval(interp,randStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("RANDOM")) {
	 Tcl_SetResult(interp,"binregionUnbin: "
		       "second argument is not a RANDOM",TCL_STATIC);
	 return(TCL_ERROR);
      }
      rand = hand.ptr;
   }
/*
 * work
 */
   reg = phBinregionUnBin(br, rand, bkgd, addcon);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = reg;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new REGION handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclConvolveWithLaplacian_use =
  "USAGE: convolveWithLaplacian <out> <in> <scr> <filtsize> <sigma> -algorithm str -lshift n";
#define tclConvolveWithLaplacian_hlp \
  "Convolve a region with the Laplacian of a Gaussian; other options as\n"\
"for convolveWithGaussian (with the addition of another scratch region)"

static ftclArgvInfo convolveWithLaplacian_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclConvolveWithLaplacian_hlp},
   {"<out>", FTCL_ARGV_STRING, NULL, NULL, "output region"},
   {"<in>", FTCL_ARGV_STRING, NULL, NULL, "input region"},
   {"<scr>", FTCL_ARGV_STRING, NULL, NULL, "scratch region (input)"},
   {"<scr2>", FTCL_ARGV_STRING, NULL, NULL, "scratch region (input)"},
   {"<filtsize>", FTCL_ARGV_INT, NULL, NULL, "size of filter"},
   {"<sigma>", FTCL_ARGV_DOUBLE, NULL, NULL, "sigma for filter"},
   {"-algorithm", FTCL_ARGV_STRING, NULL, NULL,
		     "Algorithm to use (ANY, MULT, LUT, or PLUT; default ANY)"},
   {"-lshift", FTCL_ARGV_INT, NULL, NULL,
				  "How much to left-shift answer (default: 0)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclConvolveWithLaplacian(
			 ClientData clientDatag,
			 Tcl_Interp *interp,
			 int ac,
			 char **av
			 )
{
   int i;
   CONVOLVE_ALG alg = CONVOLVE_ANY;
   HANDLE hand;
   REGION *in, *out, *scr, *scr2;
   void *vptr;				/* used by shTclHandleExprEval */
   char *outStr = NULL;			/* output region */
   char *inStr = NULL;			/* input region */
   char *scrStr = NULL;			/* scratch region (input) */
   char *scr2Str = NULL;		/* scratch region (input) */
   int filtsize = 0;			/* size of filter */
   double sigma = 0.0;			/* sigma for filter */
   char *algorithmStr = NULL;		/* Algorithm to use: ANY, MULT, LUT,
					   or PLUT; default ANY */
   int lshift = 0;			/* How much to left-shift answer */

   shErrStackClear();

   i = 1;
   convolveWithLaplacian_opts[i++].dst = &outStr;
   convolveWithLaplacian_opts[i++].dst = &inStr;
   convolveWithLaplacian_opts[i++].dst = &scrStr;
   convolveWithLaplacian_opts[i++].dst = &scr2Str;
   convolveWithLaplacian_opts[i++].dst = &filtsize;
   convolveWithLaplacian_opts[i++].dst = &sigma;
   convolveWithLaplacian_opts[i++].dst = &algorithmStr;
   convolveWithLaplacian_opts[i++].dst = &lshift;
   shAssert(convolveWithLaplacian_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,convolveWithLaplacian_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * Check arguments
 */
   if(shTclHandleExprEval(interp,outStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"convolveWithLaplacian: "
		    "first argument is not a REGION", TCL_STATIC);
      return(TCL_ERROR);
   }
   out = hand.ptr;

   if(shTclHandleExprEval(interp,inStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"convolveWithLaplacian: "
		    "second argument is not a REGION", TCL_STATIC);
      return(TCL_ERROR);
   }
   in = hand.ptr;

   if(shTclHandleExprEval(interp,scrStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"convolveWithLaplacian: "
		    "third argument is not a REGION", TCL_STATIC);
      return(TCL_ERROR);
   }
   scr = hand.ptr;

   if(shTclHandleExprEval(interp,scr2Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"convolveWithLaplacian: "
		    "fourth argument is not a REGION", TCL_STATIC);
      return(TCL_ERROR);
   }
   scr2 = hand.ptr;

   if(lshift <= 0.0) {
      Tcl_SetResult(interp,"convolveWithLaplacian: Normalisation must be >= 0",
		    TCL_STATIC);
      return(TCL_ERROR);
   }

   if(algorithmStr != NULL) {
      if(strcmp(algorithmStr,"ANY") == 0) {
	 alg = CONVOLVE_ANY;
      } else if(strcmp(algorithmStr,"MULT") == 0) {
	 alg = CONVOLVE_MULT;
      } else if(strcmp(algorithmStr,"LUT") == 0) {
	 alg = CONVOLVE_LUT;
      } else if(strcmp(algorithmStr,"PLUT") == 0) {
	 alg = CONVOLVE_PLUT;
      } else {
	 shErrStackPush("convolveWithLaplacian: unknown algorithm: %s",algorithmStr);
	 return(TCL_ERROR);
      }
   }
/*
 * work
 */
   if(phConvolveWithLaplacian(out,in,scr,scr2,filtsize,sigma,lshift,alg) !=
								  SH_SUCCESS) {
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclBinregionNewFromBinregion_use =
  "USAGE: BinregionNewFromBinregion <old>";
#define tclBinregionNewFromBinregion_hlp \
  "Make a BINREGION from a BINREGION, copying its values"

static ftclArgvInfo binregionNewFromBinregion_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclBinregionNewFromBinregion_hlp},
   {"<binregion>", FTCL_ARGV_STRING, NULL, NULL, "The old BINREGION"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclBinregionNewFromBinregion(
			     ClientData clientDatag,
			     Tcl_Interp *interp,
			     int ac,
			     char **av
			     )
{
   int i;
   BINREGION *breg;
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *binregionStr = NULL;		/* The old BINREGION */

   shErrStackClear();

   i = 1;
   binregionNewFromBinregion_opts[i++].dst = &binregionStr;
   shAssert(binregionNewFromBinregion_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,binregionNewFromBinregion_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,binregionStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("BINREGION")) {
      Tcl_SetResult(interp,"binregionNewFromBinregion: "
                    "argument is not a BINREGION",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   breg = phBinregionNewFromBinregion(hand.ptr);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = breg;
   hand.type = shTypeGetFromName("BINREGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new BINREGION handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * return a histogram-equalised lookup table (converting U16 to U8 values)
 * as a U8 region
 */
static char *tclU16ToU8LUTGet_use =
  "USAGE: u16ToU8LUTGet -histeq -sigma -min -max -type -param dval";
#define tclU16ToU8LUTGet_hlp \
  "Return the LUT needed to convert U16 to U8 data via histogram"\
"equalisation. You must call regStatsFromQuartiles first if you use -histeq"

static ftclArgvInfo u16ToU8LUTGet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclU16ToU8LUTGet_hlp},
   {"-histeq", FTCL_ARGV_CONSTANT, (void *)1, NULL,
      "Use histogram equalisation (default)"},
   {"-min", FTCL_ARGV_INT, NULL, NULL,
      "Minimum value of analytic stretch"},
   {"-max", FTCL_ARGV_INT, NULL, NULL,
      "Maximum value of analytic stretch"}, 
   {"-type", FTCL_ARGV_INT, NULL, NULL,
      "Type of analytic stretch (0:linear, 1:sqrt, 2:log, 3: asinh, 4: sqrt(asinh))"},
   {"-param", FTCL_ARGV_DOUBLE, NULL, NULL,
      "Optional parameter for stretch (softening for asinh)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define u16ToU8LUTGet_name "u16ToU8LUTGet"

static int
tclU16ToU8LUTGet(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   REGION *lut;				/* the desired LUT */
   int histeq = 0;			/* Use histogram equalisation */
   int min = -1;			/* Min val of analytic stretch */
   int max = -1;			/* Max val of analytic stretch */
   int type = -1;			/* Type of analytic stretch */
   double param = 1.0;			/* Optional parameter for stretch:
					   softening for asinh */

   shErrStackClear();

   a_i = 1;
   u16ToU8LUTGet_opts[a_i++].dst = &histeq;
   u16ToU8LUTGet_opts[a_i++].dst = &min;
   u16ToU8LUTGet_opts[a_i++].dst = &max;
   u16ToU8LUTGet_opts[a_i++].dst = &type;
   u16ToU8LUTGet_opts[a_i++].dst = &param;
   shAssert(u16ToU8LUTGet_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, u16ToU8LUTGet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     u16ToU8LUTGet_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   if(!histeq && min < 0 && max < 0 && type < 0) {
      histeq = 1;
   } else if(histeq && type != 5) {
      Tcl_SetResult(interp,"u16ToU8LUTGet: "
                    "No -type option may be specified with -hist",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(histeq || type == 5) {
      lut = phLUTByHistogramEqualize(min, max);
   } else {
      lut = phLUTByAnalyticStretch(min, max, type, param);
   }
/*
 * Return the answer
 */
   if(lut == NULL) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = lut;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"name\" to new REGION handle",
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
phTclSkyUtilsDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"binregionNew",
		(Tcl_CmdProc *)tclBinregionNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclBinregionNew_hlp,
		tclBinregionNew_use);

   shTclDeclare(interp,"binregionDel",
		(Tcl_CmdProc *)tclBinregionDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclBinregionDel_hlp,
		tclBinregionDel_use);

   shTclDeclare(interp,"binregionNewFromConst",
		(Tcl_CmdProc *)tclBinregionNewFromConst, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclBinregionNewFromConst_hlp,
		tclBinregionNewFromConst_use);

   shTclDeclare(interp,"medianSky",
		(Tcl_CmdProc *)tclMedianSky, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMedianSky_hlp,
		tclMedianSky_use);

   shTclDeclare(interp,"skySubtract",
		(Tcl_CmdProc *)tclSkySubtract, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSkySubtract_hlp,
		tclSkySubtract_use);

   shTclDeclare(interp,"binregionInterpolate",
		(Tcl_CmdProc *)tclBinregionInterpolate, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclBinregionInterpolate_hlp,
		tclBinregionInterpolate_use);

   shTclDeclare(interp,"convolveWithGaussian",
		(Tcl_CmdProc *)tclConvolveWithGaussian, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclConvolveWithGaussian_hlp,
		tclConvolveWithGaussian_use);

   shTclDeclare(interp,"regIntBin",
		(Tcl_CmdProc *)tclRegIntBin, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntBin_hlp,
		tclRegIntBin_use);

   shTclDeclare(interp,"binregionUnbin",
		(Tcl_CmdProc *)tclBinregionUnbin, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclBinregionUnbin_hlp,
		tclBinregionUnbin_use);

   shTclDeclare(interp,"convolveWithLaplacian",
		(Tcl_CmdProc *)tclConvolveWithLaplacian, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclConvolveWithLaplacian_hlp,
		tclConvolveWithLaplacian_use);

   shTclDeclare(interp,"binregionNewFromBinregion",
		(Tcl_CmdProc *)tclBinregionNewFromBinregion, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclBinregionNewFromBinregion_hlp,
		tclBinregionNewFromBinregion_use);

   shTclDeclare(interp,u16ToU8LUTGet_name,
		(Tcl_CmdProc *)tclU16ToU8LUTGet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclU16ToU8LUTGet_hlp,
		tclU16ToU8LUTGet_use);

   shTclDeclare(interp,initConvolveLUT_name,
		(Tcl_CmdProc *)tclInitConvolveLUT, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclInitConvolveLUT_hlp,
		tclInitConvolveLUT_use);

   shTclDeclare(interp,finiConvolveLUT_name,
		(Tcl_CmdProc *)tclFiniConvolveLUT, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFiniConvolveLUT_hlp,
		tclFiniConvolveLUT_use);

   shTclDeclare(interp,skyEstimateDebias_name,
		(Tcl_CmdProc *)tclSkyEstimateDebias, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSkyEstimateDebias_hlp,
		tclSkyEstimateDebias_use);
}
