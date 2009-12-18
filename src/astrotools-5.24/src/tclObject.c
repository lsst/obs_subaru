/* <AUTO>
   FILE: tclObject
   ABSTRACT:
<HTML>
   Routines to calculate quantities about astronomical objects
</HTML>
   </AUTO>
*/
/***************************************************************
 *
 * TCL verbs to calculate quantities about astronomical objects
 *
 *              atObject
 *
 **************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "ftcl.h"
#include "dervish.h"
#include "atObject.h"

static char *module = "astrotools";   /* name of this set of code */

/**************************************************************
                      tclCentroidFind
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: centroidFind

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atObject.html#atCentroidFind>atCentroidFind</A> in atObject.c

<P>
The underlying routine uses U16 only.  If the region is not U16, a scaled
U16 copy will be made.  If no min and max values are supplied in the
call, the max value will be set to the highest pixel value and the min
will be set to the lowest pixel value.
<P>
Calculate the center of an astronomical object.  If sigma<=0, it will not
smooth with a Gaussian before fitting for the centroid.  If gain<=0, the
errors will not be calculated.
<P>
The amplitude and sigmas in the row and column directions are also
calculated, but they are only approximations.
<P>
Subregions are allowed, but if they are used, both the coordinates given 
and the coordinates returned will be in the coordinates of the original 
image.  To obtain these, add reg->row0 and reg->col0 to the row and column 
coordinates in the subregion.
<P>
If it does not have to do Gaussian convolution, then centroids will be
calculated up to one pixel from the edge of the region.  If convolution
is necessary, the distance from the edge depends on the convolution sigma:
sigma=(<=1,<=1.2,<=1.5,<=1.8,<=2.1,>2.1) requires distance=(5,6,7,8,9,10)pixels
from the edge.
<P>
The algorithm will not calculate accurate widths for sigmas much under 1.0,
when the pixel size becomes important.
  </HTML>

</AUTO>*/

