/*
 * TCL bindings for Bright Objects code
 */
#include <stdlib.h>
#include "dervish.h"
#include "phBrightObjects.h"

static char *module = "phBrightObjects"; /* name of this set of code */

/*****************************************************************************/

static char *tclSaturatedStarCentroid_use =
  "USAGE: saturatedStarCentroid <reg> <obj> <level> <dgpsf> <sky>";
#define tclSaturatedStarCentroid_hlp \
  "Centroid a saturated star (level <lev> in OBJECT <obj>) in a REGION <reg>"\
"with PSF <dgpsf>, assuming a background level of <sky> and allowing for "\
"bleed trails. Returns colc rowc"

static ftclArgvInfo saturatedStarCentroid_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSaturatedStarCentroid_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "region including star"},
   {"<obj>", FTCL_ARGV_STRING, NULL, NULL, "OBJECT specifying star"},
   {"<lev>", FTCL_ARGV_INT, NULL, NULL, "the desired level in OBJECT"},
   {"<psf>", FTCL_ARGV_STRING, NULL, NULL, "DGPSF for frame"},
   {"<sky>", FTCL_ARGV_DOUBLE, NULL, NULL, "sky level"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclSaturatedStarCentroid(
			ClientData clientDatag,
			Tcl_Interp *interp,
			int ac,
			char **av
			)
{
   float colc, rowc;			/* centre of object */
   int i;
   HANDLE hand;
   OBJECT *obj;
   DGPSF *psf;
   REGION *reg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* region including star */
   char *objStr = NULL;			/* OBJECT specifying star */
   int lev = 0;				/* the desired level in OBJECT */
   char *psfStr = NULL;			/* DGPSF for frame */
   double sky = 0.0;			/* sky level */

   shErrStackClear();

   i = 0;
   saturatedStarCentroid_opts[i++].dst = &regStr;
   saturatedStarCentroid_opts[i++].dst = &objStr;
   saturatedStarCentroid_opts[i++].dst = &lev;
   saturatedStarCentroid_opts[i++].dst = &psfStr;
   saturatedStarCentroid_opts[i++].dst = &sky;
   if(get_FtclOpts(interp,&ac,av,saturatedStarCentroid_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"saturatedStarCentroid: "
		    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,objStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJECT")) {
      Tcl_SetResult(interp,"saturatedStarCentroid: "
		    "second argument is not a OBJECT",TCL_STATIC);
      return(TCL_ERROR);
   }
   obj = hand.ptr;
   if(lev < 0 || lev >= obj->nlevel) {
      Tcl_SetResult(interp,"saturatedStarCentroid: "
		    "invalid level",TCL_STATIC);
      return(TCL_ERROR);      
   }

   if(shTclHandleExprEval(interp,psfStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("DGPSF")) {
      Tcl_SetResult(interp,"saturatedStarCentroid: "
		    "fourth argument is not a DGPSF",TCL_STATIC);
      return(TCL_ERROR);
   }
   psf = hand.ptr;
/*
 * do the work
 */
   if(phSaturatedStarCentroid(reg,obj->sv[lev],psf,sky,&colc,&rowc)
							       != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   {
      char buff[40];
      sprintf(buff,"%g %g",colc,rowc);
      Tcl_SetResult(interp,buff,TCL_VOLATILE);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclStarParamsFind_use =
  "USAGE: starParamsFind <comp_profile> <psf> [options]";
#define tclStarParamsFind_hlp \
  "Return a DGPSF giving the fitted parameters for <comp_profile>; the initial"\
"values (and the ones used for non-fitted parameters) are in <psf>."\
"If fparams is provided, it's used to find saturated pixels\n"\
"-medflag: use the median rather than the mean profile\n"\
"-min: the minimum value to use in a profile (default: 5)\n"\
"-gauss2: fit a two-component Gaussian\n"\
"-power: fit a power law wing Gaussian\n"\
"-beta: fit the slope of the power law wing\n"\
"-sigmap: fit the core size of the power law wing\n"

static ftclArgvInfo starParamsFind_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclStarParamsFind_hlp},
   {"<comp_prof>", FTCL_ARGV_STRING, NULL, NULL, "Fitted profile"},
   {"<ipsf>", FTCL_ARGV_STRING, NULL, NULL, "Handle to DGPSF with starting values"},
   {"-fparams", FTCL_ARGV_STRING, NULL, NULL, "The frame's FRAMEPARAMS"},
   {"-gauss2", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Fit two Gaussians"},
   {"-power", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Fit a power law"},
   {"-sigmap", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Fit the power-law sigma"},
   {"-beta", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Fit the power-law index"},
   {"-medflg", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					 "Use median rather than mean profile"},
   {"-min", FTCL_ARGV_DOUBLE, NULL, NULL,
				     "Minimum value in profile to use for fit"},
   {"-chisq", FTCL_ARGV_STRING, NULL, NULL, "The reduced chisq of the fit"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclStarParamsFind(
		       ClientData clientDatag,
		       Tcl_Interp *interp,
		       int ac,
		       char **av
		       )
{
   int i;
   char buff[40];
   float chisq;				/* reduced chisq of fit */
   HANDLE hand;
   char name[HANDLE_NAMELEN];
   COMP_PROFILE *comp_prof;             /* profile to fit */
   DGPSF *psf;				/* the fitted PSF */
   void *vptr;				/* used by shTclHandleExprEval */
   char *comp_profStr = NULL;		/* Fitted profile */
   char *ipsfStr = NULL;		/* Handle to DGPSF with starting values */
   char *fparamsStr = NULL;		/* The frame's FRAMEPARAMS */
   int gauss2 = 0;			/* Fit two Gaussians */
   int power = 0;			/* Fit a power law */
   int sigmap = 0;			/* Fit the power-law sigma */
   int beta = 0;			/* Fit the power-law index */
   int medflg = 0;			/* Use median rather than mean profile*/
   double min = 5.0;			/* Minimum value in profile
					   to use for fit */
   char *chisqStr = NULL;		/* The reduced chisq of the fit */

   shErrStackClear();

   i = 1;
   starParamsFind_opts[i++].dst = &comp_profStr;
   starParamsFind_opts[i++].dst = &ipsfStr;
   starParamsFind_opts[i++].dst = &fparamsStr;
   starParamsFind_opts[i++].dst = &gauss2;
   starParamsFind_opts[i++].dst = &power;
   starParamsFind_opts[i++].dst = &sigmap;
   starParamsFind_opts[i++].dst = &beta;
   starParamsFind_opts[i++].dst = &medflg;
   starParamsFind_opts[i++].dst = &min;
   starParamsFind_opts[i++].dst = &chisqStr;
   shAssert(starParamsFind_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,starParamsFind_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,comp_profStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("COMP_PROFILE")) {
      Tcl_SetResult(interp,"starParamsFind: "
                    "first argument is not a COMP_PROFILE",TCL_STATIC);
      return(TCL_ERROR);
   }
   comp_prof = hand.ptr;

   if(shTclHandleExprEval(interp,ipsfStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("DGPSF")) {
      Tcl_SetResult(interp,"starParamsFind: "
		    "<ipsf> argument is not a DGPSF",TCL_STATIC);
      return(TCL_ERROR);
   }
   psf = hand.ptr;
/*
 * work
 */
   if((psf = phStarParamsFind(comp_prof,psf,min,medflg,
			   gauss2,power,sigmap,beta,&chisq)) == NULL) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * Return the answers
 */
   if(chisqStr != NULL) {
      sprintf(buff,"%g",chisq);
      if(Tcl_SetVar(interp,chisqStr,buff,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return alpha",TCL_VOLATILE);
	 phDgpsfDel(psf);
	 return(TCL_ERROR);
      }
   }
   
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      phDgpsfDel(psf);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = psf;
   hand.type = shTypeGetFromName("DGPSF");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      phDgpsfDel(psf);
      Tcl_SetResult(interp,"can't bind to new DGPSF handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclBrightStarWings_use =
  "USAGE: brightStarWings [options] -help";
#define tclBrightStarWings_hlp \
  "Add the power-law wing of a star into the given BINREGION <sky>.\n"\
"The amplitude of the wing is set by the star's PSF counts (if it's\n"\
"saturated these will be estimated from the profile, assuming that\n"\
"a complete stellar profile is provided in <cprof>. The estimation is done\n"\
"using the mean profile unless -medflg is specified, in which case the\n"\
"median profile is used.\n"\
"Only the points within rmax are used (default: infinity))\n"\
"\n"\
"The form of the wings is specified by <beta>, <sigmap>, and <frac>, but\n"\
"within <rtrans> of the centre a parabolic cap is used instead"

static ftclArgvInfo brightStarWings_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclBrightStarWings_hlp},
   {"<obj1>", FTCL_ARGV_STRING, NULL, NULL, "the object in question"},
   {"<cprof>", FTCL_ARGV_STRING, NULL, NULL, "a composite profile"},
   {"<fparams>", FTCL_ARGV_STRING, NULL, NULL,
					"a FRAMEPARAMS for the current region"},
   {"<wingreg>", FTCL_ARGV_STRING, NULL, NULL,
					   "a BINREGION to add the wings into"},
   {"<beta>", FTCL_ARGV_DOUBLE, NULL, NULL, "the power law slope"},
   {"<sigmap>", FTCL_ARGV_DOUBLE, NULL, NULL, "the power law's \"sigma\""},
   {"<frac>", FTCL_ARGV_DOUBLE, NULL, NULL,
		       "the ratio of power-law power to an object's psfCounts"},
   {"-rtrans", FTCL_ARGV_DOUBLE, NULL, NULL,
		       "the transition radius to a parabolic core to the wing"},
   {"-rmax", FTCL_ARGV_DOUBLE, NULL, NULL,
      "maximum radius used in estimating psfCounts can be 0 => use all points"},
   {"-medflg", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					 "Use median rather than mean profile"},
   {"-nsigma", FTCL_ARGV_DOUBLE, NULL, NULL,
     "mark pixels where we subtracted more than nsigma*skysigma as NOTCHECKED"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclBrightStarWings(
		   ClientData clientDatag,
		   Tcl_Interp *interp,
		   int ac,
		   char **av
		   )
{
   char name[HANDLE_NAMELEN];
   int i;
   COMP_PROFILE *cprof;
   FRAMEPARAMS *fparams;
   HANDLE hand;
   OBJECT1 *obj1;
   BINREGION *wingreg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *obj1Str = NULL;		/* the object in question */
   char *cprofStr = NULL;		/* a composite profile */
   char *fparamsStr = NULL;		/* a FRAMEPARAMS for current region */
   char *wingregStr = NULL;		/* a BINREGION to add the wings into */
   double beta = 0.0;			/* the power law slope */
   double sigmap = 0.0;			/* the power law "sigma" */
   double frac = 0.0;			/* the ratio of power-law power to
					   an object's psfCounts */
   double rtrans = 0.0;			/* the transition radius to a
					   parabolic core to the wing */
   double rmax = 0.0;			/* maximum radius used in estimating
					   psfCounts; if 0 use all points */
   int medflg = 0;			/* Use median rather than mean profile*/
   double nsigma = 10.0;		/* mark pixels where we subtracted more
					   than nsigma*skysigma as NOTCHECKED */
   WINGOBJECT *star_wing = NULL;	/* return info on subtracted wings */

   shErrStackClear();

   i = 1;
   brightStarWings_opts[i++].dst = &obj1Str;
   brightStarWings_opts[i++].dst = &cprofStr;
   brightStarWings_opts[i++].dst = &fparamsStr;
   brightStarWings_opts[i++].dst = &wingregStr;
   brightStarWings_opts[i++].dst = &beta;
   brightStarWings_opts[i++].dst = &sigmap;
   brightStarWings_opts[i++].dst = &frac;
   brightStarWings_opts[i++].dst = &rtrans;
   brightStarWings_opts[i++].dst = &rmax;
   brightStarWings_opts[i++].dst = &medflg;
   brightStarWings_opts[i++].dst = &nsigma;
   shAssert(brightStarWings_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,brightStarWings_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,obj1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJECT1")) {
      Tcl_SetResult(interp,"brightStarWings: "
                    "first argument is not a OBJECT1",TCL_STATIC);
      return(TCL_ERROR);
   }
   obj1 = hand.ptr;

   if(shTclHandleExprEval(interp,cprofStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("COMP_PROFILE")) {
      Tcl_SetResult(interp,"brightStarWings: "
                    "second argument is not a COMP_PROFILE",TCL_STATIC);
      return(TCL_ERROR);
   }
   cprof = hand.ptr;

   if(shTclHandleExprEval(interp,fparamsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FRAMEPARAMS")) {
      Tcl_SetResult(interp,"brightStarWings: "
                    "third argument is not a FRAMEPARAMS",TCL_STATIC);
      return(TCL_ERROR);
   }
   fparams = hand.ptr;

   if(shTclHandleExprEval(interp,wingregStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("BINREGION")) {
      Tcl_SetResult(interp,"brightStarWings: "
                    "fourth argument is not a BINREGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   wingreg = hand.ptr;
/*
 * work
 */
   star_wing = phBrightStarWings(obj1, cprof, fparams, medflg, rmax, wingreg,
				 beta, sigmap, frac, rtrans, nsigma);

   if(star_wing == NULL) {
      return(TCL_OK);			/* return "" */
   }
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = star_wing;
   hand.type = shTypeGetFromName("WINGOBJECT");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,
		    "can't bind \"star_wing\" to new WINGOBJECT handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * support for composite profiles
 */
static char *tclCompositeProfileNew_use =
  "USAGE: compositeProfileNew ";
#define tclCompositeProfileNew_hlp \
  "Create a new COMP_PROFILE"

static ftclArgvInfo compositeProfileNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCompositeProfileNew_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclCompositeProfileNew(
		       ClientData clientDatag,
		       Tcl_Interp *interp,
		       int ac,
		       char **av
		       )
{
   HANDLE hand;
   char name[HANDLE_NAMELEN];
   COMP_PROFILE *prof;

   shErrStackClear();

   if(get_FtclOpts(interp,&ac,av,compositeProfileNew_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * work
 */
   prof = phCompositeProfileNew();
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = prof;
   hand.type = shTypeGetFromName("COMP_PROFILE");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new COMP_PROFILE handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}


/**********************************************************************
**<AUTO EXTRACT>
** TCL VERB: compositeProfileCopy
**
** Description:
**
** Copy the content of comp_prof2 to comp_prof1.
**
**</AUTO>
**********************************************************************/

static char *tclCompositeProfileCopy_use =
  "USAGE: compositeProfileCopy <out> <in>";
#define tclCompositeProfileCopy_hlp \
  ""

static ftclArgvInfo compositeProfileCopy_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCompositeProfileCopy_hlp},
   {"<out>", FTCL_ARGV_STRING, NULL, NULL, "COMP_PROFILE to copy _to_"},
   {"<in>", FTCL_ARGV_STRING, NULL, NULL, "COMP_PROFILE to copy _from_"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define compositeProfileCopy_name "compositeProfileCopy"

static int
tclCompositeProfileCopy(ClientData clientData,
			Tcl_Interp *interp,
			int ac,
			char **av)
{
  HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *outStr = NULL;		/* COMP_PROFILE to copy _to_ */
   COMP_PROFILE *out;
   char *inStr = NULL;		/* COMP_PROFILE to copy _from_ */
   COMP_PROFILE *in;

   shErrStackClear();

   i = 1;
   compositeProfileCopy_opts[i++].dst = &outStr;
   compositeProfileCopy_opts[i++].dst = &inStr;
   shAssert(compositeProfileCopy_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, compositeProfileCopy_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     compositeProfileCopy_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,outStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("COMP_PROFILE")) {
      Tcl_AppendResult(interp,"compositeProfileCopy: "
                       "argument \"", outStr, "\" is not a COMP_PROFILE",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   out = hand.ptr;

   if(shTclHandleExprEval(interp,inStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("COMP_PROFILE")) {
      Tcl_AppendResult(interp,"compositeProfileCopy: "
                       "argument \"", inStr, "\" is not a COMP_PROFILE",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   in = hand.ptr;

/*
 * work
 */
   phCompositeProfileCopy(out, in);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclCompositeProfileDel_use =
  "USAGE: compositeProfileDel <prof>";
#define tclCompositeProfileDel_hlp \
  "Delete a COMP_PROFILE"

static ftclArgvInfo compositeProfileDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCompositeProfileDel_hlp},
   {"<prof>", FTCL_ARGV_STRING, NULL, NULL, "handle to condemned COMP_PROFILE"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclCompositeProfileDel(
		       ClientData clientDatag,
		       Tcl_Interp *interp,
		       int ac,
		       char **av
		       )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *profStr = NULL;		/* handle to condemned COMP_PROFILE */

   shErrStackClear();

   i = 1;
   compositeProfileDel_opts[i++].dst = &profStr;
   shAssert(compositeProfileDel_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,compositeProfileDel_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,profStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("COMP_PROFILE")) {
      Tcl_SetResult(interp,"compositeProfileDel: "
                    "argument is not a COMP_PROFILE",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phCompositeProfileDel(hand.ptr);
   p_shTclHandleDel(interp,profStr);
   
   return(TCL_OK);
}

/*****************************************************************************/
static char *tclCompositeProfileAdd_use =
  "USAGE: compositeProfileAdd <prof> <obj1> <satur_mask> -profmin val -med";
#define tclCompositeProfileAdd_hlp \
  "Add <obj1>'s profile into the composite profile <prof>; the frame's"\
"OBJMASK with saturated pixels is satur_mask. "\
"Only consider points in the profile which are val (default: 5)"

static ftclArgvInfo compositeProfileAdd_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCompositeProfileAdd_hlp},
   {"<prof>", FTCL_ARGV_STRING, NULL, NULL, "a COMP_PROFILE to be added to"},
   {"<obj1>", FTCL_ARGV_STRING, NULL, NULL,
				"a (measured) OBJECT1 to add into the profile"},
   {"<satur_mask>", FTCL_ARGV_STRING, NULL, NULL, "mask for saturated pixels"},
   {"<is_it_wing>", FTCL_ARGV_INT, NULL, NULL, "set to 1 for wing stars"},

   {"-profmin", FTCL_ARGV_DOUBLE, NULL, NULL, 
                           "minimum profile value to add into cumulative profile"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclCompositeProfileAdd(
		       ClientData clientDatag,
		       Tcl_Interp *interp,
		       int ac,
		       char **av
		       )
{
   int i;
   HANDLE hand;
   OBJMASK *satur_mask;
   OBJECT1 *obj1;
   COMP_PROFILE *prof;
   void *vptr;				/* used by shTclHandleExprEval */
   char *profStr = NULL;		/* a COMP_PROFILE to be added to */
   char *obj1Str = NULL;		/* a (measured) OBJECT1 to add
					   into the profile */
   char *satur_maskStr = NULL;		/* mask for saturated pixels */
   double profmin = 5.0;		/* minimum profile value to add
					   into cumulative profile */
   int is_it_wing = 0;                  /* flag signalling a wing star */

   shErrStackClear();

   i = 1;
   compositeProfileAdd_opts[i++].dst = &profStr;
   compositeProfileAdd_opts[i++].dst = &obj1Str;
   compositeProfileAdd_opts[i++].dst = &satur_maskStr;
   compositeProfileAdd_opts[i++].dst = &profmin;
   shAssert(compositeProfileAdd_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,compositeProfileAdd_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,profStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("COMP_PROFILE")) {
      Tcl_SetResult(interp,"compositeProfileAdd: "
                    "first argument is not a COMP_PROFILE",TCL_STATIC);
      return(TCL_ERROR);
   }
   prof = hand.ptr;

   if(shTclHandleExprEval(interp,obj1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJECT1")) {
      Tcl_SetResult(interp,"compositeProfileAdd: "
                    "second argument is not a OBJECT1",TCL_STATIC);
      return(TCL_ERROR);
   }
   obj1 = hand.ptr;

   if(shTclHandleExprEval(interp,satur_maskStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_SetResult(interp,"compositeProfileAdd: "
                    "third argument is not a OBJMASK",TCL_STATIC);
      return(TCL_ERROR);
   }
   satur_mask = hand.ptr;
/*
 * work
 */
   phCompositeProfileAdd(prof,obj1,satur_mask,is_it_wing,profmin);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclCompositeProfileFind_use =
  "USAGE: compositeProfileFind <cprof>";
#define tclCompositeProfileFind_hlp \
  "Given a filled COMP_PROFILE, find the composite profile. Usually the\n"\
"profile is used, but with -medflg use the median"

static ftclArgvInfo compositeProfileFind_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCompositeProfileFind_hlp},
   {"<cprof>", FTCL_ARGV_STRING, NULL, NULL, "A COMP_PROFILE (filled by compositeProfileAdd)"},
   {"-medflg", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Use median rather than mean profile"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclCompositeProfileFind(
			ClientData clientDatag,
			Tcl_Interp *interp,
			int ac,
			char **av
			)
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *cprofStr = NULL;		/* A COMP_PROFILE */
   int medflg = 0;			/* Use median rather than mean profile*/

   shErrStackClear();

   i = 1;
   compositeProfileFind_opts[i++].dst = &cprofStr;
   compositeProfileFind_opts[i++].dst = &medflg;
   shAssert(compositeProfileFind_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,compositeProfileFind_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,cprofStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("COMP_PROFILE")) {
      Tcl_SetResult(interp,"compositeProfileFind: "
                    "argument is not a COMP_PROFILE",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(phCompositeProfileFind(hand.ptr, medflg) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclWingobjectNew_use =
  "USAGE: wingobjectNew";
#define tclWingobjectNew_hlp \
  "Make a new WINGOBJECT"

static ftclArgvInfo wingobjectNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclWingobjectNew_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define wingobjectNew_name "wingobjectNew"

static int
tclWingobjectNew(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   WINGOBJECT *wobj;			/* desired object */

   shErrStackClear();

   a_i = 1;
   shAssert(wingobjectNew_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, wingobjectNew_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     wingobjectNew_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   wobj = phWingobjectNew();
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = wobj;
   hand.type = shTypeGetFromName("WINGOBJECT");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"wobj\" to new WINGOBJECT handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);   
   
   return(TCL_OK);
}

static char *tclWingobjectDel_use =
  "USAGE: wingobjectDel <wobject>";
#define tclWingobjectDel_hlp \
  "Delete a WINGOBJECT"

static ftclArgvInfo wingobjectDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclWingobjectDel_hlp},
   {"<wobject>", FTCL_ARGV_STRING, NULL, NULL, "WINGOBJECT to delete"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define wingobjectDel_name "wingobjectDel"

static int
tclWingobjectDel(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *wobjectStr = NULL;		/* WINGOBJECT to delete */
   WINGOBJECT *wobject;

   shErrStackClear();

   a_i = 1;
   wingobjectDel_opts[a_i++].dst = &wobjectStr;
   shAssert(wingobjectDel_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, wingobjectDel_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     wingobjectDel_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,wobjectStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("WINGOBJECT")) {
      Tcl_AppendResult(interp,"wingobjectDel: "
                       "argument \"", wobjectStr, "\" is not a WINGOBJECT",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   wobject = hand.ptr;   
/*
 * work
 */
   phWingobjectDel(wobject);
   p_shTclHandleDel(interp,wobjectStr);

   return(TCL_OK);
}

static char *tclObjectModelNew_use =
  "USAGE: objectModelNew";
#define tclObjectModelNew_hlp \
  "Create a new OBJECT_MODEL"

static ftclArgvInfo objectModelNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectModelNew_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objectModelNew_name "objectModelNew"

static int
tclObjectModelNew(ClientData clientData,
		  Tcl_Interp *interp,
		  int ac,
		  char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   OBJECT_MODEL *om = NULL;

   shErrStackClear();

   a_i = 1;
   shAssert(objectModelNew_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objectModelNew_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objectModelNew_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   om = phObjectModelNew();
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = om;
   hand.type = shTypeGetFromName("OBJECT_MODEL");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"om\" to new OBJECT_MODEL handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

static char *tclObjectModelDel_use =
  "USAGE: objectModelDel <om>";
#define tclObjectModelDel_hlp \
  "Delete an OBJECT_MODEL"

static ftclArgvInfo objectModelDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectModelDel_hlp},
   {"<om>", FTCL_ARGV_STRING, NULL, NULL, "OBJECT_MODEL to delete"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objectModelDel_name "objectModelDel"

static int
tclObjectModelDel(ClientData clientData,
		  Tcl_Interp *interp,
		  int ac,
		  char **av)
{
    HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *omStr = NULL;			/* OBJECT_MODEL to delete */
   OBJECT_MODEL *om;

   shErrStackClear();

   a_i = 1;
   objectModelDel_opts[a_i++].dst = &omStr;
   shAssert(objectModelDel_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objectModelDel_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objectModelDel_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,omStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJECT_MODEL")) {
      Tcl_AppendResult(interp,"objectModelDel: "
                       "argument \"", omStr, "\" is not an OBJECT_MODEL",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   om = hand.ptr;
/*
 * work
 */
   phObjectModelDel(om);
   p_shTclHandleDel(interp,omStr);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclPowerLawSetFromObject1_use =
  "USAGE: powerLawSetFromObject1 <obj1> -iprofMin ival -dgpsf sval -beta dval -useMedian";
#define tclPowerLawSetFromObject1_hlp \
  "Estimat the amplitude of a PowerLaw from an OBJECT1's profMean"

static ftclArgvInfo powerLawSetFromObject1_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPowerLawSetFromObject1_hlp},
   {"<obj1>", FTCL_ARGV_STRING, NULL, NULL, "OBJECT1 to fit"},
   {"-iprofMin", FTCL_ARGV_INT, NULL, NULL, "First value of profMean to use"},
   {"-dgpsf", FTCL_ARGV_STRING, NULL, NULL, "The PSF at obj1"},
   {"-beta", FTCL_ARGV_DOUBLE, NULL, NULL, "Power law is r^{-beta}"},
   {"-useMedian", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Use profMed, not profMean"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define powerLawSetFromObject1_name "powerLawSetFromObject1"

static int
tclPowerLawSetFromObject1(ClientData clientData,
				Tcl_Interp *interp,
				int ac,
				char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *obj1Str = NULL;		/* OBJECT1 to fit */
   OBJECT1 *obj1;
   int iprofMin = 0;			/* First value of profMean to use */
   char *dgpsfStr = NULL;		/* The PSF at obj1 */
   DGPSF *dgpsf = NULL;
   double beta = 0.0;			/* Power law is r^{-beta} */
   int useMedian = 0;			/* Use profMed, not profMean */

   shErrStackClear();

   a_i = 1;
   powerLawSetFromObject1_opts[a_i++].dst = &obj1Str;
   powerLawSetFromObject1_opts[a_i++].dst = &iprofMin;
   powerLawSetFromObject1_opts[a_i++].dst = &dgpsfStr;
   powerLawSetFromObject1_opts[a_i++].dst = &beta;
   powerLawSetFromObject1_opts[a_i++].dst = &useMedian;
   shAssert(powerLawSetFromObject1_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, powerLawSetFromObject1_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     powerLawSetFromObject1_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,obj1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJECT1")) {
      Tcl_AppendResult(interp,"powerLawSetFromObject1: "
                       "argument \"", obj1Str, "\" is not an OBJECT1",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   obj1 = hand.ptr;

   if (dgpsfStr != NULL) {
       if(shTclHandleExprEval(interp,dgpsfStr,&hand,&vptr) != TCL_OK) {
	   return(TCL_ERROR);
       }
       if(hand.type != shTypeGetFromName("DGPSF")) {
	   Tcl_AppendResult(interp,"powerLawSetFromObject1: "
			    "argument \"", dgpsfStr, "\" is not a DGPSF",
			    (char *)NULL);
	   return(TCL_ERROR);
       }
       dgpsf = hand.ptr;

       if (beta == 0.0) {
	   beta = dgpsf->beta;
       }
   }

   if (fabs(beta - 2) < 1e-4) {
      Tcl_SetResult(interp, "powerLawSetFromObject1: beta may not equal 2", TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   {
       float I0 = phPowerLawSetFromObject1(obj1, iprofMin, beta, !useMedian);
       char str[40];

       sprintf(str, "%g", I0);
       
       Tcl_AppendResult(interp, str, (char *)NULL);
   }

   return(TCL_OK);
}

static char *tclPowerLawSetFromCompositeProfile_use =
  "USAGE: powerLawSetFromCompositeProfile <obj1> -iprofMin ival -dgpsf sval -beta dval -useMedian";
#define tclPowerLawSetFromCompositeProfile_hlp \
  "Estimat the amplitude of a PowerLaw from an OBJECT1's profMean"

static ftclArgvInfo powerLawSetFromCompositeProfile_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPowerLawSetFromCompositeProfile_hlp},
   {"<cprof>", FTCL_ARGV_STRING, NULL, NULL, "COMP_PROFILE to fit"},
   {"-iprofMin", FTCL_ARGV_INT, NULL, NULL, "First value of profile to use"},
   {"-dgpsf", FTCL_ARGV_STRING, NULL, NULL, "The PSF at a point"},
   {"-beta", FTCL_ARGV_DOUBLE, NULL, NULL, "Power law is r^{-beta}"},
   {"-useMedian", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Use med, not mean"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define powerLawSetFromCompositeProfile_name "powerLawSetFromCompositeProfile"

static int
tclPowerLawSetFromCompositeProfile(ClientData clientData,
				Tcl_Interp *interp,
				int ac,
				char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *cprofStr = NULL;		/* COMP_PROFILE to fit */
   COMP_PROFILE *cprof;
   int iprofMin = 0;			/* First value of profMean to use */
   char *dgpsfStr = NULL;		/* The PSF at cprof */
   DGPSF *dgpsf = NULL;
   double beta = 0.0;			/* Power law is r^{-beta} */
   int useMedian = 0;			/* Use profMed, not profMean */

   shErrStackClear();

   a_i = 1;
   powerLawSetFromCompositeProfile_opts[a_i++].dst = &cprofStr;
   powerLawSetFromCompositeProfile_opts[a_i++].dst = &iprofMin;
   powerLawSetFromCompositeProfile_opts[a_i++].dst = &dgpsfStr;
   powerLawSetFromCompositeProfile_opts[a_i++].dst = &beta;
   powerLawSetFromCompositeProfile_opts[a_i++].dst = &useMedian;
   shAssert(powerLawSetFromCompositeProfile_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, powerLawSetFromCompositeProfile_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     powerLawSetFromCompositeProfile_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,cprofStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("COMP_PROFILE")) {
      Tcl_AppendResult(interp,"powerLawSetFromCompositeProfile: "
                       "argument \"", cprofStr, "\" is not an COMP_PROFILE",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   cprof = hand.ptr;

   if (dgpsfStr != NULL) {
       if(shTclHandleExprEval(interp,dgpsfStr,&hand,&vptr) != TCL_OK) {
	   return(TCL_ERROR);
       }
       if(hand.type != shTypeGetFromName("DGPSF")) {
	   Tcl_AppendResult(interp,"powerLawSetFromCompositeProfile: "
			    "argument \"", dgpsfStr, "\" is not a DGPSF",
			    (char *)NULL);
	   return(TCL_ERROR);
       }
       dgpsf = hand.ptr;

       if (beta == 0.0) {
	   beta = dgpsf->beta;
       }
   }

   if (fabs(beta - 2) < 1e-4) {
      Tcl_SetResult(interp, "powerLawSetFromCompositeProfile: beta may not equal 2", TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   {
       float I0 = phPowerLawSetFromCompositeProfile(cprof, iprofMin, beta, !useMedian);
       char str[40];

       sprintf(str, "%g", I0);
       
       Tcl_AppendResult(interp, str, (char *)NULL);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObjectModelIdSetFromPtr_use =
  "USAGE: objectModelIdSetFromPtr <om> <obj1>";
#define tclObjectModelIdSetFromPtr_hlp \
  "Set an OBJECT_MODEL.id[] to the provided OBJECT1*"

static ftclArgvInfo objectModelIdSetFromPtr_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectModelIdSetFromPtr_hlp},
   {"<om>", FTCL_ARGV_STRING, NULL, NULL, "OBJECT_MODEL to set"},
   {"<obj1>", FTCL_ARGV_STRING, NULL, NULL, "OBJECT1 whose address should be stored in om.id"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objectModelIdSetFromPtr_name "objectModelIdSetFromPtr"

static int
tclObjectModelIdSetFromPtr(ClientData clientData,
			 Tcl_Interp *interp,
			 int ac,
			 char **av)
{
    HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *omStr = NULL;			/* OBJECT_MODEL to set */
   OBJECT_MODEL *om;
   char *obj1Str = NULL;		/* OBJECT1 whose address should be stored in om.id */
   OBJECT1 *obj1;

   shErrStackClear();

   a_i = 1;
   objectModelIdSetFromPtr_opts[a_i++].dst = &omStr;
   objectModelIdSetFromPtr_opts[a_i++].dst = &obj1Str;
   shAssert(objectModelIdSetFromPtr_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objectModelIdSetFromPtr_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objectModelIdSetFromPtr_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,omStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJECT_MODEL")) {
      Tcl_AppendResult(interp,"objectModelIdSetFromPtr: "
                       "argument \"", omStr, "\" is not an OBJECT_MODEL",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   om = hand.ptr;

   if(shTclHandleExprEval(interp,obj1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJECT1")) {
      Tcl_AppendResult(interp,"objectModelIdSetFromPtr: "
                       "argument \"", obj1Str, "\" is not an OBJECT1",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   obj1 = hand.ptr;
/*
 * work
 */
   phObjectModelIdSetFromPtr(om, obj1);

   return(TCL_OK);
}

static char *tclObjectModelPtrGetFromId_use =
  "USAGE: objectModelPtrGetFromId <om>";
#define tclObjectModelPtrGetFromId_hlp \
  "Convert an OBJECT_MODEL's id array from a pointer to a real array"

static ftclArgvInfo objectModelPtrGetFromId_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectModelPtrGetFromId_hlp},
   {"<om>", FTCL_ARGV_STRING, NULL, NULL, "OBJECT_MODEL to modify"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objectModelPtrGetFromId_name "objectModelPtrGetFromId"

static int
tclObjectModelPtrGetFromId(ClientData clientData,
			     Tcl_Interp *interp,
			     int ac,
			     char **av)
{
   char name[HANDLE_NAMELEN];
    HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *omStr = NULL;			/* OBJECT_MODEL to modify */
   OBJECT_MODEL *om;
   OBJECT1 *obj1 = NULL;		/* pointer to return */

   shErrStackClear();

   a_i = 1;
   objectModelPtrGetFromId_opts[a_i++].dst = &omStr;
   shAssert(objectModelPtrGetFromId_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objectModelPtrGetFromId_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objectModelPtrGetFromId_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,omStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJECT_MODEL")) {
      Tcl_AppendResult(interp,"objectModelPtrGetFromId: "
                       "argument \"", omStr, "\" is not an OBJECT_MODEL",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   om = hand.ptr;
/*
 * work
 */
   obj1 = phObjectModelPtrGetFromId(om);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = obj1;
   hand.type = shTypeGetFromName("OBJECT1");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"obj1\" to new OBJECT1 handle",
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
phTclBrightObjectsDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"saturatedStarCentroid",
		(Tcl_CmdProc *)tclSaturatedStarCentroid, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSaturatedStarCentroid_hlp,
		tclSaturatedStarCentroid_use);

   shTclDeclare(interp,"starParamsFind",
		(Tcl_CmdProc *)tclStarParamsFind, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclStarParamsFind_hlp,
		tclStarParamsFind_use);

   shTclDeclare(interp,"compositeProfileNew",
		(Tcl_CmdProc *)tclCompositeProfileNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCompositeProfileNew_hlp,
		tclCompositeProfileNew_use);

   shTclDeclare(interp,"compositeProfileDel",
		(Tcl_CmdProc *)tclCompositeProfileDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCompositeProfileDel_hlp,
		tclCompositeProfileDel_use);

   shTclDeclare(interp,"compositeProfileAdd",
		(Tcl_CmdProc *)tclCompositeProfileAdd, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCompositeProfileAdd_hlp,
		tclCompositeProfileAdd_use);

   shTclDeclare(interp,"compositeProfileFind",
		(Tcl_CmdProc *)tclCompositeProfileFind, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCompositeProfileFind_hlp,
		tclCompositeProfileFind_use);

   shTclDeclare(interp,"brightStarWings",
		(Tcl_CmdProc *)tclBrightStarWings, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclBrightStarWings_hlp,
		tclBrightStarWings_use);

   shTclDeclare(interp,compositeProfileCopy_name,
		(Tcl_CmdProc *)tclCompositeProfileCopy, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCompositeProfileCopy_hlp,
		tclCompositeProfileCopy_use);

   shTclDeclare(interp,wingobjectNew_name,
		(Tcl_CmdProc *)tclWingobjectNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclWingobjectNew_hlp,
		tclWingobjectNew_use);

   shTclDeclare(interp,wingobjectDel_name,
		(Tcl_CmdProc *)tclWingobjectDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclWingobjectDel_hlp,
		tclWingobjectDel_use);

   shTclDeclare(interp,objectModelNew_name,
		(Tcl_CmdProc *)tclObjectModelNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectModelNew_hlp,
		tclObjectModelNew_use);

   shTclDeclare(interp,objectModelDel_name,
		(Tcl_CmdProc *)tclObjectModelDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectModelDel_hlp,
		tclObjectModelDel_use);

   shTclDeclare(interp,powerLawSetFromObject1_name,
		(Tcl_CmdProc *)tclPowerLawSetFromObject1, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPowerLawSetFromObject1_hlp,
		tclPowerLawSetFromObject1_use);

   shTclDeclare(interp,powerLawSetFromCompositeProfile_name,
		(Tcl_CmdProc *)tclPowerLawSetFromCompositeProfile, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPowerLawSetFromCompositeProfile_hlp,
		tclPowerLawSetFromCompositeProfile_use);

   shTclDeclare(interp,objectModelIdSetFromPtr_name,
		(Tcl_CmdProc *)tclObjectModelIdSetFromPtr, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectModelIdSetFromPtr_hlp,
		tclObjectModelIdSetFromPtr_use);

   shTclDeclare(interp,objectModelPtrGetFromId_name,
		(Tcl_CmdProc *)tclObjectModelPtrGetFromId, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectModelPtrGetFromId_hlp,
		tclObjectModelPtrGetFromId_use);
}
