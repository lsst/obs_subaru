/* <AUTO>
   FILE: tclImSim
   ABSTRACT:
<HTML>
   Routines to simulate images
</HTML>
   </AUTO>
*/
/***************************************************************
 *
 *              atImSim
 *
 *              written: May 30, 1995
 *			 Heidi Newberg
 *
 *
 **************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "atImSim.h"
#include "atGauss.h"

static char *module = "astrotools";   /* name of this set of code */

/**************************************************************
                      tclExpDiskAdd
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: expDiskAdd

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atImSim.html#atExpDiskAdd>atExpDiskAdd</A> in atImSim.c

<P>
  Add an exponential disk to an image.  All disks are face on.
  to make an inclined disk, bin the image in row or column.

  </HTML>

</AUTO>*/

static ftclArgvInfo expDiskAddTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Adds an exponential disk to an image.\n"},
  {"<region>", FTCL_ARGV_STRING, NULL, NULL, "region-handle"},
  {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "row position"},
  {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, "column position"},
  {"<Izero>", FTCL_ARGV_DOUBLE, NULL, NULL, "disk amplitude (total counts)"},
  {"<Rs>", FTCL_ARGV_DOUBLE, NULL, NULL, "disk scale length"},
  {"<theta>", FTCL_ARGV_DOUBLE, NULL, NULL, "position angle"},
  {"<axisRatio>", FTCL_ARGV_DOUBLE, NULL, NULL, "axis Ratio (0.1 < ar < 1)"},
  {"-peak", FTCL_ARGV_CONSTANT, (void*)1, NULL, "interpret Izero as peak counts not total"},
  {"-fast", FTCL_ARGV_CONSTANT, (void*)1, NULL, "fast computation (FL32 only)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *expDiskAdd_name = "expDiskAdd";

static int
tclExpDiskAdd(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  REGION *region = (REGION *)0;		/* region to add disk to */
  double row;			/* row position */
  double col;			/* column position */
  double Izero;			/* disk amplitude */
  double Rs;			/* disk half-light radius */
  int    peak=0;                  /* interpret as peak not total */
  int    fast=0;                  /* fast computation */
                                /* OUTPUT */
  double totcounts;		/* total counts added */
  double theta,axisRatio;

  char *regionName;
  char answer[80];              /* sent back to Tcl */
/*   static char *function_name = "expDiskAdd";  */


/* Parse the command */

  expDiskAddTable[1].dst = &regionName;
  expDiskAddTable[2].dst = &row;
  expDiskAddTable[3].dst = &col;
  expDiskAddTable[4].dst = &Izero;
  expDiskAddTable[5].dst = &Rs;
  expDiskAddTable[6].dst = &theta;
  expDiskAddTable[7].dst = &axisRatio;
  expDiskAddTable[8].dst = &peak;
  expDiskAddTable[9].dst = &fast;

  if ((shTclParseArgv( interp, &argc, argv, expDiskAddTable, 0, 
	expDiskAdd_name)) != FTCL_ARGV_SUCCESS) {
        return(TCL_ERROR);
  }

  if (shTclAddrGetFromName (interp, regionName, 
     (void **) &region, "REGION") != TCL_OK) {
	Tcl_SetResult
	(interp, "expDiskAdd: bad region name", TCL_VOLATILE);
	return TCL_ERROR;
  }

  if((fast==1) && (region->type != TYPE_FL32)) {
     Tcl_SetResult(interp, 
	"imSim: -fast only works with FL32 regions", TCL_VOLATILE);
        return TCL_ERROR;
    }

  atExpDiskAdd(region, row, col, Izero, Rs, theta, axisRatio, peak,fast,&totcounts);

  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%f", totcounts);
  Tcl_AppendElement(interp, answer);

  return TCL_OK;
}

/**************************************************************
                      tclDeVaucAdd
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: deVaucAdd

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atImSim.html#atDeVaucAdd>atDeVaucAdd</A> in atImSim.c

<P>
  Add a de Vaucouleurs profile to an image.  All disks are 
  face on.  To make an inclined disk, bin the image in row 
  or column.

  </HTML>

</AUTO>*/

static ftclArgvInfo deVaucAddTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Adds a de Vaucouleurs profile to an image.\n"},
  {"<region>", FTCL_ARGV_STRING, NULL, NULL, "region-handle"},
  {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "row position"},
  {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, "column position"},
  {"<Izero>", FTCL_ARGV_DOUBLE, NULL, NULL, "bulge amplitude (total counts)"},
  {"<Rs>", FTCL_ARGV_DOUBLE, NULL, NULL, "bulge scale length"},
  {"<theta>", FTCL_ARGV_DOUBLE, NULL, NULL, "position angle"},
  {"<axisRatio>", FTCL_ARGV_DOUBLE, NULL, NULL, "axis Ratio (0.1 < ar < 1)"},
  {"-peak", FTCL_ARGV_CONSTANT, (void*)1, NULL, "interpret Izero as peak counts, not total"},
  {"-fast", FTCL_ARGV_CONSTANT, (void*)1, NULL, "fast computation (image type -FL32 only"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *deVaucAdd_name = "deVaucAdd";

static int
tclDeVaucAdd(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  REGION *region = (REGION *)0;	/* region to add disk to */
  double row;			/* row position */
  double col;			/* column position */
  double Izero;			/* disk amplitude */
  double Rs;			/* disk half-light radius */
  double theta,axisRatio;
  int peak=0;
  int fast=0;
                                /* OUTPUT */
  double totcounts;		/* total counts added */

  char *regionName;
  char answer[80];              /* sent back to Tcl */
/*   static char *function_name = "deVaucAdd";  */

/* Parse the command */

  deVaucAddTable[1].dst = &regionName;
  deVaucAddTable[2].dst = &row;
  deVaucAddTable[3].dst = &col;
  deVaucAddTable[4].dst = &Izero;
  deVaucAddTable[5].dst = &Rs;
  deVaucAddTable[6].dst = &theta;
  deVaucAddTable[7].dst = &axisRatio;
  deVaucAddTable[8].dst = &peak;
  deVaucAddTable[9].dst = &fast;

  if ((shTclParseArgv( interp, &argc, argv, deVaucAddTable, 0, 
	deVaucAdd_name)) != FTCL_ARGV_SUCCESS) {
        return(TCL_ERROR);
  }

  if (shTclAddrGetFromName (interp, regionName, 
     (void **) &region, "REGION") != TCL_OK) {
	Tcl_SetResult
	(interp, "deVaucAdd: bad region name", TCL_VOLATILE);
	return TCL_ERROR;
  }

  if((fast==1) && (region->type!= TYPE_FL32)) {
     Tcl_SetResult(interp, 
	"imSim: -fast only works with FL32 regions", TCL_VOLATILE);
        return TCL_ERROR;
    }

  atDeVaucAdd(region, row, col, Izero, Rs, theta, axisRatio, peak, fast,&totcounts);

  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%f", totcounts);
  Tcl_AppendElement(interp, answer);

  return TCL_OK;

}
/*<AUTO EXTRACT>
  TCL VERB: deVaucAddSlow

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atImSim.html#atDeVaucAddSlow>atDeVaucAddSlow</A> in atImSim.c

<P>
  Add a de Vaucouleurs profile to an image.  All disks are 
  face on.  To make an inclined disk, bin the image in row 
  or column.

  </HTML>

</AUTO>*/

static ftclArgvInfo deVaucAddSlowTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Adds a de Vaucouleurs profile to an image.\n"},
  {"<region>", FTCL_ARGV_STRING, NULL, NULL, "region-handle"},
  {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "row position"},
  {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, "column position"},
  {"<counts>", FTCL_ARGV_DOUBLE, NULL, NULL, "bulge amplitude (total counts)"},
  {"<Rs>", FTCL_ARGV_DOUBLE, NULL, NULL, "bulge scale length"},
  {"<maxRad>", FTCL_ARGV_DOUBLE, NULL, NULL, "max radius to add counts"},
  {"<theta>", FTCL_ARGV_DOUBLE, NULL, NULL, "position angle"},
  {"<axisRatio>", FTCL_ARGV_DOUBLE, NULL, NULL, "axis Ratio (0.1 < ar < 1)"},
  {"<ngrid>", FTCL_ARGV_INT, NULL, NULL, "number of grid points per pixel"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static char *deVaucAddSlow_name = "deVaucAddSlow";

static int
tclDeVaucAddSlow(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  REGION *region = NULL;	/* region to add disk to */
  double row;			/* row position */
  double col;			/* column position */
  double counts;			/* total counts to add */
  double Rs;			/* disk half-light radius */
  double maxRad;
  double theta,axisRatio;

  int ngrid=1;

  char *regionName;
  double countsOut;
  char answer[80];              /* sent back to Tcl */
/* Parse the command */

  deVaucAddSlowTable[1].dst = &regionName;
  deVaucAddSlowTable[2].dst = &row;
  deVaucAddSlowTable[3].dst = &col;
  deVaucAddSlowTable[4].dst = &counts;
  deVaucAddSlowTable[5].dst = &Rs;
  deVaucAddSlowTable[6].dst = &maxRad;
  deVaucAddSlowTable[7].dst = &theta;
  deVaucAddSlowTable[8].dst = &axisRatio;
  deVaucAddSlowTable[9].dst = &ngrid;

  if ((shTclParseArgv( interp, &argc, argv, deVaucAddSlowTable, 0, 
	deVaucAddSlow_name)) != FTCL_ARGV_SUCCESS) {
        return(TCL_ERROR);
  }

  if (shTclAddrGetFromName (interp, regionName, 
     (void **) &region, "REGION") != TCL_OK) {
	Tcl_SetResult
	(interp, "deVaucAddSlow: bad region name", TCL_VOLATILE);
	return TCL_ERROR;
  }

  if(region->type!= TYPE_FL32) {
     Tcl_SetResult(interp, 
	"deVaucAddSlow only works with FL32 regions", TCL_VOLATILE);
        return TCL_ERROR;
    }

  atDeVaucAddSlow(region, row, col, counts, Rs, maxRad,
		  theta, axisRatio, ngrid, &countsOut);

  sprintf(answer, "%f", countsOut);
  Tcl_AppendElement(interp, answer);

  return TCL_OK;

}
/*<AUTO EXTRACT>
  TCL VERB: expDiskAddSlow

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atImSim.html#atexpDiskAddSlow>atexpDiskAddSlow</A> in atImSim.c

<P>
  Add an exponential disk to the region.  
  </HTML>

</AUTO>*/

static ftclArgvInfo expDiskAddSlowTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Adds a de Vaucouleurs profile to an image.\n"},
  {"<region>", FTCL_ARGV_STRING, NULL, NULL, "region-handle"},
  {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "row position"},
  {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, "column position"},
  {"<counts>", FTCL_ARGV_DOUBLE, NULL, NULL, "bulge amplitude (total counts)"},
  {"<Rs>", FTCL_ARGV_DOUBLE, NULL, NULL, "bulge scale length"},
  {"<maxRad>", FTCL_ARGV_DOUBLE, NULL, NULL, "max radius to add counts"},
  {"<theta>", FTCL_ARGV_DOUBLE, NULL, NULL, "position angle"},
  {"<axisRatio>", FTCL_ARGV_DOUBLE, NULL, NULL, "axis Ratio (0.1 < ar < 1)"},
  {"<ngrid>", FTCL_ARGV_INT, NULL, NULL, "number of grid points per pixel"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static char *expDiskAddSlow_name = "expDiskAddSlow";

static int
tclexpDiskAddSlow(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  REGION *region = NULL;	/* region to add disk to */
  double row;			/* row position */
  double col;			/* column position */
  double counts;			/* total counts to add */
  double Rs;			/* exponential scale length */
  double maxRad;
  double theta,axisRatio;

  int ngrid=1;

  char *regionName;
  double countsOut;
  char answer[80];              /* sent back to Tcl */
/* Parse the command */

  expDiskAddSlowTable[1].dst = &regionName;
  expDiskAddSlowTable[2].dst = &row;
  expDiskAddSlowTable[3].dst = &col;
  expDiskAddSlowTable[4].dst = &counts;
  expDiskAddSlowTable[5].dst = &Rs;
  expDiskAddSlowTable[6].dst = &maxRad;
  expDiskAddSlowTable[7].dst = &theta;
  expDiskAddSlowTable[8].dst = &axisRatio;
  expDiskAddSlowTable[9].dst = &ngrid;

  if ((shTclParseArgv( interp, &argc, argv, expDiskAddSlowTable, 0, 
	expDiskAddSlow_name)) != FTCL_ARGV_SUCCESS) {
        return(TCL_ERROR);
  }

  if (shTclAddrGetFromName (interp, regionName, 
     (void **) &region, "REGION") != TCL_OK) {
	Tcl_SetResult
	(interp, "expDiskAddSlow: bad region name", TCL_VOLATILE);
	return TCL_ERROR;
  }

  if(region->type!= TYPE_FL32) {
     Tcl_SetResult(interp, 
	"expDiskAddSlow only works with FL32 regions", TCL_VOLATILE);
        return TCL_ERROR;
    }

  atExpDiskAddSlow(region, row, col, counts, Rs, maxRad,
		  theta, axisRatio, ngrid, &countsOut);

  sprintf(answer, "%f", countsOut);
  Tcl_AppendElement(interp, answer);

  return TCL_OK;

}
/**************************************************************
                      tclWingAdd
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: wingAdd

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atImSim.html#atWingAdd>atWingAdd</A> in atImSim.c

<P>
  Add a power-law wing to an image. 

  </HTML>

</AUTO>*/

static ftclArgvInfo wingAddTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Adds a power law wing to an image.\n"},
  {"<region>", FTCL_ARGV_STRING, NULL, NULL, "region-handle"},
  {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "row position"},
  {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, "column position"},
  {"<Izero>", FTCL_ARGV_DOUBLE, NULL, NULL, "amplitude (total counts)"},
  {"<alpha>", FTCL_ARGV_DOUBLE, NULL, NULL, "power-law -2 > alpha > -6"},
  {"<theta>", FTCL_ARGV_DOUBLE, NULL, NULL, "position angle"},
  {"<axisRatio>", FTCL_ARGV_DOUBLE, NULL, NULL, "axis Ratio (0.1 < ar < 1)"},
  {"<minR>", FTCL_ARGV_DOUBLE, NULL, NULL, "min radius"},
  {"-peak", FTCL_ARGV_CONSTANT, (void*)1, NULL, "interpret Izero as peak not total counts"},
  {"-fast", FTCL_ARGV_CONSTANT, (void*)1, NULL, "faster computation (-FL 32 images only)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *wingAdd_name = "wingAdd";

static int
tclWingAdd(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  REGION *region = (REGION *)0;	/* region to add disk to */
  double row;			/* row position */
  double col;			/* column position */
  double Izero;			/* disk amplitude */
  double alpha,minR;			/* alpha and minimum radius(pix) */
  double theta,axisRatio;
   int peak=0;
  int fast=0;
                                /* OUTPUT */
  double totcounts;		/* total counts added */

  char *regionName;
  char answer[80];              /* sent back to Tcl */
/*   static char *function_name = "wingAdd";   */

/* Parse the command */

  wingAddTable[1].dst = &regionName;
  wingAddTable[2].dst = &row;
  wingAddTable[3].dst = &col;
  wingAddTable[4].dst = &Izero;
  wingAddTable[5].dst = &alpha;
  wingAddTable[6].dst = &theta;
  wingAddTable[7].dst = &axisRatio;
  wingAddTable[8].dst = &minR;
  wingAddTable[9].dst = &peak;
  wingAddTable[10].dst = &fast;

  if ((shTclParseArgv( interp, &argc, argv, wingAddTable, 0, 
	wingAdd_name)) != FTCL_ARGV_SUCCESS) {
        return(TCL_ERROR);
  }

  if (shTclAddrGetFromName (interp, regionName, 
     (void **) &region, "REGION") != TCL_OK) {
	Tcl_SetResult
	(interp, "wingAdd: bad region name", TCL_VOLATILE);
	return TCL_ERROR;
  }

if((fast==1) && (region->type!= TYPE_FL32)) {
     Tcl_SetResult(interp,
        "imSim: -fast only works with FL32 regions", TCL_VOLATILE);
        return TCL_ERROR;
    }


  atWingAdd(region, row, col, Izero, alpha, theta, axisRatio, minR,peak,fast,&totcounts);

  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%f", totcounts);
  Tcl_AppendElement(interp, answer);

  return TCL_OK;

}
/**************************************************************
                      tcldeltaAdd
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: deltaAdd

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atImSim.html#atDeltaAdd>atDeltaAdd</A> in atImSim.c

<P>
  Add a delta function to an image (a PSF star before convolution).  

  </HTML>

</AUTO>*/

static ftclArgvInfo deltaAddTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Adds a delta function to an image.\n"},
  {"<region>", FTCL_ARGV_STRING, NULL, NULL, "region-handle"},
  {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "row position"},
  {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, "column position"},
  {"<Izero>", FTCL_ARGV_DOUBLE, NULL, NULL, "total counts to add"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *deltaAdd_name = "deltaAdd";

static int
tcldeltaAdd(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  REGION *region = (REGION *)0;		/* region to add disk to */
  double row;			/* row position */
  double col;			/* column position */
  double Izero;			/* disk amplitude */
				/* OUTPUT */
  double totcounts;		/* total counts added */

  char *regionName;
  char answer[80];              /* sent back to Tcl */
/*   static char *function_name = "deltaAdd";  */

/* Parse the command */

  deltaAddTable[1].dst = &regionName;
  deltaAddTable[2].dst = &row;
  deltaAddTable[3].dst = &col;
  deltaAddTable[4].dst = &Izero;

  if ((shTclParseArgv( interp, &argc, argv, deltaAddTable, 0, 
	deltaAdd_name)) != FTCL_ARGV_SUCCESS) {
        return(TCL_ERROR);
  }

  if (shTclAddrGetFromName (interp, regionName, 
     (void **) &region, "REGION") != TCL_OK) {
	Tcl_SetResult
	(interp, "deltaAdd: bad region name", TCL_VOLATILE);
	return TCL_ERROR;
  }

  atDeltaAdd(region, row, col, Izero, &totcounts);

  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%f", totcounts);
  Tcl_AppendElement(interp, answer);

  return TCL_OK;

}

/**************************************************************
                      tclRegGaussConv
 **************************************************************/
/*<AUTO EXTRACT>
 TCL VERB: atRegGaussConv

  <HTML>
  C ROUTINE CALLED:
    <A HREF=atGauss.html#atRegConvolveWithGaussian>atRegConvolveWithGaussian</A> in atGauss.c

<P>
Use Michael Richmond/Don Petravick's faster Gaussian convolving.
  </HTML>

</AUTO>
*/
static char *atTclRegGaussConv_use = 
"USAGE: atRegGaussConv inRegion sigma tempRegion -sigfac";
static char *atTclRegGaussConv_help = "Use Michael Richmond/Don Pertavick's \n"
"faster gaussian convolving.";

static int atTclRegGaussConv(ClientData clientData, Tcl_Interp *interp,
		    int argc, char **argv) 
{
  REGION *inRegion=NULL, *tempRegion=NULL, *outRegion=NULL;
  char inRegionName[HANDLE_NAMELEN];
  char tempRegionName[HANDLE_NAMELEN];
  char outRegionName[HANDLE_NAMELEN];
  char sigfacName[30],sigfacVal[30];
  char *formalCmd = "inRegion sigma tempRegion -sigfac";
  float *gvector;
  double sigma, sigfac;
  int haveSigfac=0;
  int nvec,i;
  RET_CODE rstat;

  ftclParseSave("regGaussConv");
  if (ftclFullParseArg(formalCmd, argc, argv) != 0) {
    strcpy(inRegionName,ftclGetStr("inRegion"));
    if (shTclAddrGetFromName
	(interp, inRegionName, (void **) &inRegion, "REGION") != TCL_OK) {
      ftclParseRestore("regGaussConv");
      Tcl_SetResult(interp, atTclRegGaussConv_use, TCL_VOLATILE);
      return(TCL_ERROR);
    }
    strcpy(tempRegionName,ftclGetStr("tempRegion"));
    if (shTclAddrGetFromName
	(interp, tempRegionName, (void **) &tempRegion, "REGION") != TCL_OK) {
      ftclParseRestore("regGaussConv");
      Tcl_SetResult(interp, atTclRegGaussConv_use, TCL_VOLATILE);
      return(TCL_ERROR);
    }
    sigma = ftclGetDouble("sigma");
    if (ftclPresent("sigfac")) {
      strcpy(sigfacName,ftclGetStr("sigfac"));
      haveSigfac = 1;
    }
  } else {
    Tcl_SetResult(interp, atTclRegGaussConv_use, TCL_VOLATILE);
    return(TCL_ERROR);
  }
  outRegion = shRegNewFromReg(inRegion, "convolved", TYPE_FL32, 
			      (COPY_MASK | COPY_HEADER_DEEP), &rstat);

  if (shTclHandleNew(interp, outRegionName, "REGION", outRegion) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to outRegion handle", TCL_VOLATILE);
    return TCL_ERROR;
  }
  atGaussVectorMake(sigma, &nvec, &gvector);
  /* printf("after atGaussVectorMake:  nvec is %d\n",nvec); */
  sigfac = 0;
  for(i=0; i<nvec; i++) {
      sigfac += gvector[i]*gvector[i];
  }
  
  atRegConvolveWithGaussian(inRegion, tempRegion, outRegion, nvec, gvector);
  shFree(gvector);

  ftclParseRestore("regGaussConv");
  if (haveSigfac) {
    sprintf(sigfacVal, "%f", sigfac);
    Tcl_SetVar(interp, sigfacName, sigfacVal, TCL_LEAVE_ERR_MSG);
  }
  Tcl_SetResult(interp, outRegionName, TCL_VOLATILE);
  return(TCL_OK);
}

/**************************************************************
                      tclRegStatNoiseAdd
 **************************************************************/
/*<AUTO EXTRACT>
 TCL VERB: regStatNoiseAdd

  <HTML>
  C ROUTINE CALLED:
    <A HREF=atImSim.html#atRegStatNoiseAdd>atRegStatNoiseAdd</A> in atImSim.c

<P>
  Add a delta function to an image (a PSF star before convolution).  
  </HTML>

</AUTO>
*/
char g_regStatNoiseAdd[] = "regStatNoiseAdd";
ftclArgvInfo g_regStatNoiseAddTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "add statistical noise to a FL_32 region\n" },
  {"<reg>", FTCL_ARGV_STRING, NULL, NULL,
   "the region"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclRegStatNoiseAdd(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_reg = NULL;
  REGION *reg=NULL;

  g_regStatNoiseAddTbl[1].dst = &a_reg;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_regStatNoiseAddTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_regStatNoiseAdd)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  if (shTclAddrGetFromName(interp, a_reg,
                           (void **) &reg, "REGION") != TCL_OK) {
    Tcl_SetResult(interp, "bad reg name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  atRegStatNoiseAdd(reg);
  return TCL_OK;
}

/*********************************************************************/
void atTclImSimDeclare(

Tcl_Interp *interp)
{
  shTclDeclare(
               interp,
               deVaucAdd_name,
               (Tcl_CmdProc *)tclDeVaucAdd,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, deVaucAddTable, 0,
				deVaucAdd_name),
               shTclGetUsage(interp, deVaucAddTable, 0,
				deVaucAdd_name));

  shTclDeclare(
               interp,
               deVaucAddSlow_name,
               (Tcl_CmdProc *)tclDeVaucAddSlow,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, deVaucAddSlowTable, 0,
				deVaucAddSlow_name),
               shTclGetUsage(interp, deVaucAddSlowTable, 0,
				deVaucAddSlow_name));

  shTclDeclare(
               interp,
               expDiskAddSlow_name,
               (Tcl_CmdProc *)tclexpDiskAddSlow,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, expDiskAddSlowTable, 0,
				expDiskAddSlow_name),
               shTclGetUsage(interp, expDiskAddSlowTable, 0,
				expDiskAddSlow_name));

  shTclDeclare(
               interp,
               wingAdd_name,
               (Tcl_CmdProc *)tclWingAdd,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, wingAddTable, 0,
				wingAdd_name),
               shTclGetUsage(interp, wingAddTable, 0,
				wingAdd_name));
  shTclDeclare(
               interp,
               deltaAdd_name,
               (Tcl_CmdProc *)tcldeltaAdd,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, deltaAddTable, 0,
				deltaAdd_name),
               shTclGetUsage(interp, deltaAddTable, 0,
				deltaAdd_name));
  shTclDeclare(
               interp,
               expDiskAdd_name,
               (Tcl_CmdProc *)tclExpDiskAdd,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, expDiskAddTable, 0,
				expDiskAdd_name),
               shTclGetUsage(interp, expDiskAddTable, 0,
				expDiskAdd_name));

  shTclDeclare(interp, "atRegGaussConv", (Tcl_CmdProc *)atTclRegGaussConv,
	       (ClientData) 0, (Tcl_CmdDeleteProc *)NULL, module,
	       atTclRegGaussConv_use, atTclRegGaussConv_help);

  shTclDeclare(interp, g_regStatNoiseAdd,
               (Tcl_CmdProc *) tclRegStatNoiseAdd,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0,module, 
               shTclGetArgInfo(interp, g_regStatNoiseAddTbl,
                               0, g_regStatNoiseAdd),
               shTclGetUsage(interp, g_regStatNoiseAddTbl,
                             0, g_regStatNoiseAdd));
}