static ftclArgvInfo atCentroidFindTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Find the center, error in the center, peak values, and width estimates\n"
  "Experimental and currently only works for U16"},
  {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "region containing object"},
  {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "row position will be truncated to int"},
  {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, "col position will be truncated to int"}, 
  {"<sky>", FTCL_ARGV_DOUBLE, NULL, NULL, "sky value"},
  {"<widthsigma>", FTCL_ARGV_DOUBLE, NULL, NULL, "sigma of Gaussian to smooth image with (ignore if <=0)"},
  {"<skyvariance>", FTCL_ARGV_DOUBLE, NULL, NULL, "variance of the background"},
  {"<gain>", FTCL_ARGV_DOUBLE, NULL, NULL, "gain of amplifier"},
  {"-max", FTCL_ARGV_DOUBLE, NULL, NULL, "max value for scaling"},
  {"-min", FTCL_ARGV_DOUBLE, NULL, NULL, "min value for scaling"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *centroidFind_name = "centroidFind";

static int
tclCentroidFind(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  REGION *reg;			/* Input region */
  double drow=0, dcol=0;	/* initial estimate of center */
  int row=0, col=0;		/* center in ints */
  double sky=0;			/* level of background */
  double widthsigma=0;		/* sigma of Gaussian to smooth image
				   with (ignore if <= 0) */
  double dark_variance=0;	/* variance of the background */
  double gain=1;		/* gain of amplifiers */
                                /* OUTPUT */
  float xc=0, yc=0;		/* centre of image */
  float dxc=0,  dyc=0;		/* errors in xc, yc */
  double maxval=0, minval=0;	/* max and min values for scaling */
  float *dxcptr=NULL,  *dycptr=NULL;
				/* errors in xc, yc */
  float peak;			/* peak value */
  float counts;		/* counts calculated from a Gaussian */
  float siga, sigb;		/* estimates of the major and minor  widths */
  float pa;

  char tclRegionName[MAXTCLREGNAMELEN], *tclRegionNamePtr = NULL;
  /*RET_CODE returnValue;*/
  int flag=0; 	                /* 0 = ok,
				  -1 = Centre isn't locally highest pixel 
					(this is set after hunting for a 
					maximum and failing after FINDERR 
					tries
				  -2 = Object is too close to the edge
				  -3 = Object has vanishing second derivative
				 */

  char answer[300];              /* sent back to Tcl */
  char errors[80];


/* Parse the command */

  atCentroidFindTable[1].dst = &tclRegionNamePtr;
  atCentroidFindTable[2].dst = &drow;
  atCentroidFindTable[3].dst = &dcol;
  atCentroidFindTable[4].dst = &sky;
  atCentroidFindTable[5].dst = &widthsigma;
  atCentroidFindTable[6].dst = &dark_variance;
  atCentroidFindTable[7].dst = &gain;
  atCentroidFindTable[8].dst = &maxval;
  atCentroidFindTable[9].dst = &minval;

  if ((shTclParseArgv(
        interp, &argc, argv, atCentroidFindTable, FTCL_ARGV_NO_LEFTOVERS, centroidFind_name))
                                                != FTCL_ARGV_SUCCESS) {
        return(TCL_ERROR);
  }

  if (ftcl_ArgIsPresent(argc,argv,"-max",atCentroidFindTable)) {
    if (!ftcl_ArgIsPresent(argc,argv,"-min",atCentroidFindTable)) {
      sprintf(errors,"\nYou must supply a minimum if you supply a maximum");
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      return(TCL_ERROR);
    } else {
      if (maxval<=minval) {
        sprintf(errors,"\nThe maximum value must be greater than the minimum");
        Tcl_SetResult(interp, errors, TCL_VOLATILE);
        return(TCL_ERROR);
      }
    }
  } else {
    if (ftcl_ArgIsPresent(argc,argv,"-min",atCentroidFindTable)) {
      sprintf(errors,"\nYou must supply a maximum if you supply a minimum");
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      return(TCL_ERROR);
    }
  }

  row = (int) drow;
  col = (int) dcol;

  /* Get region handle */
  strncpy(tclRegionName,tclRegionNamePtr,MAXTCLREGNAMELEN);

  if (shTclRegAddrGetFromName(interp, tclRegionName, &reg)
      != TCL_OK) {
    sprintf(errors,"\nCannot find region named: %s",
            tclRegionName);
    Tcl_SetResult(interp, errors, TCL_VOLATILE);
    return(TCL_ERROR);
  }

  if (gain>0) {
    dxcptr=&dxc;
    dycptr=&dyc;
  }

/************************************
 * 
 *   Do the actual call 
 * 
 ************************************
 */

/* send back the answer */
  flag  = atCentroidFind(reg, row, col, sky, widthsigma, dark_variance, gain, minval, maxval, &xc, &yc, dxcptr, dycptr, &peak, &counts, &siga, &sigb, &pa);

  sprintf(answer, "{flag %d} {row %f} {col %f} {rowerr %f} {colerr %f} {peak %f} {counts %f} {asig %f} {bsig %f} {pa %f}", flag, yc, xc, dyc, dxc, peak, counts, siga, sigb, pa);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);

  return TCL_OK;
}


/**************************************************************
                      tcl2ndMoments
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: 2ndMoments

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atObject.html#at2ndMoments>at2ndMoments</A> in atObject.c

<P>
  This algorithm uses second moments to calculate the position angle and
widths in the directions of the major and minor axes.
  </HTML>

</AUTO>*/

