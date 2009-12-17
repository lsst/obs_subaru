/* <AUTO>
   FILE:
                hg.c
<HTML>
ABSTRACT: Simple operations on data structures for histograms (HG),
points (PT), and pgplot states (PGSTATE).
Everything is public.
</HTML>
</AUTO> */

/************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <float.h>
#if !defined(DARWIN)
#  include <values.h>
#endif
#include "shCDiskio.h"
#include "cpgplot.h"
#include "shChain.h"
#include "shCHg.h"
#include "shCGarbage.h"
#include "shCSchema.h"

#define isgood(x)   ( ( ((x) < FLT_MAX)) && ((x) > -FLT_MAX) ? (1) : (0))
#define minimum(x,y) (((x) < (y)) ? (x) : (y))
#define maximum(x,y) (((x) > (y)) ? (x) : (y))
#define included(x,y,z) ( (((x)<=(y)) && ((y)<=(z))) ? (1) : (0) )

/* External variables referenced here */
extern SAOCMDHANDLE g_saoCmdHandle;

/* Temporary -- need the prototype here until it gets put out in public */
RET_CODE saoDrawWrite(CMD_SAOHANDLE *, int, int, int, char *);

/*<AUTO EXTRACT>
  ROUTINE: shHgNew
  DESCRIPTION:
  <HTML>
  Make a new HG
  </HTML>
  RETURN VALUES:
  <HTML>
  Pointer to an HG
  </HTML>
</AUTO>*/
HG *shHgNew(
	    void /* void */
	    ) {
  static int id=0;
  HG *new_hg = (HG *)shMalloc(sizeof(HG));
  if (new_hg == NULL)
    shFatal("shHgNew: can not malloc for new HG");

  *(int *)&(new_hg->id) = id++;

  new_hg->minimum = 0;
  new_hg->maximum = 0;

  strcpy(new_hg->name," ");

  strcpy(new_hg->xLabel," ");

  strcpy(new_hg->yLabel," ");
  new_hg->nbin = 0;
  new_hg->contents = NULL;
  new_hg->error    = NULL;
  new_hg->binPosition = NULL;
  shHgClear(new_hg);

  return(new_hg);
}


/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shHgNameSet
  DESCRIPTION:
  <HTML>
  Sets the name of the HG
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS if successfull 
  </HTML>
</AUTO>*/
RET_CODE shHgNameSet(
		     HG* hg, /* the hg to name */
		     char* name /* what to name it */
		     ) {    
    if(hg == NULL) 
    {
        shErrStackPush("Bad HG info: pointer NULL");
        return SH_GENERIC_ERROR;
    }
    if(name!=NULL)
    {
      strncpy(hg->name, name, HGLABELSIZE);
      hg->name[HGLABELSIZE-1] = '\0';
    }
    return SH_SUCCESS;
}
       


/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shHgNameGet
  DESCRIPTION:
  <HTML>
  Returns the name of an HG
  </HTML>
  RETURN VALUES:
  <HTML>
  Char pointer to name of HG or NULL if Error
  </HTML>
</AUTO>*/
char *shHgNameGet(
		  HG* hg /* hg with unknown name */
		  ) {
    
    if(hg == NULL) 
    {
        shErrStackPush("Bad HG info: pointer NULL");
        return NULL;
    }
    return hg->name;
}


