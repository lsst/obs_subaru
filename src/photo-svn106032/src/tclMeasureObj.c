
/*
 * TCL Support for the Measure Objects module
 *
 * ENTRY POINT			SCOPE	DESCRIPTION
 * tclMeasureObjDeclare         public	Declare all the verbs defined in 
 *					this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 *	DERVISH		Survey Human Interface and Visualization
 *
 */
#include <stdio.h>
#include <string.h>
#include "dervish.h"
#include "phUtils.h"
#include "phMeasureObj.h"

static char *module = "phTclMeasureObjects"; /* name of this set of code */


static int
parse_values(int num_val, char *str, float *values);

/*****************************************************************************/

static char *phTclInitMeasureObj_use =
  "USAGE: initMeasureObj <fieldparams> <nfilter> <sb_const> <rand>";
#define phTclInitMeasureObj_hlp \
  "Initialize some variables used by the Measure Objects module."

static ftclArgvInfo initMeasureObj_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, phTclInitMeasureObj_hlp},
   {"<field_params>", FTCL_ARGV_STRING, NULL, NULL,
			     "FIELDPARAMS with parameters for measure objects"},
   {"<ncolor>", FTCL_ARGV_INT, NULL, NULL, "number of filters in use"},
   {"<sbmag>", FTCL_ARGV_STRING, NULL, NULL,
		      "list of isophotal surface brightnesses for each filter"},
   {"<rand>", FTCL_ARGV_STRING, NULL, NULL, "a RANDOM"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};