static ftclArgvInfo at2ndMomentsTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Find the position angle and widths in the major and minor directions"},
  {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "region containing object"},
  {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "centroided row"},
  {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, "centroided col"}, 
  {"<sky>", FTCL_ARGV_DOUBLE, NULL, NULL, "sky value"},
  {"<nrows>", FTCL_ARGV_DOUBLE, NULL, NULL, "number of rows to include in moments"},
  {"<ncols>", FTCL_ARGV_DOUBLE, NULL, NULL, "number of cols to include in moments"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *at2ndMoments_name = "2ndMoments";

static int
tcl2ndMoments(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  REGION *reg;			/* Input region */
  double row=0, col=0;		/* accurate measurement of center */
  double sky=0;			/* level of background */
  double nrows;			/* number of rows to use in moments */
  double ncols;			/* number of cols to use in moments */
                                /* OUTPUT */
  double pa;			/* position angle of object */
  double asigma, bsigma;	/* sigma on major and minor axes */

  int rstart, cstart, rend, cend;
  char tclRegionName[MAXTCLREGNAMELEN], *tclRegionNamePtr = NULL;
  /*RET_CODE returnValue;*/
  int flag=0; 	                /* 0 = ok,
				  -1 = 
				  -2 =
				 */

  char answer[300];              /* sent back to Tcl */
  char errors[80];

/* Parse the command */

  at2ndMomentsTable[1].dst = &tclRegionNamePtr;
  at2ndMomentsTable[2].dst = &row;
  at2ndMomentsTable[3].dst = &col;
  at2ndMomentsTable[4].dst = &sky;
  at2ndMomentsTable[5].dst = &nrows;
  at2ndMomentsTable[6].dst = &ncols;

  if ((shTclParseArgv(
        interp, &argc, argv, at2ndMomentsTable, FTCL_ARGV_NO_LEFTOVERS, at2ndMoments_name))
                                                != FTCL_ARGV_SUCCESS) {
        return(TCL_ERROR);
  }

  /* Get region handle */
  strncpy(tclRegionName,tclRegionNamePtr,MAXTCLREGNAMELEN);

  if (shTclRegAddrGetFromName(interp, tclRegionName, &reg) != TCL_OK) {
    sprintf(errors,"\nCannot find region named: %s",
            tclRegionName);
    Tcl_SetResult(interp, errors, TCL_VOLATILE);
    return(TCL_ERROR);
  }

  rstart = floor(row) - floor(nrows/2);
  cstart = floor(col) - floor(ncols/2);
  rend = floor(row) + floor(nrows/2);
  cend = floor(col) + floor(ncols/2);

  if (rstart<0) rstart=0;
  if (cstart<0) cstart=0;
  if (rend>reg->nrow) rend = reg->nrow;
  if (cend>reg->ncol) cend = reg->ncol;

/************************************
 * 
 *   Do the actual call 
 * 
 ************************************
 */

/* send back the answer */
  flag  = at2ndMoments(reg, row, col, sky, rstart, cstart, rend, cend, &pa, &asigma, &bsigma);

  sprintf(answer, "{flag %d} {pa %g} {asigma %g} {bsigma %g}", flag, pa, asigma, bsigma);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);

  return TCL_OK;
}

/**************************************************************
                      tclGaussianWidthFind
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: gaussianWidthFind

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atObject.html#atGaussianWidthFind>atGaussianWidthFind</A> in atObject.c

<P>
The underlying routine uses U16 only.  If the region is not U16, a scaled
U16 copy will be made.  If no min and max values are supplied in the
call, the max value will be set to the highest pixel value and the min
will be set to the lowest pixel value.
<P>
Subregions are allowed, but if they are used, both the coordinates given 
and the coordinates returned will be in the coordinates of the original 
image.  To obtain these, add reg->row0 and reg->col0 to the row and column 
coordinates in the subregion.
<P>
Returned are: errorFlag (listed in atObject.c), row,col, peak value, counts,
sigma along semi-major axis, sigma along semi-minor axis, and position angle
(0 points toward low row numbers, increasing through increasing col numbers). 
FWHM = 2.354*sigma.
  </HTML>

</AUTO>*/

