/* <AUTO>
   FILE: tclRegSci
   ABSTRACT:
<HTML>
<P> RegSci routines perform scientific operations on regions.  The
routines are intended to be multi-purpose and to act on regions of all
types that apply.  For example the regSkyFind routine finds the fit peak
of a histogram of pixel values in a region.  There are many possible
algorithms to do this, all of which will give misleading results in
some cases.  We have chosen one which can be used to give a reasonable
result in most cases for development or quality assurance purposes.  It
is not intended for detailed analysis.

<P>Arithmetic operations are performed "as if" all operands are
converted to a new type which has an exact representation for all the
values of each type of pixel in the operation.  The pixels are retrieved
and set in an image using the shRegPixGetAsDbl and shRegPixSetWithDbl
routines.

</HTML>
   </AUTO>
*/

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include "tcl.h"
#include "ftcl.h"
#include "dervish.h"
#include "atRegSci.h"
/*============================================================================
**============================================================================
*/

/*<AUTO EXTRACT>
  TCL VERB: regGaussiansAdd

  <HTML>
  C ROUTINE CALLED:
                <A HREF=atRegSci.html#atRegGaussiansAdd>atRegGaussiansAdd</A> in atRegSci.c

<P>
	Add a Gaussian to a region

  </HTML>

</AUTO>*/

static ftclArgvInfo regGaussiansAddArgTable[] = {
         { NULL,                        FTCL_ARGV_HELP,   NULL, NULL,
	     "Add a Gaussian to a region\n"},
         {"<region>",			FTCL_ARGV_STRING, NULL, NULL, "Initial region"},
         {"<listofrowpositions>", 	FTCL_ARGV_STRING, NULL, NULL, "List of row positions"},
         {"<listofcolumnpositions>",    FTCL_ARGV_STRING, NULL, NULL, "List of column positions"},
         {"<listofamplitudes>",    	FTCL_ARGV_STRING, NULL, NULL, "List of amplitudes to apply"},
         {"-rsig",    			FTCL_ARGV_DOUBLE, NULL, NULL, "Row sigma of the Gaussians;\n"
                                        "Default: 1.0"},
         {"-csig ",    			FTCL_ARGV_DOUBLE, NULL, NULL, "Column sigma of the Gaussians;\n"
                                        "Default: 1.0"},
         {"-sig",    			FTCL_ARGV_DOUBLE, NULL, NULL, "Sigma; Default: -1.0"},
         {"-radius",    		FTCL_ARGV_DOUBLE, NULL, NULL, "Radius; Default: -1.0"},
         {NULL,    			FTCL_ARGV_END, NULL, NULL, NULL}
       };

char g_regGaussiansAdd[] = "regGaussiansAdd";

