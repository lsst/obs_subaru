/* <AUTO>
   FILE: tclTrans
   ABSTRACT:
<HTML>
   Routines for creating, deleting, and using TRANS structs
</HTML>
   </AUTO>
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include "dervish.h"
#include "tcl.h"
#include "shCUtils.h"
#include "shCErrStack.h"
#include "shTclHandle.h"
#include "tclMath.h"
#include "atSlalib.h"
#include "atTrans.h"

static int
phFltArrayGetFromTclArray(Tcl_Interp *interp,
			  char *arrayName, /* name of tcl array */
			  char *indices, /* indices of tcl array, or NULL */
			  float **vals); /* array to allocate and return */

/*--------------------------------------------------------------------------------------*/

/*<AUTO EXTRACT>
  TCL VERB: transNew
  <HTML>
  <A HREF=atTrans.html#atTransNew>atTransNew</A> in atTrans.c
  </HTML>
  </AUTO>
*/

static char tclTransNewCmd[] = "transNew";
static int tclTransNewFlg = FTCL_ARGV_NO_LEFTOVERS;
static ftclArgvInfo tclTransNewTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Allocate a new TRANS object and set its elements to their unary values\n"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int TclTransNew(ClientData clientData,
		       Tcl_Interp *interp,		/* TCL interpreter */
		       int argc,			/* TCL argument count */
		       char **argv)			/* TCL argument list */
    {
    TRANS *t_ptr;
    char hname[HANDLE_NAMELEN];

   int status;

/* Parse the arguments */
   if ((status = shTclParseArgv(interp, &argc, argv, tclTransNewTbl,
				tclTransNewFlg, tclTransNewCmd))
       != FTCL_ARGV_SUCCESS)
     return status;

/* Create a new TRANS structure and handle for the product */

    t_ptr = atTransNew();
    if (shTclHandleNew(interp, hname, "TRANS", t_ptr) != TCL_OK)
      {
      Tcl_SetResult(interp, "TransNew: Unable to create handle", TCL_VOLATILE);
      return TCL_ERROR;
      }

    Tcl_SetResult(interp, hname, TCL_VOLATILE);
    return TCL_OK;

    }


/*--------------------------------------------------------------------------------------*/

