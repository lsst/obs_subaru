/* <AUTO>
   FILE: tclDustGetval
   ABSTRACT:
<HTML>
Routines to get falues from David Schlegel's dust maps.
</HTML>
   </AUTO>
*/
/* TCL interface to get values from dust maps */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"

#include "atDustGetval.h"

static char *tclFacil = "astrotools";    /* name of this set of code */

/****************************************************************************
**
** ROUTINE: tclAtExtinctionGet
**
**<AUTO EXTRACT>
**
** TCL VERB: atExtinctionGet
**
**<HTML>
**	Return the extinction in each filter at the specified galactic
**	coordinates.  The results are returned as a TCL keyed list, with each
**	filter (i.e., u) as the key, and the extinction in that filter as
**	the value.   This routine calls "atDustGetvalFast" at the specified
**	galactic coordinates, then uses hardwired Rv values to convert the
**	returned E(B-V) value to extinction values in each of the SDSS
**	filters.
**</HTML>
**</AUTO>
*/
static char g_atExtinctionGetCmd[] = "atExtinctionGet";
static int g_atExtinctionGetFlg = FTCL_ARGV_NO_LEFTOVERS;
static ftclArgvInfo g_atExtinctionGetTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Return extinction in each filter at specified galactic coordinates\n"},
   {"<l>", FTCL_ARGV_DOUBLE, NULL, NULL, "Galactic longitude (degrees)"},
   {"<b>", FTCL_ARGV_DOUBLE, NULL, NULL, "Galactic latitude (degrees)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclAtExtinctionGet (ClientData clientData,
		    Tcl_Interp *interp,
		    int argc,
		    char **argv)
{
  double l, b;
  float u, g, r, i, z;
  char out[80];
  int status;

  /* Parse the arguments */
  g_atExtinctionGetTbl[1].dst = &l;
  g_atExtinctionGetTbl[2].dst = &b;
  if ((status = shTclParseArgv(interp, &argc, argv, g_atExtinctionGetTbl,
			       g_atExtinctionGetFlg,
			       g_atExtinctionGetCmd))
      != FTCL_ARGV_SUCCESS)
    return status;

  /* Fetch the extinction */
  if (atExtinction(l, b, &u, &g, &r, &i, &z) != SH_SUCCESS)
    {
      Tcl_AppendResult(interp, g_atExtinctionGetCmd,
		       ": error getting extinction", NULL);
      return TCL_ERROR;
    }

  /* Return values as keyed list */
  sprintf(out, "{u %.4f} {g %.4f} {r %.4f} {i %.4f} {z %.4f}", u, g, r, i, z);
  Tcl_SetResult(interp, out, TCL_VOLATILE);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclAtDustGetval
**
**<AUTO EXTRACT>
** TCL VERB: atDustGetval
**
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atDustGetval.html#atDustGetval>atDustGetval</A> in atDustGetval.c
** Additional information is in the <A HREF=atDustGetval.readme>README</a> file.
** </HTML>
**
** Return the value of a dust map at the specified location
**</AUTO>
*/
char g_atDustGetval[] = "atDustGetval";
ftclArgvInfo g_atDustGetvalTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return the value of a dust map at the specified location.  The maps "
"are from Maps of Dust IR Emission for Use in Estimation of Reddening and"
" CMBR Foregrouns by Shclegel, Finkbeiner and Davie, ApJ 1998.  The maps "
"are I100 (100-micron map in MJy/Sr); X (X-map, temperature-correction "
"factor; T (temperature map in degrees Kelvin for n=2 emissivity); Ebv "
"( E(B-V) in magnitudes) and mask, and 8-bit mask"
 },
  {"<gall>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "galactic longitude in degrees"},
  {"<galb>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "galactic latitude in degrees"},
  {"-mapName", FTCL_ARGV_STRING, NULL, NULL,
   "I100, X, T, Ebv (default), mask"},
  {"-interpolate", FTCL_ARGV_INT, NULL, NULL,
   "1: linear interp. from 4 nearest pixels (default) or 0: nearest pixel"},
  {"-verbose", FTCL_ARGV_INT, NULL, NULL,
   "0: runs quietly (default) or 1: print pixel coordinates and map values"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclatDustGetval(ClientData clientData, Tcl_Interp *interp, 
		int argc, char **argv) {

  int rstat;
  double gall = 0.0;
  double galb = 0.0;
  char *pMapName = "Ebv";
  int interpolate = 1;
  int verbose = 0;
  double value = 0.0;

  char answer[100];

  g_atDustGetvalTbl[1].dst = &gall;
  g_atDustGetvalTbl[2].dst = &galb;
  g_atDustGetvalTbl[3].dst = &pMapName;
  g_atDustGetvalTbl[4].dst = &interpolate;
  g_atDustGetvalTbl[5].dst = &verbose;


  if ((rstat = shTclParseArgv(interp, &argc, argv, g_atDustGetvalTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_atDustGetval)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
/* make the call */
  if (atDustGetval(gall, galb, pMapName, interpolate, verbose, &value) 
      != SH_SUCCESS) {
    shErrStackPush("Error from atDustGetval");
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR; 
  }
  sprintf(answer, "%f", value);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;
}
/****************************************************************************
**
** ROUTINE: tclAtVDustGetval
**
**<AUTO EXTRACT>
** TCL VERB: atVDustGetval
**
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atVDustGetval.html#atVDustGetval>atVDustGetval</A> in atVDustGetval.c
** Additional information is in the <A HREF=atVDustGetval.readme>README</a> file.
** </HTML>
**
** Return the value of a VDust map at the specified location
**</AUTO>
*/
char g_atVDustGetval[] = "atVDustGetval";
ftclArgvInfo g_atVDustGetvalTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return the value of a dust map at the specified location.  The maps "
"are from Maps of Dust IR Emission for Use in Estimation of Reddening and"
" CMBR Foregrouns by Shclegel, Finkbeiner and Davie, ApJ 1998.  The maps "
"are I100 (100-micron map in MJy/Sr); X (X-map, temperature-correction "
"factor; T (temperature map in degrees Kelvin for n=2 emissivity); Ebv "
"( E(B-V) in magnitudes) and mask, and 8-bit mask"
 },
  {"<gall>", FTCL_ARGV_STRING, NULL, NULL, 
   "galactic longitude in degrees"},
  {"<galb>", FTCL_ARGV_STRING, NULL, NULL, 
   "galactic latitude in degrees"},
  {"<value>", FTCL_ARGV_STRING, NULL, NULL, 
   "vector to store the returned values"},
  {"-mapName", FTCL_ARGV_STRING, NULL, NULL,
   "I100, X, T, Ebv (default), mask"},
  {"-interpolate", FTCL_ARGV_INT, NULL, NULL,
   "1: linear interp. from 4 nearest pixels (default) or 0: nearest pixel"},
  {"-verbose", FTCL_ARGV_INT, NULL, NULL,
   "0: runs quietly (default) or 1: print pixel coordinates and map values"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclatVDustGetval(ClientData clientData, Tcl_Interp *interp, 
		 int argc, char **argv) {

  int rstat;
  char *a_gall = NULL;
  VECTOR * gall = NULL;
  char *a_galb = NULL;
  VECTOR * galb = NULL;
  char *a_value = NULL;
  VECTOR * value = NULL;

  char *pMapName = "Ebv";
  int interpolate = 1;
  int verbose = 0;

  g_atVDustGetvalTbl[1].dst = &a_gall;
  g_atVDustGetvalTbl[2].dst = &a_galb;
  g_atVDustGetvalTbl[3].dst = &a_value;
  g_atVDustGetvalTbl[4].dst = &pMapName;
  g_atVDustGetvalTbl[5].dst = &interpolate;
  g_atVDustGetvalTbl[6].dst = &verbose;


  if ((rstat = shTclParseArgv(interp, &argc, argv, g_atVDustGetvalTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_atVDustGetval)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_gall, (void **) &gall, "VECTOR") 
      != TCL_OK) {
    Tcl_SetResult(interp, "Unable to get gall address", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (shTclAddrGetFromName(interp, a_galb, (void **) &galb, "VECTOR") 
      != TCL_OK) {
    Tcl_SetResult(interp, "Unable to get galb address", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (shTclAddrGetFromName(interp, a_value, (void **) &value, "VECTOR") 
      != TCL_OK) {
    Tcl_SetResult(interp, "Unable to get value address", TCL_VOLATILE);
    return TCL_ERROR;
  }

/* make the call */
  if (atVDustGetval(gall, galb, pMapName, interpolate, verbose, value) 
      != SH_SUCCESS) {
    shErrStackPush("Error from atVDustGetval");
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR; 
  }
  return TCL_OK;
}



/****************************************************************************
**
** ROUTINE: tclAtDustGetvalFast
**
**<AUTO EXTRACT>
** TCL VERB: atDustGetvalFast
**
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atDustGetvalFast.html#atDustGetvalFast>atDustGetvalFast</A> in atDustGetval.c
** Additional information is in the <A HREF=atDustGetvalFast.readme>README</a> file.
** </HTML>
**
** Return the value of a dust map at the specified location
**</AUTO>
*/
char g_atDustGetvalFast[] = "atDustGetvalFast";
ftclArgvInfo g_atDustGetvalFastTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return the value of a dust map at the specified location.  The maps "
"are from Maps of Dust IR Emission for Use in Estimation of Reddening and"
" CMBR Foregrouns by Shclegel, Finkbeiner and Davie, ApJ 1998.  The maps "
"are I100 (100-micron map in MJy/Sr); X (X-map, temperature-correction "
"factor; T (temperature map in degrees Kelvin for n=2 emissivity); Ebv "
"( E(B-V) in magnitudes) and mask, and 8-bit mask.  The first time called "
"for a map, read in the FITS file to a REGION.  Clean-up with "
"atDustGetvalFastClean."
 },
  {"<gall>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "galactic longitude in degrees"},
  {"<galb>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "galactic latitude in degrees"},
  {"-mapName", FTCL_ARGV_STRING, NULL, NULL,
   "I100, X, T, Ebv (default), mask"},
  {"-interpolate", FTCL_ARGV_INT, NULL, NULL,
   "1: linear interp. from 4 nearest pixels (default) or 0: nearest pixel"},
  {"-verbose", FTCL_ARGV_INT, NULL, NULL,
   "0: runs quietly (default) or 1: print pixel coordinates and map values"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclatDustGetvalFast(ClientData clientData, Tcl_Interp *interp, 
		int argc, char **argv) {

  int rstat;
  double gall = 0.0;
  double galb = 0.0;
  char *pMapName = "Ebv";
  int interpolate = 1;
  int verbose = 0;
  double value = 0.0;

  char answer[100];

  g_atDustGetvalFastTbl[1].dst = &gall;
  g_atDustGetvalFastTbl[2].dst = &galb;
  g_atDustGetvalFastTbl[3].dst = &pMapName;
  g_atDustGetvalFastTbl[4].dst = &interpolate;
  g_atDustGetvalFastTbl[5].dst = &verbose;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_atDustGetvalFastTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_atDustGetvalFast)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

/* make the call */
  if (atDustGetvalFast(gall, galb, pMapName, interpolate, verbose, &value) 
      != SH_SUCCESS) {
    shErrStackPush("Error from atDustGetvalFast");
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR; 
  }
  sprintf(answer, "%f", value);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclAtVDustGetvalFast
**
**<AUTO EXTRACT>
** TCL VERB: atVDustGetvalFast
**
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atVDustGetvalFast.html#atVDustGetvalFast>atVDustGetvalFast</A> in atVDustGetvalFast.c
** Additional information is in the <A HREF=atDustGetvalFast.readme>README</a> file.
** </HTML>
**
** Return the value of a VDust map at the specified location
**</AUTO>
*/
char g_atVDustGetvalFast[] = "atVDustGetvalFast";
ftclArgvInfo g_atVDustGetvalFastTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return the value of a dust map at the specified location.  The maps "
"are from Maps of Dust IR Emission for Use in Estimation of Reddening and"
" CMBR Foregrouns by Shclegel, Finkbeiner and Davie, ApJ 1998.  The maps "
"are I100 (100-micron map in MJy/Sr); X (X-map, temperature-correction "
"factor; T (temperature map in degrees Kelvin for n=2 emissivity); Ebv "
"( E(B-V) in magnitudes) and mask, and 8-bit mask"
 },
  {"<gall>", FTCL_ARGV_STRING, NULL, NULL, 
   "galactic longitude in degrees"},
  {"<galb>", FTCL_ARGV_STRING, NULL, NULL, 
   "galactic latitude in degrees"},
  {"<value>", FTCL_ARGV_STRING, NULL, NULL, 
   "vector to store the returned values"},
  {"-mapName", FTCL_ARGV_STRING, NULL, NULL,
   "I100, X, T, Ebv (default), mask"},
  {"-interpolate", FTCL_ARGV_INT, NULL, NULL,
   "1: linear interp. from 4 nearest pixels (default) or 0: nearest pixel"},
  {"-verbose", FTCL_ARGV_INT, NULL, NULL,
   "0: runs quietly (default) or 1: print pixel coordinates and map values"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclatVDustGetvalFast(ClientData clientData, Tcl_Interp *interp, 
		 int argc, char **argv) {

  int rstat;
  char *a_gall = NULL;
  VECTOR * gall = NULL;
  char *a_galb = NULL;
  VECTOR * galb = NULL;
  char *a_value = NULL;
  VECTOR * value = NULL;

  char *pMapName = "Ebv";
  int interpolate = 1;
  int verbose = 0;

  g_atVDustGetvalFastTbl[1].dst = &a_gall;
  g_atVDustGetvalFastTbl[2].dst = &a_galb;
  g_atVDustGetvalFastTbl[3].dst = &a_value;
  g_atVDustGetvalFastTbl[4].dst = &pMapName;
  g_atVDustGetvalFastTbl[5].dst = &interpolate;
  g_atVDustGetvalFastTbl[6].dst = &verbose;


  if ((rstat = shTclParseArgv(interp, &argc, argv, g_atVDustGetvalFastTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_atVDustGetvalFast)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_gall, (void **) &gall, "VECTOR") 
      != TCL_OK) {
    Tcl_SetResult(interp, "Unable to get gall address", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (shTclAddrGetFromName(interp, a_galb, (void **) &galb, "VECTOR") 
      != TCL_OK) {
    Tcl_SetResult(interp, "Unable to get galb address", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (shTclAddrGetFromName(interp, a_value, (void **) &value, "VECTOR") 
      != TCL_OK) {
    Tcl_SetResult(interp, "Unable to get value address", TCL_VOLATILE);
    return TCL_ERROR;
  }

/* make the call */
  if (atVDustGetvalFast(gall, galb, pMapName, interpolate, verbose, value) 
      != SH_SUCCESS) {
    shErrStackPush("Error from atVDustGetvalFast");
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR; 
  }
  return TCL_OK;
}


/****************************************************************************
**
** ROUTINE: tclAtDustGetvalFastClean
**
**<AUTO EXTRACT>
** TCL VERB: atDustGetvalFastClean
**
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atDustGetvalFastClean.html#atDustGetvalFastClean>atDustGetvalFastClean</A> in atDustGetval.c
** Additional information is in the <A HREF=atDustGetvalFastClean.readme>README</a> file.
** </HTML>
**
** Remove region from memory previously retrieved from a dust map.
**</AUTO>
*/
char g_atDustGetvalFastClean[] = "atDustGetvalFastClean";
ftclArgvInfo g_atDustGetvalFastCleanTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Removes the regions previously read into memory by the task"
" atDustGetvalFastClean.  This simply frees memory."
 },
  {"-mapName", FTCL_ARGV_STRING, NULL, NULL,
   "I100, X, T, Ebv (default), mask"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclatDustGetvalFastClean(ClientData clientData, Tcl_Interp *interp, 
		int argc, char **argv) {

  int rstat;
  char *pMapName = "Ebv";
  g_atDustGetvalFastCleanTbl[1].dst = &pMapName;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_atDustGetvalFastCleanTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_atDustGetvalFastClean)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

/* make the call */
  if (atDustGetvalFastClean(pMapName) 
      != SH_SUCCESS) {
    shErrStackPush("Error from atDustGetvalFastClean");
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR; 
  }
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclAtDustMaskParse
**
**<AUTO EXTRACT>
** TCL VERB: atDustMaskParse
**
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atDustMaskParse.html#atDustMaskParse>atDustMaskParse</A> in atDustGetval.c
** Additional information is in the <A HREF=atDustMaskParse.readme>README</a> file.
** </HTML>
**
** Remove region from memory previously retrieved from a dust map.
**</AUTO>
*/
char g_atDustMaskParse[] = "atDustMaskParse";
ftclArgvInfo g_atDustMaskParseTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Returns dust mask bit info for input mask value.  Mask values are "
     "determined from the atDustGetval set of commands."
 },
  {"<maskval>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "value of dust mask returned from atDustGetval"},
   {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclatDustMaskParse(ClientData clientData, Tcl_Interp *interp, 
		int argc, char **argv) {

  int rstat;
  double maskval = 0.0;
  char maskinfo[100];

  g_atDustMaskParseTbl[1].dst = &maskval;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_atDustMaskParseTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_atDustMaskParse)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }


/* make the call */
  if (atDustMaskParse(maskval, maskinfo) 
      != SH_SUCCESS) {
    shErrStackPush("Error from atDustMaskParse");
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR; 
  }
  Tcl_SetResult(interp, maskinfo, TCL_VOLATILE);
  return TCL_OK;
}



/**********************************************************************/
/* Declare new tcl verbs for ASTROTOOLS dust getval functions*/
void atTclDustGetvalDeclare(Tcl_Interp *interp) {
  
  int flags = FTCL_ARGV_NO_LEFTOVERS;

  shTclDeclare(interp, g_atExtinctionGetCmd, 
               (Tcl_CmdProc *) tclAtExtinctionGet,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_atExtinctionGetTbl,
			       g_atExtinctionGetFlg, 
			       g_atExtinctionGetCmd),
               shTclGetUsage(interp, g_atExtinctionGetTbl,
			     g_atExtinctionGetFlg, 
			     g_atExtinctionGetCmd));

  shTclDeclare(interp, g_atDustGetval, 
               (Tcl_CmdProc *) tclatDustGetval,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_atDustGetvalTbl, flags, 
			       g_atDustGetval),
               shTclGetUsage(interp, g_atDustGetvalTbl, flags, 
			     g_atDustGetval));

  shTclDeclare(interp, g_atVDustGetval, 
               (Tcl_CmdProc *) tclatVDustGetval,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_atVDustGetvalTbl, flags, 
			       g_atVDustGetval),
               shTclGetUsage(interp, g_atVDustGetvalTbl, flags, 
			     g_atVDustGetval));

  shTclDeclare(interp, g_atDustGetvalFast, 
               (Tcl_CmdProc *) tclatDustGetvalFast,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_atDustGetvalFastTbl, flags, 
			       g_atDustGetvalFast),
               shTclGetUsage(interp, g_atDustGetvalFastTbl, flags, 
			     g_atDustGetvalFast));

  shTclDeclare(interp, g_atVDustGetvalFast, 
               (Tcl_CmdProc *) tclatVDustGetvalFast,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_atVDustGetvalFastTbl, flags, 
			       g_atVDustGetvalFast),
               shTclGetUsage(interp, g_atVDustGetvalFastTbl, flags, 
			     g_atVDustGetvalFast));

  shTclDeclare(interp, g_atDustGetvalFastClean, 
               (Tcl_CmdProc *) tclatDustGetvalFastClean,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_atDustGetvalFastCleanTbl, flags, 
			       g_atDustGetvalFastClean),
               shTclGetUsage(interp, g_atDustGetvalFastCleanTbl, flags, 
			     g_atDustGetvalFastClean));

  shTclDeclare(interp, g_atDustMaskParse, 
               (Tcl_CmdProc *) tclatDustMaskParse,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_atDustMaskParseTbl, flags,
			       g_atDustMaskParse),
               shTclGetUsage(interp, g_atDustMaskParseTbl, flags, 
			     g_atDustMaskParse));


}





