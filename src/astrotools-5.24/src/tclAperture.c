/* WARNING -- this file was generated automatically */
/* by sdssmath/etc/makeTclBindings.pl */
/* <AUTO>
FILE : tclAperture.c

ABSTRACT:
<HTML>
Measuring aperture counts in regions
</HTML>
</AUTO>
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "dervish.h"
#include "astrotools.h"
static char *tclFacil  = "astrotools";

/*********************************************************
**
** ROUTINE tcl_meanFindInAperture 
**
**<AUTO EXTRACT>
** TCL VERB: meanFindInAperture
**
** <HTML>
** C ROUTINE CALLED :  
** <A HREF="atAperture.html#atMeanFindInAperture">MeanFindInAperture</A>
** in atAperture.html
** </HTML>
**</AUTO>
*********************************************************/

char g_meanFindInAperture[] = "meanFindInAperture";
ftclArgvInfo g_meanFindInApertureTbl[] = {
{NULL, FTCL_ARGV_HELP, NULL, NULL,
      "Find the mean pixel value in an annulus centered at (row,column)"
      "between inRadius and outRadius. Return a keyed list of the mean,"
      "meanErr, pixels (the number of pixels used) and retVal (return value:"
      "0 is ok; 1 if radius extends over edge of image; 2 if there is a"
      "serious memory problem)"
    },
    {"<reg>", FTCL_ARGV_STRING,NULL, NULL, "the region to measure"},
    {"<row>", FTCL_ARGV_DOUBLE,NULL, NULL, "row of aperture center"},
    {"<col>", FTCL_ARGV_DOUBLE,NULL, NULL, "column of aperture center"},
    {"<outRadius>", FTCL_ARGV_DOUBLE,NULL, NULL, "outer radius (in pixels)"},
    {"-inRadius", FTCL_ARGV_DOUBLE,NULL, NULL, "inner radius (in pixels)    Default : 0"},
    {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tcl_meanFindInAperture(ClientData clientData, Tcl_Interp *interp, int argc, char **argv) {

int rstat;
char tempString[100];
REGION* reg = NULL; 
char* handle_reg = NULL;
double row = 0.0; 
double col = 0.0; 
double inRadius = 0.0; 
double outRadius = 0.0; 
double mean, meanErr, pixels;
int retVal;

/* parse inputs  */
g_meanFindInApertureTbl[1].dst = &handle_reg;
g_meanFindInApertureTbl[2].dst = &row;
g_meanFindInApertureTbl[3].dst = &col;
g_meanFindInApertureTbl[4].dst = &outRadius;
g_meanFindInApertureTbl[5].dst = &inRadius;

if ((rstat = shTclParseArgv(interp, &argc, argv,g_meanFindInApertureTbl, 
      FTCL_ARGV_NO_LEFTOVERS, g_meanFindInAperture))!= FTCL_ARGV_SUCCESS) return rstat;

/* do so error checks */
if (handle_reg==NULL) {
  reg = NULL;
} else {
  if (shTclAddrGetFromName(interp, handle_reg, (void **) &reg, "REGION") != TCL_OK) {
    Tcl_SetResult(interp, "Bad REGION for reg", TCL_VOLATILE); 
    return TCL_ERROR;
  }
}

if ( (inRadius <0) || (outRadius < 0) ) {
  Tcl_SetResult(interp, "negative radius", TCL_VOLATILE);
  return TCL_ERROR;
}

if ( inRadius > outRadius ) {
  Tcl_SetResult(interp, "inRadius greater than outRadius", TCL_VOLATILE);
  return TCL_ERROR;
}

/* make the C-call */
retVal = atMeanFindInAperture(reg, row, col, inRadius, outRadius, 
         &mean, &meanErr, &pixels, 0, 0);

/* return a keyed list */
sprintf(tempString, "mean %g", mean);
Tcl_AppendElement(interp, tempString);
sprintf(tempString, "meanErr %g", meanErr);
Tcl_AppendElement(interp, tempString);
sprintf(tempString, "pixels %g", pixels);
Tcl_AppendElement(interp, tempString);
sprintf(tempString, "retVal %d", retVal);
Tcl_AppendElement(interp, tempString);

return TCL_OK;
}

/*********************************************************
**
** ROUTINE tcl_medianFindInAperture 
**
**<AUTO EXTRACT>
** TCL VERB: medianFindInAperture
**
** <HTML>
** C ROUTINE CALLED :  
** <A HREF="atAperture.html#atMeanFindInAperture">MeanFindInAperture</A>
** in atAperture.html
** </HTML>
**</AUTO>
*********************************************************/

char g_medianFindInAperture[] = "medianFindInAperture";
ftclArgvInfo g_medianFindInApertureTbl[] = { 
      {NULL, FTCL_ARGV_HELP, NULL, NULL,
      "Find the median pixel value in an annulus centered at (row,column)"
      "between inRadius and outRadius. Return a keyed list of the mean,"
      "meanErr, pixels (the number of pixels used) and retVal (return value:" 
      "0 is ok; 1 if radius extends over edge of image; 2 if there is a"
      "serious memory problem)"
    },
    {"<reg>", FTCL_ARGV_STRING,NULL, NULL, "the region to measure"},
    {"<row>", FTCL_ARGV_DOUBLE,NULL, NULL, "row of aperture center"},
    {"<col>", FTCL_ARGV_DOUBLE,NULL, NULL, "column of aperture center"},
    {"<outRadius>", FTCL_ARGV_DOUBLE,NULL, NULL, "outer radius (in pixels)"},
    {"-inRadius", FTCL_ARGV_DOUBLE,NULL, NULL, "inner radius (in pixels)    Default : 0"},
    {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tcl_medianFindInAperture(ClientData clientData, 
Tcl_Interp *interp, int argc, char **argv) {
int rstat;
char tempString[100];
REGION* reg = NULL; 
char* handle_reg = NULL;
double row = 0.0; 
double col = 0.0; 
double inRadius = 0.0; 
double outRadius = 0.0; 
double mean, meanErr, pixels;
int retVal;

/* parse inputs */
g_medianFindInApertureTbl[1].dst = &handle_reg;
g_medianFindInApertureTbl[2].dst = &row;
g_medianFindInApertureTbl[3].dst = &col;
g_medianFindInApertureTbl[4].dst = &outRadius;
g_medianFindInApertureTbl[5].dst = &inRadius;

if ((rstat = shTclParseArgv(interp, &argc, argv,g_medianFindInApertureTbl, 
     FTCL_ARGV_NO_LEFTOVERS, g_medianFindInAperture))!= FTCL_ARGV_SUCCESS) return rstat;

/* do some error checks */
if (handle_reg==NULL) {
  reg = NULL;
} else {
  if (shTclAddrGetFromName(interp, handle_reg, (void **) &reg, "REGION") != TCL_OK) {
    Tcl_SetResult(interp, "Bad REGION for reg", TCL_VOLATILE); 
    return TCL_ERROR;
  }
}

if ( (outRadius <0) || (inRadius < 0) ) {
  Tcl_SetResult(interp, "negative radius", TCL_VOLATILE);
  return TCL_ERROR;
}

if ( inRadius > outRadius ) {
  Tcl_SetResult(interp, "inRadius greater than outRadius", TCL_VOLATILE);
  return TCL_ERROR;
}


/* make the actual C call */
retVal = atMeanFindInAperture(reg, row, col, inRadius, outRadius, 
	&mean, &meanErr, &pixels, -1, 0);

/* return answer as keyed list */
sprintf(tempString, "mean %g", mean);
Tcl_AppendElement(interp, tempString);
sprintf(tempString, "meanErr %g", meanErr);
Tcl_AppendElement(interp, tempString);
sprintf(tempString, "pixels %g", pixels);
Tcl_AppendElement(interp, tempString);
sprintf(tempString, "retVal %d", retVal);
Tcl_AppendElement(interp, tempString);
return TCL_OK;
}

/*********************************************************
**
** ROUTINE tcl_clippedMeanFindInAperture 
**
**<AUTO EXTRACT>
** TCL VERB: clippedMeanFindInAperture
**
** <HTML>
** C ROUTINE CALLED :  
** <A HREF="atAperture.html#atMeanFindInAperture">MeanFindInAperture</A>
** in atAperture.html
** </HTML>
**</AUTO>
*********************************************************/

char g_clippedMeanFindInAperture[] = "clippedMeanFindInAperture";
ftclArgvInfo g_clippedMeanFindInApertureTbl[] = {
{NULL, FTCL_ARGV_HELP, NULL, NULL,
      "Find the clipped mean pixel value in an annulus centered at (row,column)"
      "between inRadius and outRadius.  Reject at nsig sigmas at each of niter"
      "iterations. Return a keyed list of the mean, meanErr, pixels (the number"
      "of pixels used) and retVal (return value:  0 is ok; 1 if radius extends"
      "over edge of image; 2 if there is a serious memory problem)"
    },
    {"<reg>", FTCL_ARGV_STRING,NULL, NULL, "the region to measure"},
    {"<row>", FTCL_ARGV_DOUBLE,NULL, NULL, "row of aperture center"},
    {"<col>", FTCL_ARGV_DOUBLE,NULL, NULL, "column of aperture center"},
    {"<outRadius>", FTCL_ARGV_DOUBLE,NULL, NULL, "outer radius (in pixels)"},
    {"-inRadius", FTCL_ARGV_DOUBLE,NULL, NULL, "inner radius (in pixels)    Default : 0"},
    {"-nsig", FTCL_ARGV_DOUBLE,NULL, NULL, "number of sigmas for rejection    Default : 5.0"},
    {"-niter", FTCL_ARGV_DOUBLE,NULL, NULL, "number of iterations    Default : 2"},
    {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tcl_clippedMeanFindInAperture(ClientData clientData, Tcl_Interp *interp, int argc, char **argv) {

int rstat;
char tempString[100];
REGION* reg = NULL; 
char* handle_reg = NULL;
double row = 0.0; 
double col = 0.0; 
double inRadius = 0.0; 
double outRadius = 0.0; 
double nsig = 5.0; 
double niter = 2; 
double mean, meanErr, pixels;
int retVal;

/* parse inputs */
g_clippedMeanFindInApertureTbl[1].dst = &handle_reg;
g_clippedMeanFindInApertureTbl[2].dst = &row;
g_clippedMeanFindInApertureTbl[3].dst = &col;
g_clippedMeanFindInApertureTbl[4].dst = &outRadius;
g_clippedMeanFindInApertureTbl[5].dst = &inRadius;
g_clippedMeanFindInApertureTbl[6].dst = &nsig;
g_clippedMeanFindInApertureTbl[7].dst = &niter;

if ((rstat = shTclParseArgv(interp, &argc, argv,g_clippedMeanFindInApertureTbl, 
    FTCL_ARGV_NO_LEFTOVERS, g_clippedMeanFindInAperture))!= FTCL_ARGV_SUCCESS) return rstat;

/* do some error checks */
if (handle_reg==NULL) {
  reg = NULL;
} else {
  if (shTclAddrGetFromName(interp, handle_reg, (void **) &reg, "REGION") != TCL_OK) {
    Tcl_SetResult(interp, "Bad REGION for reg", TCL_VOLATILE); 
    return TCL_ERROR;
  }
}

if ( (outRadius < 0) || (inRadius < 0) ) {
  Tcl_SetResult(interp, "negative radius", TCL_VOLATILE);
  return TCL_ERROR;
}

if ( inRadius > outRadius ) {
  Tcl_SetResult(interp, "inRadius greater than outRadius", TCL_VOLATILE);
  return TCL_ERROR;
}


if ( niter < 0 ) {
  Tcl_SetResult(interp, "negative niter", TCL_VOLATILE);
  return TCL_ERROR;
}

if ( nsig < 0 ) {
  Tcl_SetResult(interp, "negative nsig", TCL_VOLATILE);
  return TCL_ERROR;
}

/* make the actual C-call */
retVal = atMeanFindInAperture(reg, row, col, inRadius, outRadius, 
	&mean, &meanErr, &pixels, nsig, niter);

/* return the answer as a keyed list */
sprintf(tempString, "mean %g", mean);
Tcl_AppendElement(interp, tempString);
sprintf(tempString, "meanErr %g", meanErr);
Tcl_AppendElement(interp, tempString);
sprintf(tempString, "pixels %g", pixels);
Tcl_AppendElement(interp, tempString);
sprintf(tempString, "retVal %d", retVal);
Tcl_AppendElement(interp, tempString);
return TCL_OK;
}

/*******************************************************/
/*** declare these tcl verbs ***/
void tcl_aperture_declare(Tcl_Interp *interp) {
int flags = FTCL_ARGV_NO_LEFTOVERS;

shTclDeclare(interp, g_meanFindInAperture, (Tcl_CmdProc *) tcl_meanFindInAperture,
(ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
shTclGetArgInfo(interp, g_meanFindInApertureTbl, flags, g_meanFindInAperture),
shTclGetUsage(interp, g_meanFindInApertureTbl, flags, g_meanFindInAperture));

shTclDeclare(interp, g_medianFindInAperture, (Tcl_CmdProc *) tcl_medianFindInAperture,
(ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
shTclGetArgInfo(interp, g_medianFindInApertureTbl, flags, g_medianFindInAperture),
shTclGetUsage(interp, g_medianFindInApertureTbl, flags, g_medianFindInAperture));

shTclDeclare(interp, g_clippedMeanFindInAperture, (Tcl_CmdProc *) tcl_clippedMeanFindInAperture,
(ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
shTclGetArgInfo(interp, g_clippedMeanFindInApertureTbl, flags, g_clippedMeanFindInAperture),
shTclGetUsage(interp, g_clippedMeanFindInApertureTbl, flags, g_clippedMeanFindInAperture));
}
