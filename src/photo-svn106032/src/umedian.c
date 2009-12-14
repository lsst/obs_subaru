/*
 * <AUTO>
 *
 * FILE: umedian.c
 *
 * DESCRIPTION: This file contains a routine to estimate the median of
 * undersampled data.

 * </AUTO> */

/*
 * For now, we'll put contributed code here
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "atConversions.h"		/* for M_PI, M_SQRT2 */
#include "phMath.h"
#include "phUtils.h"
#include "phHist.h"
#include "phLibrary.h"
#include "phMathUtils.h"

#undef DEBUG
#undef ALLBINS			/* Use all bins for fit */

static PHHIST *hist = NULL; 
static int hstart = 0, hend = 0, hsum = 0;

/* Set to 1/0 by phInitGaussFit/phFiniGaussFit */
static int initialized_erf = 0;    

/* Stores erf values over range large enough for fitting */
static double *erf_array;

/*****************************************************************************/
/*
 * <EXTRACT AUTO>
 * Check if erf array is initialized.
 */
int
phGaussFitIsInitialised(void)
{
    return(initialized_erf == 0 ? 0 : 1);
}



/* Allocate and fill erf array */

#define MAXERF    3.5          /* for x > MAXERF, approx erf(x) = 1.0 */
#define MINERF    0.0-MAXERF   /* for x < MINERF, approx erf(x) = -1.0 */
#define NPTERF 50000.0     /* make grid of erf values have this many */
                           /* entries between each integer */

int
phInitGaussFit(void)
{
   int i, npt;
   double start, end;
   double x, dx;

   if(!initialized_erf) {
      initialized_erf = 1;
      npt = (int) (MAXERF*NPTERF);
      start = 0.0;
      end = MAXERF;
      shAssert((npt > 0) && (end > start));
      erf_array = (double *) shMalloc(npt*sizeof(double));
      dx = (end - start)/npt;
      for (i = 0, x = start; i < npt; i++, x += dx) {
	 erf_array[i] = erf(x);
      }
   }
   
   return(SH_SUCCESS);
}

/*
 * de-allocate the static arrays we've allocated in this file.  This must
 * be called when we are about to terminate the pipeline.
 */

int
phFiniGaussFit(void)
{

   if (initialized_erf == 1) {
      initialized_erf = 0;

      shFree(erf_array);
   }
   return(SH_SUCCESS);
}

#define MYERF(x) (x <= MINERF ? -1.0 : \
                  (x < 0 ? 0.0-erf_array[(int)((0.0-x)*NPTERF+0.5)] : \
                  (x >= MAXERF ? 1.0 : \
                   erf_array[(int)(x*NPTERF+0.5)])))

#define OLDMYERF(x) (x <= MINERF ? -1.0 : \
                  (x < 0 ? 0.0-erf_array[(int)((0.0-x)*NPTERF)] : \
                  (x >= MAXERF ? 1.0 : \
                   erf_array[(int)(x*NPTERF)])))




/***********************************************************************/
/* Estimate median of bias by fitting erf to narrow histogram of data. */
/* Data is stored in static variables phhist, hstart, and hend, which  */
/* indicate the part of the histogram to fit.                          */

