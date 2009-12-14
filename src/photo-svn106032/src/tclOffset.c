/*
 * TCL support for coordinate transformations in photo
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "phUtils.h"
#include "dervish.h"
#include "phOffset.h"
#include "atTrans.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

/*****************************************************************************/

static char *tclOffsetDo_use =
  "USAGE: offsetDo <fiparams> <row0> <col0> <band0> <band1> [filterlist] [mag] -magErr -toCanonical";
#define tclOffsetDo_hlp \
  "Given a row and a column in band <band0>, return the offset required to \n\
take those coordinates to band <band1>; offsetDo returns\n\
  \"drow drowErr dcol dcolErr\", the new coordinates are \n\
row + drow, col + dcol with errors drowErr and dcolErr (not including errors\n\
in row and col).\n\
\n\
If you want to provide a TCL array of magnitudes, you must also specify the \n\
names used to index it."

static ftclArgvInfo offsetDo_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclOffsetDo_hlp},
   {"<fiparams>", FTCL_ARGV_STRING, NULL, NULL,
				       "FIELDPARAMS giving TRANS information"},
   {"<row0>", FTCL_ARGV_DOUBLE, NULL, NULL,"input row coordinate (pixels)"},
   {"<col0>", FTCL_ARGV_DOUBLE, NULL, NULL,"input column coordinate (pixels)"},
   {"<band0>", FTCL_ARGV_INT, NULL, NULL, "band of (row0, col0)"},
   {"<band1>", FTCL_ARGV_INT, NULL, NULL, "Band to transform to"},
   {"[filterlist]", FTCL_ARGV_STRING, NULL, NULL, "List of filters"},
   {"[mag]", FTCL_ARGV_STRING, NULL, NULL, "Magnitudes in all bands"},
   {"-magErr", FTCL_ARGV_STRING, NULL, NULL, "Magnitude errors in all bands"},
   {"-relativeErrors", FTCL_ARGV_CONSTANT, (void *)1, NULL,
   "Calculate errors assuming that we are interested in band-to-band offsets"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define offsetDo_name "offsetDo"

static int
tclOffsetDo(ClientData clientData,
	    Tcl_Interp *interp,
	    int ac,
	    char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *fiparamsStr = NULL;		/* TRANS information */
   FIELDPARAMS *fiparams;
   double row0 = 0.0;			/* input row coordinate (pixels) */
   double col0 = 0.0;			/* input column coordinate (pixels) */
   int band0 = 0;			/* Coordinate system of (row0, col0) */
   int band1 = 0;			/* Band to transform to */
   float drow, dcol;			/* desired offsets */
   float drowErr, dcolErr;		/* errors in offsets */
   char *filterlist = NULL;		/* List of filters */
   char *magStr = NULL;			/* Magnitudes in all bands */
   float *mag;
   char *magErrStr = NULL;		/* Magnitude errors in all bands */
   float *magErr;
   float magErrArray[NCOLOR] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
   int relativeErrors = 0;			/* Calculate errors assuming that we
					   are interested in band-to-band
					   offsets */

   shErrStackClear();

   a_i = 1;
   offsetDo_opts[a_i++].dst = &fiparamsStr;
   offsetDo_opts[a_i++].dst = &row0;
   offsetDo_opts[a_i++].dst = &col0;
   offsetDo_opts[a_i++].dst = &band0;
   offsetDo_opts[a_i++].dst = &band1;
   offsetDo_opts[a_i++].dst = &filterlist;
   offsetDo_opts[a_i++].dst = &magStr;
   offsetDo_opts[a_i++].dst = &magErrStr;
   offsetDo_opts[a_i++].dst = &relativeErrors;
   shAssert(offsetDo_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, offsetDo_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     offsetDo_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,fiparamsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDPARAMS")) {
      Tcl_AppendResult(interp,"offsetDo: argument \"%s\" is not a FIELDPARAMS",
                       fiparamsStr, NULL);
      return(TCL_ERROR);
   }
   fiparams = hand.ptr;

   if(magStr == NULL) {
      mag = NULL;
   } else {
      if(phFltArrayGetFromTclArray(interp, magStr, filterlist, &mag) < 0) {
	 shErrStackPush("offsetDo: can't get mag");
	 return(TCL_ERROR);
      }
   }

   if(magErrStr == NULL) {
      magErr = NULL;
   } else {
      if(phFltArrayGetFromTclArray(interp, magErrStr, filterlist, &magErr)
									 < 0) {
	 shErrStackPush("offsetDo: can't get magErr");
	 return(TCL_ERROR);
      }
   }

   if(mag != NULL && magErr == NULL) {
      magErr = magErrArray;
   }
/*
 * work
 */
   phOffsetDo(fiparams, row0, col0, band0, band1,
	      relativeErrors, mag, magErr, &drow, &drowErr, &dcol, &dcolErr);
/*
 * return result
 */
   {
      char buff[100];
      sprintf(buff,"%g %g %g %g", drow, drowErr, dcol, dcolErr);
      Tcl_SetResult(interp,buff,TCL_VOLATILE);
   }

   if(magErr != magErrArray) {
      shFree(magErr);
   }
   shFree(mag);
   return(TCL_OK);
}

/*****************************************************************************/
static char *tclAstromOffsetsFind_use =
  "USAGE: astromOffsetsFind <objects> <ncolor> <fiparams> <fieldstat>";
#define tclAstromOffsetsFind_hlp \
  "Find the astrometric offsets between the bands from a chain of merged objects"

static ftclArgvInfo astromOffsetsFind_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclAstromOffsetsFind_hlp},
   {"<objects>", FTCL_ARGV_STRING, NULL, NULL, "Chain of OBJCs"},
   {"<ncolor>", FTCL_ARGV_INT, NULL, NULL,
				     "Number of colours present in each OBJC"},
   {"<fiparams>", FTCL_ARGV_STRING, NULL, NULL, "FIELDPARAMS for field"},
   {"<fieldstat>", FTCL_ARGV_STRING, NULL, NULL, "FIELDSTAT for field"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define astromOffsetsFind_name "astromOffsetsFind"

static int
tclAstromOffsetsFind(ClientData clientData,
		     Tcl_Interp *interp,
		     int ac,
		     char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *objectsStr = NULL;		/* Chain of OBJCs */
   CHAIN *objects;
   int ncolor = 0;			/* Number of colours present in each OBJC */
   char *fiparamsStr = NULL;		/* FIELDPARAMS for field */
   FIELDPARAMS *fiparams;
   char *fieldstatStr = NULL;		/* FIELDSTAT for field */
   FIELDSTAT *fieldstat;

   shErrStackClear();

   i = 1;
   astromOffsetsFind_opts[i++].dst = &objectsStr;
   astromOffsetsFind_opts[i++].dst = &ncolor;
   astromOffsetsFind_opts[i++].dst = &fiparamsStr;
   astromOffsetsFind_opts[i++].dst = &fieldstatStr;
   shAssert(astromOffsetsFind_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, astromOffsetsFind_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     astromOffsetsFind_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objectsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"astromOffsetsFind: "
                       "argument \"", objectsStr, "\" is not an CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objects = hand.ptr;

   if(objects->type != shTypeGetFromName("OBJC")) {
      Tcl_SetResult(interp,"CHAIN \"objects\" doesn't have type OBJC",
								   TCL_STATIC);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,fiparamsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDPARAMS")) {
      Tcl_AppendResult(interp,"astromOffsetsFind: "
                       "argument \"", fiparamsStr, "\" is not an FIELDPARAMS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   fiparams = hand.ptr;

   if(shTclHandleExprEval(interp,fieldstatStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDSTAT")) {
      Tcl_AppendResult(interp,"astromOffsetsFind: "
                       "argument \"", fieldstatStr, "\" is not an FIELDSTAT",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   fieldstat = hand.ptr;   
/*
 * work
 */
   phAstromOffsetsFind(objects, ncolor, fiparams, fieldstat);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclOffsetDeclare(Tcl_Interp *interp)
{

   shTclDeclare(interp,offsetDo_name,
		(Tcl_CmdProc *)tclOffsetDo, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclOffsetDo_hlp,
		tclOffsetDo_use);

   shTclDeclare(interp,astromOffsetsFind_name,
		(Tcl_CmdProc *)tclAstromOffsetsFind, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclAstromOffsetsFind_hlp,
		tclAstromOffsetsFind_use);
}