static ftclArgvInfo atGaussianWidthFindTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Find the best fit Gaussian sigma\n"},
  {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "region containing object"},
  {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "row position will be truncated to int"},
  {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, "col position will be truncated to int"}, 
  {"<sky>", FTCL_ARGV_DOUBLE, NULL, NULL, "sky value"},
  {"-max", FTCL_ARGV_DOUBLE, NULL, NULL, "max value for scaling"},
  {"-min", FTCL_ARGV_DOUBLE, NULL, NULL, "min value for scaling"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *gaussianWidthFind_name = "gaussianWidthFind";

static int
tclGaussianWidthFind(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  REGION *reg;			/* Input region */
  double drow=0, dcol=0;	/* initial estimate of center */
  int row=0, col=0;		/* center in ints */
  double sky=0;			/* level of background */
  double maxval=0, minval=0;	/* max and min values for scaling */
  float siga;			/* Gaussian width */
  float sigb;			/*  */
  float pa;			/*  */
  float peak;			/*  */
  float counts;		/*  */
  float xf;			/* x position */
  float yf;			/* y position */

  char tclRegionName[MAXTCLREGNAMELEN], *tclRegionNamePtr = NULL;
  /*RET_CODE returnValue;*/
  int flag=0; 	                /* 0 = ok,
				 */

  char answer[300];              /* sent back to Tcl */
  char errors[80];


/* Parse the command */

  atGaussianWidthFindTable[1].dst = &tclRegionNamePtr;
  atGaussianWidthFindTable[2].dst = &drow;
  atGaussianWidthFindTable[3].dst = &dcol;
  atGaussianWidthFindTable[4].dst = &sky;
  atGaussianWidthFindTable[5].dst = &maxval;
  atGaussianWidthFindTable[6].dst = &minval;

  if ((shTclParseArgv(
        interp, &argc, argv, atGaussianWidthFindTable, FTCL_ARGV_NO_LEFTOVERS, gaussianWidthFind_name))
                                                != FTCL_ARGV_SUCCESS) {
        return(TCL_ERROR);
  }

  if (ftcl_ArgIsPresent(argc,argv,"-max",atCentroidFindTable)) {
    if (!ftcl_ArgIsPresent(argc,argv,"-min",atCentroidFindTable)) {
      sprintf(errors,"\nYou must supply a minimum if you supply a maximum");
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      return(TCL_ERROR);
    } else {
      if (maxval<=minval) {
        sprintf(errors,"\nThe maximum value must be greater than the minimum");
        Tcl_SetResult(interp, errors, TCL_VOLATILE);
        return(TCL_ERROR);
      }
    }
  } else {
    if (ftcl_ArgIsPresent(argc,argv,"-min",atCentroidFindTable)) {
      sprintf(errors,"\nYou must supply a maximum if you supply a minimum");
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      return(TCL_ERROR);
    }
  }

  row = (int) drow;
  col = (int) dcol;

  /* Get region handle */
  strncpy(tclRegionName,tclRegionNamePtr,MAXTCLREGNAMELEN);

  if (shTclRegAddrGetFromName(interp, tclRegionName, &reg)
      != TCL_OK) {
    sprintf(errors,"\nCannot find region named: %s",
            tclRegionName);
    Tcl_SetResult(interp, errors, TCL_VOLATILE);
    return(TCL_ERROR);
  }

/************************************
 * 
 *   Do the actual call 
 * 
 ************************************
 */

/* send back the answer */
  flag  = atGaussianWidthFind(reg, row, col, sky, minval, maxval, &siga, &sigb, &pa, &peak, &counts, &xf, &yf);

  sprintf(answer, "{flag %d} {row %f} {col %f} {peak %f} {counts %f} {asig %f} {bsig %f} {pa %f}", flag, yf, xf, peak, counts, siga, sigb, pa);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);

  return TCL_OK;
}