/*<AUTO EXTRACT>
  ROUTINE: shHgDefine
  DESCRIPTION:
  <HTML>
  Define a HG; labels, limits, and number of bins
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS or HG_ERROR
  </HTML>
</AUTO>*/
RET_CODE shHgDefine(
		    HG *hg,           /* hg to be defined */ 
		    char *xLabel,     /* label for x data */
		    char *yLabel,     /* label for y data */
		    char *name,       /* name of the new hg */
		    double minimum,   /* minimum value */
		    double maximum,   /* maximum value */
		    unsigned int nbin /* number of bins to create */
		    ) {
  if (hg->contents != NULL) return HG_ERROR;
  shHgNameSet(hg, name);
/*  strncpy(hg->name, name, HGLABELSIZE); */
  strncpy(hg->xLabel, xLabel, HGLABELSIZE);
  hg->xLabel[HGLABELSIZE-1] ='\0';
  strncpy(hg->yLabel, yLabel, HGLABELSIZE);
  hg->yLabel[HGLABELSIZE-1] ='\0';
  hg->nbin = nbin;
  hg->contents = (float *) shMalloc(hg->nbin*sizeof(float));
  hg->error = (float *) shMalloc(hg->nbin*sizeof(float));
  hg->binPosition = (float *) shMalloc(hg->nbin*sizeof(float));
  hg->minimum = minimum;
  hg->maximum = maximum;
  shHgClear(hg);
  return(SH_SUCCESS);
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shHgClear
  DESCRIPTION:
  <HTML>
  Set all the contents and errors to zero in a HG
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS
  </HTML>
</AUTO>*/
RET_CODE shHgClear (
		    HG *hg /* the HG to be worked upon */
		    ) {
  unsigned int ibin;
  if (hg->nbin != 0) 
    for (ibin=0; ibin<hg->nbin; ibin++) {
      hg->contents[ibin] = 0;
      hg->error[ibin] = 0;
      hg->binPosition[ibin] = 
	hg->minimum + 
	  ( (float) ibin+0.5) *
	    (hg->maximum - hg->minimum) / ( (float) hg->nbin);
    }
  hg->underflow = 0;
  hg->overflow  = 0;
  hg->entries   = 0;
  hg->wsum  = 0.0;
  hg->sum   = 0.0;
  hg->sum2  = 0.0;

  return(SH_SUCCESS);
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shHgFill
  DESCRIPTION:
  <HTML>
  Increment one bin of a HG.
  Add weight to the bin that value falls in
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS
  </HTML>
</AUTO>*/
RET_CODE shHgFill (
		   HG *hg, /* the HG to be worked upon */
		   double value, /* the value that determines the bin */
		   double weight /* the amount of increase */
		   ) {
  int bin;
  double term;
  if (value < hg->minimum) {
    hg->underflow += weight;
  } else if ( value >= hg->maximum) {
    hg->overflow += weight;
  } else {
    bin = (int)(hg->nbin * (value-hg->minimum) / (hg->maximum - hg->minimum));
    hg->contents[bin] += weight;
    term = weight*hg->binPosition[bin];
    hg->wsum += weight;
    hg->sum  += term;
    hg->sum2 += term*term;
  }
  hg->entries++;
  return SH_SUCCESS;
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shHgMean
  DESCRIPTION:
  <HTML>
  Return the mean of a HG
  </HTML>
  RETURN VALUES:
  <HTML>
  Mean as a double
  </HTML>
</AUTO>*/
double shHgMean(
		HG *hg /* the HG to be worked upon */
		) {
  if (hg->wsum != 0) {
    return hg->sum/hg->wsum;
  } else {
    return (double) 0.0;
  }
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shHgSigma 
  DESCRIPTION:
  <HTML>
  Return the sigma of a HG 
  </HTML>
  RETURN VALUES:
  <HTML>
  Returns the sigma as a double 
  </HTML>
</AUTO>*/
double shHgSigma(
		 HG *hg /* the HG to be worked upon */
		 ) {
  double sigma, mean, w;
  double sum = 0;
  double wsum = 0;
  int ibin;

  mean = shHgMean(hg);
  for (ibin=0; ibin<hg->nbin; ibin++) {
    w = hg->contents[ibin];
    wsum += w;
    sum += w * pow ( (hg->binPosition[ibin]-mean), 2.0 );
  }
  if (wsum == 0) {
    return 0.0;
  }
  sigma = sum / wsum;
  if (sigma > 0 ) {
    return sqrt(sigma);
  } else {
    return (double) 0.0;
  }
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shHgPrint 
  DESCRIPTION:
  <HTML>
  Short printout of a HG; mostly for debugging  
  </HTML>
  RETURN VALUES:
  <HTML>
  HG_OK
  </HTML>
</AUTO>*/
RET_CODE shHgPrint(
		   HG *hg  /* the HG to be worked upon */
		   ) {
     printf("id = %d \n",hg->id);
     printf("name = %s \n",hg->name);
     printf("xLabel = %s \n",hg->xLabel);
     printf("yLabel = %s \n",hg->yLabel);
     printf("nbin = %d \n",hg->nbin);
     printf("minimum = %f \n", hg->minimum);
     printf("maximum = %f \n", hg->maximum);
     printf("underflow = %f \n", hg->underflow);
#ifdef shHgPrintBINS
     for (ibin=0; ibin<hg->nbin; ibin++) {
       printf("%d %f %f \n",ibin,hg->binPosition[ibin],hg->contents[ibin]);
     }
#endif
     printf("overflow = %f \n", hg->overflow);
     printf("entries  = %d \n", hg->entries);
     printf(" mean  = %f \n",shHgMean(hg));
     printf(" sigma = %f \n",shHgSigma(hg));

     return HG_OK;
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shHgOper
  DESCRIPTION:
  <HTML>
  Return a HG that is hg1 and hg2 operating on each other.  Set 
  operation to "+", "m", "*", "/".  "m" (not "-") is subtraction.
  </HTML>
  RETURN VALUES:
  <HTML>
  A pointer to a HG containing the result of the input hg's
  </HTML>
</AUTO>*/
HG *shHgOper(
	     HG *hg1, /* the first HG to be worked upon */
	     char *operation, /*  what to do to the two HGs */
	     HG *hg2  /* the other HG to be worked upon */
	     ) {
  HG *hg;
  char name[100];
  unsigned int nbin, ibin;

  nbin = minimum(hg1->nbin, hg2->nbin);
  hg = shHgNew();
  strncpy(name, hg1->name, 40);
  strcat(name, " ");
  strcat(name, operation);
  strcat(name, " ");
  strncat(name, hg2->name, 40);
  shHgDefine(hg, hg1->xLabel, name, name, hg1->minimum, hg1->maximum, nbin);
  for (ibin=0; ibin<nbin; ibin++) {

    hg->contents[ibin] = 0.0; hg->error[ibin] = 0.0;

    if (strcmp(operation,"+") == 0)  {
      hg->contents[ibin] = hg1->contents[ibin] + hg2->contents[ibin];
      if (!isgood(hg->contents[ibin])) {hg->contents[ibin] = 0.0;}
      hg->error[ibin] = sqrt( pow(hg1->error[ibin],2) + 
			      pow(hg2->error[ibin],2));
      if (!isgood(hg->error[ibin])) {hg->error[ibin] = 0.0;};
    }

    if (strcmp(operation,"m") == 0)  {
      hg->contents[ibin] = hg1->contents[ibin] - hg2->contents[ibin];
      if (!isgood(hg->contents[ibin])) {hg->contents[ibin] = 0.0;}
      hg->error[ibin] = sqrt( pow(hg1->error[ibin],2) + 
			      pow(hg2->error[ibin],2));
      if (!isgood(hg->error[ibin])) {hg->error[ibin] = 0.0;}
    }

    if (strcmp(operation,"*") == 0)  {
      hg->contents[ibin] = hg1->contents[ibin] * hg2->contents[ibin];
      if (!isgood(hg->contents[ibin])) {hg->contents[ibin] = 0.0;}
      hg->error[ibin] = hg->contents[ibin] * sqrt( 
	  pow(hg1->error[ibin]/hg1->contents[ibin],2) + 
	  pow(hg2->error[ibin]/hg2->contents[ibin],2));
      if (!isgood(hg->error[ibin])) {hg->error[ibin] = 0.0;}
    }

    if (strcmp(operation,"/") == 0)  {
      hg->contents[ibin] = hg1->contents[ibin] / hg2->contents[ibin];
      if (!isgood(hg->contents[ibin])) {hg->contents[ibin] = 0.0;}
      hg->error[ibin] = hg->contents[ibin] * sqrt( 
	  pow(hg1->error[ibin]/hg1->contents[ibin],2) + 
	  pow(hg2->error[ibin]/hg2->contents[ibin],2));
      if (!isgood(hg->error[ibin])) {hg->error[ibin] = 0.0;}
    } 
  }
  return hg;
}


/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shHgDel
  DESCRIPTION:
  <HTML>
  Delete a HG
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS
  </HTML>
</AUTO>*/
RET_CODE shHgDel(
		 HG *hg /* the HG to be worked upon */
		 ) {
  if (hg != NULL) {
    if (hg->contents != NULL) shFree(hg->contents);
    if (hg->error    != NULL) shFree(hg->error);
    if (hg->binPosition != NULL) shFree(hg->binPosition);
    shFree(hg);
  }
  return(SH_SUCCESS);
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shPgstateNew
  DESCRIPTION:
  <HTML>
  Make a new PGSTATE
  </HTML>
  RETURN VALUES:
  <HTML>
  A pointer to the new PGSTATE
  </HTML>
</AUTO>*/
PGSTATE *shPgstateNew(
		      void /* void */
		      ) {
  static int id=0;
  PGSTATE *new_Pgstate = (PGSTATE *)shMalloc(sizeof(PGSTATE));
  *(int *)&(new_Pgstate->id) = id++;

  shPgstateDefault(new_Pgstate);
  new_Pgstate->full = 0;
  return(new_Pgstate);
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shPgstateDefault 
  DESCRIPTION:
  <HTML>
  Set everything in a PGSTATE to a good default value
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS
  </HTML>
</AUTO>*/
RET_CODE shPgstateDefault(
			  PGSTATE *pgstate /* the PGSTATE to be worked upon */
			  ) {
  strcpy(pgstate->device,"/XWINDOW");
  pgstate->nxwindow = 1;
  pgstate->nywindow = 1;
  pgstate->xfract = 0.2;
  pgstate->yfract = 0.4;
  pgstate->ixwindow = 0;
  pgstate->iywindow = 0;
  pgstate->just = 0;
  pgstate->axis = 0;
  strcpy(pgstate->xopt, "BCNST");
  strcpy(pgstate->yopt, "BCNST");
  pgstate->xtick = 0.0;
  pgstate->ytick = 0.0;
  pgstate->nxsub = 0;
  pgstate->nysub = 0;
  strcpy(pgstate->xSchemaName, "empty");
  strcpy(pgstate->ySchemaName, "empty");
  pgstate->hg = NULL;
  strcpy(pgstate->plotType, "MT");
  strcpy(pgstate->plotTitle, " ");
  pgstate->symb = 3;
  pgstate->isLine = 0;
  pgstate->icMark = 1;
  pgstate->icLine = 1;
  pgstate->icBox = 1;
  pgstate->icLabel = 1;
  pgstate->icError = 0;
  pgstate->lineWidth = 1;
  pgstate->lineStyle = 1;
  pgstate->isNewplot = 1;
  return(SH_SUCCESS);
}


/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shPgstatePrint
  DESCRIPTION:
  <HTML>
  Simple dump of a PGSTATE
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS
  </HTML>
</AUTO>*/
RET_CODE shPgstatePrint(
			PGSTATE *pgstate  /* the PGSTATE to be worked upon */
			) {
  printf("     device = %s \n",pgstate->device);
  printf("   nxwindow = %d \n",pgstate->nxwindow);
  printf("   nywindow = %d \n",pgstate->nywindow);
  printf("     xfract = %f \n",pgstate->xfract);
  printf("     yfract = %f \n",pgstate->yfract);
  printf("   ixwindow = %d \n",pgstate->ixwindow);
  printf("   iywindow = %d \n",pgstate->iywindow);
  printf("       just = %d \n",pgstate->just);
  printf("       axis = %d \n",pgstate->axis);
  printf("       full = %d \n",pgstate->full);
  printf("       xopt = %s \n",pgstate->xopt);
  printf("       yopt = %s \n",pgstate->yopt);
  printf("      xtick = %f \n",pgstate->xtick);
  printf("      ytick = %f \n",pgstate->ytick);
  printf("      nxsub = %d \n",pgstate->nxsub);
  printf("      nysub = %d \n",pgstate->nysub);
  printf("xSchemaName = %s \n",pgstate->xSchemaName);
  printf("ySchemaName = %s \n",pgstate->ySchemaName);
  printf("   plotType = %s \n",pgstate->plotType);
  printf("  plotTitle = %s \n",pgstate->plotTitle);
  printf("       symb = %d \n",pgstate->symb);
  printf("     isLine = %d \n",pgstate->isLine);
  printf("     icMark = %d \n",pgstate->icMark);
  printf("     icLine = %d \n",pgstate->icLine);
  printf("      icBox = %d \n",pgstate->icBox);
  printf("    icLabel = %d \n",pgstate->icLabel);
  printf("    icError = %d \n",pgstate->icError);
  printf("  lineWidth = %d \n",pgstate->lineWidth);
  printf("  lineStyle = %d \n",pgstate->lineStyle);
  printf("  isNewplot = %d \n",pgstate->isNewplot);
  
  return(SH_SUCCESS);
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shPgstateNextWindow
  DESCRIPTION:
  <HTML>
  Make the plot show up in the "next" window
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS
  </HTML>
</AUTO>*/
RET_CODE shPgstateNextWindow(
			     PGSTATE *pgstate /* the PGSTATE to be worked upon */
			     ) {
  float xsize, xborder, ysize, yborder;
  float xmin, xmax, ymin, ymax;
  if (pgstate->full == 0) {
    pgstate->full = 1;
    pgstate->ixwindow = 0;
    pgstate->iywindow = pgstate->nywindow - 1;
  } else {
    pgstate->ixwindow++;
    if (pgstate->ixwindow > pgstate->nxwindow-1) {
      pgstate->ixwindow = 0;
      pgstate->iywindow--;
      if (pgstate->iywindow < 0) {
	pgstate->iywindow = pgstate->nywindow-1;
	cpgpage(); 
      }
    }
  }
  xsize = 1.0 / (float) pgstate->nxwindow;
  xborder = xsize * pgstate->xfract / 2.0;
  xmin = pgstate->ixwindow       * xsize + xborder;
  xmax = (pgstate->ixwindow + 1) * xsize - xborder;

  ysize = 1.0 / (float) pgstate->nywindow;
  yborder = ysize * pgstate->yfract / 2.0;
  ymin = pgstate->iywindow       * ysize + yborder;
  ymax = (pgstate->iywindow + 1) * ysize - yborder;

  cpgsvp(xmin, xmax, ymin, ymax);

  return(SH_SUCCESS);
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shPgstateDel
  DESCRIPTION:
  <HTML>
  Delete a PGSTATE 
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS
  </HTML>
</AUTO>*/
RET_CODE shPgstateDel(
		      PGSTATE *pgstate  /* the PGSTATE to be worked upon */
		      ) {
  if (pgstate != NULL) {
    shFree(pgstate);
  }
  return(SH_SUCCESS);
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shPgstateOpen 
  DESCRIPTION:
  <HTML>
  Open a PGSTATE (xwindow or pgplot.ps file, for example)
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS
  </HTML>
</AUTO>*/
RET_CODE shPgstateOpen(
		       PGSTATE *pgstate /* the PGSTATE to be worked upon */
		       ) {
  int unit=0;  int nx=1;  int ny=1;
  printf("Open %s \n",pgstate->device);
  if ( cpgbeg(unit, pgstate->device, nx, ny) != 1) {
    return(SH_GENERIC_ERROR);
  }
  pgstate->full = 0;
  return(SH_SUCCESS);
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shPgstateTitle
  DESCRIPTION:
  <HTML>
  Put the title on the page.  
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS
  </HTML>
</AUTO>*/
RET_CODE shPgstateTitle(
			PGSTATE *pgstate  /* the PGSTATE to be worked upon */
			) {
  float xmin,  xmax,  ymin,  ymax;
  float xmin0, xmax0, ymin0, ymax0;
  int zero=0;
  char dummy[10];
  strcpy(dummy, " ");
/* Find out what the viewport is and remember this */
  cpgqvp(zero, &xmin, &xmax, &ymin, &ymax);
/* Set viewport to the whole page */
  xmin0=0.1; xmax0=0.9; ymin0=0.1; ymax0=0.9;
  cpgsvp(xmin0,  xmax0, ymin0, ymax0);
/* Put the labels */
  cpglab(dummy, dummy, pgstate->plotTitle);
/* Put back the viewport the way you found it */
  cpgsvp(xmin,  xmax, ymin, ymax);
  return SH_SUCCESS;
}
  
/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shHgPlot
  DESCRIPTION:
  <HTML>
  Plot a HG
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS
  </HTML>
</AUTO>*/
RET_CODE shHgPlot(
		  PGSTATE *pgstate, /* the PGSTATE to plot the HG on */
		  HG *hg, /* the HG to be plotted  */
		  double xminIn,    /* min value for x */
		  double xmaxIn,    /* max value for x */
		  double yminIn,    /* min value for y */
		  double ymaxIn,    /* max value for y */
		  int xyOptMask /* XMINOPT | XMAXOPT, etc., to use above 
				   values, not calculated ones */   
		  ) {
  int center=1;
  int nbin;
  
  float xmin, xmax, ymin, ymax;
  float ydelta;

  int ibin;
  int just, axis;
  int icSave, icTemp;
  int lineWidthSave, lineStyleSave;
  int lineWidthTemp, lineStyleTemp;

  if (pgstate->isNewplot == 1) {
    shPgstateNextWindow(pgstate);

    xmin = hg->minimum;
    xmax = hg->maximum;
    ymin = hg->contents[0];
    ymax = hg->contents[0];
    for (ibin=0; ibin<hg->nbin; ibin++) {
      if (hg->contents[ibin] < ymin) ymin = hg->contents[ibin];
      if (hg->contents[ibin] > ymax) ymax = hg->contents[ibin];
    }
    ydelta = ymax - ymin;
    if (ydelta == 0) {
      ymax = ymax + 1.0; ymin = ymin - 1.0;
    } else {
      ymax = ymax + 0.1*ydelta; ymin = ymin - 0.1*ydelta;
    }

    /* Figure out whether to use the calculated minima and maxima or if the
       user has entered values */
    if (xyOptMask & XMINOPT)
      xmin = xminIn;               /* User entered a value so use it. */
    if (xyOptMask & XMAXOPT)
      xmax = xmaxIn;               /* User entered a value so use it. */
    if (xyOptMask & YMINOPT)
      ymin = yminIn;               /* User entered a value so use it. */
    if (xyOptMask & YMAXOPT)
      ymax = ymaxIn;               /* User entered a value so use it. */

    /* Give the user a warning if the minimum value is greater than the
       maximum.  It is up to the user to decide what to do about it. */
    if (xmin > xmax)
       shErrStackPush("shHgPlot: xmin value is greater than xmax value");
    if (ymin > ymax)
       shErrStackPush("shHgPlot: ymin value is greater than ymax value");

    if (pgstate->just == 0) {
      cpgswin(xmin, xmax, ymin, ymax);
    } else {
      cpgwnad(xmin, xmax, ymin, ymax);
    }
    cpgqci(&icSave);
    icTemp = pgstate->icBox;
    cpgsci(icTemp);
    if (pgstate->isTime == 1) {
      cpgtbox(pgstate->xopt, pgstate->xtick, pgstate->nxsub, 
	     pgstate->yopt, pgstate->ytick, pgstate->nysub);
    } else {
      cpgbox(pgstate->xopt, pgstate->xtick, pgstate->nxsub, 
	     pgstate->yopt, pgstate->ytick, pgstate->nysub);
    }
    cpgsci(icSave);
  }

  just = pgstate->just;
  axis = pgstate->axis;
  nbin = hg->nbin;
  cpgqci(&icSave);
  cpgqls(&lineStyleSave);
  cpgqlw(&lineWidthSave);
  icTemp = pgstate->icLine;
  lineStyleTemp = pgstate->lineStyle;
  lineWidthTemp = pgstate->lineWidth;
  cpgsls(lineStyleTemp);
  cpgslw(lineWidthTemp);
  cpgsci(icTemp);
  cpgbin(nbin, hg->binPosition, hg->contents, center);
  cpgsci(icSave);
  cpgsls(lineStyleSave);
  cpgslw(lineWidthSave);
  
  if(strcmp(hg->xLabel,"none")==0) {
    strcpy(hg->xLabel,"");
  }
  if(strcmp(hg->yLabel,"none")==0) {
    strcpy(hg->yLabel,"");
  }
  if(strcmp(hg->name,"none")==0) {
    strcpy(hg->name,"");
  }

  /* put the text titles */
  if (pgstate->isNewplot == 1) {
    cpgqci(&icSave);
    icTemp = pgstate->icLabel;
    cpgsci(icTemp);
    cpglab(hg->xLabel, hg->yLabel, hg->name);
    cpgsci(icSave);
  }

  strcpy(pgstate->plotType, "HG");
  pgstate->hg = hg;
  strcpy(pgstate->xSchemaName, hg->name);
  strcpy(pgstate->ySchemaName, "ignore");

  return(SH_SUCCESS);
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shPgstateClose
  DESCRIPTION:
  <HTML>
  Close a PGSTATE
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS
  </HTML>
</AUTO>*/
RET_CODE shPgstateClose(
			PGSTATE *pgstate /* the PGSTATE to be closed */
			) {
  printf("Close %s \n",pgstate->device);
  cpgend();
  return(SH_SUCCESS);
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shHgReg
  DESCRIPTION:
  <HTML>
  Put the pixel values of a REGION in a HG 
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS
  </HTML>
</AUTO>*/
RET_CODE shHgReg(
		 HG *hg, /* the HG to be worked upon */
		 REGION *reg, /* the region whose values will be used */
		 int maskBits	/* exclude pixels with matches to this mask */
		 ) {
  int icol, irow;
  if (maskBits == 0) {
    for (icol=0; icol < reg->ncol; icol++) {
      for (irow=0; irow< reg->nrow; irow++) {
	shHgFill(hg, shRegPixGetAsDbl(reg, irow, icol) , (double) 1.0);
      }
    }
    return(SH_SUCCESS);
  }
  if (reg->mask == NULL) {
    shErrStackPush("region has no mask");
    return SH_GENERIC_ERROR;
  }

  for (icol=0; icol < reg->ncol; icol++) {
    for (irow=0; irow< reg->nrow; irow++) {
      if ( !(maskBits & reg->mask->rows[irow][icol]) ) {
	shHgFill(hg, shRegPixGetAsDbl(reg, irow, icol) , (double) 1.0);
      }
    }
  }
  return(SH_SUCCESS);

}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shPtNew
  DESCRIPTION:
  <HTML>
  Make a new point
  </HTML>
  RETURN VALUES:
  <HTML>
  New pt
  </HTML>
</AUTO>*/
PT * shPtNew(
	     void /* void */
	     ) {
  static int id=0;
  PT *pt = (PT *)shMalloc(sizeof(PT));
  *(int *)&(pt->id) = id++;

  pt->row=0;
  pt->col=0;
  pt->radius=0;
  return pt;
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shPtDefine
  DESCRIPTION:
  <HTML>
  Define the values of a PT
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS if successfull
  </HTML>
</AUTO>*/
RET_CODE shPtDefine(
		    PT *pt,      /* pt to work on */
		    float row,   /* row value */
		    float col,   /* column value */
		    float radius /* radius value */
		    ) {
  if (pt != NULL) {
    pt->row = row;
    pt->col = col;
    pt->radius = radius;
    return SH_SUCCESS;
  } else {
    return SH_GENERIC_ERROR;
  }
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shPtPrint
  DESCRIPTION:
  <HTML>
  Simple dump of a PT
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS
  </HTML>
</AUTO>*/
RET_CODE shPtPrint(
		   PT *pt /* unknown point */
		   ) {
  if (pt != NULL) {
    printf("id=%d row=%f col=%f radius=%f \n",
	   pt->id, pt->row, pt->col, pt->radius);
  }
  return SH_SUCCESS;
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shPtDel
  DESCRIPTION:
  <HTML>
  Delete a point
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS
  </HTML>
</AUTO>*/
RET_CODE shPtDel(
		 PT *pt /* pt to delete */
		 ) {
  if (pt != NULL) {
    shFree(pt);
  }
  return SH_SUCCESS;
}






/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shChainFromPlot
  DESCRIPTION:
  <HTML>
  Create a chain by selecting from a plot (HG or V) in X
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS if successfull
  </HTML>
</AUTO>*/

RET_CODE shChainFromPlot(
			 PGSTATE *pgstate,   /* what plot? */
			 CHAIN *outputChain, /* destination chain */
			 CHAIN *inputChain,  /* source chain */
			 VECTOR *vMask       /* mask */
			 ) {
  int nPt;
  unsigned int nLink;
  TYPE firstType;

  PT *pt1=shPtNew();
  PT *pt2=shPtNew();
  void *closestLink;

  if(pt1==NULL || pt2==NULL ||pgstate==NULL)
  {
       if(pt1!=NULL) shPtDel(pt1);
       if(pt2!=NULL) shPtDel(pt2);
       shErrStackPush("shChainFromPlot: NULL pointers passed in");
       return SH_GENERIC_ERROR;
  }

  if ( (strcmp(pgstate->plotType, "VECTOR") != 0) && 
       (strcmp(pgstate->plotType, "HG") != 0)) {
    printf("not a good plotType %s \n",pgstate->plotType);
    shPtDel(pt1);                           /* EFB */
    shPtDel(pt2);                           /* EFB */
    return SH_GENERIC_ERROR;
  }
  nPt = shGetCoord(pgstate, pt1, pt2);
  if (nPt == 0) 
     {
     shPtDel(pt1);                           /* EFB */
     shPtDel(pt2);                           /* EFB */
     return SH_SUCCESS;
     }
/* Fix up pt1 and pt2 to reflect binning, if a HG */
  if (strcmp(pgstate->plotType,"HG") == 0) {
    if (nPt == 1) {
      shPtDefine(pt2, pt1->row, pt1->col, pt1->radius);
    }
    shPtBin(pgstate->hg, pt1, "min");
    shPtBin(pgstate->hg, pt2, "max");
    nPt = 2;
  }
  if (nPt == 1) {
    closestLink = 
      shGetClosestFromChain(inputChain, vMask,
		 pt1, pgstate->xSchemaName, pgstate->ySchemaName);
    if (closestLink != NULL) {
      firstType = shChainElementTypeGetByPos(inputChain, HEAD);
      shChainElementAddByPos(outputChain, closestLink, 
                             shNameGetFromType(firstType), TAIL, AFTER);
    }
    shPtDel(pt1);                           /* EFB */
    shPtDel(pt2);                           /* EFB */
    return SH_SUCCESS;
  } 
  if (nPt == 2) {
    nLink = shAppendIncludedElementsToChain(outputChain, inputChain, 
					 vMask, pt1, pt2, 
					 pgstate->xSchemaName, 
					 pgstate->ySchemaName);
    shPtDel(pt1);                           /* EFB */
    shPtDel(pt2);                           /* EFB */
    return SH_SUCCESS;
  }
  shPtDel(pt1);                           /* EFB */
  shPtDel(pt2);                           /* EFB */
  return SH_GENERIC_ERROR;
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shVIndexFromPlot
  DESCRIPTION:
  <HTML>
  Create an array of points by selecting from a plot (HG or V) in X
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS if successful, vIndex containing index of points in vX, vY
  </HTML>
</AUTO>*/

RET_CODE shVIndexFromPlot(
			  PGSTATE *pgstate,   /* what plot? */
			  VECTOR *vIndex,
			  VECTOR *vX,
			  VECTOR *vY,
			  VECTOR *vMask       /* mask */
			 ) {
  int nPt;
  unsigned int nIncl;

  PT *pt1=shPtNew();
  PT *pt2=shPtNew();
  int closestIndex;

  if(pt1==NULL || pt2==NULL ||pgstate==NULL)  {
    if(pt1!=NULL) shPtDel(pt1);
    if(pt2!=NULL) shPtDel(pt2);
    shErrStackPush("shVIndexFromPlot: NULL pointers passed in");
    return SH_GENERIC_ERROR;
  }
  
  if ( (strcmp(pgstate->plotType, "VECTOR") != 0) != 0) {
    printf("not a good plotType %s \n",pgstate->plotType);
    shPtDel(pt1);                           /* EFB */
    shPtDel(pt2);                           /* EFB */
    return SH_GENERIC_ERROR;
  }
  nPt = shGetCoord(pgstate, pt1, pt2);
  if (nPt == 0) {
    shPtDel(pt1);                           /* EFB */
    shPtDel(pt2);                           /* EFB */
    return SH_SUCCESS;
  }

  if (nPt == 1) {
    closestIndex = shGetClosestFromVectors(vX, vY, vMask, pt1);
    if (closestIndex >= 0) {
      shVectorResize(vIndex, 1);
      vIndex->vec[0] = closestIndex;
    }
    shPtDel(pt1);                           /* EFB */
    shPtDel(pt2);                           /* EFB */
    return SH_SUCCESS;
  } 
  if (nPt == 2) {
    nIncl = shGetIncludedFromVectors(vIndex, vX, vY,
				     vMask, pt1, pt2);

    shPtDel(pt1);                           /* EFB */
    shPtDel(pt2);                           /* EFB */
    return SH_SUCCESS;
  }
  shPtDel(pt1);                           /* EFB */
  shPtDel(pt2);                           /* EFB */
  return SH_GENERIC_ERROR;



}
/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shVIndexFromHg
  DESCRIPTION:
  <HTML>
  Create an array of points by selecting from an HG
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS if successful, vIndex containing index of points in vX
  </HTML>
</AUTO>*/

RET_CODE shVIndexFromHg(
			PGSTATE *pgstate,   /* what plot? */
			VECTOR *vIndex,
			VECTOR *vX,
			VECTOR *vMask       /* mask */
			) {
  int nPt;
  unsigned int nIncl;

  PT *pt1=shPtNew();
  PT *pt2=shPtNew();

  if(pt1==NULL || pt2==NULL ||pgstate==NULL)  {
    if(pt1!=NULL) shPtDel(pt1);
    if(pt2!=NULL) shPtDel(pt2);
    shErrStackPush("shVIndexFromHg: NULL pointers passed in");
    return SH_GENERIC_ERROR;
  }
  
  if ( (strcmp(pgstate->plotType, "HG") != 0) != 0) {
    printf("not a good plotType %s \n",pgstate->plotType);
    shPtDel(pt1);                           /* EFB */
    shPtDel(pt2);                           /* EFB */
    return SH_GENERIC_ERROR;
  }
  nPt = shGetCoord(pgstate, pt1, pt2);
  if (nPt == 0) {
    shPtDel(pt1);                           /* EFB */
    shPtDel(pt2);                           /* EFB */
    return SH_SUCCESS;
  }

  if (nPt == 1) {
    shPtDefine(pt2, pt1->row, pt1->col, pt1->radius);
  }
  shPtBin(pgstate->hg, pt1, "min");
  shPtBin(pgstate->hg, pt2, "max");
  nPt = 2;
  nIncl = shGetIncludedFromVector(vIndex, vX, vMask, pt1, pt2);
  shPtDel(pt1);                           /* EFB */
  shPtDel(pt2);                           /* EFB */
  return SH_SUCCESS;
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shPtBin
  DESCRIPTION:
  <HTML>
  Get a PT from a histogram bin; min or max value
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS, point in pt
  </HTML>
</AUTO>*/
RET_CODE shPtBin(
		 HG *hg, /* hg to copy from */
		 PT *pt, /* pt to get */
		 char *minORmax /* min or max value */
		 ) {
  int bin;
  float binsiz;
  binsiz = (hg->maximum - hg->minimum) / hg->nbin;
  if ((pt->col > hg->minimum) || (pt->col < hg->maximum)) {
    bin = hg->nbin * (pt->col-hg->minimum) / (hg->maximum - hg->minimum);
    pt->col = hg->binPosition[bin] - binsiz/2.0;
    if (!strcmp(minORmax, "max")) { pt->col += binsiz; }
  }
  return SH_SUCCESS;
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shGetClosestFromChain
  DESCRIPTION:
  <HTML>
  Return the thing on the chain closest to the picked point
  </HTML>
  RETURN VALUES:
  <HTML>
  void, point returned in pt
  </HTML>
</AUTO>*/

void *shGetClosestFromChain(
			    CHAIN *inputChain, /* chain to use */
			    VECTOR *vMask, /* what part? */
			    PT *pt,        /* point to return */
			    char *xName,   /* name of x value */
			    char *yName    /* name of y value */
			    ) {
  void *closestItem=NULL;
  float thisX, thisY, delta, deltaMin;
  void *voidLink, *item, *element;
  TYPE type, firstType;
  const SCHEMA *schema;
  const SCHEMA_ELEM* sch_elemX = NULL, *sch_elemY = NULL;
  unsigned iItem;
  int isGood, isFirst, chainSize;
    
  if(inputChain==NULL|| pt==NULL ||xName==NULL || yName==NULL)
  {
      shErrStackPush("shGetClosestFromChain: NULL pointers passed in");
      return NULL;
  }
    
  if(vMask!=NULL)
  {   
    if(vMask->vec==NULL)
      {
          shErrStackPush("shGetClosest: vMask has no data");
          return NULL;
       }
  }


/* see if these names exist in the schema */
  voidLink = NULL;
  item = shChainElementGetByPos(inputChain, HEAD);
  type = shChainElementTypeGetByPos(inputChain, HEAD);
  firstType = type;
  schema = (SCHEMA *) shSchemaGet(shNameGetFromType(type));
  if (schema == NULL) {
    shErrStackPush("shGetClosest:  can not get schema for %s\n",
		   shNameGetFromType(type));
    return NULL;
  }

  sch_elemX = shSchemaElemGet((char*)schema->name, xName);
  sch_elemY = shSchemaElemGet((char*)schema->name, yName);
  
  if ( sch_elemX==NULL || sch_elemY==NULL ) {
    shErrStackPush("shGetClosest:  %s and/or %s not in %s schema\n",
		   xName, yName, schema->name);
    return NULL;
  }

/* zip through the chain and pick out the closest */

  isFirst = 1;
  deltaMin = 0;
  chainSize = shChainSize(inputChain);
  for (iItem=0; iItem < chainSize; iItem++) {
    if (vMask==NULL) {isGood=1;} else {isGood=vMask->vec[iItem];}
    if (isGood == 1) {
      item = shChainElementGetByPos(inputChain, iItem);
            element = shElemGet(item, (SCHEMA_ELEM*)sch_elemX, &type);
      sscanf(shPtrSprint(element, type), "%f", &thisX);
      element = shElemGet(item, (SCHEMA_ELEM*)sch_elemY, &type);
      sscanf(shPtrSprint(element, type), "%f", &thisY);
      delta = pow( (thisX-pt->col), 2) + pow( (thisY-pt->row), 2);
      if ( (isFirst==1) || (delta<deltaMin)) {
	isFirst = 0;
	closestItem = item;
	deltaMin = delta;
      }
    }
  }
  if (isFirst == 1) {
    shErrStackPush("shGetClosestFromChain:  nothing on the chain\n");
    closestItem = NULL;
  }
  return closestItem;
}
/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shGetClosestFromVectors
  DESCRIPTION:
  <HTML>
  Return the thing on the vector closest to the picked point
  </HTML>
  RETURN VALUES:
  <HTML>
  index of closest point in vectors as int, or -1 if error.
  </HTML>
</AUTO>*/

int shGetClosestFromVectors(
			    VECTOR *vX, /* x vector to use */
			    VECTOR *vY, /* y vector to use */
			    VECTOR *vMask, /* what part? */
			    PT *pt        /* point to return */
			    ) {
  unsigned iItem;
  int closestItem=0;
  VECTOR_TYPE delta, deltaMin;
  int isGood, isFirst;
    
  if(vX==NULL|| vY==NULL|| pt==NULL)
  {
      shErrStackPush("shGetClosestFromVectors: NULL pointers passed in");
      return -1;
  }

  if(vX->dimen!=vY->dimen) {
    shErrStackPush("shGetClosestFromVectors: vectors are different sizes");
    return -1;
  }
    
  if(vMask!=NULL)
  {   
    if(vMask->vec==NULL) {
      shErrStackPush("shGetClosest: vMask has no data");
      return -1;
    }
    if(vMask->dimen!=vX->dimen) {
      shErrStackPush("shGetClosestFromVectors: vMask must match vectors in size");
      return -1;
    }
  }

  /* zip through the vectors and pick out the closest */

  isFirst = 1;
  deltaMin = 0;
  for (iItem=0; iItem < vX->dimen; iItem++) {
    if (vMask==NULL) {
      isGood=1;
    } else {
      isGood=vMask->vec[iItem];
    }
    if (isGood == 1) {
      delta = pow( (vX->vec[iItem] - pt->col), 2) +
	      pow( (vY->vec[iItem] - pt->row), 2);
      if ( (isFirst==1) || (delta<deltaMin)) {
	isFirst = 0;
	closestItem = iItem;
	deltaMin = delta;
      }
    }
  }
  if (isFirst == 1) {
    shErrStackPush("shGetClosestFromVectors:  nothing in the vectors\n");
    closestItem = -1;
  }
  return closestItem;
}


/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shGetCoord
  DESCRIPTION:
  <HTML>
  Return coordinate of points picked from pgstate X window.
  Left button picks closest point.
  Middle button picks first and second corner of rectangle.
  Right button cancels operation.
  </HTML>
  RETURN VALUES:
  <HTML>
  number telling what button, or 0
  </HTML>
</AUTO>*/
int shGetCoord(
	       PGSTATE *pgstate, /* what plot */
	       PT *pt1, /* first button point */
	       PT *pt2  /* second button point */
	       ) {
  unsigned short nPick=0;
  char cReturn[2];
/* This is the character returned when the mouse buttons are clicked in 
   a pgstate window.  It does not detect whether the SHIFT or CONTROL 
   keys are also depressed. */
  char leftButton[2] = "A";
  char middleButton[2] = "D";
  char rightButton[2] = "X";
  float xReturn=0.0, yReturn=0.0;
  printf("Waiting for cursor input. Press \'h\' for help.\n");
  while (cReturn[0] != rightButton[0]) {
    strcpy(cReturn,"0");
    cpgupdt();
    cpgcurs( &xReturn, &yReturn, cReturn);
    cpgupdt();
    if (cReturn[0] == 'h') {
        printf("Left button picks closest point.\n"
	       "Middle button picks first and second corner of rectangle.\n"
	       "Right button cancels operation.\n");
    }
    if (cReturn[0] == leftButton[0]) {
      pt1->col = xReturn;
      pt1->row = yReturn;
      pt1->radius = 0;
      pt2->row = pt2->col = pt2->radius = 0;
      return 1;
    }
    if (cReturn[0] == middleButton[0]) {
      if (nPick == 0) {
	nPick++;
	pt1->col = xReturn;
	pt1->row = yReturn;
	pt1->radius = 0;
	printf("Waiting for second point.\n");
      } else {
	pt2->col = xReturn;
	pt2->row = yReturn;
	pt2->radius = 0;
	return 2;
      }
    }
  }
/* right button selected so bail out returning blanks */
  pt1->row = pt1->col = pt1->radius = 0;
  pt2->row = pt2->col = pt2->radius = 0;
  return 0;
}


/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shChainToSao 
  DESCRIPTION:
  <HTML>
  Plot row and col values in a chain in saoDisplay window
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS if successfull
  </HTML>
</AUTO>*/
RET_CODE shChainToSao(ClientData clientData, /* window info */
		      CHAIN *tchain, /* chain to plot */
		      char *rowName, /* name of row */
		      char *colName, /* name of column */
		      char *radiusName /* name of radius */
		      ) {
  int nItem, iSchema;
  void  *voidLink, *item, *element;
  const SCHEMA *schema;
  TYPE type, firstType;
  char *name;
  RET_CODE rstatus;
  unsigned int iSchemaRow=0, iSchemaCol=0, iSchemaRadius=0;
  float row, col, radius;
#define PARAMS_SIZE 2048
  char params[PARAMS_SIZE];
  int exclude_flag, annulus, saoindex;
  CURSOR_T crsr;

  nItem = shChainSize(tchain);
  printf("in shChainToSao:  Number of elements to draw is %d \n",nItem);
  if (nItem <= 0) return SH_SUCCESS;

  voidLink = NULL;
  item = shChainElementGetByPos(tchain, HEAD);
  type = shChainElementTypeGetByPos(tchain, HEAD);
  if ((schema = 
       (SCHEMA *)
       shSchemaGet(shNameGetFromType(type))) == NULL) {
    printf("Can't get schema for %s\n", shNameGetFromType(type));
    return SH_GENERIC_ERROR;
  }
  firstType = type;

  for (iSchema=0; iSchema < schema->nelem; iSchema++) {
    name = schema->elems[iSchema].name;
    if ( strcmp(name, rowName) == 0) iSchemaRow = iSchema;
    if ( strcmp(name, colName) == 0) iSchemaCol = iSchema;
    if ( strcmp(name, radiusName) == 0) iSchemaRadius = iSchema;
  }

  if ((iSchemaRow==0) || (iSchemaCol==0)) return SH_GENERIC_ERROR;
  if (iSchemaRadius==0) {
    radius=1.0;
    sscanf(radiusName, "%f", &radius);
  }

  /* Get a cursor to enable us to walk through the chain */
  crsr = shChainCursorNew(tchain);

  voidLink = NULL;
  params[0] = '\0';
  while ((item = shChainWalk(tchain, crsr, NEXT)) != NULL) {
    type = firstType;
    element = shElemGet(item, &schema->elems[iSchemaRow], &type);
    sscanf(shPtrSprint(element, type), "%f", &row);

    type = firstType;
    element = shElemGet(item, &schema->elems[iSchemaCol], &type);
    sscanf(shPtrSprint(element, type), "%f", &col);

    if (iSchemaRadius>0) {
      type = firstType;
      element = shElemGet(item, &schema->elems[iSchemaRadius], &type);
      sscanf(shPtrSprint(element, type), "%f", &radius);
    }

    sprintf(params, "(%d,%d,%d)", (int) col, (int) row, (int) radius);
    saoindex = 0;
    exclude_flag = 0;
    annulus = 0;
    
    rstatus = saoDrawWrite(&(g_saoCmdHandle.saoHandle[saoindex]), D_CIRCLE, 
			   exclude_flag, annulus, params);
  }

  /* Get rid of the cursor */
  (void )shChainCursorDel(tchain, crsr);
  return SH_SUCCESS;
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shRegFluctuateAsSqrt
  DESCRIPTION:
  <HTML>
  Add noise to a regions; simple poisson model.
  </HTML>
  RETURN VALUES:
  <HTML>
  0 as int
  </HTML>
</AUTO>*/
int shRegFluctuateAsSqrt(
			 REGION *regPtr, /* the region */
			 double gain     /* the gain in electrons per ADU */
			 ) {
  int irow, icol;		/* step through rows and columns */
  double value;			/* the value of a pixel */
  double r1, r2;		/* two random numbers */
  double width;			/* width of the distribution for this pixel */
  double delta;			/* how much to change this pixel by */
  for (icol=0; icol<regPtr->ncol; icol++) {
    for (irow=0; irow<regPtr->nrow; irow++){
      value = gain * shRegPixGetAsDbl(regPtr, irow, icol);
      width = sqrt(value);
      r1 = (double) (rand()) / (double) (RAND_MAX);
/* r2 is not allowed to be zero */
      r2=0;
      do {
	r2 = (double) (rand()) / (double) (RAND_MAX);
      } while (r2 == 0);
      delta = sin(3.14159*2.0*r1) * sqrt(-2.0*log(r2));
      value = value + width*delta;
      value = value/gain;
      shRegPixSetWithDbl(regPtr, irow, icol, value);
    }
  }
  return 0;
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shAppendIncludedElementsToChain
  DESCRIPTION:
  <HTML>
  Append the elements in the plot that are included in the box
  bounded by the two points.
  </HTML>
  RETURN VALUES:
  <HTML>
  Number of winners (??) or 0 upon error
  </HTML>
</AUTO>*/
unsigned int shAppendIncludedElementsToChain(
	CHAIN *outputChain, /* output chain */
	CHAIN *inputChain, /* input chain */
	VECTOR *vMask, /* mask on chains */
	PT *pt1, /* point 1 */
	PT *pt2, /* point 2 */
	char *xName, /* name of x */
	char *yName /* name of y */
        ) {
  float thisX, thisY;
  float minX, maxX, minY, maxY;
  void *item, *element;
  TYPE type, firstType;
  const SCHEMA *schema;
  const SCHEMA_ELEM *sch_elemX, *sch_elemY;
  unsigned int iItem, nItem;
  unsigned int iWinner;
  int isGood, isIncluded;
  
  if(inputChain==NULL || xName == NULL || yName==NULL)
  {
       shErrStackPush("shAppendIncludedElementsToChain: NULL pointers passed in");
       return 0;
  }

  if(vMask!=NULL)
  {   
      if(vMask->vec==NULL)
      {
          shErrStackPush("shAppendIncludedElementsToChain: vMask has no data");
          return 0;
       }
  }

/* see if these names exist in the schema */
  item = shChainElementGetByPos(inputChain, HEAD);
  type = shChainElementTypeGetByPos(inputChain, HEAD);

  if ((schema = shSchemaGet(shNameGetFromType(type))) == NULL) {
    shErrStackPush("shAppendIncludedElementsToChain:  cant get schema for %s\n",
		   shNameGetFromType(type));
    return 0;
  }
  firstType = type;
  sch_elemX = shSchemaElemGet((char*)schema->name, xName);
  sch_elemY = shSchemaElemGet((char*)schema->name, yName);

  if ( (sch_elemX==NULL) || 
      ((sch_elemY==NULL) && 
       strcmp(yName, "ignore"))) return 0;

/* get the min and max all squared away */
  minX = minimum(pt1->col, pt2->col);
  maxX = maximum(pt1->col, pt2->col);
  minY = minimum(pt1->row, pt2->row);
  maxY = maximum(pt1->row, pt2->row);

/* zip through the chain and add the winners to the output chain */
  iWinner = 0;
  nItem = shChainSize(inputChain);
  for (iItem=0; iItem < nItem; iItem++) {
    item = shChainElementGetByPos(inputChain, iItem);
    if (vMask==NULL) {isGood=1;} else { isGood=vMask->vec[iItem];}
    if (isGood==1) {
      element = shElemGet(item, (SCHEMA_ELEM*)sch_elemX, &type);
      sscanf(shPtrSprint(element, type), "%f", &thisX);
      if (sch_elemY==NULL) {
	thisY = minY;
      } else {
	element = shElemGet(item, (SCHEMA_ELEM*)sch_elemY, &type);
	sscanf(shPtrSprint(element, type), "%f", &thisY);
      }
      isIncluded=(included(minX, thisX, maxX) && included(minY, thisY, maxY));
      if (isIncluded) {
	shChainElementAddByPos(outputChain, item, shNameGetFromType(firstType),
			       TAIL, AFTER);
	iWinner++;
      }
    }
  }
  return iWinner;
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shGetIncludedFromVectors
  DESCRIPTION:
  <HTML>
  Append the elements in the plot that are included in the box
  bounded by the two points.
  </HTML>
  RETURN VALUES:
  <HTML>
  Number of winners (??) or 0 upon error
  </HTML>
</AUTO>*/
unsigned int shGetIncludedFromVectors(
	VECTOR *vIndex, /* output vector */
	VECTOR *vX, /* input vector */
	VECTOR *vY, /* input vector */
	VECTOR *vMask, /* mask on vectors */
	PT *pt1, /* point 1 */
	PT *pt2 /* point 2 */
        ) {
  VECTOR_TYPE minX, maxX, minY, maxY;
  unsigned int iItem;
  unsigned int iWinner;
  int isGood, isIncluded;
  
  if(vX==NULL || vY == NULL)
  {
       shErrStackPush("shGetIncludedFromVectors: NULL pointers passed in");
       return 0;
  }

  if(vX->dimen!=vY->dimen) {
    shErrStackPush("shGetIncludedFromVectors: vectors are different sizes");
    return -1;
  }

  if(vMask!=NULL)    {   
    if(vMask->vec==NULL) {
      shErrStackPush("shGetIncludedFromVectors: vMask has no data");
      return 0;
    }
    if(vMask->dimen!=vX->dimen) {
      shErrStackPush("shGetIncludedFromVectors: vMask must match vectors in size");
      return 0;
    }
  }
  /* get the min and max all squared away */
  minX = minimum(pt1->col, pt2->col);
  maxX = maximum(pt1->col, pt2->col);
  minY = minimum(pt1->row, pt2->row);
  maxY = maximum(pt1->row, pt2->row);
  
  /* zip through the vector and add the winners to the output vector */
  iWinner = 0;
  /* make the vector as big as ever needed */
  shVectorResize(vIndex, vX->dimen);
  for (iItem=0; iItem < vX->dimen; iItem++) {
    if (vMask==NULL) {
      isGood=1;
    } else { 
      isGood=vMask->vec[iItem];
    }
    if (isGood==1) {
      isIncluded=(included(minX, vX->vec[iItem], maxX) && 
		  included(minY, vY->vec[iItem], maxY));
      if (isIncluded) {
	vIndex->vec[iWinner]=iItem;
	iWinner++;
      }
    }
  }
  /* now clip it to the right size */
  shVectorResize(vIndex, iWinner);
  return iWinner;
}
/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shGetIncludedFromVector
  DESCRIPTION:
  <HTML>
  Append the elements in the plot that are included in the bin
  bounded by the two points.
  </HTML>
  RETURN VALUES:
  <HTML>
  Number of winners (??) or 0 upon error
  </HTML>
</AUTO>*/
unsigned int shGetIncludedFromVector(
	VECTOR *vIndex, /* output vector */
	VECTOR *vX, /* input vector */
	VECTOR *vMask, /* mask on vX */
	PT *pt1, /* point 1 */
	PT *pt2 /* point 2 */
        ) {
  VECTOR_TYPE minX, maxX;
  unsigned int iItem;
  unsigned int iWinner;
  int isGood, isIncluded;
  
  if(vX==NULL)  {
    shErrStackPush("shGetIncludedFromVector: NULL pointers passed in");
    return SH_GENERIC_ERROR;
  }

  if(vMask!=NULL)    {   
    if(vMask->vec==NULL) {
      shErrStackPush("shGetIncludedFromVector: vMask has no data");
      return SH_GENERIC_ERROR;
    }
    if(vMask->dimen!=vX->dimen) {
      shErrStackPush("shGetIncludedFromVector: vMask, vX  must be same size");
      return 0;
    }
  }
  /* get the min and max all squared away */
  minX = minimum(pt1->col, pt2->col);
  maxX = maximum(pt1->col, pt2->col);
  
  /* zip through the vector and add the winners to the output vector */
  iWinner = 0;
  /* make the vector as big as ever needed */
  shVectorResize(vIndex, vX->dimen);
  for (iItem=0; iItem < vX->dimen; iItem++) {
    if (vMask==NULL) {
      isGood=1;
    } else { 
      isGood=vMask->vec[iItem];
    }
    if (isGood==1) {
      isIncluded=(included(minX, vX->vec[iItem], maxX));
      if (isIncluded) {
	vIndex->vec[iWinner]=iItem;
	iWinner++;
      }
    }
  }
  /* now clip it to the right size */
  shVectorResize(vIndex, iWinner);
  return iWinner;
}

int doubleCompare(const void *d1, const void *d2) {
  double *v1, *v2;
  v1 = (double *) d1;
  v2 = (double *) d2;
  return (*v1<*v2) ? -1 : (*v1>*v2) ? 1: 0;
}