/*<AUTO EXTRACT>
  TCL VERB: transDel
  <HTML>
  <A HREF=atTrans.html#atTransDel>atTransDel</A> in atTrans.c
  </HTML>
  </AUTO>
*/
static char tclTransDelCmd[] = "transDel";
static int tclTransDelFlg = FTCL_ARGV_NO_LEFTOVERS;
static ftclArgvInfo tclTransDelTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Delete a TRANS.\n"},
   {"<handle>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle of TRANS to delete"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

  static int TclTransDel(ClientData clientData,
			 Tcl_Interp *interp,		/* TCL interpreter */
			 int argc,			/* TCL argument count */
			 char **argv)		/* TCL argument list */
{
   TRANS *t_ptr = (TRANS *) 0;
   char *name;
   int status;

/* Parse the arguments */
   tclTransDelTbl[1].dst = &name;
   if ((status = shTclParseArgv(interp, &argc, argv, tclTransDelTbl,
				tclTransDelFlg, tclTransDelCmd))
       != FTCL_ARGV_SUCCESS)
     return status;
   
/* Get a pointer to the input TRANS structure */

    if (shTclAddrGetFromName(interp, name, (void **) &t_ptr, "TRANS") !=
        TCL_OK)
      {
      Tcl_SetResult(interp, "TransDel: Invalid argument 1", TCL_VOLATILE);
      return TCL_ERROR;
      }

/* Delete the TRANS structure */

    atTransDel(t_ptr);

/* Delete the handle */

    p_shTclHandleDel(interp, name);

    Tcl_SetResult(interp, "", TCL_VOLATILE);
    return TCL_OK;

    }


/*--------------------------------------------------------------------------------------*/

/*<AUTO EXTRACT>
  TCL VERB: transApply

<HTML>
  <A HREF=atTrans.html#atTransApply>atTransApply</A> in atTrans.c
</HTML>

  DESCRIPTION:
<HTML>
	Apply a TRANS structure to convert CCD coordinates (row,col) in pixels
	to great circle coordinates (mu,nu) in degrees for an object measured
	in band <filter>.
<p>
        Returns the answer as a TCL keyed list, composed of the following
	keyword/value pairs: mu = great circle longitude (degrees); muErr =
	error in mu (degrees); nu = great circle latitude (degrees);
	nuErr = error in nu (degrees); status = 0 if calibrated using measued
	colors, 1 if calibrated using the cosmic color.
</HTML>

  </AUTO>
*/
static char *tclTransApply_use =
  "USAGE: transApply <trans> <filter> <row> <rowErr> <col> <colErr>\n"
  "                  -mag array -magErr array";
#define tclTransApply_hlp \
  "Apply a TRANS, converting (row,col) to (mu,nu).\n"

static ftclArgvInfo transApply_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclTransApply_hlp},
   {"<trans>", FTCL_ARGV_STRING, NULL, NULL, "The TRANS in question"},
   {"<filter>", FTCL_ARGV_STRING, NULL, NULL,
					"Name of band (row,col)'s measured in"},
   {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "Frame row position (pixels)"},
   {"<rowErr>", FTCL_ARGV_DOUBLE, NULL, NULL, "Error in row"},
   {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, "Frame column position (pixels)"},
   {"<colErr>", FTCL_ARGV_DOUBLE, NULL, NULL, "Error in col"},
   {"-mag", FTCL_ARGV_STRING, NULL, NULL, "Array of magnitudes in each band"},
   {"-magErr", FTCL_ARGV_STRING, NULL, NULL, "Array of errors in mag"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define transApply_name "transApply"

static int
tclTransApply(ClientData clientData,
	      Tcl_Interp *interp,
	      int ac,
	      char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *transStr = NULL;		/* The TRANS in question */
   TRANS *trans;
   char *filterStr = NULL;		/* band (row,col)'s measured in*/
   double row = 0.0;			/* Frame row position (pixels) */
   double rowErr = 0.0;			/* Error in row */
   double col = 0.0;			/* Frame column position (pixels) */
   double colErr = 0.0;			/* Error in col */
   char *magStr = NULL;			/* Array of magnitudes in each band */
   float *mag;				/* mag as an array */
   char *magErrStr = NULL;		/* Errors in mag */
   float *magErr;			/* magErr as an array */
   char indices[2*AT_NBAND + 1];	/* space-separated version
					   of atFilternames */
   double mu, muErr, nu, nuErr;		/* returned great circle position */
   int status;				/* return code */

   shErrStackClear();

   i = 1;
   transApply_opts[i++].dst = &transStr;
   transApply_opts[i++].dst = &filterStr;
   transApply_opts[i++].dst = &row;
   transApply_opts[i++].dst = &rowErr;
   transApply_opts[i++].dst = &col;
   transApply_opts[i++].dst = &colErr;
   transApply_opts[i++].dst = &magStr;
   transApply_opts[i++].dst = &magErrStr;
   shAssert(transApply_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, transApply_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     transApply_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   for(i = 0; i < AT_NBAND; i++) {	/* needed for mag/magErr */
      sprintf(&indices[2*i], "%c ", atFilternames[i]);
   }
   
   if(shTclHandleExprEval(interp,transStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("TRANS")) {
      Tcl_AppendResult(interp,"transApply: "
                       "argument \"", transStr, "\" is not a TRANS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   trans = hand.ptr;

   if(strlen(filterStr) != 1) {
      Tcl_AppendResult(interp, "Filtername must have only one character: ",
		       filterStr, (char *)NULL);
      return(TCL_ERROR);
   }
   if(strchr(atFilternames, *filterStr) == NULL) {
      Tcl_AppendResult(interp, "Invalid filtername ", filterStr,
		       " please use one of ", atFilternames, (char *)NULL);
      return(TCL_ERROR);
   }

   if(magStr == NULL) {
      mag = NULL;
   } else {
      if(phFltArrayGetFromTclArray(interp, magStr, indices, &mag) != AT_NBAND) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
   }

   if(magErrStr == NULL) {
      magErr = NULL;
   } else {
      if(phFltArrayGetFromTclArray(interp, magErrStr, indices, &magErr) !=
								     AT_NBAND) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
   }
/*
 * work
 */
   status = atTransApply(trans, *filterStr, row, rowErr, col, colErr,
			 mag, magErr, &mu, &muErr, &nu, &nuErr);
   shFree(mag); shFree(magErr);
/*
 * Return results
 */
   {
      char results[200];
      sprintf(results, "{mu %.17g} {muErr %.17g} "
	      "{nu %.17g} {nuErr %.17g} {status %d}",
	      mu, muErr, nu, nuErr, status);

      Tcl_SetResult(interp, results, TCL_VOLATILE);
   }
   
   return TCL_OK;
}


/*--------------------------------------------------------------------------------------*/

/*<AUTO EXTRACT>
  TCL VERB: transInverseApply

<HTML>
  <A HREF=atTrans.html#atTransInverseApply>atTransInverseApply</A> in atTrans.c
</HTML>

  DESCRIPTION:
<HTML>
	Apply the inverse of a TRANS structure to convert great circle
	coordinates (mu,nu) in degrees to CCD coordinates (row,col) in pixels
	for an object measured in band <filter>.

<p>
        Returns the answer as a TCL keyed list, composed of the following
	keyword/value pairs: row = row position (pixels); rowErr =
	error in row (pixels); col = column position (pixels);
	colErr = error in column (pixels); status = -1 if error inverting
	the trans, 0 if calibrated using measued colors, 1 if calibrated
	using the cosmic color.
</HTML>

  </AUTO>
*/

static char *tclTransInverseApply_use =
  "USAGE: transInverseApply <trans> <filter> <mu> <muErr> <nu> <nuErr>\n"
"                         -mag array -magErr array";
#define tclTransInverseApply_hlp \
  "Apply the inverse of a TRANS, converting (mu,nu) to (row,col).\n"

static ftclArgvInfo transInverseApply_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclTransInverseApply_hlp},
   {"<trans>", FTCL_ARGV_STRING, NULL, NULL,
				       "Trans whose inverse should be applied"},
   {"<filter>", FTCL_ARGV_STRING, NULL, NULL,
					   "Band in which (mu,nu) is measured"},
   {"<mu>", FTCL_ARGV_DOUBLE, NULL, NULL, "Great circle longitude (degrees)"},
   {"<muErr>", FTCL_ARGV_DOUBLE, NULL, NULL, "Error in mu"},
   {"<nu>", FTCL_ARGV_DOUBLE, NULL, NULL, "Great circle latitude (degrees)"},
   {"<nuErr>", FTCL_ARGV_DOUBLE, NULL, NULL, "Error in nu"},
   {"-mag", FTCL_ARGV_STRING, NULL, NULL, "Array of magnitudes in each band"},
   {"-magErr", FTCL_ARGV_STRING, NULL, NULL, "Array of errors in mag"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define transInverseApply_name "transInverseApply"

static int
tclTransInverseApply(ClientData clientData,
		     Tcl_Interp *interp,
		     int ac,
		     char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *transStr = NULL;		/* Trans whose inverse should be used */
   TRANS *trans;
   char *filterStr = NULL;		/* Band in which (mu,nu) is measured */
   double mu = 0.0;			/* Great circle longitude (degrees) */
   double muErr = 0.0;			/* Error in mu */
   double nu = 0.0;			/* Great circle latitude (degrees) */
   double nuErr = 0.0;			/* Error in nu */
   char *magStr = NULL;			/* Array of magnitudes in each band */
   float *mag;				/* mag as an array */
   char *magErrStr = NULL;		/* Array of errors in mag */
   float *magErr;			/* magErr as an array */
   char indices[2*AT_NBAND + 1];	/* space-separated version
					   of atFilternames */
   double row, rowErr, col, colErr;	/* returned pixel position */
   int status;				/* return code */

   shErrStackClear();

   i = 1;
   transInverseApply_opts[i++].dst = &transStr;
   transInverseApply_opts[i++].dst = &filterStr;
   transInverseApply_opts[i++].dst = &mu;
   transInverseApply_opts[i++].dst = &muErr;
   transInverseApply_opts[i++].dst = &nu;
   transInverseApply_opts[i++].dst = &nuErr;
   transInverseApply_opts[i++].dst = &magStr;
   transInverseApply_opts[i++].dst = &magErrStr;
   shAssert(transInverseApply_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, transInverseApply_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     transInverseApply_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   for(i = 0; i < AT_NBAND; i++) {	/* needed for mag/magErr */
      sprintf(&indices[2*i], "%c ", atFilternames[i]);
   }

   if(shTclHandleExprEval(interp,transStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("TRANS")) {
      Tcl_AppendResult(interp,"transInverseApply: "
                       "argument \"", transStr, "\" is not a TRANS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   trans = hand.ptr;

   if(strlen(filterStr) != 1) {
      Tcl_AppendResult(interp, "Filtername must have only one character: ",
		       filterStr, (char *)NULL);
      return(TCL_ERROR);
   }
   if(strchr(atFilternames, *filterStr) == NULL) {
      Tcl_AppendResult(interp, "Invalid filtername ", filterStr,
		       " please use one of ", atFilternames, (char *)NULL);
      return(TCL_ERROR);
   }
   
   if(magStr == NULL) {
      mag = NULL;
   } else {
      if(phFltArrayGetFromTclArray(interp, magStr, indices, &mag) != AT_NBAND) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
   }

   if(magErrStr == NULL) {
      magErr = NULL;
   } else {
      if(phFltArrayGetFromTclArray(interp, magErrStr, indices, &magErr)
								  != AT_NBAND) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
   }
/*
 * work
 */
   status = atTransInverseApply(trans, *filterStr, mu, muErr, nu, nuErr,
				mag, magErr, &row, &rowErr, &col, &colErr);
   shFree(mag); shFree(magErr);
/*
 * return result
 */
   {
      char results[200];
      sprintf(results, "{row %.17g} {rowErr %.17g} "
	      "{col %.17g} {colErr %.17g} {status %d}",
	      row, rowErr, col, colErr, status);
      Tcl_SetResult(interp, results, TCL_VOLATILE);
   }

   return(TCL_OK);
}


/*--------------------------------------------------------------------------------------*/

/*<AUTO EXTRACT>
  TCL VERB: transInverseApplyNowrap

<HTML>
  <A HREF=atTrans.html#atTransInverseApplyNowrap>atTransInverseApplyNowrap</A> in atTrans.c
</HTML>

  DESCRIPTION:
<HTML>
	Apply the inverse of a TRANS structure to convert great circle
	coordinates (mu,nu) in degrees to CCD coordinates (row,col) in pixels
	for an object measured in band <filter>.

<p>
        Returns the answer as a TCL keyed list, composed of the following
	keyword/value pairs: row = row position (pixels); rowErr =
	error in row (pixels); col = column position (pixels);
	colErr = error in column (pixels); status = -1 if error inverting
	the trans, 0 if calibrated using measued colors, 1 if calibrated
	using the cosmic color.
</HTML>

  </AUTO>
*/

static char *tclTransInverseApplyNowrap_use =
  "USAGE: transInverseApplyNowrap <trans> <filter> <mu> <muErr> <nu> <nuErr>\n"
"                         -mag array -magErr array";
#define tclTransInverseApplyNowrap_hlp \
  "Apply the inverse of a TRANS, converting (mu,nu) to (row,col).\n"

static ftclArgvInfo transInverseApplyNowrap_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclTransInverseApplyNowrap_hlp},
   {"<trans>", FTCL_ARGV_STRING, NULL, NULL,
				       "Trans whose inverse should be applied"},
   {"<filter>", FTCL_ARGV_STRING, NULL, NULL,
					   "Band in which (mu,nu) is measured"},
   {"<mu>", FTCL_ARGV_DOUBLE, NULL, NULL, "Great circle longitude (degrees)"},
   {"<muErr>", FTCL_ARGV_DOUBLE, NULL, NULL, "Error in mu"},
   {"<nu>", FTCL_ARGV_DOUBLE, NULL, NULL, "Great circle latitude (degrees)"},
   {"<nuErr>", FTCL_ARGV_DOUBLE, NULL, NULL, "Error in nu"},
   {"-mag", FTCL_ARGV_STRING, NULL, NULL, "Array of magnitudes in each band"},
   {"-magErr", FTCL_ARGV_STRING, NULL, NULL, "Array of errors in mag"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define transInverseApplyNowrap_name "transInverseApplyNowrap"

static int
tclTransInverseApplyNowrap(ClientData clientData,
		     Tcl_Interp *interp,
		     int ac,
		     char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *transStr = NULL;		/* Trans whose inverse should be used */
   TRANS *trans;
   char *filterStr = NULL;		/* Band in which (mu,nu) is measured */
   double mu = 0.0;			/* Great circle longitude (degrees) */
   double muErr = 0.0;			/* Error in mu */
   double nu = 0.0;			/* Great circle latitude (degrees) */
   double nuErr = 0.0;			/* Error in nu */
   char *magStr = NULL;			/* Array of magnitudes in each band */
   float *mag;				/* mag as an array */
   char *magErrStr = NULL;		/* Array of errors in mag */
   float *magErr;			/* magErr as an array */
   char indices[2*AT_NBAND + 1];	/* space-separated version
					   of atFilternames */
   double row, rowErr, col, colErr;	/* returned pixel position */
   int status;				/* return code */

   shErrStackClear();

   i = 1;
   transInverseApplyNowrap_opts[i++].dst = &transStr;
   transInverseApplyNowrap_opts[i++].dst = &filterStr;
   transInverseApplyNowrap_opts[i++].dst = &mu;
   transInverseApplyNowrap_opts[i++].dst = &muErr;
   transInverseApplyNowrap_opts[i++].dst = &nu;
   transInverseApplyNowrap_opts[i++].dst = &nuErr;
   transInverseApplyNowrap_opts[i++].dst = &magStr;
   transInverseApplyNowrap_opts[i++].dst = &magErrStr;
   shAssert(transInverseApplyNowrap_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, transInverseApplyNowrap_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     transInverseApplyNowrap_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   for(i = 0; i < AT_NBAND; i++) {	/* needed for mag/magErr */
      sprintf(&indices[2*i], "%c ", atFilternames[i]);
   }

   if(shTclHandleExprEval(interp,transStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("TRANS")) {
      Tcl_AppendResult(interp,"transInverseApplyNowrap: "
                       "argument \"", transStr, "\" is not a TRANS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   trans = hand.ptr;

   if(strlen(filterStr) != 1) {
      Tcl_AppendResult(interp, "Filtername must have only one character: ",
		       filterStr, (char *)NULL);
      return(TCL_ERROR);
   }
   if(strchr(atFilternames, *filterStr) == NULL) {
      Tcl_AppendResult(interp, "Invalid filtername ", filterStr,
		       " please use one of ", atFilternames, (char *)NULL);
      return(TCL_ERROR);
   }
   
   if(magStr == NULL) {
      mag = NULL;
   } else {
      if(phFltArrayGetFromTclArray(interp, magStr, indices, &mag) != AT_NBAND) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
   }

   if(magErrStr == NULL) {
      magErr = NULL;
   } else {
      if(phFltArrayGetFromTclArray(interp, magErrStr, indices, &magErr)
								  != AT_NBAND) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
   }
/*
 * work
 */
   status = atTransInverseApplyNowrap(trans, *filterStr, mu, muErr, nu, nuErr,
				mag, magErr, &row, &rowErr, &col, &colErr);
   shFree(mag); shFree(magErr);
/*
 * return result
 */
   {
      char results[200];
      sprintf(results, "{row %.17g} {rowErr %.17g} "
	      "{col %.17g} {colErr %.17g} {status %d}",
	      row, rowErr, col, colErr, status);
      Tcl_SetResult(interp, results, TCL_VOLATILE);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclTransCosmicMagSet_use =
  "USAGE: transCosmicMagSet <cosmicMag>";
#define tclTransCosmicMagSet_hlp \
 "Set an array of magnitudes (one for each filter) for \"average\" objects\n"\
"The return is a list of the old values, ready for an \"array set\" command\n"\
"\n"\
"If <cosmicMag> is \"\" or \"NULL\", no new values are set."

static ftclArgvInfo transCosmicMagSet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclTransCosmicMagSet_hlp},
   {"<cosmicMag>", FTCL_ARGV_STRING, NULL, NULL, "Name of magnitudes array"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define transCosmicMagSet_name "transCosmicMagSet"

static int
tclTransCosmicMagSet(ClientData clientData,
		     Tcl_Interp *interp,
		     int ac,
		     char **av)
{
   int i;
   char *cosmicMagStr = NULL;		/* Name of array */
   float *cosmicMag;			/* the array itelf */
   char indices[2*AT_NBAND + 1];	/* space-separated version
					   of atFilternames */
   const float *old;			/* old values of cosmicMag */

   shErrStackClear();

   i = 1;
   transCosmicMagSet_opts[i++].dst = &cosmicMagStr;
   shAssert(transCosmicMagSet_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, transCosmicMagSet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     transCosmicMagSet_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(*cosmicMagStr == '\0' || strcmp(cosmicMagStr, "NULL") == 0) {
      cosmicMag = NULL;
   } else {
      for(i = 0; i < AT_NBAND; i++) {
	 sprintf(&indices[2*i], "%c ", atFilternames[i]);
      }

      if(phFltArrayGetFromTclArray(interp,cosmicMagStr, indices, &cosmicMag) !=
								    AT_NBAND) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
   }
/*
 * work
 */
   old = atCosmicMagSet(cosmicMag);
/*
 * return old values
 */
   {
      char buff[40];
      for(i = 0; i < AT_NBAND; i++) {
	 sprintf(buff, "%c %.17g ", atFilternames[i], old[i]);
	 Tcl_AppendResult(interp, buff, (char *)NULL);
      }
   }

   shFree(cosmicMag);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclTransCosmicScatterSet_use =
  "USAGE: transCosmicScatterSet <cosmicScatter>";
#define tclTransCosmicScatterSet_hlp \
 "Set an array of scatter in magnitudes (one for each filter) for \"average\""\
"\nobjects.\n"\
"The return is a list of the old values, ready for an \"array set\" command\n"\
"\n"\
"If <cosmicScatter> is \"\" or \"NULL\", no new values are set."

static ftclArgvInfo transCosmicScatterSet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclTransCosmicScatterSet_hlp},
   {"<cosmicScatter>", FTCL_ARGV_STRING, NULL, NULL,
					       "Name of cosmic scatter array"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define transCosmicScatterSet_name "transCosmicScatterSet"

static int
tclTransCosmicScatterSet(ClientData clientData,
		     Tcl_Interp *interp,
		     int ac,
		     char **av)
{
   int i;
   char *cosmicScatterStr = NULL;	/* Name of array */
   float *cosmicScatter;		/* the array itelf */
   char indices[2*AT_NBAND + 1];	/* space-separated version
					   of atFilternames */
   const float *old;			/* old values of cosmicScatter */

   shErrStackClear();

   i = 1;
   transCosmicScatterSet_opts[i++].dst = &cosmicScatterStr;
   shAssert(transCosmicScatterSet_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, transCosmicScatterSet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     transCosmicScatterSet_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(*cosmicScatterStr == '\0' || strcmp(cosmicScatterStr, "NULL") == 0) {
      cosmicScatter = NULL;
   } else {
      for(i = 0; i < AT_NBAND; i++) {
	 sprintf(&indices[2*i], "%c ", atFilternames[i]);
      }

      if(phFltArrayGetFromTclArray(interp, cosmicScatterStr, indices,
						 &cosmicScatter) != AT_NBAND) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
   }
/*
 * work
 */
   old = atCosmicMagScatterSet(cosmicScatter);
/*
 * return old values
 */
   {
      char buff[40];
      for(i = 0; i < AT_NBAND; i++) {
	 sprintf(buff, "%c %.17g ", atFilternames[i], old[i]);
	 Tcl_AppendResult(interp, buff, (char *)NULL);
      }
   }

   shFree(cosmicScatter);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Given the name of a TCL array, generate an array of floats corresponding
 * to its elements, and return the number of elements (or -1 in case of error)
 *
 * Do not forget to shFree *vals when you are done with it
 */
static int
phFltArrayGetFromTclArray(Tcl_Interp *interp,
			  char *arrayName, /* name of tcl array */
			  char *indices, /* indices of tcl array */
			  float **vals)	/* array to allocate and return */
{
   int ac;
   char **av;
   double dbl;				/* value of an array element */
   int i;
   char *ptr;

   /* Make the ascii list accessible to C */
   if(Tcl_SplitList(interp, indices, &ac, &av) == TCL_ERROR) {
      shErrStackPush("Error parsing input list of indices");
      return(-1);
   }

   *vals = shMalloc(ac*sizeof(float));
/*
 * read all those variables, and convert to float
 */
   for(i = 0;i < ac;i++) {
      if((ptr = Tcl_GetVar2(interp, arrayName, av[i], 0)) == NULL) {
	 shErrStackPush("phFltArrayGetFromTclArray: %s(%s) doesn't exist",
							    arrayName,av[i]);
	 free(av);
	 shFree(*vals); *vals = NULL;
	 return(-1);
      }

      if(Tcl_GetDouble(interp, ptr, &dbl) == TCL_OK) {
	 (*vals)[i] = dbl;
      } else {
	 free(av);
	 shFree(*vals); *vals = NULL;

	 return(-1);
      }
   }

   free(av);
   
   return(ac);
}


/*----------------------------------------------------------------------------*/
void atTclTransDeclare(Tcl_Interp *interp)
{

    static char *module = "astrotools";

    shTclDeclare(interp, tclTransNewCmd, (Tcl_CmdProc *)TclTransNew,
		 (ClientData) 0,	(Tcl_CmdDeleteProc *)NULL, module,
		 shTclGetArgInfo(interp, tclTransNewTbl, tclTransNewFlg,
				 tclTransNewCmd),
		 shTclGetUsage(interp, tclTransNewTbl, tclTransNewFlg,
			       tclTransNewCmd));
    shTclDeclare(interp, tclTransDelCmd, (Tcl_CmdProc *)TclTransDel,
		 (ClientData) 0,	(Tcl_CmdDeleteProc *)NULL, module,
		 shTclGetArgInfo(interp, tclTransDelTbl, tclTransDelFlg,
				 tclTransDelCmd),
		 shTclGetUsage(interp, tclTransDelTbl, tclTransDelFlg,
			       tclTransDelCmd));

   shTclDeclare(interp,transApply_name,
		(Tcl_CmdProc *)tclTransApply, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclTransApply_hlp,
		tclTransApply_use);

   shTclDeclare(interp,transInverseApply_name,
		(Tcl_CmdProc *)tclTransInverseApply, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclTransInverseApply_hlp,
		tclTransInverseApply_use);

   shTclDeclare(interp,transInverseApplyNowrap_name,
		(Tcl_CmdProc *)tclTransInverseApplyNowrap, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclTransInverseApplyNowrap_hlp,
		tclTransInverseApplyNowrap_use);

   shTclDeclare(interp,transCosmicMagSet_name,
		(Tcl_CmdProc *)tclTransCosmicMagSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclTransCosmicMagSet_hlp,
		tclTransCosmicMagSet_use);

   shTclDeclare(interp,transCosmicScatterSet_name,
		(Tcl_CmdProc *)tclTransCosmicScatterSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclTransCosmicScatterSet_hlp,
		tclTransCosmicScatterSet_use);
}