static int tclRegGaussiansAdd(ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{

  int rstatus;
  REGION *reg;
  float *rpos, *cpos, *amplitude;
  double rsig = 1.0, csig = 1.0, radius = -1.0, sig = -1.;
  int nrpos, ncpos, namp, i;
  char tclRegionName[MAXTCLREGNAMELEN], *tclRegionNamePtr = NULL;
  char errors[120];
  char* listofrowpositionsPtr = NULL;
  char* listofcolumnpositionsPtr = NULL;
  char* listofamplitudesPtr      = NULL;
  RET_CODE retcode;
  
  regGaussiansAddArgTable[1].dst = &tclRegionNamePtr;
  regGaussiansAddArgTable[2].dst = &listofrowpositionsPtr;
  regGaussiansAddArgTable[3].dst = &listofcolumnpositionsPtr;
  regGaussiansAddArgTable[4].dst = &listofamplitudesPtr;
  regGaussiansAddArgTable[5].dst = &rsig;
  regGaussiansAddArgTable[6].dst = &csig;
  regGaussiansAddArgTable[7].dst = &sig;
  regGaussiansAddArgTable[8].dst = &radius;

 
  if ((rstatus = shTclParseArgv(interp, &argc, argv,regGaussiansAddArgTable,
				FTCL_ARGV_NO_LEFTOVERS, g_regGaussiansAdd))
      !=FTCL_ARGV_SUCCESS)
   { 
      return(rstatus);

   } else {

   /** get region make sure it exists and has pixels **/
    strncpy(tclRegionName,tclRegionNamePtr,MAXTCLREGNAMELEN);
    
    if (shTclRegAddrGetFromName(interp, tclRegionName, &reg) != TCL_OK) {
      sprintf(errors,"\nCannot find region named: %s",
	      tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }
    if ( (reg->nrow * reg->ncol) == 0) {
      sprintf(errors,"No pixels in region named: %s\n", tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }
    
    if (sig != -1.) {        rsig = sig;
			     csig = sig; }
    if (radius == -1.) radius = 3 * sqrt(rsig*rsig + csig*csig);

    if( rsig <= 0 || csig <= 0) {
      sprintf(errors,"\nThe sigma of the Gaussians must be greater then 0.");
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

    if( radius <= 0 ) {
      sprintf(errors,"\nThe radius must be greater then 0.\n");
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

  }

  /* Put the lists into some form that we can access each element */
  rpos = ftclList2Floats 
	(interp, listofrowpositionsPtr, &nrpos, 0);
  cpos = ftclList2Floats 
	(interp, listofcolumnpositionsPtr, &ncpos, 0);
  amplitude = ftclList2Floats 
	(interp, listofamplitudesPtr, &namp, 0);

  if ( (rpos == NULL) || (cpos == NULL ) || (amplitude == NULL) ) {
    free (rpos); free (cpos); free (amplitude);
    Tcl_SetResult(interp, "Lists must be floating point", TCL_STATIC);
    return(TCL_ERROR);
  } 

  if ((nrpos != ncpos) || (nrpos != namp)) {
    free (rpos); free (cpos); free (amplitude);
    Tcl_SetResult(interp, "Lists must be of equal length", TCL_STATIC);
    return(TCL_ERROR);
  }

  for (i=0; i<nrpos; i++) {
    retcode = atRegGaussianAdd(reg, rpos[i], cpos[i], amplitude[i], rsig, csig, radius);
    if ( retcode != SH_SUCCESS ) {
      shTclInterpAppendWithErrStack(interp);
      free (rpos); free (cpos); free (amplitude);
      return(TCL_ERROR);
    }
  }

  free (rpos); free (cpos); free (amplitude);
  return (TCL_OK);

}


/*<AUTO EXTRACT>
  TCL VERB: regMap

  <HTML>
  C ROUTINE CALLED:
                <A HREF=atRegSci.html#atRegMap>atRegMap</A> in atRegSci.c

<P>
Shift, rotate, and magnify (in that order) the SOURCE region to produce the
TARGET region.  The rotation is input in degrees.  If there is no pixel in
the SOURCE region for the corresponding pixel in the TARGET region, a zero
will be substituted.  There is no requirement that the SOURCE and TARGET
regions must be the same size.

  </HTML>

</AUTO>*/

static ftclArgvInfo  regMapArgTable[] = {
      { NULL,           FTCL_ARGV_HELP,     NULL, NULL,
         "Magnify, rotate, and shift SOURCE region to produce TARGET region.\n"},
      {"<regsource>",   FTCL_ARGV_STRING,   NULL, NULL, "Initial region"},
      {"-regtarget",    FTCL_ARGV_STRING,   NULL, NULL, "Resulting region; Default: create new region"},
      {"-rowoff",     	FTCL_ARGV_DOUBLE,   NULL, NULL, "Offset in pixels; Default: 0.0"},
      {"-coloff",     	FTCL_ARGV_DOUBLE,   NULL, NULL, "Offset in pixels; Default: 0.0"},
      {"-theta",     	FTCL_ARGV_DOUBLE,   NULL, NULL, "Rotation in degrees; Default: 0.0"},
      {"-rcenter",     	FTCL_ARGV_DOUBLE,   NULL, NULL, "Center of rotation; Default: -1.0"},
      {"-ccenter",     	FTCL_ARGV_DOUBLE,   NULL, NULL, "Center of rotation; Default: -1.0"},
      {"-mag",     	FTCL_ARGV_DOUBLE,   NULL, NULL, "Magnification (r&c); Default: -1.0"},
      {"-rowmag",     	FTCL_ARGV_DOUBLE,   NULL, NULL, "Magnification (r only); Default: 1.0"},
      {"-colmag",     	FTCL_ARGV_DOUBLE,   NULL, NULL, "Magnification (c only); Default: 1.0"},
      {NULL, 		FTCL_ARGV_END, NULL, NULL, NULL}
     };

char g_regMap[] = "regMap";

static int tclRegMap(ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{

  int rstatus;
  REGION *regsource, *regtarget;
  double rowoff =0., coloff =0., theta = 0., rcenter = -1., ccenter = -1., rowmag = 1., colmag = 1., mag = -1.;
  int r,c;
  char tclRegionName[MAXTCLREGNAMELEN], *tclRegionNamePtr = NULL;
  char errors[100];
  char*   regtargetPtr = NULL;
  RET_CODE retcode;

  regMapArgTable[1].dst = &tclRegionNamePtr;
  regMapArgTable[2].dst = &regtargetPtr;
  regMapArgTable[3].dst = &rowoff;
  regMapArgTable[4].dst = &coloff;
  regMapArgTable[5].dst = &theta;
  regMapArgTable[6].dst = &rcenter;
  regMapArgTable[7].dst = &ccenter;
  regMapArgTable[8].dst = &mag;
  regMapArgTable[9].dst = &rowmag;
  regMapArgTable[10].dst = &colmag;

 
  if((rstatus = shTclParseArgv(interp, &argc, argv, regMapArgTable,
			       FTCL_ARGV_NO_LEFTOVERS, g_regMap))
      != FTCL_ARGV_SUCCESS) {

         return(rstatus);

    } else {

    strncpy(tclRegionName,tclRegionNamePtr,MAXTCLREGNAMELEN);

    /** get source region, make sure it exists and has pixels **/
    if (shTclRegAddrGetFromName(interp, tclRegionName, &regsource) 
	!= TCL_OK) {
      sprintf(errors,"\nCannot find region named: %s",tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

    if ( (regsource->nrow * regsource->ncol) == 0) {
      sprintf(errors,"No pixels in region named: %s\n", tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }


    if (regtargetPtr!=NULL) {

      /** get user suplied target region, make sure it exists and has pixels **/
      strncpy(tclRegionName, regtargetPtr,MAXTCLREGNAMELEN);
      if (shTclRegAddrGetFromName(interp, tclRegionName, &regtarget) 
	  != TCL_OK) {
	sprintf(errors,"\nCannot find region named: %s",tclRegionName);
	Tcl_SetResult(interp ,errors,TCL_VOLATILE);
	return(TCL_ERROR);
      }
      if ( (regtarget->nrow * regtarget->ncol) == 0) {
        sprintf(errors,"No pixels in region named: %s\n", tclRegionName);
        Tcl_SetResult(interp ,errors,TCL_VOLATILE);
        return(TCL_ERROR);
      }
    
    } else {
      /* Get a new region of the same size and type as regsource */
      if (shTclRegNameGet(interp, tclRegionName) != TCL_OK) {
	sprintf(errors,"\nCannot find region named: %s",tclRegionName);
	Tcl_SetResult(interp ,errors,TCL_VOLATILE);
	return(TCL_ERROR);
      }
      regtarget = shRegNew(tclRegionName, regsource->nrow, regsource->ncol,
			   regsource->type);
      if (regtarget == NULL) {
	shTclRegNameDel(interp, tclRegionName);
	return(TCL_ERROR);
      }
    }

    if (rcenter == -1.)  rcenter = ((double)regsource->nrow)/2.;
    if (ccenter  == -1.)  ccenter = ((double)regsource->ncol)/2.;
    
    if (mag != -1) { rowmag = mag; colmag = rowmag; }
    
  }

  retcode = atRegMap(regsource, regtarget, rowoff, coloff, theta, 
                     rcenter, ccenter, rowmag, colmag);
  if ( retcode != SH_SUCCESS) {
    shTclInterpAppendWithErrStack(interp);
    return(TCL_ERROR);
  }

  if (regtargetPtr==NULL) {
  /* Copy all of the pixels from target to source and free the target */ 
    for (r=0; r<regsource->nrow; r++) {
      for (c=0; c<regsource->ncol; c++) 
	shRegPixSetWithDbl(regsource,r,c,shRegPixGetAsDbl(regtarget,r,c));
    }
    shRegDel(regtarget);
    shTclRegNameDel(interp, tclRegionName);
  }

  return TCL_OK;
}


/*<AUTO EXTRACT>
  TCL VERB: regMedianFindAsDbl

  <HTML>
  C ROUTINE CALLED:
                <A HREF=atRegSci.html#atRegMedianFindAsDbl>atRegMedianFindAsDbl</A> in atRegSci.c

<P>
  Find the median pixel in a region.

  </HTML>

</AUTO>*/

static ftclArgvInfo regMedianFindAsDblArgTable[] = {
       { NULL,          FTCL_ARGV_HELP,      NULL, NULL,
  "Find the median pixel in a region.\n"},
       {"<region>",     FTCL_ARGV_STRING,    NULL, NULL, "Region containing pixels to median"},
       {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
    };

char g_regMedianFindAsDbl[] = "regMedianFindAsDbl";

static int tclRegMedianFindAsDbl(ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{

  int rstatus;
  REGION *reg;
  double median;
  char m[20];
  char tclRegionName[MAXTCLREGNAMELEN], *tclRegionNamePtr=NULL;
  char errors[80];
  RET_CODE retcode;

  regMedianFindAsDblArgTable[1].dst = &tclRegionNamePtr;

  if((rstatus = shTclParseArgv(interp, &argc, argv, regMedianFindAsDblArgTable,
			       FTCL_ARGV_NO_LEFTOVERS, g_regMedianFindAsDbl))
      !=  FTCL_ARGV_SUCCESS) 
   {
      return(rstatus);
  
   } else {

    strncpy(tclRegionName,tclRegionNamePtr,MAXTCLREGNAMELEN);

    if (shTclRegAddrGetFromName(interp, tclRegionName, &reg) != TCL_OK) {
      sprintf(errors,"\nCannot find region named: %s",
	      tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

    if (reg->nrow * reg->ncol<=0) {
      sprintf(errors,"Region has no pixels\n");
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

  }

  retcode = atRegMedianFindAsDbl(reg, &median);

  if ( retcode != SH_SUCCESS) {
    shTclInterpAppendWithErrStack(interp);
    return(TCL_ERROR);
  }

  sprintf(m, "%f", median);
  Tcl_SetResult (interp, m, TCL_VOLATILE);

  return(TCL_OK); 

}


/*<AUTO EXTRACT>
  TCL VERB: regModeFind

  <HTML>
  C ROUTINE CALLED:
      <A HREF=atRegSci.html#atRegModeFind>atRegModeFind</A> in atRegSci.c

<P>
Histograms all of the pixels in a region and finds the most common value.
This is only defined for integer region types.  The minimum and maximum
values for the histogram are passed as min and max, respectively.  If all
the bins have values below min, the value returned is max + 1.

  </HTML>

</AUTO>*/

static ftclArgvInfo regModeFindArgTable[] = {
       { NULL,          FTCL_ARGV_HELP,      NULL, NULL,
  "Histogram all of the pixels in a region and find the most common value.\n"},
       {"<region>",     FTCL_ARGV_STRING,    NULL, NULL, "Region containing pixels to histogram"},
       {"-min",		FTCL_ARGV_DOUBLE,    NULL, NULL, "minimum value; Default: 0.0"},
       {"-max",		FTCL_ARGV_DOUBLE,    NULL, NULL, "maximum value; Default: 65535.0"},
       {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
    };

char g_regModeFind[] = "regModeFind";

static int tclRegModeFind(ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{

  int rstatus;
  REGION *reg;
  double min = 0., max = 65535., mode;
  char m[20];
  char tclRegionName[MAXTCLREGNAMELEN], *tclRegionNamePtr=NULL;
  char errors[80];
  RET_CODE retcode; 

  regModeFindArgTable[1].dst = &tclRegionNamePtr;
  regModeFindArgTable[2].dst = &min;
  regModeFindArgTable[3].dst = &max;

  if((rstatus = shTclParseArgv(interp, &argc, argv, regModeFindArgTable,
			       FTCL_ARGV_NO_LEFTOVERS, g_regModeFind))
      !=  FTCL_ARGV_SUCCESS) 
   {
      return(rstatus);

   } else {

    /** get REGION, make sure it exists and has pixels **/
    strncpy(tclRegionName,tclRegionNamePtr,MAXTCLREGNAMELEN);

    if (shTclRegAddrGetFromName(interp, tclRegionName, &reg) 
	!= TCL_OK) {
      sprintf(errors,"\nCannot find region named: %s",
	      tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

    if ( (reg->nrow * reg->ncol) == 0) {
      sprintf(errors,"No pixels in region named: %s\n", tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

    if( min >= max) {
      Tcl_SetResult(interp, "The minimum value must be less then the maximum."
		    , TCL_STATIC);
      return(TCL_ERROR);
    }

  }

  if  (!( (reg->type == TYPE_U8) || (reg->type == TYPE_S8) ||
	(reg->type == TYPE_U16) || (reg->type == TYPE_S16) ||
	(reg->type == TYPE_U32) || (reg->type == TYPE_S32))) {
    Tcl_SetResult(interp, "Can not find mode for this type",
		  TCL_STATIC);
    return(TCL_ERROR);
  }

  retcode = atRegModeFind(reg, min, max, &mode);
  if ( retcode != SH_SUCCESS ) {
    shTclInterpAppendWithErrStack(interp);
    return(TCL_ERROR);
  }

  sprintf(m, "%#.0f", mode);
  Tcl_SetResult (interp, m, TCL_VOLATILE);

  return(TCL_OK); 

}


/*<AUTO EXTRACT>
  TCL VERB: regStatsFind

  <HTML>
  C ROUTINE CALLED:
                <A HREF=atRegSci.html#atRegStatsFind>atRegStatsFind</A> in atRegSci.c

<P>
Finds the mean and sigma on the mean in an image, plus the high and low
values and their positions.

One of three subroutines is called depending on the presence or absence
of flags.

If -min/-max are present then  atRegClippedStatsFind is called, which 
computes mean and sigma, throwing out pixels with values above max and
below min from the average.
Hi/Lo pixel value and position are calculated in this case between min and max.

If -nsigma/-niterations are present (niteration>0) then atRegSigmaClip
is called, and the mean and sigma are determined iteratively, with values
above nsigma thrown out after each iteration.  Hi/Lo pixel value and position 
are calculated in this case without any pixel rejection.  (If this option
is invoked the clipped mean above is ignored even if the -min/-max flags
are present.)

If neither of these sets of switches are present, then a straight mean
and sigma are calculated.

  </HTML>

</AUTO>*/

static ftclArgvInfo regStatsFindArgTable[] = {
       { NULL,          FTCL_ARGV_HELP,      NULL, NULL,
	   "Find region statistics.  Optional clipping.\n"},
       {"<region>",     FTCL_ARGV_STRING,    NULL, NULL, 
             "region to compute stats on"},
       {"-min",		FTCL_ARGV_DOUBLE,    NULL, NULL, 
             "min pix value for clipping; Default: 0"},
       {"-max",		FTCL_ARGV_DOUBLE,    NULL, NULL, 
             "max pix value for clipping; Default: 65535"},
       {"-nsigma",	FTCL_ARGV_DOUBLE,    NULL, NULL, 
             "number of sigma to clip at; Default: 3.0"},
       {"-niterations",	FTCL_ARGV_INT,       NULL, NULL, 
             "number of iterations for sigma clipping; Default=0"},
       {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
    };

char g_regStatsFind[] = "regStatsFind";

static int tclRegStatsFind (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
        int rstatus;
        REGION *reg;
        double mean, sigma;
        double high, low;
	double min, max;
	double nsigma;
	double total;
	int niterations;
	int clipped;
        int hrow, hcol, lrow, lcol;
        char s[300];
	char tclRegionName[MAXTCLREGNAMELEN], *tclRegionNamePtr = NULL;
	char errors[80];
        RET_CODE retcode;
	
	min = 0;
	max = 65535;
	niterations=0;
        nsigma = 3.0;
	sigma=3.0;

        regStatsFindArgTable[1].dst = &tclRegionNamePtr;
        regStatsFindArgTable[2].dst = &min;
        regStatsFindArgTable[3].dst = &max;
        regStatsFindArgTable[4].dst = &nsigma;
        regStatsFindArgTable[5].dst = &niterations;
  
	clipped = 0;

	if ((rstatus = shTclParseArgv(interp, &argc, argv,
				      regStatsFindArgTable,
				      FTCL_ARGV_NO_LEFTOVERS, g_regStatsFind))
           !=  FTCL_ARGV_SUCCESS) 
           {
             return(rstatus);
      
           } else {

	  strncpy(tclRegionName,tclRegionNamePtr ,MAXTCLREGNAMELEN);

	  if (shTclRegAddrGetFromName(interp, tclRegionName, &reg) 
	      != TCL_OK) {
	    sprintf(errors,"\nCannot find region named: %s",
		    tclRegionName);
	    Tcl_SetResult(interp ,errors,TCL_VOLATILE);
	    return(TCL_ERROR);
	  }

	  if( min >= max) {
	    Tcl_SetResult(interp, "The minimum value must be less then the maximum.", TCL_STATIC);
	    return(TCL_ERROR);
	  }

	  if( nsigma < 0 || niterations <0 ) {
	    Tcl_SetResult(interp, "nsigma and niterations must be positive", TCL_STATIC);
	    return(TCL_ERROR);
	  }
	  
	  if (ftcl_ArgIsPresent(argc, argv, "-min", regStatsFindArgTable) ||
	     ftcl_ArgIsPresent(argc, argv, "-max", regStatsFindArgTable)) {
	    clipped = 1;
	  }
	}

	if ((reg->nrow == 0) || (reg->ncol == 0)) {
	  Tcl_SetResult(interp, "There are no pixels in this region",
	    TCL_STATIC);
	  return(TCL_ERROR);
	}

/* actually call the C-layer and return any errors */
	total = 0.0;
	if(niterations>0) {
	  retcode = atRegStatsFind (reg,&mean,&sigma,&high,&hrow,&hcol,&low,&lrow,&lcol,&total);
          if ( retcode != SH_SUCCESS ) {
            shTclInterpAppendWithErrStack(interp);
            return(TCL_ERROR);
          }
          
	  retcode = atRegSigmaClip(reg,nsigma,niterations,&mean,&sigma);
          if ( retcode != SH_SUCCESS ) {
            shTclInterpAppendWithErrStack(interp);
            return(TCL_ERROR);
          }

	} else if (clipped) {
	  retcode = atRegClippedStatsFind(reg,min,max,&mean,&sigma, 
                        &high,&hrow,&hcol,&low,&lrow,&lcol);
          if ( retcode != SH_SUCCESS ) {
            shTclInterpAppendWithErrStack(interp);
            return(TCL_ERROR);
          }

        } else {
	  retcode = atRegStatsFind (reg,&mean,&sigma,&high,&hrow,&hcol,&low,&lrow,&lcol,&total);
          if ( retcode != SH_SUCCESS ) {
            shTclInterpAppendWithErrStack(interp);
            return(TCL_ERROR);
          }

	}

        /*format is keyed list*/
        sprintf (s,"{mean %f} {sigma %f} {high %f} {hrow %d} {hcol %d} {low %f} {lrow %d} {lcol %d} {total %f}",
                     mean,     sigma,     high,     hrow,     hcol,     low,
 lrow,     lcol, total);
        Tcl_SetResult (interp, s, TCL_VOLATILE);
        return (TCL_OK);
}



/***************************************************************************/
/*                      tclSigmaClip 	 	                            */
/***************************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: regSigmaClip

  <HTML>
  C ROUTINE CALLED:
                <A HREF=atRegSci.html#atRegSigmaClip>atRegSigmaClip</A> in atRegSci.c

<P>
Over a number of iterations, it finds the sigma, clips out pixels 
farther away than n sigma and goes on to the next iteration.

  </HTML>

</AUTO>*/
char g_sigmaClip[] = "regSigmaClip";
ftclArgvInfo g_sigmaClipTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Finds the mean and variance using a sigma clipping alogorithm. " },
  {"<regName>", FTCL_ARGV_STRING, NULL, NULL,
   "The REGION's handle"},
  {"<nsigma>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "number of sigmas for rejection"},
  {"<niterations>", FTCL_ARGV_INT, NULL, NULL,
   "number of iterations"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int 
tclSigmaClip(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                ) {
                                /* INPUT */
  char *a_reg;			/* region name */
  REGION *regPtr;               /* pointer to the region */
  double a_nsigma;              /* how many sigma away to clip */
  int a_niterations;            /* how many times to iterate */
                                /* INTERNAL */
  double mean=0;		/* the sigma-clipped mean */
  double sigma=0;		/* the sigma-clipped sigma */

  int rstat;
  RET_CODE retcode;             /* check from errors from C-layer */
  char answer[40];              /* sent back to Tcl */

  g_sigmaClipTbl[1].dst = &a_reg;
  g_sigmaClipTbl[2].dst = &a_nsigma;
  g_sigmaClipTbl[3].dst = &a_niterations;

/* Parse the command */

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_sigmaClipTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_sigmaClip)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

/* get and check region */
    if (shTclRegAddrGetFromName(interp, a_reg, &regPtr) != TCL_OK) {
      Tcl_SetResult
        (interp, "SigmaClip: bad region name", TCL_VOLATILE);
      return TCL_ERROR;
    }
    if ( (regPtr->nrow * regPtr->ncol) == 0) {
      Tcl_SetResult(interp ,"No pixels in region",TCL_STATIC);
      return(TCL_ERROR);
    }

/* check for the rest of the arguments */
    if (a_nsigma <= 0) {
      Tcl_SetResult (interp, 
		     "SigmaClip: nsigma <= 0", 
		     TCL_VOLATILE);
      return TCL_ERROR;
    } 
    if (a_niterations <= 0) {
      Tcl_SetResult (interp, 
		     "SigmaClip: niterations <= 0", 
		     TCL_VOLATILE);
      return TCL_ERROR;
    } 

/*  call the routine and check for errors from the C-layer */
    retcode = atRegSigmaClip(regPtr, a_nsigma, a_niterations, &mean, &sigma);
    if ( retcode != SH_SUCCESS ) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
    }

/* send back the answer */
    Tcl_SetResult(interp, "", TCL_VOLATILE);
    sprintf(answer, "mean %f", mean); Tcl_AppendElement(interp, answer);
    sprintf(answer, "sigma %f", sigma); Tcl_AppendElement(interp, answer);
    
    ftclParseRestore("SigmaClip");
    return TCL_OK;
}


/*<AUTO EXTRACT>
  TCL VERB: regConvolve

  <HTML>
  C ROUTINE CALLED:
                <A HREF=atRegSci.html#atRegConvolve>atRegConvolve</A> in atRegSci.c

<P>
	  Convolve region with template to produce target.

  </HTML>

</AUTO>*/

static ftclArgvInfo regConvolveArgTable[] = {
      { NULL,           FTCL_ARGV_HELP,     NULL, NULL,
	  "Convolve region with template to produce target.\n"},
      {"<regsource>",   FTCL_ARGV_STRING,   NULL, NULL, "source region"},
      {"<regconvolve>",   FTCL_ARGV_STRING,   NULL, NULL, "convolving region"},
      {"-regtarget",   FTCL_ARGV_STRING,   NULL, NULL, "target region; Default: create new region"},
      {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
     };

char g_regConvolve[] = "regConvolve";

static int tclRegConvolve(ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{

  int rstatus;
  REGION *regsource, *regconvolve, *regtarget;
  int r,c;
  char tclRegionName[MAXTCLREGNAMELEN], *tclRegionNamePtr= NULL;
  char errors[80];
  char* regconvolvePtr = NULL, *regtargetPtr = NULL;
  RET_CODE retcode;

  regConvolveArgTable[1].dst = &tclRegionNamePtr;
  regConvolveArgTable[2].dst = &regconvolvePtr;
  regConvolveArgTable[3].dst = &regtargetPtr;
  
  if ((rstatus = shTclParseArgv(interp, &argc, argv, regConvolveArgTable,
				FTCL_ARGV_NO_LEFTOVERS, g_regConvolve))
      !=FTCL_ARGV_SUCCESS) 
  {  
    return(rstatus);

  } else {

    strncpy(tclRegionName, tclRegionNamePtr, MAXTCLREGNAMELEN);
    
    if (shTclRegAddrGetFromName(interp, tclRegionName, &regsource) 
	!= TCL_OK) {
      sprintf(errors,"\nCannot find region named: %s",
	      tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

    strncpy(tclRegionName,regconvolvePtr,MAXTCLREGNAMELEN);

    /** get source REGION, make sure exists and has pixels **/
    if (shTclRegAddrGetFromName(interp, tclRegionName, &regconvolve) 
	!= TCL_OK) {
      sprintf(errors,"\nCannot find region named: %s",
	      tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }
    if ( (regconvolve->nrow * regconvolve->ncol) == 0) {
      sprintf(errors,"No pixels in region named: %s\n", tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

    if (regtargetPtr!=NULL) {
      strncpy(tclRegionName,regtargetPtr,MAXTCLREGNAMELEN);
     
      /** get target REGION, make sure it exists and has pixels **/ 
      if (shTclRegAddrGetFromName(interp, tclRegionName, &regtarget) 
	  != TCL_OK) {
	sprintf(errors,"\nCannot find region named: %s",
		tclRegionName);
	Tcl_SetResult(interp ,errors,TCL_VOLATILE);
	return(TCL_ERROR);
      }
      if ( (regtarget->nrow * regtarget->ncol) == 0) {
        sprintf(errors,"No pixels in region named: %s\n", tclRegionName);
        Tcl_SetResult(interp ,errors,TCL_VOLATILE);
        return(TCL_ERROR);
      }

    } else {
      /* Get a new region of the same size and type as regsource */
      if (shTclRegNameGet(interp, tclRegionName) != TCL_OK) {
	return(TCL_ERROR);
      }
      regtarget = shRegNew(tclRegionName, regsource->nrow, regsource->ncol,
        regsource->type);
      if (regtarget == NULL) {
        shTclRegNameDel(interp, tclRegionName);
	return(TCL_ERROR);
      }
    }

  }

  if ((regsource->nrow==regtarget->nrow)&&(regsource->ncol==regtarget->ncol)) {

    retcode = atRegConvolve(regsource, regconvolve, regtarget);
    if ( retcode != SH_SUCCESS ) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
    }
  } else {
    Tcl_SetResult(interp, "Source and target regions must be the same size",
      TCL_STATIC);
    return(TCL_ERROR);
  }

  if (regtargetPtr==NULL) {

  /* Copy all of the pixels from target to source and free the target */ 
    for (r=0; r<regsource->nrow; r++) {
      for (c=0; c<regsource->ncol; c++) 
	shRegPixSetWithDbl(regsource,r,c,shRegPixGetAsDbl(regtarget,r,c));
    }
    shRegDel(regtarget);
    shTclRegNameDel(interp, tclRegionName);
  }

  return TCL_OK;

}


/*<AUTO EXTRACT>
  TCL VERB: regSkyFind
  <HTML>
  C ROUTINE CALLED:
                <A HREF=atRegSci.html#atRegSkyFind>atRegSkyFind</A> in atRegSci.c
<P>
  Find the sky background in an image by fitting a Gaussian
  </HTML>

</AUTO>*/

static ftclArgvInfo regSkyFindTbl[] = {
  { NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Find the sky background in a region"},
  {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "region" },
  {(char *)NULL, FTCL_ARGV_END, NULL, NULL, (char *)NULL}
};

char g_regSkyFind[] = "regSkyFind";

static int tclRegSkyFind (ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
        int rstatus;
        double sky;
        REGION *reg;
        char s[20];
	char tclRegionName[MAXTCLREGNAMELEN], *tclRegionNamePtr=NULL;
	const char *p_errors;
	char errors[80];

/* parse inputs */
        regSkyFindTbl[1].dst = &tclRegionNamePtr;

        if( (rstatus = shTclParseArgv(interp, &argc, argv, regSkyFindTbl,
                      FTCL_ARGV_NO_LEFTOVERS, g_regSkyFind)) != FTCL_ARGV_SUCCESS) {
          return( rstatus );
        }
        strncpy(tclRegionName, tclRegionNamePtr, MAXTCLREGNAMELEN);

/* do some simple error checks */
        if (shTclRegAddrGetFromName(interp, tclRegionName, &reg) != TCL_OK) {
          sprintf(errors, "Unable to find region named: %s", tclRegionName);
          Tcl_SetResult(interp, errors, TCL_VOLATILE);
          return(TCL_ERROR);
        }

        if ( reg->nrow * reg->ncol <=0 ) {
          sprintf(errors, "Region has no pixels");
          Tcl_SetResult(interp, errors, TCL_VOLATILE);
          return(TCL_ERROR);
        }

/* make the actual call */
        sky = atRegSkyFind(reg);

/* report any errors */
	p_errors = shErrStackGetEarliest();
	if ( p_errors != NULL) {
	  sprintf(errors,"%s",p_errors);
	  Tcl_SetResult(interp,errors,TCL_VOLATILE);
	  shTclInterpAppendWithErrStack(interp);
	  return(TCL_ERROR);
	}

/* return the answer */
        sprintf (s,"%g",sky);
        Tcl_SetResult (interp, s, TCL_VOLATILE);
        return (TCL_OK);

}


/*<AUTO EXTRACT>
  TCL VERB: regBin

  <HTML>
  C ROUTINE CALLED:
                <A HREF=atRegSci.html#atRegBin>atRegBin</A> in atRegSci.c

<P>
  Bin an image into larger pixels by an integer factor.  
  Specify one number for both directions or a number for each direction.
  If the optional 'normalize' flag is given a value of 1, it 
  means the user wants an average value in each superpixel, rather than a sum.

  </HTML>

</AUTO>*/

static ftclArgvInfo regBinArgTable[] = {
      { NULL,          FTCL_ARGV_HELP,    NULL, NULL,
  "Bin an image into larger pixels by an integer factor.  \n Specify one number for both directions or a number for each direction.\n Set superpixels to sum of all included pixels unless 'normalize' is 1.\n" },
      {"<region>",     FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-row",         FTCL_ARGV_INT,  NULL, NULL, "Row binning factor"},
      {"-col",         FTCL_ARGV_INT,  NULL, NULL, "Column binning factor"},
      {"-both",         FTCL_ARGV_INT,  NULL, NULL, "Binning factor or row and column"},
      {"-target",      FTCL_ARGV_STRING,  NULL, NULL, "Target region; Default: create new region"},
      {"-normalize",   FTCL_ARGV_INT,  NULL, NULL, "If 1 use average; Default: 0"},
      {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
    };

char g_regBin[] = "regBin";

static int tclRegBin (ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{

  int rstatus;
  int rowfactor =1, colfactor = 1; /* These are the binning factors in the row and 
			       column directions respectively. */
  REGION *reg, *regtarget;
  char tclRegionName[MAXTCLREGNAMELEN], sourceRegionName[MAXTCLREGNAMELEN];
  char *tclRegionNamePtr = NULL,  *targetPtr = NULL;
  char errors[80];
  int  both = 0;
  /*int  oldArgc;*/
  int normalize = 0;
  RET_CODE retcode;
  
  regBinArgTable[1].dst = &tclRegionNamePtr;
  regBinArgTable[2].dst = &rowfactor;
  regBinArgTable[3].dst = &colfactor;
  regBinArgTable[4].dst = &both;
  regBinArgTable[5].dst = &targetPtr;
  regBinArgTable[6].dst = &normalize;
  
  /* To insure that the user input both a target and an argument on what to
       do with the target, I make sure the number of arguments is at least
       three.  This way no time is wasted binning an image with a factor of
       one in each direction. */
  /* oldArgc = argc; */     /* Save because ftcl_ParseARgv will change  it */

  if ((rstatus = shTclParseArgv(interp, &argc, argv, regBinArgTable,
				FTCL_ARGV_NO_LEFTOVERS, g_regBin))
      != FTCL_ARGV_SUCCESS) 
  {
    return(rstatus);

  } else {
    
    strncpy(sourceRegionName, tclRegionNamePtr,MAXTCLREGNAMELEN);

    /*  Here is where I get the original region.  Make sure it exists, has pixels */

    if (shTclRegAddrGetFromName(interp, sourceRegionName, &reg) 
	!= TCL_OK) {
      sprintf(errors,"\nCannot find region named: %s",sourceRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }
    if ( (reg->nrow * reg->ncol) == 0) {
      sprintf(errors,"No pixels in region named: %s\n", tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }


    /*  Here is where I parse through the user input. */ 
    
    if (both!=0) {
      colfactor = both;
      rowfactor = colfactor;
    } 
    
    /* Fortunately, ftclGetInt gets only the integer part of any number input
       so I don't need to check for whether or not the number is an int. */
    
    if(colfactor > reg->ncol || rowfactor > reg->nrow) {
      Tcl_SetResult(interp,
		    "Binning factors must be smaller then the image size.",
		    TCL_STATIC);
      return(TCL_ERROR);
    } /* This is done so that the image is not binned into half of a pixel. */

    /* If the target region is present, here I go and get it. */

    if (targetPtr!=NULL) {
      strncpy(tclRegionName, targetPtr,MAXTCLREGNAMELEN);
      if (shTclRegAddrGetFromName(interp, tclRegionName, &regtarget) 
	  != TCL_OK) {
	sprintf(errors,"\nCannot find region named: %s",tclRegionName);
	Tcl_SetResult(interp,errors,TCL_VOLATILE);
	return(TCL_ERROR);
      } 
      
      if( regtarget->ncol < (reg->ncol)/colfactor || 
	 regtarget->nrow < reg->nrow/rowfactor) {
	Tcl_SetResult(interp,
		      "Target image is too small.",
		      TCL_STATIC);
	return(TCL_ERROR);
      } 

      if(colfactor<1 || rowfactor<1) {
	Tcl_SetResult(interp,
		      "Row and column compression factors must be > 0",
		      TCL_STATIC);
	return(TCL_ERROR);
      } 

    } else {
      
      /* Get a new region of the same type as reg but the size is that of the
       binned image.  Ie. if an 100x100 is binned by a factor of 2 in each
       direction, I only get a 50x50 image.  */
      
      if (shTclRegNameGet(interp, tclRegionName) != TCL_OK) {
	sprintf(errors,"\nCannot get new region.");
	Tcl_SetResult(interp,errors,TCL_VOLATILE);
	return(TCL_ERROR);
      }
      regtarget = shRegNew(tclRegionName, (reg->nrow)/rowfactor, 
			   (reg->ncol)/colfactor, reg->type);
      
      /*  I use reg->ncol/colfactor and reg->nrow/rowfactor as the size
	  of the new target region.  Therefore I throw away any extra pixels at
	  the edge.  Ie. 100/3 == 33 so that the last row of pixels is not 
	  included in the output region. */

      if (regtarget == NULL) {
	sprintf(errors,"\nCannot get new region named: %s",tclRegionName);
	Tcl_SetResult(interp,errors,TCL_VOLATILE);
	shTclRegNameDel(interp, tclRegionName);
	return(TCL_ERROR);
      }

      shTclRegNameSetWithAddr(interp,regtarget,tclRegionName);
    }
      
    
  }

  retcode = atRegBin(reg, regtarget, rowfactor, colfactor, normalize);
  if ( retcode != SH_SUCCESS ) {
    shTclInterpAppendWithErrStack(interp);
    return(TCL_ERROR);
  }
  
  Tcl_SetResult(interp, tclRegionName, TCL_VOLATILE);
 
  return(TCL_OK);
  
}


/*<AUTO EXTRACT>
  TCL VERB: regMedianFind

  <HTML>
  C ROUTINE CALLED:
                <A HREF=atRegSci.html#atRegMedianFind>atRegMedianFind</A> in atRegSci.c

<P>
	 Find the pixel median of a list of regions

  </HTML>

</AUTO>*/

static ftclArgvInfo  regMedianFindArgTable[] = {
     { NULL,          FTCL_ARGV_HELP,   NULL, NULL,
	 "Find the pixel median of a list of regions.\n"},
     {"<reglist>",    FTCL_ARGV_STRING, NULL, NULL, "List of regions"},
     {"-regtarget",   FTCL_ARGV_STRING, NULL, NULL, "Target region; Default: 1st region on list"},
     {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
    };

char g_regMedianFind[] = "regMedianFind";

static int tclRegMedianFind(ClientData clientData, Tcl_Interp *interp, 
			      int argc, char **argv) 
{

  int rstatus;
  REGION *regtarget, **reglist; /* reglist will be the array of input regions
				   of which I will find the median. */
  char tclRegionName[MAXTCLREGNAMELEN];
  char *p_list = NULL, *regtargetPtr = NULL;
  int l_argc;  
  char **l_argv;  /* Both l_argc and largv are the number and names of the 
		     regions in the input list respectively. */
  int counter;
  char errors[80];
  RET_CODE retcode;


  regMedianFindArgTable[1].dst = &p_list;
  regMedianFindArgTable[2].dst = &regtargetPtr;

  
  if ((rstatus = shTclParseArgv(interp, &argc, argv,regMedianFindArgTable,
				FTCL_ARGV_NO_LEFTOVERS, g_regMedianFind))
      != FTCL_ARGV_SUCCESS)
  {
    return(rstatus);

  } else {

    /* Tcl_SplitList, if given a string, will split it into l_argv where each
       element of l_argv is a element in the original input list.  Elements
       are seperated by white space. */

    if (Tcl_SplitList(interp, p_list, &l_argc, &l_argv) == TCL_ERROR ) {  
      sprintf(errors,"Error in parsing input list of regions.\n");
      Tcl_SetResult(interp,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

    /*  Here I get enough space for the input list. */

    reglist = (REGION **)shMalloc((l_argc)*sizeof(REGION *));

    /*  I get each element of the input list */
    
    for(counter = 0; counter < (l_argc); counter++) {
      if (shTclRegAddrGetFromName(interp, l_argv[counter], 
				  (&reglist[counter]) ) != TCL_OK) {
	sprintf(errors,"\nCannot find region named: %s\n",l_argv[counter]);
	Tcl_SetResult(interp ,errors,TCL_VOLATILE);
	free((char *)l_argv);
	shFree(reglist);
	return(TCL_ERROR);
      }
    }
 
    /*  A check to make sure that each element of the input list is of the 
	same type and size. */

    for(counter = 1; counter < (l_argc); counter++) {
      if ((reglist[0]->nrow!=reglist[counter]->nrow) ||
	  (reglist[0]->ncol!=reglist[counter]->ncol) ||
	  (reglist[0]->type!=reglist[counter]->type)   ) {

	Tcl_SetResult(interp,
	     "All regions must be the same size and have the same pixel type."
	     ,TCL_STATIC);
	free((char *)l_argv);
	shFree(reglist); 
	return(TCL_ERROR);
      }
    }

    /*  If no target is specified, then the target that is used is the first
	element in the list.  */

    strncpy(tclRegionName, l_argv[0],MAXTCLREGNAMELEN);
    regtarget = reglist[0];

    /*  If there is a target, then it is used in place of the first region 
	in the list. */

    if (regtargetPtr!=NULL) {

      strncpy(tclRegionName, regtargetPtr,MAXTCLREGNAMELEN);
      if (shTclRegAddrGetFromName(interp, tclRegionName, &regtarget) 
	  != TCL_OK) {
	sprintf(errors,"\nCannot find region named: %s\n",tclRegionName);
	Tcl_SetResult(interp ,errors,TCL_VOLATILE);
	free((char *)l_argv);
	shFree(reglist);
	return(TCL_ERROR);
      } 
      if ((reglist[0]->nrow!=regtarget->nrow) ||
	  (reglist[0]->ncol!=regtarget->ncol) ||
	  (reglist[0]->type!=regtarget->type)   ) {
	Tcl_SetResult(interp,
	     "All regions must be the same size and have the same pixel type."
	     ,TCL_STATIC);
	free((char *)l_argv);
	shFree(reglist); 
        return(TCL_ERROR);
      }
    }
  }

  retcode = atRegMedianFind((const REGION **)reglist,(int)(l_argc),regtarget);
  if ( retcode != SH_SUCCESS ) {
    shTclInterpAppendWithErrStack(interp);
    free((char *)l_argv);
    shFree(reglist);
    return(TCL_ERROR);
  }

  shFree((void *)reglist);
  return TCL_OK;
  
}


/*****************************************************************************
 * <AUTO EXTRACT>
 *
 * TCL VERB: regClippedMeanFind
 *
 * <HTML>
 * C ROUTINE CALLED:
 *    <A HREF=atRegSci.html#atRegClippedMeanFind>atRegClippedMeanFind</A> in atRegSci.c
 *
 * <P>
 * From a list of input regions determine the mean on a pixel by pixel basis.
 * If 'niter' > 1 find a clipped mean, where pixel values more than
 * 'nsigma' sigma from the previous solution are ignored and a new mean found.
 * The resulting values for each pixel are placed in the output region.
 * If the '-correct' switch is used the sigma used to edit the data is corrected
 * (increased slightly) to compensate for the truncated wings, by assuming the
 * underlying distribution is a gaussian.  With this option 'nsigma' >= 1.5.
 *
 *
 * </HTML>
 *
 * </AUTO>
 */

static ftclArgvInfo  regClippedMeanFindArgTable[] = {
     { NULL,          FTCL_ARGV_HELP,   NULL, NULL,
	 "Find the clipped mean of each pixel in a list of regions.\n"},
     {"<reglist>",    FTCL_ARGV_STRING, NULL, NULL, "List of regions"},
     { "<niter>",     FTCL_ARGV_INT,    NULL, NULL,
         "Number of iterations in calculation of clipped mean.\n"},
     { "<nsigma>",    FTCL_ARGV_DOUBLE, NULL, NULL, 
         "Discard values more than 'nsigma' times the stdev from the mean.\n"},
     {"-regtarget",   FTCL_ARGV_STRING, NULL, NULL, "Target region (Default: 1st region on list)"},
     {"-correct",     FTCL_ARGV_CONSTANT, (void *)1, NULL,
         "Turn on clipping sigma correction" },
     {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
    };

char g_regClippedMeanFind[] = "regClippedMeanFind";

static int 
tclRegClippedMeanFind(ClientData clientData, Tcl_Interp *interp, 
			      int argc, char **argv) 
{

  int rstatus;
  REGION *regtarget, **reglist; /* reglist will be the array of input regions
				   of which I will find the median. */
  char tclRegionName[MAXTCLREGNAMELEN];
  char *p_list = NULL, *regtargetPtr = NULL;
  int l_argc;  
  char **l_argv;  /* Both l_argc and largv are the number and names of the 
		     regions in the input list respectively. */
  int counter;
  char errors[80];
  int niter = -1;
  double nsigma = -1.0;
  int clipCorrectFlag = 0;
  RET_CODE retcode;

  regClippedMeanFindArgTable[1].dst = &p_list;
  regClippedMeanFindArgTable[2].dst = &niter;
  regClippedMeanFindArgTable[3].dst = &nsigma;
  regClippedMeanFindArgTable[4].dst = &regtargetPtr;
  regClippedMeanFindArgTable[5].dst = &clipCorrectFlag;


  if ((rstatus = shTclParseArgv(interp, &argc, argv, regClippedMeanFindArgTable,
				FTCL_ARGV_NO_LEFTOVERS, g_regClippedMeanFind))
      != FTCL_ARGV_SUCCESS) {

         return(rstatus);

      } else {

    /* Tcl_SplitList, if given a string, will split it into l_argv where each
       element of l_argv is a element in the original input list.  Elements
       are seperated by white space. */

    if (Tcl_SplitList(interp, p_list, &l_argc, &l_argv) == TCL_ERROR ) {  
      sprintf(errors,"Error in parsing input list of regions.\n");
      Tcl_SetResult(interp,errors,TCL_VOLATILE);
      free((char *)l_argv);
      return(TCL_ERROR);
    }

    /*  Here I get enough space for the input list. */

    reglist = (REGION **)shMalloc((l_argc)*sizeof(REGION *));

    /*  I get each element of the input list */
    
    for(counter = 0; counter < (l_argc); counter++) {
      if (shTclRegAddrGetFromName(interp, l_argv[counter], 
				  (&reglist[counter]) ) != TCL_OK) {
	sprintf(errors,"\nCannot find region named: %s\n",l_argv[counter]);
	Tcl_SetResult(interp, errors, TCL_VOLATILE);
	free((char *)l_argv);
	shFree(reglist);
	return(TCL_ERROR);
      }
    }

    /* Check to make sure that the target region - and therefore all the
       rest of the regions - actually contain pixels */

    if ( (reglist[0]->nrow * reglist[0]->ncol) == 0 ) {
      sprintf(errors, "Target region has no pixels");
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      free((char *)l_argv);
      shFree(reglist);
      return(TCL_ERROR);
    }
 
    /*  A check to make sure that each element of the input list is of the 
	same type and size. */

    for(counter = 1; counter < (l_argc); counter++) {
      if ((reglist[0]->nrow!=reglist[counter]->nrow) ||
	  (reglist[0]->ncol!=reglist[counter]->ncol) ||
	  (reglist[0]->type!=reglist[counter]->type)   ) {

	Tcl_SetResult(interp,
	     "All regions must be the same size and have the same pixel type."
	     ,TCL_STATIC);
	free((char *)l_argv);
	shFree(reglist); 
	return(TCL_ERROR);
      }
    }

    /*  If no target is specified, then the target that is used is the first
	element in the list.  */

    strncpy(tclRegionName, l_argv[0],MAXTCLREGNAMELEN);
    regtarget = reglist[0];

    /*  If there is a target, then it is used in place of the first region 
	in the list. */

    if (regtargetPtr!=NULL) {

      strncpy(tclRegionName, regtargetPtr,MAXTCLREGNAMELEN);
      if (shTclRegAddrGetFromName(interp, tclRegionName, &regtarget) 
	  != TCL_OK) {
	sprintf(errors,"\nCannot find region named: %s\n",tclRegionName);
	Tcl_SetResult(interp ,errors,TCL_VOLATILE);
	free((char *)l_argv);
	shFree(reglist);
	return(TCL_ERROR);
      } 
      if ( (reglist[0]->nrow != regtarget->nrow) ||
           (reglist[0]->ncol != regtarget->ncol) ||
           (reglist[0]->type != regtarget->type)  ) {
        sprintf(errors, "\nAll regions must be of the same size and pixel type\n");
        Tcl_SetResult(interp, errors, TCL_VOLATILE);
        free((char *)l_argv);
        shFree(reglist);
        return(TCL_ERROR);
      }
    }
  }

  /*
   * make sure that 'niter' > 0 and 'nsigma' > 0
   *   but allow nsigma to be anything if niter == 1 (to maintain 
   *   functional compatibility with this case in previous version)
   */
  if ( niter <= 0 ) {
    Tcl_SetResult(interp, "invalid value for niter", TCL_STATIC);
    free((char *)l_argv);
    shFree(reglist);
    return(TCL_ERROR);
  }
  if ( (nsigma <= 0) && (niter !=1) ) {
    Tcl_SetResult(interp, "invalid value for nsigma", TCL_STATIC);
    free((char *)l_argv);
    shFree(reglist);
    return(TCL_ERROR);
  }

  /* now actually call the C routine */

  retcode = atRegClippedMeanFind((const REGION **)reglist,(int)(l_argc),
                       niter, nsigma, regtarget, clipCorrectFlag);

  if ( retcode != SH_SUCCESS ) {
    shTclInterpAppendWithErrStack(interp);
    free((char *)l_argv); 
    shFree((void *)reglist);
    return TCL_ERROR;
  }

  free((char *)l_argv); 
  shFree((void *)reglist);
  return TCL_OK;
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * TCL VERB: regMedianFindByColumn
 *
 *  <HTML>
 * C ROUTINE CALLED:
 * <A HREF=atRegSci.html#atRegMedianFindByColumn>atRegMedianFindByColumn</A> 
 * in atRegSci.c.
 *
 * <P>
 * DESCRIPTION:
 * Given a M-row x N-col REGION, and an empty 1-row x N-col REGION,
 * fill the 1xN region with values which are the median value of the
 * pixels in each column of the original.
 *
 * </HTML>
 *
 * </AUTO>
 */

static ftclArgvInfo  regMedianFindByColumn_ArgTable[] = {
     { NULL,          FTCL_ARGV_HELP,   NULL, NULL,
	 "Make a 1xN region from the median of each column in a region."},
     {"<input>",      FTCL_ARGV_STRING, NULL, NULL, 
         "MxN region from whose cols we calculate median"},
     {"<output>",     FTCL_ARGV_STRING, NULL, NULL, 
         "1xN region into which we place median of each column"},
     {NULL,           FTCL_ARGV_END,    NULL, NULL, NULL}
};

static char g_regMedianFindByColumn[] = "regMedianFindByColumn";

static int 
tclRegMedianFindByColumn
   (
   ClientData clientData, 
   Tcl_Interp *interp, 
   int argc, 
   char **argv
   ) 
{

  int rstatus;
  REGION *input_reg, *output_reg;
  char *input_name = NULL, *output_name = NULL;
  char errors[200];
  RET_CODE retcode;

  regMedianFindByColumn_ArgTable[1].dst = &input_name;
  regMedianFindByColumn_ArgTable[2].dst = &output_name;

  if ((rstatus = shTclParseArgv(interp, &argc, argv,
                     regMedianFindByColumn_ArgTable, FTCL_ARGV_NO_LEFTOVERS, 
                     g_regMedianFindByColumn)) != FTCL_ARGV_SUCCESS) {
    return(rstatus);
  }

  /* get the input and output regions */
  if (shTclRegAddrGetFromName(interp, input_name, &input_reg) != TCL_OK) {
      sprintf(errors, "\nCannot find region named: %s", input_name);
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      return(TCL_ERROR);
  }
  if (shTclRegAddrGetFromName(interp, output_name, &output_reg) != TCL_OK) {
      sprintf(errors, "\nCannot find region named: %s", output_name);
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      return(TCL_ERROR);
  }

  /* make sure input and output REGIONS actually have pixels */
  if ( (input_reg->nrow * input_reg->ncol) == 0 ) {
    Tcl_SetResult(interp,
          "Input region does not have any pixels", TCL_VOLATILE);
  }
  if ( (output_reg->nrow * output_reg->ncol) == 0 ) {
    Tcl_SetResult(interp,
          "Output region does not have any pixels", TCL_VOLATILE);
  }

  /* make sure that the REGIONs have the same number of cols */
  if (input_reg->ncol != output_reg->ncol) {
    Tcl_SetResult(interp,
           "Regions do not have the same number of columns", TCL_STATIC);
    return(TCL_ERROR);
  }

  /* call the C function that calculates the median */

  retcode = atRegMedianFindByColumn(input_reg, output_reg);
  if ( retcode != SH_SUCCESS ) {
    shTclInterpAppendWithErrStack(interp);
    return(TCL_ERROR);
  }

  return(TCL_OK);
}


/***************************************************************************
 * <AUTO EXTRACT>
 *
 * TCL VERB: regMedianFindByRow
 *
 *  <HTML>
 * C ROUTINE CALLED:
 * <A HREF=atRegSci.html#atRegMedianFindByRow>atRegMedianFindByRow</A> 
 * in atRegSci.c.
 *
 * <P>
 * DESCRIPTION:
 * Given a N-row x M-col REGION, and an empty N-row x 1-col REGION,
 * fill the Nx1 region with values which are the median value of the
 * pixels in each column of the original.
 *
 * </HTML>
 *
 * </AUTO>
 */

static ftclArgvInfo  regMedianFindByRow_ArgTable[] = {
     { NULL,          FTCL_ARGV_HELP,   NULL, NULL,
	 "Make an Nx1 region from the median of each row in a region."},
     {"<input>",      FTCL_ARGV_STRING, NULL, NULL, 
         "NxM region from whose rows we calculate median"},
     {"<output>",     FTCL_ARGV_STRING, NULL, NULL, 
         "Nx1 region into which we place median of each row"},
     {NULL,           FTCL_ARGV_END,    NULL, NULL, NULL}
};

static char g_regMedianFindByRow[] = "regMedianFindByRow";

static int 
tclRegMedianFindByRow
   (
   ClientData clientData, 
   Tcl_Interp *interp, 
   int argc, 
   char **argv
   ) 
{

  int rstatus;
  REGION *input_reg, *output_reg;
  char *input_name = NULL, *output_name = NULL;
  char errors[200];
  RET_CODE retcode;

  regMedianFindByRow_ArgTable[1].dst = &input_name;
  regMedianFindByRow_ArgTable[2].dst = &output_name;

  if ((rstatus = shTclParseArgv(interp, &argc, argv,
                     regMedianFindByRow_ArgTable, FTCL_ARGV_NO_LEFTOVERS, 
                     g_regMedianFindByRow)) != FTCL_ARGV_SUCCESS) {
    return(rstatus);
  }

  /* get the input and output regions */
  if (shTclRegAddrGetFromName(interp, input_name, &input_reg) != TCL_OK) {
      sprintf(errors, "\nCannot find region named: %s", input_name);
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      return(TCL_ERROR);
  }
  if (shTclRegAddrGetFromName(interp, output_name, &output_reg) != TCL_OK) {
      sprintf(errors, "\nCannot find region named: %s", output_name);
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      return(TCL_ERROR);
  }

  /* make sure REGIONS have pixels */
  if ( (input_reg->nrow * input_reg->ncol) == 0 ) {
    Tcl_SetResult(interp,
           "No pixels in input region", TCL_VOLATILE);
    return(TCL_ERROR);
  }
  if ( (output_reg->nrow * output_reg->ncol) == 0 ) {
    Tcl_SetResult(interp,
           "No pixels in output region", TCL_VOLATILE);
    return(TCL_ERROR);
  }

  /* make sure that the REGIONs have the same number of rows */
  if (input_reg->nrow != output_reg->nrow) {
    Tcl_SetResult(interp,
           "Regions do not have the same number of rows", TCL_STATIC);
    return(TCL_ERROR);
  }

  /* call the C function that calculates the median */
  retcode = atRegMedianFindByRow(input_reg, output_reg);
  if ( retcode != SH_SUCCESS ) {
    shTclInterpAppendWithErrStack(interp);
    return(TCL_ERROR);
  } 

  return(TCL_OK);
}



/***************************************************************************
 * <AUTO EXTRACT>
 *
 * TCL VERB: regPixValSubstitute
 *
 *  <HTML>
 * C ROUTINE CALLED:
 * <A HREF=atRegSci.html#atRegPixValSubstitute>atRegPixValSubstitute</A> 
 * in atRegSci.c.
 *
 * <P>
 * DESCRIPTION:
 * Given a REGION, a (double) value, and a (double) new value, 
 * replace all instances of pixels having 'value' with 'newvalue'.
 *
 * </HTML>
 *
 * </AUTO>
 */

static ftclArgvInfo  regPixValSubstitute_ArgTable[] = {
     { NULL,          FTCL_ARGV_HELP,   NULL, NULL,
	 "replace all occurences of 'oldval' with 'newval'."},
     {"<reg>",        FTCL_ARGV_STRING, NULL, NULL, 
         "region in which we make substitutions"},
     {"<oldval>",     FTCL_ARGV_DOUBLE, NULL, NULL, 
         "replace all occurences of this value ..."},
     {"<newval>",     FTCL_ARGV_DOUBLE, NULL, NULL, 
         "... with this value"},
     {"-compare",     FTCL_ARGV_INT, NULL, NULL, 
         "0: replace pixel=oldval (default)\n 1: replace pixel<=oldval\n else: replace pixel>=oldval"},
     {NULL,           FTCL_ARGV_END,    NULL, NULL, NULL}
};

static char g_regPixValSubstitute[] = "regPixValSubstitute";

static int 
tclRegPixValSubstitute
   (
   ClientData clientData, 
   Tcl_Interp *interp, 
   int argc, 
   char **argv
   ) 
{

  int rstatus;
  REGION *input_reg;
  char *input_name = NULL;
  char errors[200];
  double oldval, newval;
  int compare=0;
  RET_CODE retcode;

  regPixValSubstitute_ArgTable[1].dst = &input_name;
  regPixValSubstitute_ArgTable[2].dst = &oldval;
  regPixValSubstitute_ArgTable[3].dst = &newval;
  regPixValSubstitute_ArgTable[4].dst = &compare;

  if ((rstatus = shTclParseArgv(interp, &argc, argv,
                     regPixValSubstitute_ArgTable, FTCL_ARGV_NO_LEFTOVERS, 
                     g_regPixValSubstitute)) != FTCL_ARGV_SUCCESS) {
    return(rstatus);
  }

  /* get the input region and make sure it has pixels */
  if (shTclRegAddrGetFromName(interp, input_name, &input_reg) != TCL_OK) {
      sprintf(errors, "Cannot find region named: %s", input_name);
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      return(TCL_ERROR);
  }
 
  if ( (input_reg->nrow * input_reg->ncol) == 0) {
    sprintf(errors,"Region has no pixels");
    Tcl_SetResult(interp, errors, TCL_VOLATILE);
    return(TCL_ERROR);
  }

  retcode = atRegPixValSubstitute(input_reg, oldval, newval, compare);
  if ( retcode != SH_SUCCESS ) {
    shTclInterpAppendWithErrStack(interp);
    return(TCL_ERROR);
  }

  return(TCL_OK);
}


/***************************************************************************
 * <AUTO EXTRACT>
 *
 * TCL VERB: regPixRangeSubstitute
 *
 *  <HTML>
 * C ROUTINE CALLED:
 * <A HREF=atRegSci.html#atRegPixRangeSubstitute>atRegPixRangeSubstitute</A> 
 * in atRegSci.c.
 *
 * <P>
 * DESCRIPTION:
 * Given a REGION, the values of all pixels within a given range are
 * replaced with a new value.
 *
 * </HTML>
 *
 * </AUTO>
 */

static ftclArgvInfo  regPixRangeSubstitute_ArgTable[] = {
     { NULL,          FTCL_ARGV_HELP,   NULL, NULL,
	 "replace all values in range with 'newval'."},
     {"<reg>",        FTCL_ARGV_STRING, NULL, NULL, 
         "region in which we make substitutions"},
     {"<minval>",     FTCL_ARGV_DOUBLE, NULL, NULL, 
         "replace all occurences greater than this value ..."},
     {"<maxval>",     FTCL_ARGV_DOUBLE, NULL, NULL, 
         "... and less than this value ..."},
     {"<newval>",     FTCL_ARGV_DOUBLE, NULL, NULL, 
         "... with this value"},
     {NULL,           FTCL_ARGV_END,    NULL, NULL, NULL}
};

static char g_regPixRangeSubstitute[] = "regPixRangeSubstitute";

static RET_CODE
tclRegPixRangeSubstitute
   (
   ClientData clientData, 
   Tcl_Interp *interp, 
   int argc, 
   char **argv
   ) 
{

  int rstatus;
  REGION *input_reg;
  char *input_name = NULL;
  char errors[200];
  double minval=-0.1, maxval=0.1, newval=0.1;
  RET_CODE retcode;

  regPixRangeSubstitute_ArgTable[1].dst = &input_name;
  regPixRangeSubstitute_ArgTable[2].dst = &minval;
  regPixRangeSubstitute_ArgTable[3].dst = &maxval;
  regPixRangeSubstitute_ArgTable[4].dst = &newval;

  if ((rstatus = shTclParseArgv(interp, &argc, argv,
                     regPixRangeSubstitute_ArgTable, FTCL_ARGV_NO_LEFTOVERS, 
                     g_regPixRangeSubstitute)) != FTCL_ARGV_SUCCESS) {
    return(rstatus);
  }

  /* get the region and make sure has pixels */
  if (shTclRegAddrGetFromName(interp, input_name, &input_reg) != TCL_OK) {
      sprintf(errors, "\nCannot find region named: %s", input_name);
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      return(TCL_ERROR);
  }

  if ( (input_reg->nrow * input_reg->ncol) == 0 ) {
    sprintf(errors,"\nRegion has no pixels");
    Tcl_SetResult(interp, errors, TCL_VOLATILE);
    return(TCL_ERROR);
  }

  if ( minval > maxval ) {
    sprintf(errors,"\nminval is greater than maxval");
    Tcl_SetResult(interp, errors, TCL_VOLATILE);
    return(TCL_ERROR);
  }

  /* call the C function that does the substitution */
  retcode = atRegPixRangeSubstitute(input_reg, minval, maxval, newval);
  if ( retcode != SH_SUCCESS ) {
    shTclInterpAppendWithErrStack(interp);
    return(TCL_ERROR);
  } 

  return(TCL_OK);
}

/*<AUTO EXTRACT>
  TCL VERB: regCorrectWithVec

  <HTML>
  C ROUTINE CALLED:
      <A HREF=atRegCorrectWithVec.html#atRegCorrectWithVec>
      atRegCorrectWithVec</A> in atRegSci.c

<P> Multiply corrections specifed in VECTOR to the pixels in a region. The
first argument is the region to apply corrections to. The second argument is a
VECTOR of corrections. By default, the first element of the VECTOR is the
correction to pixels with value is 0, the second is a correction for pixels
with value 1, etc. Corrections may be specified as floating point numbers.
One imagines, for instance, a list of 65535 corrections...

<p>If REGION is of type FL32, and the pixel happens to have no fractional
part, the corresponding vector element is multiplied to it. If the pixel has a
fractional part, a correction is generated by interpolating, and the
interpolateded correction is multipled to the pixel.

<p>If region is of any type othe than FL32, the pixel value is used to look up
a correction, and the correction is multipled with the pixel.  Any fractional
part correction is dithered in.  The first pixel in the image is used
in the computation of the random seed for the dither making the correction
reproduceable.

<p>
The -firstC and -lastC flags may be used to specify the pixel
values the first element of Vec and last element of Vec correspond to.
In this case the values of vector are assumed to apply to linearly
spaced values, with spacing (lastC-firstC)/length, where length
is the length of the vector of corrections. Simple two-point interpolation 
is used among the elements of the correction vector to obtain a
correction to multiply pixels by. Pixel values smaller than -firstC
and larger than -lastC are left unmodified.

  </HTML>

</AUTO>*/

static ftclArgvInfo regCorrectWithVec_ArgTable[] = {
       { NULL,      FTCL_ARGV_HELP,   NULL, NULL,
  "Multiply pixels in a region by a correction table (i.e., a list).\n"},
       {"<region>", FTCL_ARGV_STRING, NULL, NULL, "Region containing pixels to correct"},
       {"<corrections>",   FTCL_ARGV_STRING, NULL, NULL, "vector of corrections"},
       {"-firstC",  FTCL_ARGV_INT,    NULL, NULL, "Pixel value for first correction in Vec; Default: 0"},
       {"-lastC ",  FTCL_ARGV_INT,    NULL, NULL,
	    "Pixel value for last correction in Vec; Default: sizeof(Vec)-1, olny used for FL32 regions"},
       {NULL,       FTCL_ARGV_END,    NULL, NULL, NULL}
    };

char g_regCorrectWithVec[] = "regCorrectWithVec";

static int tclRegCorrectWithVec(ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{

  int rstatus;
  REGION *reg;
  char tclRegionName[MAXTCLREGNAMELEN], *tclRegionNamePtr=NULL;
  char *tclVectorNamePtr=NULL;
  char errors[80];
  int firstC = -1, lastC = -1;
  RET_CODE retcode; 

  regCorrectWithVec_ArgTable[1].dst = &tclRegionNamePtr;
  regCorrectWithVec_ArgTable[2].dst = &tclVectorNamePtr;
  regCorrectWithVec_ArgTable[3].dst = &firstC; /* will be -1 if not specified */
  regCorrectWithVec_ArgTable[4].dst = &lastC;  /* will be -1 if not specified */
  

  if((rstatus = shTclParseArgv(interp, &argc, argv, regCorrectWithVec_ArgTable,
			       FTCL_ARGV_NO_LEFTOVERS, g_regModeFind))
      !=  FTCL_ARGV_SUCCESS) 
  {
    return(rstatus);

  } else {

    strncpy(tclRegionName, tclRegionNamePtr, MAXTCLREGNAMELEN);

    if (shTclRegAddrGetFromName(interp, tclRegionNamePtr, &reg) 
	!= TCL_OK) {
      sprintf(errors,"\nCannot find region named: %s",
	      tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }
  }

  {
 
     VECTOR *corr = NULL;
     
     if (shTclAddrGetFromName
	 (interp, tclVectorNamePtr, (void **) &corr, "VECTOR") != TCL_OK) {
	Tcl_SetResult(interp, "VECTOR name", TCL_VOLATILE);
	return TCL_ERROR;
     }

     if (firstC == -1) firstC = 0;
     if (lastC == -1) lastC = firstC + (corr->dimen)-1 ;
     if (lastC < firstC) {
	   Tcl_SetResult(interp ,"lastC is smaller than firstC ",TCL_STATIC);
	   return TCL_ERROR;
     }
     if (reg->type == TYPE_FL32) {
	retcode = atRegCorrectFL32 (reg, corr->vec, corr->dimen, firstC, lastC);
        if ( retcode != SH_SUCCESS ) {
          shTclInterpAppendWithErrStack(interp);
          return(TCL_ERROR);
        }
     } 
     else {
	retcode = atRegCorrectINT (reg, corr->vec, corr->dimen, firstC);
        if ( retcode != SH_SUCCESS ) {
          shTclInterpAppendWithErrStack(interp);
          return(TCL_ERROR);
        }
     }
  }

  return(TCL_OK); 

}

/*<AUTO EXTRACT>
  TCL VERB: regDistortWithVec

  <HTML>
  C ROUTINE CALLED:
      <A HREF=atRegDistortWithVec.html#atRegDistortWithVec>
      atRegDistortWithVec</A> in atRegSci.c

<P> Multiply inverse corrections specifed in VECTOR to the pixels in a 
region. The first argument is the region to apply corrections to. The second 
argument is a VECTOR of corrections. By default, the first element of the 
VECTOR is the correction to pixels with value is 0, the second is a 
correction for pixels with value 1, etc. Corrections may be specified as 
floating point numbers.  One imagines, for instance, a list of 65535 
orrections...

<p> Unlike regCorrectWithVec, this only works with a REGION of type FL32.

<p>
The -firstC and -lastC flags may be used to specify the pixel
values the first element of Vec and last element of Vec correspond to.
In this case the values of vector are assumed to apply to linearly
spaced values, with spacing (lastC-firstC)/length, where length
is the length of the vector of corrections. Simple two-point interpolation 
is used among the elements of the correction vector to obtain a
correction to multiply pixels by. Pixel values smaller than -firstC
and larger than -lastC are left unmodified.

  </HTML>

</AUTO>*/

static ftclArgvInfo regDistortWithVec_ArgTable[] = {
       { NULL,      FTCL_ARGV_HELP,   NULL, NULL,
  "Multiply pixels in a region by inverse of a correction table (i.e., a list).\n"},
       {"<region>", FTCL_ARGV_STRING, NULL, NULL, "Region containing pixels to correct"},
       {"<corrections>",   FTCL_ARGV_STRING, NULL, NULL, "vector of corrections"},
       {"-firstC",  FTCL_ARGV_INT,    NULL, NULL, "Pixel value for first correction in Vec (def:0)"},
       {"-lastC ",  FTCL_ARGV_INT,    NULL, NULL,
	    "Pixel value for last correction in Vec (def:sizeof(Vec)-1, only used for FL32 regions"},
       {NULL,       FTCL_ARGV_END,    NULL, NULL, NULL}
    };

char g_regDistortWithVec[] = "regDistortWithVec";

static int tclRegDistortWithVec(ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{

  int rstatus;
  REGION *reg;
  char tclRegionName[MAXTCLREGNAMELEN], *tclRegionNamePtr=NULL;
  char *tclVectorNamePtr=NULL;
  char errors[80];
  int firstC = -1, lastC = -1;
  RET_CODE retcode; 

  regDistortWithVec_ArgTable[1].dst = &tclRegionNamePtr;
  regDistortWithVec_ArgTable[2].dst = &tclVectorNamePtr;
  regDistortWithVec_ArgTable[3].dst = &firstC; /* will be -1 if not specified */
  regDistortWithVec_ArgTable[4].dst = &lastC;  /* will be -1 if not specified */
  

  if((rstatus = shTclParseArgv(interp, &argc, argv, regDistortWithVec_ArgTable,
			       FTCL_ARGV_NO_LEFTOVERS, g_regModeFind))
      !=  FTCL_ARGV_SUCCESS) 
   {
    return(rstatus);
  } else {

    strncpy(tclRegionName, tclRegionNamePtr, MAXTCLREGNAMELEN);

    if (shTclRegAddrGetFromName(interp, tclRegionNamePtr, &reg) 
	!= TCL_OK) {
      sprintf(errors,"\nCannot find region named: %s",
	      tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

  }

  {
 
     VECTOR *corr = NULL;
     
     if (shTclAddrGetFromName
	 (interp, tclVectorNamePtr, (void **) &corr, "VECTOR") != TCL_OK) {
	Tcl_SetResult(interp, "VECTOR name", TCL_VOLATILE);
	return TCL_ERROR;
     }

     if (firstC == -1) firstC = 0;
     if (lastC == -1) lastC = firstC + (corr->dimen)-1 ;
     if (lastC < firstC) {
	   Tcl_SetResult(interp ,"lastC is smaller than firstC ",TCL_STATIC);
	   return TCL_ERROR;
     }
     if (reg->type == TYPE_FL32) {
	retcode = atRegDistortFL32 (reg, corr->vec, corr->dimen, firstC, lastC);
        if ( retcode != SH_SUCCESS ) {
          shTclInterpAppendWithErrStack(interp);
          return(TCL_ERROR);
        }
     }else{
       Tcl_SetResult(interp, "region must be of TYPE FL32", TCL_STATIC);
       return TCL_ERROR;
     }
  }

  return(TCL_OK); 

}

/*<AUTO EXTRACT>
  TCL VERB: regCorrectQuick

  <HTML>
  C ROUTINE CALLED:
      <A HREF=atRegCorrectQuick.html#atRegCorrectQuick>
      atRegCorrectQuick</A> in atRegSci.c

<P>
This routine is a special version of the 
<A HREF=atRegCorrectWithVec.html#atRegCorrectWithVec>
    atRegCorrectWithVec</A>
routine that is designed specically for the Monitor Telescope.
It should operate more quickly than the other versions,
but may yield output REGIONs that have discernible
patterns.

<P>
The routine takes as input a type U16 REGION, a VECTOR
of correction factors, and a range over which to
apply the corrections.  The correction factors 
are assumed to be given for uniform steps between
firstC and lastC.
For example, given 
<pre>
       firstC = 10000
       lastC  = 50000
       vector = 1.0   1.1   1.2   1.3   1.4     
</pre>
we would apply a multiplicative correction factor
<pre> 
       1.0    to pixels with values < 10,000 (no correction)
       1.0    to pixels with values approx 10,000   
       1.1    to pixels with values approx 20,000
       1.2    to pixels with values approx 30,000
       1.3    to pixels with values approx 40,000
       1.4    to pixels with values approx 50,000
       1.0    to pixels with values > 50,000 (no correction)
</pre>
<P>
The routine uses some form of interpolation (currently simple
linear interpolation) to apply intermediate correction factors
to the pixels with values between those specified by firstC and lastC.

<P>
Algorithmically, 
the routine builds two tables with 65536 elements, with one entry
for each possible input pixel value.
The first table contains the corrected output value 
(i.e. integer part of input*correction factor),
and the second contains a floating-point value between 0.0 and 1.0,
denoting the fractional portion of the input*correction.
To translate input to output values, the routine
<pre>
      a. sets i = input pixel value
      b. looks up corrected integer portion j = integer_table[i]
      c. looks up corrected fractional portion f = fractional_table[i]
      d. picks a random number R between 0.0 and 1.0
      e. if R > f, adds 1 to j
      f. sets output pixel value to j
</pre>

  </HTML>

</AUTO>*/

static ftclArgvInfo regCorrectQuick_ArgTable[] = {
       { NULL,      FTCL_ARGV_HELP,   NULL, NULL,
  "Multiply pixels in a region by a correction table (i.e., a list).\n"},
       {"<region>", FTCL_ARGV_STRING, NULL, NULL, "U16 region containing pixels to correct"},
       {"<corrections>",   FTCL_ARGV_STRING, NULL, NULL, "vector of corrections"},
       {"-firstC",  FTCL_ARGV_INT,    NULL, NULL, "Pixel value for first correction in Vec (def:0)"},
       {"-lastC ",  FTCL_ARGV_INT,    NULL, NULL,
	    "Pixel value for last correction in Vec (def:65535)"},
       {NULL,       FTCL_ARGV_END,    NULL, NULL, NULL}
    };

char g_regCorrectQuick[] = "regCorrectQuick";

static RET_CODE tclRegCorrectQuick(ClientData clientData, 
        Tcl_Interp *interp, int argc, char **argv) 
{

  int rstatus;
  REGION *reg;
  char tclRegionName[MAXTCLREGNAMELEN], *tclRegionNamePtr=NULL;
  char *tclVectorNamePtr=NULL;
  char errors[80];
  int firstC = 0;
  int lastC  = 65535;
  RET_CODE retcode;

  regCorrectQuick_ArgTable[1].dst = &tclRegionNamePtr;
  regCorrectQuick_ArgTable[2].dst = &tclVectorNamePtr;
  regCorrectQuick_ArgTable[3].dst = &firstC; /* will be -1 if not specified */
  regCorrectQuick_ArgTable[4].dst = &lastC;  /* will be -1 if not specified */

  if((rstatus = shTclParseArgv(interp, &argc, argv, regCorrectQuick_ArgTable,
			       FTCL_ARGV_NO_LEFTOVERS, g_regCorrectQuick))
      !=  FTCL_ARGV_SUCCESS) 
   {

      return(rstatus);

   } else {

    /* make sure the region exists, is the right type, has pixels */
    strncpy(tclRegionName, tclRegionNamePtr, MAXTCLREGNAMELEN);
    if (shTclRegAddrGetFromName(interp, tclRegionNamePtr, &reg) 
	!= TCL_OK) {
      sprintf(errors,"Cannot find region named: %s\n", tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

    if (reg->type != TYPE_U16) {
      sprintf(errors, "Region is type %d, not type U16\n", reg->type);
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      return(TCL_ERROR);
    }

    if ( (reg->nrow * reg->ncol) == 0) {
      sprintf(errors,"No pixels in region named: %s\n", tclRegionName);
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

  }

  {
     VECTOR *corr = NULL;

     /* make sure vector exists */
     if (shTclAddrGetFromName
	 (interp, tclVectorNamePtr, (void **) &corr, "VECTOR") != TCL_OK) {
	Tcl_SetResult(interp, "VECTOR name", TCL_VOLATILE);
	return(TCL_ERROR);
     }

    if ( corr->dimen < 0 ) {
      sprintf(errors,"Vector dimension less than zero\n");
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

    if ( corr->dimen == 0 ) {
      sprintf(errors,"Vector dimension is zero\n");
      Tcl_SetResult(interp ,errors,TCL_VOLATILE);
      return(TCL_ERROR);
    }

     if (lastC < firstC) {
	   Tcl_SetResult(interp ,"lastC is smaller than firstC ",TCL_STATIC);
	   return TCL_ERROR;
     }

     retcode = atRegCorrectQuick (reg, corr->vec, corr->dimen, firstC, lastC);
    if ( retcode != SH_SUCCESS ) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
    }

  }

  return(TCL_OK); 
}


/***************************************************************************
 * <AUTO EXTRACT>
 *
 * TCL VERB: regMedianSmooth
 *
 *  <HTML>
 * C ROUTINE CALLED:
 * <A HREF=atRegSci.html#atRegMedianSmooth>atRegMedianSmooth</A> 
 * in atRegSci.c.
 *
 * <P>
 * DESCRIPTION:
 * Smooth a REGION using a M-row x N-col median filter.
 *
 * </HTML>
 *
 * </AUTO>
 */

char g_regMedianSmooth[] = "regMedianSmooth";
ftclArgvInfo regMedianSmoothTable[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Smooth a REGION using a M-row x N-col median filter"},
  {"<regName>", FTCL_ARGV_STRING, NULL, NULL,
   "The input REGION's handle"},
  {"<m>", FTCL_ARGV_INT, NULL, NULL,
	"Number of rows to use in filter"},
  {"<n>", FTCL_ARGV_INT, NULL, NULL,
	"Number of cols to use in filter"},
  {"<outRegName>", FTCL_ARGV_STRING, NULL, NULL,
   "The output REGION's handle"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int 
tclRegMedianSmooth(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                ) {
                                /* INPUT */
  char *a_reg;			/* region name */
  REGION *regPtr;               /* pointer to the region */
  char *a_oreg;			/* region name */
  REGION *oregPtr;              /* pointer to the region */
  int m,n;	 	        /* number of rows, cols to use in median */
                                /* INTERNAL */
  int i;

  int rstat;
  RET_CODE retcode;             /* check from errors from C-layer */

  i = 1 ;
  regMedianSmoothTable[i++].dst = &a_reg;
  regMedianSmoothTable[i++].dst = &m;
  regMedianSmoothTable[i++].dst = &n;
  regMedianSmoothTable[i++].dst = &a_oreg;

/* Parse the command */

  if ((rstat = shTclParseArgv(interp, &argc, argv, regMedianSmoothTable,
       FTCL_ARGV_NO_LEFTOVERS, g_regMedianSmooth)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

/* get and check region */
    if (shTclRegAddrGetFromName(interp, a_reg, &regPtr) != TCL_OK) {
      Tcl_SetResult
        (interp, "regMedianSmooth: bad region name", TCL_VOLATILE);
      return TCL_ERROR;
    }
    if ( (regPtr->nrow * regPtr->ncol) == 0) {
      Tcl_SetResult(interp ,"No pixels in region",TCL_STATIC);
      return(TCL_ERROR);
    }
    if (shTclRegAddrGetFromName(interp, a_oreg, &oregPtr) != TCL_OK) {
      Tcl_SetResult
        (interp, "regMedianSmooth: bad region name", TCL_VOLATILE);
      return TCL_ERROR;
    }
    if ( (oregPtr->nrow * oregPtr->ncol) == 0) {
      Tcl_SetResult(interp ,"No pixels in output region (try regNewFromReg)",TCL_STATIC);
      return(TCL_ERROR);
    }

/*  call the routine and check for errors from the C-layer */
    retcode = atRegMedianSmooth(regPtr, m, n, oregPtr);
    if ( retcode != SH_SUCCESS ) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
    }

    ftclParseRestore("regMedianSmooth");
    return TCL_OK;
}



/***************** Declaration of TCL verbs in this module *****************/
static char *facil = "astrotools";

void atTclRegSciDeclare (Tcl_Interp *interp) 
{

        int flags = FTCL_ARGV_NO_LEFTOVERS;

	shTclDeclare (interp,"regGaussiansAdd",
                      (Tcl_CmdProc *)tclRegGaussiansAdd,
	              (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regGaussiansAddArgTable, flags,
				      g_regGaussiansAdd),
		      shTclGetUsage(interp, regGaussiansAddArgTable, flags,
				    g_regGaussiansAdd)
                      );

	shTclDeclare (interp,g_regMap,
                      (Tcl_CmdProc *)tclRegMap,
	              (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regMapArgTable, flags, g_regMap),
		      shTclGetUsage(interp, regMapArgTable, flags, g_regMap)
                      );

        shTclDeclare (interp, g_regMedianFindAsDbl, 
                      (Tcl_CmdProc *)tclRegMedianFindAsDbl,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regMedianFindAsDblArgTable, flags,
				      g_regMedianFindAsDbl),
		      shTclGetUsage(interp, regMedianFindAsDblArgTable, flags,
				    g_regMedianFindAsDbl)
                      );

        shTclDeclare (interp, g_regModeFind, 
                      (Tcl_CmdProc *)tclRegModeFind,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regModeFindArgTable, flags,
				      g_regModeFind),
		      shTclGetUsage(interp, regModeFindArgTable, flags,
				    g_regModeFind)
                      );

        shTclDeclare (interp, g_regStatsFind, 
                      (Tcl_CmdProc *)tclRegStatsFind,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regStatsFindArgTable, flags,
				      g_regStatsFind),
		      shTclGetUsage(interp, regStatsFindArgTable, flags,
				    g_regStatsFind)
                      );

  shTclDeclare(interp, g_sigmaClip,
               (Tcl_CmdProc *) tclSigmaClip,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, 
	       facil,
               shTclGetArgInfo(interp, g_sigmaClipTbl,
                               flags, g_sigmaClip),
               shTclGetUsage(interp, g_sigmaClipTbl,
                             flags, g_sigmaClip));

        shTclDeclare (interp, g_regConvolve, 
                      (Tcl_CmdProc *)tclRegConvolve,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regConvolveArgTable, flags,
				      g_regConvolve),
		      shTclGetUsage(interp, regConvolveArgTable, flags,
				    g_regConvolve)
                      );

        shTclDeclare (interp, g_regSkyFind, 
                      (Tcl_CmdProc *)tclRegSkyFind,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regSkyFindTbl, flags,
				      g_regSkyFind),
		      shTclGetUsage(interp, regSkyFindTbl, flags,
				    g_regSkyFind)
                      );

        shTclDeclare (interp, g_regBin, 
                      (Tcl_CmdProc *)tclRegBin,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regBinArgTable, flags, g_regBin),
		      shTclGetUsage(interp, regBinArgTable, flags, g_regBin)
                      );

        shTclDeclare (interp, g_regMedianFind, 
                      (Tcl_CmdProc *)tclRegMedianFind,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regMedianFindArgTable, flags,
				      g_regMedianFind),
		      shTclGetUsage(interp, regMedianFindArgTable, flags,
				    g_regMedianFind)
                      );

        shTclDeclare (interp, g_regClippedMeanFind, 
                      (Tcl_CmdProc *)tclRegClippedMeanFind,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regClippedMeanFindArgTable, flags,
				      g_regClippedMeanFind),
		      shTclGetUsage(interp, regClippedMeanFindArgTable, flags,
				    g_regClippedMeanFind)
                      );

        shTclDeclare (interp, g_regMedianFindByColumn, 
                      (Tcl_CmdProc *) tclRegMedianFindByColumn,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regMedianFindByColumn_ArgTable, 
                                      flags, g_regMedianFindByColumn),
		      shTclGetUsage(interp, regMedianFindByColumn_ArgTable, 
                                      flags, g_regMedianFindByColumn)
                      );

        shTclDeclare (interp, g_regMedianFindByRow, 
                      (Tcl_CmdProc *) tclRegMedianFindByRow,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regMedianFindByRow_ArgTable, 
                                      flags, g_regMedianFindByRow),
		      shTclGetUsage(interp, regMedianFindByRow_ArgTable, 
                                      flags, g_regMedianFindByRow)
                      );

        shTclDeclare (interp, g_regPixValSubstitute, 
                      (Tcl_CmdProc *) tclRegPixValSubstitute,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regPixValSubstitute_ArgTable, 
                                      flags, g_regPixValSubstitute),
		      shTclGetUsage(interp, regPixValSubstitute_ArgTable, 
                                      flags, g_regPixValSubstitute)
                      );
        shTclDeclare (interp, g_regPixRangeSubstitute, 
                      (Tcl_CmdProc *) tclRegPixRangeSubstitute,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regPixRangeSubstitute_ArgTable, 
                                      flags, g_regPixRangeSubstitute),
		      shTclGetUsage(interp, regPixRangeSubstitute_ArgTable, 
                                      flags, g_regPixRangeSubstitute)
                      );

	shTclDeclare (interp, g_regCorrectWithVec, 
                      (Tcl_CmdProc *) tclRegCorrectWithVec,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regCorrectWithVec_ArgTable, 
                                      flags, g_regCorrectWithVec),
		      shTclGetUsage(interp, regCorrectWithVec_ArgTable, 
                                      flags, g_regCorrectWithVec)
                      );

	        shTclDeclare (interp, g_regDistortWithVec, 
                      (Tcl_CmdProc *) tclRegDistortWithVec,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regDistortWithVec_ArgTable, 
                                      flags, g_regDistortWithVec),
		      shTclGetUsage(interp, regDistortWithVec_ArgTable, 
                                      flags, g_regDistortWithVec)
                      );

	shTclDeclare (interp, g_regCorrectQuick, 
                      (Tcl_CmdProc *) tclRegCorrectQuick,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regCorrectQuick_ArgTable, 
                                      flags, g_regCorrectQuick),
		      shTclGetUsage(interp, regCorrectQuick_ArgTable, 
                                      flags, g_regCorrectQuick)
                      );

        shTclDeclare (interp, g_regMedianSmooth, 
                      (Tcl_CmdProc *) tclRegMedianSmooth,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      facil,
		      shTclGetArgInfo(interp, regMedianSmoothTable, 
                                      flags, g_regMedianSmooth),
		      shTclGetUsage(interp, regMedianSmoothTable, 
                                      flags, g_regMedianSmooth)
                      );

}