/**************************************************************
                      tclDACentroidFind
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: daCentroidFind

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atObject.html#atDACentroidFind>atDACentroidFind</A> in atObject.c

<P>
The underlying routine uses U16 only.  If the region is not U16, a scaled
U16 copy will be made.  If no min and max values are supplied in the
call, the max value will be set to the highest pixel value and the min
will be set to the lowest pixel value.  To get the same values as the
gang files, use the U16 regions.
<P>
Calculate the center of an astronomical object.  If sigma<=0, it will not
smooth with a Gaussian before fitting for the centroid.  If gain<=0, the
errors will not be calculated.
<P>
The amplitude and sigmas in the row and column directions are also
calculated.
<P>
Subregions are allowed, but if they are used, both the coordinates given 
and the coordinates returned will be in the coordinates of the original 
image.  To obtain these, add reg->row0 and reg->col0 to the row and column 
coordinates in the subregion.
<P>
If it does not have to do Gaussian convolution, then centroids will be
calculated up to one pixel from the edge of the region.  If convolution
is necessary, the distance from the edge depends on the convolution sigma:
sigma=(<=1,<=1.2,<=1.5,<=1.8,<=2.1,>2.1) requires distance=(5,6,7,8,9,10)pixels
from the edge.
<P>
  </HTML>

</AUTO>*/

static ftclArgvInfo atDACentroidFindTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Find the center, error in the center, peak values, and width estimates\n"
  "Experimental and currently only works for U16"},
  {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "region containing object"},
  {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "row position will be truncated to int"},
  {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, "col position will be truncated to int"}, 
  {"<sky>", FTCL_ARGV_DOUBLE, NULL, NULL, "sky value"},
  {"<widthsigma>", FTCL_ARGV_DOUBLE, NULL, NULL, "sigma of Gaussian to smooth image with (ignore if <=0)"},
  {"<skyvariance>", FTCL_ARGV_DOUBLE, NULL, NULL, "variance of the background"},
  {"<gain>", FTCL_ARGV_DOUBLE, NULL, NULL, "gain of amplifier"},
  {"-max", FTCL_ARGV_DOUBLE, NULL, NULL, "max value for scaling"},
  {"-min", FTCL_ARGV_DOUBLE, NULL, NULL, "min value for scaling"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *daCentroidFind_name = "daCentroidFind";

static int
tclDACentroidFind(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  REGION *reg;			/* Input region */
  double drow=0, dcol=0;	/* initial estimate of center */
  int row=0, col=0;		/* center in ints */
  double sky=0;			/* level of background */
  double widthsigma=0;		/* sigma of Gaussian to smooth image
				   with (ignore if <= 0) */
  double dark_variance=0;	/* variance of the background */
  double gain=1;		/* gain of amplifiers */
                                /* OUTPUT */
  float rowc, colc;		/* centre of image */
  float drowc=0,  dcolc=0;	/* errors in xc, yc */
  double maxval=0, minval=0;	/* max and min values for scaling */
  float *drowcptr=NULL,  *dcolcptr=NULL;
				/* errors in xc, yc */
  float peak;			/* peak value */
  float counts;		/* counts calculated from a Gaussian */
  float siga, sigb;		/* estimates of the major and minor  widths */
  float pa;

  char tclRegionName[MAXTCLREGNAMELEN], *tclRegionNamePtr = NULL;
  /*RET_CODE returnValue;*/
  int flag=0;

  char answer[300];              /* sent back to Tcl */
  char errors[80];


/* Parse the command */

  atDACentroidFindTable[1].dst = &tclRegionNamePtr;
  atDACentroidFindTable[2].dst = &drow;
  atDACentroidFindTable[3].dst = &dcol;
  atDACentroidFindTable[4].dst = &sky;
  atDACentroidFindTable[5].dst = &widthsigma;
  atDACentroidFindTable[6].dst = &dark_variance;
  atDACentroidFindTable[7].dst = &gain;
  atDACentroidFindTable[8].dst = &maxval;
  atDACentroidFindTable[9].dst = &minval;

  if ((shTclParseArgv(
        interp, &argc, argv, atDACentroidFindTable, FTCL_ARGV_NO_LEFTOVERS, daCentroidFind_name))
                                                != FTCL_ARGV_SUCCESS) {
        return(TCL_ERROR);
  }

  if (ftcl_ArgIsPresent(argc,argv,"-max",atDACentroidFindTable)) {
    if (!ftcl_ArgIsPresent(argc,argv,"-min",atDACentroidFindTable)) {
      sprintf(errors,"\nYou must supply a minimum if you supply a maximum");
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      return(TCL_ERROR);
    } else {
      if (maxval<=minval) {
        sprintf(errors,"\nThe maximum value must be greater than the minimum");
        Tcl_SetResult(interp, errors, TCL_VOLATILE);
        return(TCL_ERROR);
      }
    }
  } else {
    if (ftcl_ArgIsPresent(argc,argv,"-min",atDACentroidFindTable)) {
      sprintf(errors,"\nYou must supply a maximum if you supply a minimum");
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      return(TCL_ERROR);
    }
  }

  row = (int) drow;
  col = (int) dcol;

  /* Get region handle */
  strncpy(tclRegionName,tclRegionNamePtr,MAXTCLREGNAMELEN);

  if (shTclRegAddrGetFromName(interp, tclRegionName, &reg)
      != TCL_OK) {
    sprintf(errors,"\nCannot find region named: %s",
            tclRegionName);
    Tcl_SetResult(interp, errors, TCL_VOLATILE);
    return(TCL_ERROR);
  }

  if (gain>0) {
    drowcptr=&drowc;
    dcolcptr=&dcolc;
  }

/************************************
 * 
 *   Do the actual call 
 * 
 ************************************
 */

/* send back the answer */
  flag  = atDACentroidFind(reg, row, col, sky, widthsigma, dark_variance, gain, minval, maxval, &rowc, &colc, drowcptr, dcolcptr, &peak, &counts, &siga, &sigb, &pa);

  sprintf(answer, "{flag %d} {row %f} {col %f} {rowerr %f} {colerr %f} {peak %f} {counts %f} {asig %f} {bsig %f} {pa %f}", flag, rowc, colc, drowc, dcolc, peak, counts, siga, sigb, pa);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);

  return TCL_OK;
}