static int
phTclInitMeasureObj(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   int i;
   FIELDPARAMS *fparams;
   HANDLE hand;
   RANDOM *rand;
   float sb_array[NCOLOR];
   void *vptr;				/* used by shTclHandleExprEval */
   char *field_paramsStr = NULL;	/* FIELDPARAMS with needed parameters */
   int ncolor = 0;			/* number of filters in use */
   char *sbmagStr = NULL;		/* list of isophotal surface
					   brightnesses for each filter */
   char *randStr = NULL;		/* a RANDOM */

   shErrStackClear();

   i = 1;
   initMeasureObj_opts[i++].dst = &field_paramsStr;
   initMeasureObj_opts[i++].dst = &ncolor;
   initMeasureObj_opts[i++].dst = &sbmagStr;
   initMeasureObj_opts[i++].dst = &randStr;
   shAssert(initMeasureObj_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&argc,argv,initMeasureObj_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Process arguments
 */
   if(shTclHandleExprEval(interp,field_paramsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDPARAMS")) {
      Tcl_SetResult(interp,"initMeasureObj: "
                    "first argument is not a FIELDPARAMS",TCL_STATIC);
      return(TCL_ERROR);
   }
   fparams = hand.ptr;

   if(strcmp(randStr,"null") == 0 || strcmp(randStr,"NULL") == 0) {
      rand = NULL;
   } else {
      if(shTclHandleExprEval(interp,randStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("RANDOM")) {
	 Tcl_SetResult(interp,"initMeasureObj: "
		       "second argument is not a RANDOM",TCL_STATIC);
	 return(TCL_ERROR);
      }
      rand = hand.ptr;
   }

   shAssert(ncolor <= NCOLOR);
   parse_values(ncolor, sbmagStr, sb_array);
/*
 * do the work
 */
   if(phInitMeasureObj(fparams, ncolor, sb_array, rand) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*************************************************************************
 *  Free up memory and/or structures used by Measure Objects module. 
 */
static char *phTclFiniMeasureObj_use =
  "USAGE: finiMeasureObj";
#define phTclFiniMeasureObj_hlp \
  "De-allocate some variables used by the Measure Objects module."

static ftclArgvInfo finiMeasureObj_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, phTclFiniMeasureObj_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
phTclFiniMeasureObj(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   shErrStackClear();
   
   if(get_FtclOpts(interp,&argc,argv,finiMeasureObj_opts) != TCL_OK) {
      return(TCL_ERROR);
   }

   if(phFiniMeasureObj() != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclMeasureObjColorSet_use =
  "USAGE: MeasureObjColorSet ";
#define tclMeasureObjColorSet_hlp \
  ""

static ftclArgvInfo measureObjColorSet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMeasureObjColorSet_hlp},
   {"<fieldparams>", FTCL_ARGV_STRING, NULL, NULL,
			    "FIELDPARAMS with parameters for measure objects"},
   {"<fieldstat>", FTCL_ARGV_STRING, NULL, NULL,
					  "FIELDSTAT for statistics of field"},
   {"<color_index>", FTCL_ARGV_INT, NULL, NULL, "index of colour"},
   {"<calib1>", FTCL_ARGV_STRING, NULL, NULL, "handle to CALIB1"},
   {"<corrected>", FTCL_ARGV_STRING, NULL, NULL, "handle to corrected frame"},
   {"<sky>", FTCL_ARGV_STRING, NULL, NULL, "BINREGION of sky level"},
   {"<skyErr>", FTCL_ARGV_STRING, NULL, NULL, "BINREGION of sky sigma"},
   {"<basis>", FTCL_ARGV_STRING, NULL, NULL, "PSF_BASIS or NULL"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclMeasureObjColorSet(
		      ClientData clientDatag,
		      Tcl_Interp *interp,
		      int ac,
		      char **av
		      )
{
   int i;
   CALIB1 *calib;
   REGION *corr;
   FIELDPARAMS *fiparams;
   FIELDSTAT *fieldstat;
   HANDLE hand;
   BINREGION *sky, *skyErr;
   void *vptr;				/* used by shTclHandleExprEval */
   char *fieldparamsStr = NULL;		/* FIELDPARAMS for measure objects */
   char *fieldstatStr = NULL;		/* field's FIELDSTAT */
   int color_index = 0;			/* index of colour */
   char *calib1Str = NULL;		/* handle to CALIB1 */
   char *correctedStr = NULL;		/* handle to corrected frame */
   char *skyStr = NULL;			/* BINREGION of sky level */
   char *skyErrStr = NULL;		/* BINREGION of sky sigma */
   char *basisStr = NULL;		/* PSF_BASIS */
   PSF_BASIS *basis;

   shErrStackClear();

   i = 1;
   measureObjColorSet_opts[i++].dst = &fieldparamsStr;
   measureObjColorSet_opts[i++].dst = &fieldstatStr;
   measureObjColorSet_opts[i++].dst = &color_index;
   measureObjColorSet_opts[i++].dst = &calib1Str;
   measureObjColorSet_opts[i++].dst = &correctedStr;
   measureObjColorSet_opts[i++].dst = &skyStr;
   measureObjColorSet_opts[i++].dst = &skyErrStr;
   measureObjColorSet_opts[i++].dst = &basisStr;
   shAssert(measureObjColorSet_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,measureObjColorSet_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * process those arguments
 */
   if(shTclHandleExprEval(interp,fieldparamsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDPARAMS")) {
      Tcl_SetResult(interp,"measureObjColorSet: "
		    "first argument is not a FIELDPARAMS",TCL_STATIC);
      return(TCL_ERROR);
   }
   fiparams = hand.ptr;

   if(shTclHandleExprEval(interp,fieldstatStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDSTAT")) {
      Tcl_SetResult(interp,"measureObjColorSet: "
                    "argument \"fieldstat\" is not a FIELDSTAT",TCL_STATIC);
      return(TCL_ERROR);
   }
   fieldstat = hand.ptr;

   if(shTclHandleExprEval(interp,calib1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CALIB1")) {
      Tcl_SetResult(interp,"measureObjColorSet: third argument is not a "
		    					  "CALIB1",TCL_STATIC);
      return(TCL_ERROR);
   }
   calib = hand.ptr;

   if(shTclHandleExprEval(interp,correctedStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"measureObjColorSet: fourth argument is not a "
		    					  "REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   corr = hand.ptr;
   if(corr != NULL && corr->type != TYPE_PIX) {
      Tcl_SetResult(interp,"measureObjColorSet: fourth argument is not U16",
		    					  	   TCL_STATIC);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,skyStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("BINREGION")) {
      Tcl_SetResult(interp,"measureObjcColorSet: "
                    "fifth argument is not a BINREGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   sky = hand.ptr;

   if(shTclHandleExprEval(interp,skyErrStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("BINREGION")) {
      Tcl_SetResult(interp,"measureObjcColorSet: "
                    "sixth argument is not a BINREGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   skyErr = hand.ptr;

   if(strcmp(basisStr, "") == 0 ||
      strcmp(basisStr, "null") == 0 || strcmp(basisStr, "NULL") == 0) {
      basis = NULL;
   } else {
      if(shTclHandleExprEval(interp,basisStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("PSF_BASIS")) {
	 Tcl_AppendResult(interp,"measureObjcColorSet: "
			  "argument \"", basisStr, "\" is not a PSF_BASIS",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      basis = hand.ptr;
   }
/*
 * Do the work
 */
   if(phMeasureObjColorSet(fiparams, fieldstat, color_index, calib, corr,
					   sky, skyErr, basis) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   
   return(TCL_OK);
}

/*****************************************************************************/
static char *tclMeasureObjUnset_use =
  "USAGE: MeasureObjUnset fparams";
#define tclMeasureObjUnset_hlp \
  "The inverse of measureObjColorSet (but need only be called once, not "\
"once per colour)"

static ftclArgvInfo measureObjUnset_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMeasureObjUnset_hlp},
   {"<fieldparams>", FTCL_ARGV_STRING, NULL, NULL,
			     "FIELDPARAMS with parameters for measure objects"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclMeasureObjUnset(
		   ClientData clientDatag,
		   Tcl_Interp *interp,
		   int ac,
		   char **av
		   )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *fieldparamsStr = NULL;	/* FIELDPARAMS for measure objects */

   shErrStackClear();

   i = 1;
   measureObjUnset_opts[i++].dst = &fieldparamsStr;
   shAssert(measureObjUnset_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,measureObjUnset_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * process those arguments
 */
   if(shTclHandleExprEval(interp,fieldparamsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDPARAMS")) {
      Tcl_SetResult(interp,"measureObjColorUnset: "
		    "first argument is not a FIELDPARAMS",TCL_STATIC);
      return(TCL_ERROR);
   }

   phMeasureObjUnset(hand.ptr);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclDeblendSet_use =
  "USAGE: deblendSet ";
#define tclDeblendSet_hlp \
  ""

static ftclArgvInfo deblendSet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclDeblendSet_hlp},
   {"<scr0>", FTCL_ARGV_STRING, NULL, NULL, "scratch space for deblender"},
   {"<scr1>", FTCL_ARGV_STRING, NULL, NULL, "scratch space for deblender"},
   {"<scr2>", FTCL_ARGV_STRING, NULL, NULL, "scratch space for deblender"},
   {"<scr3>", FTCL_ARGV_STRING, NULL, NULL, "scratch space for deblender"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclDeblendSet(
	      ClientData clientDatag,
	      Tcl_Interp *interp,
	      int ac,
	      char **av
	      )
{
   int i;
   HANDLE hand;
   REGION *scr0, *scr1, *scr2, *scr3;
   void *vptr;				/* used by shTclHandleExprEval */
   char *scr0Str = NULL;		/* scratch space for deblender */
   char *scr1Str = NULL;		/* scratch space for deblender */
   char *scr2Str = NULL;		/* scratch space for deblender */
   char *scr3Str = NULL;		/* scratch space for deblender */

   shErrStackClear();

   i = 1;
   deblendSet_opts[i++].dst = &scr0Str;
   deblendSet_opts[i++].dst = &scr1Str;
   deblendSet_opts[i++].dst = &scr2Str;
   deblendSet_opts[i++].dst = &scr3Str;
   shAssert(deblendSet_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,deblendSet_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(strcmp(scr0Str,"NULL") == 0 || strcmp(scr0Str,"null") == 0) {
      scr0 = NULL;
   } else {
      if(shTclHandleExprEval(interp,scr0Str,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_SetResult(interp,"measureObjc: "
		       "first argument is not a REGION",TCL_STATIC);
	 return(TCL_ERROR);
      }
      scr0 = hand.ptr;
   }
   
   if(strcmp(scr1Str,"NULL") == 0 || strcmp(scr1Str,"null") == 0) {
      scr1 = NULL;
   } else {
      if(shTclHandleExprEval(interp,scr1Str,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_SetResult(interp,"measureObjc: "
		       "second argument is not a REGION",TCL_STATIC);
	 return(TCL_ERROR);
      }
      scr1 = hand.ptr;
   }

   if(strcmp(scr2Str,"NULL") == 0 || strcmp(scr2Str,"null") == 0) {
      scr2 = NULL;
   } else {
      if(shTclHandleExprEval(interp,scr2Str,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_SetResult(interp,"measureObjc: "
		       "third argument is not a REGION",TCL_STATIC);
	 return(TCL_ERROR);
      }
      scr2 = hand.ptr;
   }   

   if(strcmp(scr3Str,"NULL") == 0 || strcmp(scr3Str,"null") == 0) {
      scr3 = NULL;
   } else {
      if(shTclHandleExprEval(interp,scr3Str,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_SetResult(interp,"measureObjc: "
		       "third argument is not a REGION",TCL_STATIC);
	 return(TCL_ERROR);
      }
      scr3 = hand.ptr;
   }   
/*
 * work
 */
   phDeblendSet(scr0, scr1, scr2, scr3);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclDeblendUnset_use =
  "USAGE: DeblendUnset ";
#define tclDeblendUnset_hlp \
  ""

static ftclArgvInfo deblendUnset_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclDeblendUnset_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclDeblendUnset(
		ClientData clientDatag,
		Tcl_Interp *interp,
		int ac,
		char **av
		)
{
   shErrStackClear();

   if(get_FtclOpts(interp,&ac,av,deblendUnset_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */

/*
 * work
 */
   phDeblendUnset();

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Measure a single OBJC
 */
static char *tclMeasureObjc_use =
  "USAGE: measureObjc <objc> <field_params> <fieldstat> -no_model";
#define tclMeasureObjc_hlp \
  "Measure an object; you must call measureObjColorSet first and "\
"measureObjUnset afterwards."

static ftclArgvInfo measureObjc_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMeasureObjc_hlp},
   {"<objc>", FTCL_ARGV_STRING, NULL, NULL, "handle to OBJC"},
   {"<field_params>", FTCL_ARGV_STRING, NULL, NULL,
			    "FIELDPARAMS with parameters for measure objects"},
   {"<fieldstat>", FTCL_ARGV_STRING, NULL, NULL,
					   "a FIELDSTAT to collect a summary"},
   {"-remove_obj", FTCL_ARGV_CONSTANT, (void *)1, NULL,
	    "replace all pixels in atlas image with background level + noise"},
   {"-bright", FTCL_ARGV_CONSTANT, (void *)1, NULL,
				"measure bright objects (e.g. no deblending)"},
   {"-no_model", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					       "Don't run the model-fit code"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define measureObjc_name "measureObjc"

static int
tclMeasureObjc(
	       ClientData clientDatag,
	       Tcl_Interp *interp,
	       int ac,
	       char **av
	       )
{
   int i;
   HANDLE hand;
   FIELDPARAMS *fparams;
   FIELDSTAT *fieldstat;
   OBJC *objc;
   void *vptr;				/* used by shTclHandleExprEval */
   char *objcStr = NULL;		/* handle to OBJC */
   char *field_paramsStr = NULL;	/* FIELDPARAMS for measure objects */
   char *fieldstatStr = NULL;		/* a FIELDSTAT to collect a summary */
   int remove_obj = 0;			/* replace all pixels in atlas image
					   with background level + noise */
   int bright = 0;			/* measure bright objects */
   int no_model = 0;			/* Don't run the model-fit code */

   shErrStackClear();

   i = 1;
   measureObjc_opts[i++].dst = &objcStr;
   measureObjc_opts[i++].dst = &field_paramsStr;
   measureObjc_opts[i++].dst = &fieldstatStr;
   measureObjc_opts[i++].dst = &remove_obj;
   measureObjc_opts[i++].dst = &bright;
   measureObjc_opts[i++].dst = &no_model;
   shAssert(measureObjc_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, measureObjc_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     measureObjc_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * process args
 */
   if(shTclHandleExprEval(interp,objcStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJC")) {
      Tcl_SetResult(interp,"measureObjc: first argument is not a OBJC",
		    						   TCL_STATIC);
      return(TCL_ERROR);
   }
   objc = hand.ptr;

   if(shTclHandleExprEval(interp, field_paramsStr, &hand, &vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDPARAMS")) {
      Tcl_SetResult(interp,"measureObjc: "
		    "second argument is not a FIELDPARAMS", TCL_STATIC);
      return(TCL_ERROR);
   }
   fparams = hand.ptr;

   if(shTclHandleExprEval(interp,fieldstatStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDSTAT")) {
      Tcl_SetResult(interp,"measureObjc: "
                    "third argument is not a FIELDSTAT",TCL_STATIC);
      return(TCL_ERROR);
   }
   fieldstat = hand.ptr;
/*
 * Do the work
 */
   if(phMeasureObjc(objc, fparams, fieldstat, remove_obj, bright, !no_model)
							       != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/**************************************************************************
 * given a string of the form
 *
 *       "1.2 2 3.5 6 10 20"
 *
 * i.e. with numerical values separated by spaces, parse the string
 * to retrieve the numerical values.  Make sure that there are exactly
 * 'num_val' of them.
 * Place the values into the array 'values'.
 *
 * return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if there's
 * a problem.
 */

static int
parse_values(int num_val, char *str, float *values)
{
   int i, pos, len;

   pos = 0;
   len = strlen(str);
   for (i = 0; i < num_val; i++) {
      if (pos >= len) {
         shErrStackPush("parse_radii: ran out of values in string %s",
               str);
         return(SH_GENERIC_ERROR);
      }
      if (sscanf(str + pos, "%f", &(values[i])) != 1) {
         shErrStackPush("parse_values: can't read %d'th number in string %s",
               i, str);
         return(SH_GENERIC_ERROR);
      }
      /* skip to the end of this number ... */
      while ((str[pos] != ' ') && (str[pos] != '\0')) {
         pos++;
      }
      /* and skip to the start of the next number */
      while (str[pos] == ' ') {
         pos++;
      }
   }

#ifdef DEBUG
   for (i = 0; i < num_val; i++) {
      printf("parse_values: value %2d is %6.2f\n", i, values[i]);
   }
#endif

   return(SH_SUCCESS);
}

/*****************************************************************************/
static char *tclAtlasImageCut_use =
  "USAGE: AtlasImageCut <objc> <fparams> [-replace val] [-sigma sig] [-rand rand]";
#define tclAtlasImageCut_hlp \
  "Set the atlas image fields in <objc>, using the offsets and data "\
"defined by <fparams>. Assumes that the OBJC has it's colc, rowc fields set, "\
"and that the master_mask is set in the OBJC's ATLAS_IMAGE"\
"If -replace is specified, replace pixels in the atlas image's master_mask "\
"val (and if -sigma's specified too, add Gaussian N(0,sigma^2) noise too, "\
"in which case -rand _must_ specify a RANDOM)."

static ftclArgvInfo atlasImageCut_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclAtlasImageCut_hlp},
   {"<objc>", FTCL_ARGV_STRING, NULL, NULL, "The Objc to set"},
   {"<fparams>", FTCL_ARGV_STRING, NULL, NULL,
					  "A properly initialised FIELDPARAMS"},
   {"-replace", FTCL_ARGV_INT, NULL, NULL,
					"set all pixels in mask to this value"},
   {"-sigma", FTCL_ARGV_DOUBLE, NULL, NULL,
				      "standard deviation of val (default: 0)"},
   {"-rand", FTCL_ARGV_STRING, NULL, NULL, "A RANDOM"},
   {"-color", FTCL_ARGV_INT, NULL, NULL, "which colour to set (default: all)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclAtlasImageCut(
		 ClientData clientDatag,
		 Tcl_Interp *interp,
		 int ac,
		 char **av
		 )
{
   int i;
   OBJC *objc;
   FIELDPARAMS *fparams;
   RANDOM *rand;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *objcStr = NULL;		/* The Objc to set */
   char *fparamsStr = NULL;		/* A properly initialised FIELDPARAMS */
   int replace = -1;			/* set all pixels in mask to this */
   double sigma = -1.0;			/* standard deviation of val */
   char *randStr = NULL;		/* A RANDOM */
   int color = -1;			/* which colour to set (default: all) */

   shErrStackClear();

   i = 1;
   atlasImageCut_opts[i++].dst = &objcStr;
   atlasImageCut_opts[i++].dst = &fparamsStr;
   atlasImageCut_opts[i++].dst = &replace;
   atlasImageCut_opts[i++].dst = &sigma;
   atlasImageCut_opts[i++].dst = &randStr;
   atlasImageCut_opts[i++].dst = &color;
   shAssert(atlasImageCut_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,atlasImageCut_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objcStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJC")) {
      Tcl_SetResult(interp,"atlasImageCut: "
                    "first argument is not a OBJC",TCL_STATIC);
      return(TCL_ERROR);
   }
   objc = hand.ptr;

   if(!(objc->flags3 & OBJECT3_HAS_CENTER)) {
      Tcl_SetResult(interp,"atlasImageCut: "
                    "OBJC hasn't had its colc, rowc set",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(objc->aimage->master_mask == NULL) {
      Tcl_SetResult(interp,"atlasImageCut: "
	       "OBJC's ATLAS_IMAGE hasn't had its master_mask set",TCL_STATIC);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,fparamsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDPARAMS")) {
      Tcl_SetResult(interp,"atlasImageCut: "
                    "second argument is not a FIELDPARAMS",TCL_STATIC);
      return(TCL_ERROR);
   }
   fparams = hand.ptr;

   if(sigma <= 0) {			/* no need for rand */
      rand = NULL;
   } else {				/* must specify -rand */
      if(randStr == NULL) {
	 Tcl_SetResult(interp,"atlasImageCut: "
		       "you _must_ provide -rand if sigma > 0",TCL_STATIC);
	 return(TCL_ERROR);
      }
      if(shTclHandleExprEval(interp,randStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("RANDOM")) {
	 Tcl_SetResult(interp,"atlasImageCut: "
		       "-rand's argument is not a RANDOM",TCL_STATIC);
	 return(TCL_ERROR);
      }
      rand = hand.ptr;
   }
/*
 * work
 */
   phAtlasImageCut(objc, color, fparams, replace, sigma, rand);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclRegSetFromAtlasImage_use =
  "USAGE: regSetFromAtlasImage <ai> <color> <region> -add -subtract";
#define tclRegSetFromAtlasImage_hlp \
"Set pixels in the <region> from the <color>'th data in the atlas image <ai> "\
"The logical origin of the region is taken to be at (row0,col0) (default: 0) "\
"If you specify -ignore_off, the values of drow and dcol in the atlas image "\
"are ignored"

static ftclArgvInfo regSetFromAtlasImage_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegSetFromAtlasImage_hlp},
   {"<ai>", FTCL_ARGV_STRING, NULL, NULL, "The ATLAS_IMAGE"},
   {"<color>", FTCL_ARGV_INT, NULL, NULL, "The colour to use"},
   {"<region>", FTCL_ARGV_STRING, NULL, NULL, "The region to set"},
   {"-row0", FTCL_ARGV_INT, NULL, NULL, "Logical row-origin of region"},
   {"-col0", FTCL_ARGV_INT, NULL, NULL, "Logical column origin of region"},
   {"-ignore_off", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					   "Ignore offsets specified in <ai>"},
   {"-sky", FTCL_ARGV_DOUBLE, NULL, NULL, "Sky level to add to atlas image"},
   {"-clip", FTCL_ARGV_CONSTANT, (void *)1, NULL,
    "Clip atlas image to pixels within mask?"},
   {"-add", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Add atlas image to region"},
   {"-subtract", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Subtract atlas image from region"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRegSetFromAtlasImage(
			ClientData clientDatag,
			Tcl_Interp *interp,
			int ac,
			char **av
			)
{
   int a_i;
   ATLAS_IMAGE *ai;
   REGION *reg;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *aiStr = NULL;			/* The ATLAS_IMAGE */
   int color = 0;			/* The colour to use */
   char *regionStr = NULL;		/* The region to set */
   int row0 = 0;			/* Logical row-origin of region */
   int col0 = 0;			/* Logical column origin of region */
   int ignore_off = 0;			/* Ignore offsets specified in <ai> */
   double sky = 0;			/* Sky level to add to atlas image */
   int clip = 0;			/* Clip atlas image to pixels within mask? */
   int operator = '\0';			/* set, add, or subtract */
   int add = 0;				/* Add atlas image to region */
   int subtract = 0;			/* Subtract atlas image from region */

   shErrStackClear();

   a_i = 1;
   regSetFromAtlasImage_opts[a_i++].dst = &aiStr;
   regSetFromAtlasImage_opts[a_i++].dst = &color;
   regSetFromAtlasImage_opts[a_i++].dst = &regionStr;
   regSetFromAtlasImage_opts[a_i++].dst = &row0;
   regSetFromAtlasImage_opts[a_i++].dst = &col0;
   regSetFromAtlasImage_opts[a_i++].dst = &ignore_off;
   regSetFromAtlasImage_opts[a_i++].dst = &sky;
   regSetFromAtlasImage_opts[a_i++].dst = &clip;
   regSetFromAtlasImage_opts[a_i++].dst = &add;
   regSetFromAtlasImage_opts[a_i++].dst = &subtract;
   shAssert(regSetFromAtlasImage_opts[a_i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,regSetFromAtlasImage_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,aiStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("ATLAS_IMAGE")) {
      Tcl_SetResult(interp,"regSetFromAtlasImage: "
                    "first argument is not a ATLAS_IMAGE",TCL_STATIC);
      return(TCL_ERROR);
   }
   ai = hand.ptr;

   if(color < 0 || color > ai->ncolor) {
      Tcl_SetResult(interp,"regSetFromAtlasImage: "
                    "Desired colour isn't present in ATLAS_IMAGE",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(ai->pix[color] == NULL) {
      Tcl_SetResult(interp,"regSetFromAtlasImage: "
                    "pixel data isn't present in ATLAS_IMAGE",
		    TCL_STATIC);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,regionStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regSetFromAtlasImage: "
                    "third argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(ignore_off) {
      row0 += ai->drow[color];
      col0 += ai->dcol[color];
   }

   if (add && subtract) {
       Tcl_SetResult(interp, "regSetFromAtlasImage: Please choose either -add or -subtract", TCL_STATIC);
      return(TCL_ERROR);
   }
   if(add) {
       operator = '+';
   } else if(subtract) {
       operator = '-';
   } else {
       operator = '\0';
   }
/*
 * work
 */
   phRegionSetFromAtlasImage(ai, color, reg, row0, col0, sky, operator, clip);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclFieldstatSetFromMO_use =
  "USAGE: FieldstatSetFromMO <fieldstat>";
#define tclFieldstatSetFromMO_hlp \
  "Calculate statistics accumulated from measuring objects, and put in <fieldstat>"

static ftclArgvInfo fieldstatSetFromMO_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFieldstatSetFromMO_hlp},
   {"<fieldstat>", FTCL_ARGV_STRING, NULL, NULL, "The FIELDSTAT to fill"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFieldstatSetFromMO(
		      ClientData clientDatag,
		      Tcl_Interp *interp,
		      int ac,
		      char **av
		      )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *fieldstatStr = NULL;		/* The FIELDSTAT to fill */

   shErrStackClear();

   i = 1;
   fieldstatSetFromMO_opts[i++].dst = &fieldstatStr;
   shAssert(fieldstatSetFromMO_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fieldstatSetFromMO_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,fieldstatStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDSTAT")) {
      Tcl_SetResult(interp,"fieldstatSetFromMO: "
                    "argument is not a FIELDSTAT",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phFieldstatSetFromMO(hand.ptr);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * procs to access the psf magnitude code from TCL
 */
static char *tclPsfCountsFromRegionSetup_use =
  "USAGE: psfCountsFromRegionSetup <psf> [band]";
#define tclPsfCountsFromRegionSetup_hlp \
  "Setup measure objects to use a given DGPSF (as set by fitPsfFromReg)"

static ftclArgvInfo psfCountsFromRegionSetup_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfCountsFromRegionSetup_hlp},
   {"<psf>", FTCL_ARGV_STRING, NULL, NULL, "DGPSF describing seeing"},
   {"[band]", FTCL_ARGV_INT, NULL, NULL,
				      "The colour that the PSF's measured in"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfCountsFromRegionSetup_name "psfCountsFromRegionSetup"

static int
tclPsfCountsFromRegionSetup(ClientData clientData,
			    Tcl_Interp *interp,
			    int ac,
			    char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   int band = 0;			/* colour that the PSF's measured in */
   char *psfStr = NULL;			/* DGPSF describing seeing */
   DGPSF *psf;
   float neff;

   shErrStackClear();

   i = 1;
   psfCountsFromRegionSetup_opts[i++].dst = &psfStr;
   psfCountsFromRegionSetup_opts[i++].dst = &band;
   shAssert(psfCountsFromRegionSetup_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfCountsFromRegionSetup_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfCountsFromRegionSetup_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,psfStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("DGPSF")) {
      Tcl_AppendResult(interp,"psfCountsFromRegionSetup: "
                       "argument \"", psfStr, "\" is not a DGPSF",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   psf = hand.ptr;

   if(band < 0 || band >= NCOLOR) {
      char str[10]; sprintf(str, "%d", band);
      Tcl_SetResult(interp, "psfCountsFromRegionSetup: ", TCL_STATIC);
      Tcl_AppendResult(interp, "band ", str, " is out of range", (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   neff = phPsfCountsSetupFromDgpsf(band, psf, 1);
   {
      char str[20]; sprintf(str, "%.3f", neff);
      Tcl_AppendResult(interp, str, (char *)NULL);
   }

   return(TCL_OK);
}

static char *tclPsfCountsFromRegion_use =
  "USAGE: psfCountsFromRegion <reg> <rowc> <colc> -band -bkgd -gain -dark_variance -subtracted_bkgd -error -aperture_correction dval";
#define tclPsfCountsFromRegion_hlp \
  "Return the psfCounts measured at a position in a REGION"

static ftclArgvInfo psfCountsFromRegion_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfCountsFromRegion_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to be measured"},
   {"<rowc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Row position to measure"},
   {"<colc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Column position to measure"},
   {"-band", FTCL_ARGV_INT, NULL, NULL, "Band data was taken in"},
   {"-bkgd", FTCL_ARGV_DOUBLE, NULL, NULL, "Background level of reg"},
   {"-gain", FTCL_ARGV_DOUBLE, NULL, NULL, "Gain of amplifiers"},
   {"-dark_variance", FTCL_ARGV_DOUBLE, NULL, NULL,
				    "Per-pixel variance in absence of signal"},
   {"-subtracted_bkgd", FTCL_ARGV_DOUBLE, NULL, NULL,
			    "Background level already subtracted from region"},
   {"-error", FTCL_ARGV_STRING, NULL, NULL,
			     "Name of variable to contain error in psfCounts"},
   {"-aperture_correction", FTCL_ARGV_DOUBLE, NULL, NULL,
					    "Aperture correction to apply"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfCountsFromRegion_name "psfCountsFromRegion"

static int
tclPsfCountsFromRegion(ClientData clientData,
		       Tcl_Interp *interp,
		       int ac,
		       char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to be measured */
   REGION *reg;
   int band = 0;			/* Band data was taken in */
   double rowc = 0.0;			/* Row position to measure */
   double colc = 0.0;			/* Column position to measure */
   double bkgd = 0.0;			/* Background level of reg */
   double gain = 2.0;			/* Gain of amplifiers */
   double dark_variance = 0.0;		/* Per-pixel variance when signal==0 */
   double subtracted_bkgd = 0.0;	/* Background level already sub'ed */
   char *errorStr = NULL;		/* Name of variable for psfCountsErr */
   float psfCounts;
   float psfCountsErr = 0;
   double aperture_correction = 1.0;	/* Aperture correction to apply */

   shErrStackClear();

   i = 1;
   psfCountsFromRegion_opts[i++].dst = &regStr;
   psfCountsFromRegion_opts[i++].dst = &rowc;
   psfCountsFromRegion_opts[i++].dst = &colc;
   psfCountsFromRegion_opts[i++].dst = &band;
   psfCountsFromRegion_opts[i++].dst = &bkgd;
   psfCountsFromRegion_opts[i++].dst = &gain;
   psfCountsFromRegion_opts[i++].dst = &dark_variance;
   psfCountsFromRegion_opts[i++].dst = &subtracted_bkgd;
   psfCountsFromRegion_opts[i++].dst = &errorStr;
   psfCountsFromRegion_opts[i++].dst = &aperture_correction;
   shAssert(psfCountsFromRegion_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfCountsFromRegion_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfCountsFromRegion_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"psfCountsFromRegion: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(reg->type != TYPE_PIX) {
      Tcl_SetResult(interp, "psfCountsFromRegion: ", TCL_STATIC);
      Tcl_AppendResult(interp, "REGION ", regStr, " is not U16", (char *)NULL);
      return(TCL_ERROR);
   }

   if(band < 0 || band >= NCOLOR) {
      char str[10]; sprintf(str, "%d", band);
      Tcl_SetResult(interp, "psfCountsFromRegion: ", TCL_STATIC);
      Tcl_AppendResult(interp, "band ", str, " is out of range", (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   psfCounts = phPsfCountsFromRegion(reg, band, rowc, colc,
				     aperture_correction, bkgd, gain,
				     dark_variance, subtracted_bkgd,
				     &psfCountsErr);
/*
 * Return answer
 */
   {
      char str[40]; sprintf(str, "%.3f", psfCounts);
      Tcl_AppendResult(interp, str, (char *)NULL);
   }

   if(errorStr != NULL) {
      char str[40]; sprintf(str, "%.3f", psfCountsErr);
      if(Tcl_SetVar(interp, errorStr, str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return error",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 
   
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclPsfSetAtPoint_use =
  "USAGE: psfSetAtPoint <basis> <color> <rowc> <colc> -sigma dval";
#define tclPsfSetAtPoint_hlp \
  "\
Given the position in a frame, set the PSF to the local value, including\n\
calculating and returning the proper aperture correction\n\
\n\
N.b. This routine is not responsible for setting the PSF coefficients\n\
used for the entire frame, it merely adjusts things for the local PSF\n\
"

static ftclArgvInfo psfSetAtPoint_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPsfSetAtPoint_hlp},
   {"<basis>", FTCL_ARGV_STRING, NULL, NULL, \
      "PSF_BASIS for this frame (or NULL)"},
   {"<color>", FTCL_ARGV_INT, NULL, NULL, "in this band"},
   {"<rowc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Row-centre of desired PSF"},
   {"<colc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Column-centre of desired PSF"},
   {"-sigma", FTCL_ARGV_DOUBLE, NULL, NULL,
				  "Smooth with N(0,sigma^2) when centroiding"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define psfSetAtPoint_name "psfSetAtPoint"

static int
tclPsfSetAtPoint(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *basisStr = NULL;		/* PSF_BASIS for this frame */
   PSF_BASIS *basis;
   int color = 0;			/* in this band */
   double rowc = 0.0;			/* Row-centre of desired PSF */
   double colc = 0.0;			/* Column-centre of desired PSF */
   double sigma = 1.0;			/* Smooth with N(0,sigma^2)
					   when centroiding */

   shErrStackClear();

   a_i = 1;
   psfSetAtPoint_opts[a_i++].dst = &basisStr;
   psfSetAtPoint_opts[a_i++].dst = &color;
   psfSetAtPoint_opts[a_i++].dst = &rowc;
   psfSetAtPoint_opts[a_i++].dst = &colc;
   psfSetAtPoint_opts[a_i++].dst = &sigma;
   shAssert(psfSetAtPoint_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, psfSetAtPoint_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     psfSetAtPoint_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(*basisStr == '\0' || strcmp(basisStr, "NULL") == 0) {
      basis = NULL;
   } else {
      if(shTclHandleExprEval(interp,basisStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("PSF_BASIS")) {
	 Tcl_AppendResult(interp,"psfSetAtPoint: "
			  "argument \"", basisStr, "\" is not a PSF_BASIS",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      basis = hand.ptr;
   }

   if(color < 0 || color >= NCOLOR) {
      char buff[100];
      sprintf(buff, "psfSetAtPoint: invalid colour %d (must be 0..%d)",
	      color, NCOLOR - 1);
      Tcl_SetResult(interp, buff, TCL_VOLATILE);
      return(TCL_ERROR);
   }
/*
 * work
 */
   (void)phPsfSetAtPoint(color, basis, rowc, colc, sigma, NULL);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclCalcPetrosian_use =
  "USAGE: calcPetrosian <reg> <rowc> <colc> -gain dval -dark_variance dval -petro_f1 dval -petro_f2 dval -petro_f4 dval -petro_f5 dval -bkgd dval";
#define tclCalcPetrosian_hlp \
  ""

static ftclArgvInfo calcPetrosian_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCalcPetrosian_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region containing object"},
   {"<rowc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Row-centre of object"},
   {"<colc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Column-centre of object"},
   {"-gain", FTCL_ARGV_DOUBLE, NULL, NULL, "Gain of amplifier"},
   {"-sky", FTCL_ARGV_DOUBLE, NULL, NULL,
				     "Background level (excluding SOFT_BIAS)"},
   {"-dark_variance", FTCL_ARGV_DOUBLE, NULL, NULL, "Background Variance"},
   {"-petro_f1", FTCL_ARGV_DOUBLE, NULL, NULL, "Petrosian Radius"},
   {"-petro_f2", FTCL_ARGV_DOUBLE, NULL, NULL,
			    "Mimumum flux at Petrosian radius (counts/pixel)"},
   {"-petro_f4", FTCL_ARGV_DOUBLE, NULL, NULL,
			      "Measure flux within this many Petrosian Radii"},
   {"-petro_f5", FTCL_ARGV_DOUBLE, NULL, NULL,
				   "Radius to use if all else fails (pixels)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define calcPetrosian_name "calcPetrosian"

static int
tclCalcPetrosian(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   HANDLE hand;
   char buff[40];			/* buffer to return results */
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region containing object */
   REGION *reg;
   double rowc = 0.0;			/* Row-centre of object */
   double colc = 0.0;			/* Column-centre of object */
   double gain = 1.0;			/* Gain of amplifier */
   double dark_variance = 0.0;		/* Background Variance */
   double petro_f1 = 0.2;		/* Petrosian Radius */
   double petro_f2 = 0.0;		/* Mimumum flux at Petrosian radius
					   (counts/pixel) */
   double petro_f4 = 2.0;		/* Measure flux within this many
					   Petrosian Radii */
   double petro_f5 = 7.5;		/* Radius to use if all else fails
					   (pixels) */
   float petroRad = 0;			/* measured Petrosian radius */
   float petroRadErr = 0;		/* measured Petrosian radius error */
   float petroR50 = 0;			/* measured Petrosian 50% radius */
   float petroR50Err = 0;		/* measured Petrosian 50% error */
   float petroR90 = 0;			/* measured Petrosian 90% radius */
   float petroR90Err = 0;		/* measured Petrosian 90% error */
   float petroCounts = 0;		/* measured Petrosian counts */
   float petroCountsErr = 0;		/* measured Petrosian counts error */
   double sky = 0.0;			/* Background level (excluding SOFT_BIAS) */

   shErrStackClear();

   a_i = 1;
   calcPetrosian_opts[a_i++].dst = &regStr;
   calcPetrosian_opts[a_i++].dst = &rowc;
   calcPetrosian_opts[a_i++].dst = &colc;
   calcPetrosian_opts[a_i++].dst = &gain;
   calcPetrosian_opts[a_i++].dst = &sky;
   calcPetrosian_opts[a_i++].dst = &dark_variance;
   calcPetrosian_opts[a_i++].dst = &petro_f1;
   calcPetrosian_opts[a_i++].dst = &petro_f2;
   calcPetrosian_opts[a_i++].dst = &petro_f4;
   calcPetrosian_opts[a_i++].dst = &petro_f5;
   shAssert(calcPetrosian_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, calcPetrosian_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     calcPetrosian_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"calcPetrosian: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;
/*
 * work
 */
   (void)phTclCalcPetrosian(reg, rowc, colc, sky, gain, dark_variance,
			    petro_f1, petro_f2, petro_f4, petro_f5,
			    &petroRad, &petroRadErr, &petroR50, &petroR50Err,
			    &petroR90, &petroR90Err,
			    &petroCounts, &petroCountsErr);

   sprintf(buff, "petroRad %g", petroRad);
   Tcl_AppendElement(interp, buff);
   sprintf(buff, "petroRadErr %g", petroRadErr);
   Tcl_AppendElement(interp, buff);
   sprintf(buff, "petroR50 %g", petroR50);
   Tcl_AppendElement(interp, buff);
   sprintf(buff, "petroR50Err %g", petroR50Err);
   Tcl_AppendElement(interp, buff);
   sprintf(buff, "petroR90 %g", petroR90);
   Tcl_AppendElement(interp, buff);
   sprintf(buff, "petroR90Err %g", petroR90Err);
   Tcl_AppendElement(interp, buff);
   sprintf(buff, "petroCounts %g", petroCounts);
   Tcl_AppendElement(interp, buff);
   sprintf(buff, "petroCountsErr %g", petroCountsErr);
   Tcl_AppendElement(interp, buff);
   
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclCentroidAndDebias_use =
  "USAGE: centroidAndDebias <reg> <psfBasis> <psf> <rpeak> <cpeak> -bkgd dval -dark_variance dval -gain dval -sky dval";
#define tclCentroidAndDebias_hlp \
  "Calculate an object's centre, and debias it using the provided KL PSF.\n\
Note that the <[rc]peak> values refer to the _frame_, not the region.\n\
\n\
A list \"rowc colc\" giving the debiased centre is returned"

static ftclArgvInfo centroidAndDebias_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCentroidAndDebias_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL,
				"Region containing object (may be subregion)"},
   {"<psfBasis>", FTCL_ARGV_STRING, NULL, NULL, "PSF_BASIS describing KL PSF"},
   {"<psf>", FTCL_ARGV_STRING, NULL, NULL,
				    "DGPSF giving analytical estimate of PSF"},
   {"<rpeak>", FTCL_ARGV_DOUBLE, NULL, NULL, "Row-centre of object in frame"},
   {"<cpeak>", FTCL_ARGV_DOUBLE, NULL, NULL,
					   "Column-centre of object in frame"},
   {"-bkgd", FTCL_ARGV_DOUBLE, NULL, NULL,
			      "The sky level _not_ subtracted from <reg>"},
   {"-dark_variance", FTCL_ARGV_DOUBLE, NULL, NULL,
			   "The per-pixel variance in the absence of photons"},
   {"-gain", FTCL_ARGV_DOUBLE, NULL, NULL,
			       "The number of electrons corresponding to 1DN"},
   {"-sky", FTCL_ARGV_DOUBLE, NULL, NULL, "The sky level"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define centroidAndDebias_name "centroidAndDebias"

static int
tclCentroidAndDebias(ClientData clientData,
		     Tcl_Interp *interp,
		     int ac,
		     char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region containing object */
   REGION *reg;
   char *psfBasisStr = NULL;		/* PSF_BASIS describing KL PSF */
   PSF_BASIS *psfBasis;
   char *psfStr = NULL;			/* DGPSF giving analytical PSF */
   DGPSF *psf;
   double rpeak = 0.0;			/* Row-centre of object in frame */
   double cpeak = 0.0;			/* Column-centre of object in frame */
   double bkgd = 0.0;			/* Sky level _not_ subtracted */
   double dark_variance = 0.0;		/* The dark per-pixel variance */
   double gain = 1.0;			/* ratio electrons/DN */
   double sky = 0.0;			/* The sky level */
   float rowc = 0, colc = 0;		/* desired centroid */
   float rowcErr = 0, colcErr = 0;	/* error in desired centroid */
   FRAMEPARAMS fparams = {'0'};
   int ret;				/* return from phCentroidAndDebias() */

   shErrStackClear();

   a_i = 1;
   centroidAndDebias_opts[a_i++].dst = &regStr;
   centroidAndDebias_opts[a_i++].dst = &psfBasisStr;
   centroidAndDebias_opts[a_i++].dst = &psfStr;
   centroidAndDebias_opts[a_i++].dst = &rpeak;
   centroidAndDebias_opts[a_i++].dst = &cpeak;
   centroidAndDebias_opts[a_i++].dst = &bkgd;
   centroidAndDebias_opts[a_i++].dst = &dark_variance;
   centroidAndDebias_opts[a_i++].dst = &gain;
   centroidAndDebias_opts[a_i++].dst = &sky;
   shAssert(centroidAndDebias_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, centroidAndDebias_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     centroidAndDebias_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"centroidAndDebias: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,psfBasisStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_BASIS")) {
      Tcl_AppendResult(interp,"centroidAndDebias: "
                       "argument \"", psfBasisStr, "\" is not a PSF_BASIS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   psfBasis = hand.ptr;

   if(shTclHandleExprEval(interp,psfStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("DGPSF")) {
      Tcl_AppendResult(interp,"centroidAndDebias: "
                       "argument \"", psfStr, "\" is not a DGPSF",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   psf = hand.ptr;
/*
 * work
 */
   fparams.bkgd = bkgd;
   fparams.dark_variance = phBinregionNewFromConst(dark_variance, 1, 1, 1, 1, MAX_U16);
   fparams.gain = phBinregionNewFromConst(gain, 1, 1, 1, 1, MAX_U16);
   fparams.psf = psf;
   fparams.psfBasis = psfBasis;
   fparams.sky = phBinregionNewFromConst(sky, 1, 1, 1, 1, 0);

   ret = phCentroidAndDebias(reg, &fparams, rpeak, cpeak,
			     &rowc, &colc, &rowcErr, &colcErr);

   phBinregionDel((BINREGION *)fparams.sky);
/*
 * Return result
 */
   if(ret < 0) {
      Tcl_SetResult(interp, "centroidAndDebias: ", TCL_STATIC);
      shTclInterpAppendWithErrStack(interp);
      
      return(TCL_ERROR);
   } else {
      char buff[100];

      Tcl_ResetResult(interp);

      sprintf(buff, "rowc %.10g", rowc);
      Tcl_AppendElement(interp, buff);

      sprintf(buff, "colc %.10g", colc);
      Tcl_AppendElement(interp, buff);

      sprintf(buff, "rowcErr %.10g", rowcErr);
      Tcl_AppendElement(interp, buff);

      sprintf(buff, "colcErr %.10g", colcErr);
      Tcl_AppendElement(interp, buff);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclSaturSetInObject1Chain_use =
  "USAGE: saturSetInObject1Chain <reg> <objs>";
#define tclSaturSetInObject1Chain_hlp \
  "Given a region (with mask) and chain of OBJECT1s, set the obj1->satur " \
"pointers appropriately"

static ftclArgvInfo saturSetInObject1Chain_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSaturSetInObject1Chain_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region containing objects"},
   {"<objs>", FTCL_ARGV_STRING, NULL, NULL, "Chain of OBJECT1s"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define saturSetInObject1Chain_name "saturSetInObject1Chain"

static int
tclSaturSetInObject1Chain(ClientData clientData,
			  Tcl_Interp *interp,
			  int ac,
			  char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region containing objects */
   REGION *reg;
   char *objsStr = NULL;		/* Chain of OBJECT1s */
   CHAIN *objs;

   shErrStackClear();

   a_i = 1;
   saturSetInObject1Chain_opts[a_i++].dst = &regStr;
   saturSetInObject1Chain_opts[a_i++].dst = &objsStr;
   shAssert(saturSetInObject1Chain_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, saturSetInObject1Chain_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     saturSetInObject1Chain_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"saturSetInObject1Chain: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(reg->mask == NULL) {
      Tcl_SetResult(interp, "saturSetInObject1Chain: reg->mask is not set",
		    TCL_STATIC);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,objsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"saturSetInObject1Chain: "
                       "argument \"", objsStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objs = hand.ptr;

   if(objs->type != shTypeGetFromName("OBJECT1")) {
      Tcl_AppendResult(interp,
                       "CHAIN \"", objsStr, "\" doesn't have type OBJECT1",
								 (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phSaturSetInObject1Chain(reg, objs);
  
   return(TCL_OK);
}

/******************************************************************************/

static char *tclAdaptiveMomentsGet_use =
  "USAGE: adaptiveMomentsGet <reg> <rowc> <colc> <bkgd> -bkgdVariance dval -shiftmax dval -flags sval -radius dval";
#define tclAdaptiveMomentsGet_hlp \
  ""

static ftclArgvInfo adaptiveMomentsGet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclAdaptiveMomentsGet_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region containing object"},
   {"<rowc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Row-centre of object"},
   {"<colc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Col-centre of object"},
   {"<bkgd>", FTCL_ARGV_DOUBLE, NULL, NULL, 
    "Non-subtracted background (excluding soft bias)"},
   {"-radius", FTCL_ARGV_DOUBLE, NULL, NULL, 
    "Initial radius to estimate moments within"},
   {"-bkgdVariance", FTCL_ARGV_DOUBLE, NULL, NULL, 
    "Background per-pixel variance"},
   {"-shiftmax", FTCL_ARGV_DOUBLE, NULL, NULL,"Maximum allowed centroid shift"},
   {"-flags", FTCL_ARGV_STRING, NULL, NULL,
    "Variable to hold flags bits (supresses throwing errors)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define adaptiveMomentsGet_name "adaptiveMomentsGet"

static int
tclAdaptiveMomentsGet(ClientData clientData,
		      Tcl_Interp *interp,
		      int ac,
		      char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;		/* Region containing object */
   REGION *reg;
   double rowc = 0.0;		/* Row-centre of object */
   double colc = 0.0;		/* Col-centre of object */
   double bkgd = 0.0;		/* Non-subtracted background
				   (excluding soft bias) */
   double bkgdVariance = 1.0;	/* Background per-pixel variance */
   double shiftmax = 1.0;	/* Maximum allowed centroid shift */
   char *flagsStr = NULL;	/* Variable to hold flags bits
				   (supresses throwing errors) */
   float M_e1;			/* desired moments */
   float M_e2;
   float M_rr_cc;
   float M_cr4;			/* 4-th order moment */
   float M_e1e1Err;		/* errors in desired moments */
   float M_e1e2Err;
   float M_e2e2Err;
   float M_rr_ccErr;
   float amp_w;			/* amplitide of Gaussian */
   int flags2;			/* returned flags */
   double radius = 10;		/* Initial radius to estimate moments within */

   shErrStackClear();

   a_i = 1;
   adaptiveMomentsGet_opts[a_i++].dst = &regStr;
   adaptiveMomentsGet_opts[a_i++].dst = &rowc;
   adaptiveMomentsGet_opts[a_i++].dst = &colc;
   adaptiveMomentsGet_opts[a_i++].dst = &bkgd;
   adaptiveMomentsGet_opts[a_i++].dst = &radius;
   adaptiveMomentsGet_opts[a_i++].dst = &bkgdVariance;
   adaptiveMomentsGet_opts[a_i++].dst = &shiftmax;
   adaptiveMomentsGet_opts[a_i++].dst = &flagsStr;
   shAssert(adaptiveMomentsGet_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, adaptiveMomentsGet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     adaptiveMomentsGet_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"adaptiveMomentsGet: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(reg->type != TYPE_PIX) {
      Tcl_SetResult(interp, "adaptiveMomentsGet: region must be U16",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   flags2 = phAdaptiveMomentsGet(reg, rowc, colc, radius,
				 bkgd + SOFT_BIAS, bkgdVariance, shiftmax,
				 &M_e1, &M_e2, &M_rr_cc, &M_cr4,
				 &M_e1e1Err, &M_e1e2Err, &M_e2e2Err,
				 &M_rr_ccErr, &amp_w);
/*
 * Return values
 */
   if(flagsStr == NULL) {
      if(flags2 != 0) {
	 char buff[100];
	 sprintf(buff, "flags2 = 0x%x", flags2);
	 Tcl_SetResult(interp, "adaptiveMomentsGet: ", TCL_STATIC);
	 Tcl_AppendResult(interp, buff, (char *)NULL);
	 return(TCL_ERROR);
      }
   } else {
      char str[30];
      sprintf(str,"0x%x", flags2);
      if(Tcl_SetVar(interp, flagsStr, str, TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return flags",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   }
/*
 * create a keyed list to return values
 */
   {
      char str[100];
      sprintf(str, "e1 %.10g", M_e1);
      Tcl_AppendElement(interp, str);
      sprintf(str, "e2 %.10g", M_e2);
      Tcl_AppendElement(interp, str);
      sprintf(str, "rr_cc %.10g", M_rr_cc);
      Tcl_AppendElement(interp, str);
      sprintf(str, "cr4 %.10g", M_cr4);
      Tcl_AppendElement(interp, str);
      sprintf(str, "e1e1Err %.10g", M_e1e1Err);
      Tcl_AppendElement(interp, str);
      sprintf(str, "e1e2Err %.10g", M_e1e2Err);
      Tcl_AppendElement(interp, str);
      sprintf(str, "e2e2Err %.10g", M_e2e2Err);
      Tcl_AppendElement(interp, str);
      sprintf(str, "rr_ccErr %.10g", M_rr_ccErr);
      Tcl_AppendElement(interp, str);
      sprintf(str, "amp %.10g", amp_w);
      Tcl_AppendElement(interp, str);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Test Texture code
 */
static char *tclTextureGet_use =
  "USAGE: textureGet <reg> <pa>";
#define tclTextureGet_hlp \
  ""

static ftclArgvInfo textureGet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclTextureGet_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to be measured"},
   {"<pa>", FTCL_ARGV_DOUBLE, NULL, NULL,
      "P.A. of major axis; +ve wrt. scan direction (deg)"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define textureGet_name "textureGet"

static int
tclTextureGet(ClientData clientData,
	      Tcl_Interp *interp,
	      int ac,
	      char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to be measured */
   REGION *reg;
   double pa = 0.0;			/* P.A. of major axis;
					   +ve wrt. scan direction (deg) */

   shErrStackClear();

   a_i = 1;
   textureGet_opts[a_i++].dst = &regStr;
   textureGet_opts[a_i++].dst = &pa;
   shAssert(textureGet_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, textureGet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     textureGet_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"textureGet: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;
/*
 * work
 */
   {
      REGION *shRegIntRotate(REGION *, double, int, int, int);

      REGION *rot = shRegIntRotate(reg, 90 - pa, SOFT_BIAS, 1, 0);
/*
 * Return the answer
 */
      if(p_shTclHandleNew(interp,name) != TCL_OK) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
      hand.ptr = rot;
      hand.type = shTypeGetFromName("REGION");
      
      if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
	 Tcl_SetResult(interp,"can't bind \"rot\" to new REGION handle",
		       TCL_STATIC);
	 return(TCL_ERROR);
      }
      
      Tcl_SetResult(interp, name, TCL_VOLATILE);
   }
   
   return(TCL_OK);
}

/************************************************************************************************************/

static char *tclInsertAtlasImage_use =
  "USAGE: insertAtlasImage <objc> <fiparams>";
#define tclInsertAtlasImage_hlp \
  "Insert an atlas image into the data specified in a FIELDPARAMS"

static ftclArgvInfo insertAtlasImage_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclInsertAtlasImage_hlp},
   {"<objc>", FTCL_ARGV_STRING, NULL, NULL, "An OBJC"},
   {"<fiparams>", FTCL_ARGV_STRING, NULL, NULL, "The FIELDPARAMS describing the data"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define insertAtlasImage_name "insertAtlasImage"

static int
tclInsertAtlasImage(ClientData clientData,
		    Tcl_Interp *interp,
		    int ac,
		    char **av)
{
    HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *objcStr = NULL;		/* An OBJC */
   OBJC *objc;
   char *fiparamsStr = NULL;		/* The FIELDPARAMS describing the data */
   FIELDPARAMS *fiparams;

   shErrStackClear();

   a_i = 1;
   insertAtlasImage_opts[a_i++].dst = &objcStr;
   insertAtlasImage_opts[a_i++].dst = &fiparamsStr;
   shAssert(insertAtlasImage_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, insertAtlasImage_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     insertAtlasImage_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objcStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJC")) {
      Tcl_AppendResult(interp,"insertAtlasImage: "
                       "argument \"", objcStr, "\" is not an OBJC",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objc = hand.ptr;

   if(shTclHandleExprEval(interp,fiparamsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDPARAMS")) {
      Tcl_AppendResult(interp,"insertAtlasImage: "
                       "argument \"", fiparamsStr, "\" is not a FIELDPARAMS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   fiparams = hand.ptr;
/*
 * work
 */
   phInsertAtlasImage(objc, fiparams);

   return(TCL_OK);
}

/************************************************************************************************************/

static char *tclRegSetValFromAtlasImage_use =
  "USAGE: regSetValFromAtlasImage <reg> <band> <ai> <val> -row0 ival -col0 ival -rand sval -sigma dval -bandMask";
#define tclRegSetValFromAtlasImage_hlp \
  ""

static ftclArgvInfo regSetValFromAtlasImage_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegSetValFromAtlasImage_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "The region whose pixels should be set"},
   {"<band>", FTCL_ARGV_INT, NULL, NULL, "The band to use"},
   {"<ai>", FTCL_ARGV_STRING, NULL, NULL, "The atlas image defining the object"},   
   {"<val>", FTCL_ARGV_DOUBLE, NULL, NULL, "Value to set footprint to"},
   {"-row0", FTCL_ARGV_INT, NULL, NULL, "Logical row-origin of region"},
   {"-col0", FTCL_ARGV_INT, NULL, NULL, "Logical column-origin of region"},
   {"-rand", FTCL_ARGV_STRING, NULL, NULL, "a RANDOM"},
   {"-sigma", FTCL_ARGV_DOUBLE, NULL, NULL, "Standard deviation of noise to add"},
   {"-bandMask", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Use the atlas image's per-band, not master, mask"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regSetValFromAtlasImage_name "regSetValFromAtlasImage"

static int
tclRegSetValFromAtlasImage(ClientData clientData,
			   Tcl_Interp *interp,
			   int ac,
			   char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* The region whose pixels should be set */
   REGION *reg;
   int band = 0;			/* The band to use */
   char *aiStr = NULL;			/* The atlas image defining the object */
   ATLAS_IMAGE *ai;
   int row0 = 0;			/* Logical row-origin of region */
   int col0 = 0;			/* Logical column-origin of region */
   char *randStr = NULL;		/* a RANDOM; may be \"NULL\" */
   RANDOM *rand;
   double sigma = -1;			/* Standard deviation of noise to add */
   double val = 0.0;			/* Value to set footprint to */
   int bandMask = 0;			/* Use the atlas image's per-band, not master, mask */

   shErrStackClear();

   a_i = 1;
   regSetValFromAtlasImage_opts[a_i++].dst = &regStr;
   regSetValFromAtlasImage_opts[a_i++].dst = &band;
   regSetValFromAtlasImage_opts[a_i++].dst = &aiStr;
   regSetValFromAtlasImage_opts[a_i++].dst = &val;
   regSetValFromAtlasImage_opts[a_i++].dst = &row0;
   regSetValFromAtlasImage_opts[a_i++].dst = &col0;
   regSetValFromAtlasImage_opts[a_i++].dst = &randStr;
   regSetValFromAtlasImage_opts[a_i++].dst = &sigma;
   regSetValFromAtlasImage_opts[a_i++].dst = &bandMask;
   shAssert(regSetValFromAtlasImage_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regSetValFromAtlasImage_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regSetValFromAtlasImage_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"regSetValFromAtlasImage: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,aiStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("ATLAS_IMAGE")) {
      Tcl_AppendResult(interp,"regSetValFromAtlasImage: "
                       "argument \"", aiStr, "\" is not an ATLAS_IMAGE",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   ai = hand.ptr;

   if(randStr == NULL) {
       rand = NULL;
   } else {
       if(shTclHandleExprEval(interp,randStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
       if(hand.type != shTypeGetFromName("RANDOM")) {
	   Tcl_AppendResult(interp,"regSetValFromAtlasImage: "
			    "argument \"", randStr, "\" is not a RANDOM",
			    (char *)NULL);
	   return(TCL_ERROR);
       }
       rand = hand.ptr;
   }

   if(sigma > 0 && rand == NULL) {
       Tcl_SetResult(interp, "regSetValFromAtlasImage: Please provide rand as sigma > 0", TCL_STATIC);
       return(TCL_ERROR);
   }
/*
 * work
 */
   phRegionSetValFromAtlasImage(ai, band, (bandMask ? 0 : 1), reg, val, sigma, rand, row0, col0);

   return(TCL_OK);
}



/*****************************************************************************
 * Declare my new tcl verbs to tcl
 */
void
phTclMeasureObjDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"initMeasureObj",
                (Tcl_CmdProc *)phTclInitMeasureObj,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, phTclInitMeasureObj_hlp, phTclInitMeasureObj_use);
   shTclDeclare(interp,"finiMeasureObj",
                (Tcl_CmdProc *)phTclFiniMeasureObj,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, phTclFiniMeasureObj_hlp, phTclFiniMeasureObj_use);
   shTclDeclare(interp,"measureObjColorSet",
		(Tcl_CmdProc *)tclMeasureObjColorSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMeasureObjColorSet_hlp,
		tclMeasureObjColorSet_use);

   shTclDeclare(interp,"measureObjc",
		(Tcl_CmdProc *)tclMeasureObjc, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMeasureObjc_hlp,
		tclMeasureObjc_use);

   shTclDeclare(interp,"measureObjUnset",
		(Tcl_CmdProc *)tclMeasureObjUnset, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMeasureObjUnset_hlp,
		tclMeasureObjUnset_use);

   shTclDeclare(interp,"atlasImageCut",
		(Tcl_CmdProc *)tclAtlasImageCut, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclAtlasImageCut_hlp,
		tclAtlasImageCut_use);

   shTclDeclare(interp,"regSetFromAtlasImage",
		(Tcl_CmdProc *)tclRegSetFromAtlasImage, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegSetFromAtlasImage_hlp,
		tclRegSetFromAtlasImage_use);

   shTclDeclare(interp,"fieldstatSetFromMO",
		(Tcl_CmdProc *)tclFieldstatSetFromMO, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFieldstatSetFromMO_hlp,
		tclFieldstatSetFromMO_use);

   shTclDeclare(interp,"deblendSet",
		(Tcl_CmdProc *)tclDeblendSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDeblendSet_hlp,
		tclDeblendSet_use);

   shTclDeclare(interp,"deblendUnset",
		(Tcl_CmdProc *)tclDeblendUnset, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDeblendUnset_hlp,
		tclDeblendUnset_use);

   shTclDeclare(interp,psfCountsFromRegionSetup_name,
		(Tcl_CmdProc *)tclPsfCountsFromRegionSetup, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfCountsFromRegionSetup_hlp,
		tclPsfCountsFromRegionSetup_use);

   shTclDeclare(interp,psfCountsFromRegion_name,
		(Tcl_CmdProc *)tclPsfCountsFromRegion, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfCountsFromRegion_hlp,
		tclPsfCountsFromRegion_use);

   shTclDeclare(interp,psfSetAtPoint_name,
		(Tcl_CmdProc *)tclPsfSetAtPoint, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPsfSetAtPoint_hlp,
		tclPsfSetAtPoint_use);

   shTclDeclare(interp,calcPetrosian_name,
		(Tcl_CmdProc *)tclCalcPetrosian, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCalcPetrosian_hlp,
		tclCalcPetrosian_use);

   shTclDeclare(interp,centroidAndDebias_name,
		(Tcl_CmdProc *)tclCentroidAndDebias, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCentroidAndDebias_hlp,
		tclCentroidAndDebias_use);

   shTclDeclare(interp,saturSetInObject1Chain_name,
		(Tcl_CmdProc *)tclSaturSetInObject1Chain, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSaturSetInObject1Chain_hlp,
		tclSaturSetInObject1Chain_use);

   shTclDeclare(interp,adaptiveMomentsGet_name,
		(Tcl_CmdProc *)tclAdaptiveMomentsGet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclAdaptiveMomentsGet_hlp,
		tclAdaptiveMomentsGet_use);

   shTclDeclare(interp,textureGet_name,
		(Tcl_CmdProc *)tclTextureGet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclTextureGet_hlp,
		tclTextureGet_use);

   shTclDeclare(interp,insertAtlasImage_name,
		(Tcl_CmdProc *)tclInsertAtlasImage, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclInsertAtlasImage_hlp,
		tclInsertAtlasImage_use);

   shTclDeclare(interp,regSetValFromAtlasImage_name,
		(Tcl_CmdProc *)tclRegSetValFromAtlasImage, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegSetValFromAtlasImage_hlp,
		tclRegSetValFromAtlasImage_use);
}