static void
fiterf(int ndata,		/* Number of data points */
       int npar,		/* Number of parameters */
       double params[],		/* Parameters to be fit */
       double residuals[],	/* Vector of (ndata) residuals */
       int *iflag)
{

    int bin, i=0;
    float mean, sigma;
    float x, x1, x2, start, end;
    double g;

    shAssert(*iflag >= 0 && npar == 2);

    mean = params[0];
    sigma = params[1]*M_SQRT2;

    bin = hstart;
    start = hstart + hist->min_bin - 0.5;
    end = hend + hist->min_bin - 0.5;
    for (x = (float)start; x <= (float)end; x++) {
	x1 = x - mean;
	x2 = x1 + 1;
#if 0
	g = hsum*(erf(x2/sigma)-erf(x1/sigma))/2.;
#else
	g = hsum*(MYERF(x2/sigma)-MYERF(x1/sigma))/2.;
/*	printf("%f\n",erf(x2/sigma)/MYERF(x2/sigma));*/
#endif
	if (hist->value[bin] == 0) {
	    residuals[i] = 0;
	} else {
	    residuals[i] = (g-hist->value[bin])/sqrt(hist->value[bin]);
	}
	bin++;
	i++;
    }
    shAssert(i == ndata);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * ROUTINE: phHistU16StatsFit
 * 
 * DESCRIPTION: Given an histU16, presumably with undersampled data,
 * estimate the quartiles of the data by fitting a Gaussian to the
 * peak.  This routine assumes that the data values have a Gaussian
 * distribution, except for outliers.
 *
 * Return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 *
 * </AUTO> */

int
phHistU16StatsFit(PHHIST *in_hist, float *fit_quartiles, float *fit_sigma, int *error)
{

  int i, nfit, ret, nfev, bin=0, medbin, nbins;
  float quartiles[3], mean, mode, sigma, diff;
  double norm=-1e10, params[2], tol = 1e-6, sumsq_tol;

  hist = in_hist;
  phHistU16Stats(hist,0,quartiles,&mean,&mode,&sigma);


#ifdef ALLBINS

  hsum = hist->total_number;
  hstart = 0;
  hend = hist->num_bin - 1;

#else 
  /* We only want to fit the part of the histogram around the peak
   * where all the bins are non-zero. We assume that the median 
   * determined above is either the peak itself or connected to the 
   * peak by occupied bins. Then we look at the surrounding bins until
   * we find empty bins on either side.
   */

  medbin = quartiles[1] - hist->min_bin + 0.5;
  bin = medbin;
  hsum = 0;
  while (hist->value[bin] != 0 && bin >= 0) {
    hsum += hist->value[bin];
    bin--;
  }
  hstart = bin + 1;

  bin = medbin + 1;
  while (hist->value[bin] != 0 && bin < hist->num_bin) {
    hsum += hist->value[bin];
    bin++;
  }
  hend = bin - 1;

#endif
  
  nbins = hend - hstart + 1;
  params[0] = quartiles[1];
  params[1] = sigma;
  nfit = 2;
  sumsq_tol = nbins - nfit;
  ret = phLMMinimise(fiterf, nbins, nfit, params, &norm, &nfev,
		     tol, sumsq_tol, 0, NULL);

#ifdef DEBUG
  printf("c = %d  q = %9.2f %9.2f %9.2f  m = %9.2f s = %9.2f\n", *error,
	 quartiles[0], quartiles[1], quartiles[2], mean, sigma);
  printf("\tfit-> %d  %9.2f  %9.2f\n",ret, params[0], params[1]);
#endif

  if (ret == 0) {
    shDebug(0,"phRegUMedianByCol: bad inputs to phLMMinimise");
    for (i = 0; i < 3; i++) {
      fit_quartiles[i] = quartiles[i];
    }
    *fit_sigma = sigma;
    *error = 1;
  } else if (ret & ~07) {
    shDebug(0,"phRegUMedianByCol: bad outputs from phLMMinimise");
    for (i = 0; i < 3; i++) {
      fit_quartiles[i] = quartiles[i];
    }
    *fit_sigma = sigma;
    *error = 2;

  } else {
    diff = params[1] / 1.518;	/* fraction from library.c, hope it's ok */
    fit_quartiles[1] = params[0];
    fit_quartiles[0] = params[0] - diff;
    fit_quartiles[2] = params[0] + diff;
    *fit_sigma = params[1];
    *error = 0;
  }

  return(SH_SUCCESS);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * ROUTINE: phRegMeanFitByCol 
 * 
 * DESCRIPTION: Given a N-row x M-col REGION, and an empty N-row x
 * 1-col REGION, fill the Nx1 region with values which are the median
 * value of the pixels in each column of the original. This routine
 * assumes that the data values have a Gaussian distribution, except
 * for outliers.  It finds the median by making a histogram of the
 * pixel values and chi-square fitting the nbins bins around the peak
 * to a gaussian.
 *
 * Return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 *
 * </AUTO> */

int
phRegMeanFitByCol(
   REGION *reg,          /* I: NxM region from which we calculate medians */
   int bias_scale,	 /* I: amount by which to scale the result */
   REGION *vector        /* O: Nx1 region into which we place medians */
		      
   )
{
  int col, error;
  float fit_quartiles[3], fit_sigma;
   REGION *sub;

   if (vector->ncol != reg->ncol) {
      shError("phRegMeanFitByCol: reg and vector 'nrow' don't match");
      return(SH_GENERIC_ERROR);
   }
   if (vector->nrow != 1) {
      shError("phRegMeanFitByCol: output REGION does not have nrow = 1");
      return(SH_GENERIC_ERROR);
   }
   
   for (col = 0; col < reg->ncol; col++) {   
       sub = shSubRegNew("umedian",reg,reg->nrow,1,0,col,NO_FLAGS);

#ifdef DEBUG
       error = col;
#endif
       /* Get histogram of region */
       hist = phHistU16FromRegion(sub,0);

       if (phHistU16StatsFit(hist, fit_quartiles, &fit_sigma, &error) 
							       == SH_SUCCESS) {
	 vector->ROWS[0][col] = FLT2PIX(fit_quartiles[1]*bias_scale);
       } else {
	 vector->ROWS[0][col] = 0;
       }

       phPhhistDel(hist);
       shRegDel(sub);

   }

   return(SH_SUCCESS);
}