/*********************************************************************/
void atTclObjectDeclare(

Tcl_Interp *interp)
{
  shTclDeclare( interp,
               centroidFind_name,
               (Tcl_CmdProc *)tclCentroidFind,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, atCentroidFindTable, 
				FTCL_ARGV_NO_LEFTOVERS, centroidFind_name),
               shTclGetUsage(interp,   atCentroidFindTable, 
				FTCL_ARGV_NO_LEFTOVERS, centroidFind_name));
  shTclDeclare( interp,
               gaussianWidthFind_name,
               (Tcl_CmdProc *)tclGaussianWidthFind,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, atGaussianWidthFindTable, 
				FTCL_ARGV_NO_LEFTOVERS, gaussianWidthFind_name),
               shTclGetUsage(interp,   atGaussianWidthFindTable, 
				FTCL_ARGV_NO_LEFTOVERS, gaussianWidthFind_name));
  shTclDeclare( interp,
               at2ndMoments_name,
               (Tcl_CmdProc *)tcl2ndMoments,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, at2ndMomentsTable, 
				FTCL_ARGV_NO_LEFTOVERS, at2ndMoments_name),
               shTclGetUsage(interp,   at2ndMomentsTable, 
				FTCL_ARGV_NO_LEFTOVERS, at2ndMoments_name));
  shTclDeclare( interp,
               daCentroidFind_name,
               (Tcl_CmdProc *)tclDACentroidFind,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, atDACentroidFindTable, 
				FTCL_ARGV_NO_LEFTOVERS, daCentroidFind_name),
               shTclGetUsage(interp,   atDACentroidFindTable, 
				FTCL_ARGV_NO_LEFTOVERS, daCentroidFind_name));
}
