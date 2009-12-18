/* <AUTO>
  FILE: atRegSci
<HTML>
  Science routines that are applied to REGIONs
</HTML>
  </AUTO>
*/

/*
** ---------------------------------------------------------------------------
** atRegGaussianAdd             public  Rather self explanatory.
** atRegMap                     public  Magnify, rotate and shift a region.
** atRegModeFind                public  Find the mode of an integer region.
** atRegMedianFindAsDbl		public  Find the median pix val in a region.
** atRegStatsFind               public  Finds the mean and standard deviation.
** atRegConvolve                public  Convolves a template region with a 
**                                      region.  Doesn't use a FFT.
** atRegSkyFind                 public  Finds the sky background by a Gaussian
**                                      fit to the region histogram.
** atRegBin                     public  Bins n by m pixels into one pixel. 
**                                      Throws out all extra pixels.
** atRegMedianFind              public  Makes a median region of a list of 
**                                      input regions.
** atRegClippedMeanFind         public  For each pixel in list of input regions,
**                                      calculates clipped mean and sets output
**                                      region's pixel to that value.
** atRegSigmaClip               public  iterate, find a clip sigma mean/stddev
** atRegMedianFindByColumn      public  Given an NxM region, create Nx1 region
**                                      which has median of each col 
** atRegMedianFindByRow         public  Given an NxM region, create 1xM region
**                                      which has median of each row 
** atRegPixValSubstitute        public  replace all occurences of a particular 
**                                      pixel value
** atRegCorrectWithList        public   Apply a non-linearlity Correction to
**                                      a region, by multiplication
** atRegCorrectQuick           public   Apply a non-linearlity Correction to
**                                      a region by multiplication; a special
**                                      case for U16 regions and speed.
**
** Conditional compilation is controlled by the following macros:
**
**	none
**
** AUTHORS:	Creation date: May 5, 1993
**		Heidi Newberg
**              Bradford Holden
**
** MODIFICATIONS:
**		Heidi Newberg: regMap now takes angles in degrees May 1996
**		Heidi Newberg: Moved to ASTROTOOLS	Oct 1995
**		Heidi Newberg: Added regMedianFindAsDbl Oct 1995
**		Heidi Newberg: Added documentation	Oct 1995
**              Michael Richmond: added atMedianFindByColumn   Mar 1996
**                                      atMedianFindByRow
**              Michael Richmond: added atRegClippedMeanFind   Apr 1996
**              Michael Richmond: added atRegPixValuSubstitute Apr 1996
**              Michael Richmond: atRegMedianFindByRow/Column now have
**                                      special cases for input/output 
**                                      type FL32 and U16      Aug 1996
**              Ron Kollgaard: regCorrectQuick bugs fixed March 1997
**              Ron Kollgaard: regClippedMeanFind bugs fixed March 1997
**              Ron Kollgaard: fixed error handling, etc from code review Apr 97
**
******************************************************************************
*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include "tcl.h"
#include "ftcl.h"
#include "dervish.h"
#include "atRegSci.h"
#include "atAperture.h"
#include "sdssmath.h"
#include "alloca.h"
#include "atConversions.h"

/*********************** statics local to this module ************************/

/* Given a region and a location within a region, this function returns the
   linearly interpolated value for the rebinned pixel centered at this
   location.  The routine assumes the values of y and x must be greater than
   0.5 and less than the size of the region minus 0.5 */

static double atRegPixRebin (const REGION *reg, const double y, const double x) 
{

/**   int rlower, clower, r, c;   don't use r or c */
  int rlower, clower;
  double ylower, xlower, pix1, pix2, pix3, pix4, t, u, bilininterp;

  shAssert(y > 0.5);
  shAssert(x > 0.5);
  shAssert(y < reg->nrow - 0.5);
  shAssert(x < reg->ncol - 0.5);

/*   r = (int) floor(y);    don't use */
/*   c = (int) floor(x);    don't use */

  ylower = floor(y-0.5) + 0.5;
  xlower = floor(x-0.5) + 0.5;
  rlower = (int) floor(ylower);
  clower = (int) floor(xlower);
  
  t = y - ylower;
  u = x - xlower;

  pix1 = shRegPixGetAsDbl(reg, rlower, clower);
  pix2 = shRegPixGetAsDbl(reg, rlower+1, clower);
  pix3 = shRegPixGetAsDbl(reg, rlower+1, clower+1);
  pix4 = shRegPixGetAsDbl(reg, rlower, clower+1);

  bilininterp = (1-t)*(1-u)*pix1 + t*(1-u)*pix2 + t*u*pix3 + (1-t)*u*pix4;

  return (bilininterp);

}



static double Gaussian(const double dx, const double dy, const double A, const double sigma_x, const double sigma_y) 
{

	double G;

	G = A * (exp( -(dx*dx/(2*sigma_x*sigma_x)+dy*dy/(2*sigma_y*sigma_y)) ));

	return (G);
}



/* Get the pixel values of integer regions as U32.  This is not defined
   for floating point regions.  Conversions from unsigned types are
   straightforward.  Conversions from signed types preserve the order of
   the integers rather than the magnitude of the integers.  This facilitates
   functions like histogramming the pixels using common subroutines. */

static unsigned int atRegPixGetAsU32 (const REGION *reg, const int r, const int c) 
{

	switch (reg->type) {
	   case TYPE_U8:	return((unsigned int) reg->rows_u8[r][c]);
	   case TYPE_S8:	return(0x00000080 
				^ (unsigned int)reg->rows_s8[r][c]);
	   case TYPE_U16:	return((unsigned int) reg->rows [r][c]);
	   case TYPE_S16:	return(0x00008000
				^ (unsigned int)reg->rows_s16[r][c]);
	   case TYPE_U32:	return(reg->rows_u32[r][c]);
	   case TYPE_S32:	return(0x80000000
				^ (unsigned int)reg->rows_s32[r][c]);
	   default:		shErrStackPush ("region type is unsupported");
	}
	return (0.0);
}


/* This is quicksort
*/

#ifdef NOTDEFINED
static void sort(double * const varr, int count)
{
  int first, last;		/* indexes which converge towards the center */
  double divider;		/* Value to divide data on */
  double temp;			/* Temporary space for swaps */

  while (count > 1) {

    /* Partition the array in two */
    divider = varr[count>>1];	/* A value to divide the array */
    first = 0;			/* The leftmost array index */
    last = count-1;		/* The rightmost array index */

    while (varr[last] > divider) /* Can't use >= unless array bounds checked! */
      last--;
    while (varr[first] < divider)
      first++;

    while (first < last) {
      temp = varr[first];
      varr[first] = varr[last];
      varr[last] = temp;

      first++; last--;

      while (varr[last] > divider)
	last--;
      while (varr[first] < divider)
	first++;
    }

    /* Recurse on both halves */
    sort(&varr[last+1], count-last-1);	/* First half */
    count = first;			/* Second half */
  }
}
#endif



/* This routine finds the kth "index" value in varr.  That is, k=0 is
the smallest, k=1 is the second smallest, ..., k=count-1 is the
largest.  varr[] is rearranged so that the found value is in varr[k],
all values to the left of k are smaller than or equal to k, and
all values to the right of k are larger than or equal to k.
*/

static double indexfind(double *varr, int count, int k)
{
  int first, last;		/* indexes which converge towards the center */
  double divider;		/* Value to divide data on */
  double temp;			/* Temporary space for swaps */

  while (count > 1) {

    /* Partition the array in two */
    divider = varr[count>>1];	/* A value to divide the array */
    first = 0;			/* The leftmost array index */
    last = count-1;		/* The rightmost array index */

    while (varr[last] > divider) /* Can't use >= unless array bounds checked! */
      last--;
    while (varr[first] < divider)
      first++;

    while (first < last) {
      temp = varr[first];
      varr[first] = varr[last];
      varr[last] = temp;

      first++; last--;

      while (varr[last] > divider)
	last--;
      while (varr[first] < divider)
	first++;
    }

    /* Recurse on the half containing k */
    if (k >= first) {
      varr += first;
      count -= first;
      k -= first;
    } else {
      count = first;
    }

  }

  return varr[0];
}



/* 
 * This is almost an exact copy of the above "indexfind", but
 * it works on values which are all U16, not double.  It
 * is used by atRegFindMedian, if all input regions are TYPE_U16.
 *
 * This routine finds the kth "index" value in varr.  That is, k=0 is
 * the smallest, k=1 is the second smallest, ..., k=count-1 is the
 * largest.  varr[] is rearranged so that the found value is in varr[k],
 * all values to the left of k are smaller than or equal to k, and
 * all values to the right of k are larger than or equal to k.
 */

static U16 indexfindU16(U16 *varr, int count, int k)
{
  int first, last;		/* indexes which converge towards the center */
  U16 divider;		        /* Value to divide data on */
  U16 temp;			/* Temporary space for swaps */

  while (count > 1) {

    /* Partition the array in two */
    divider = varr[count>>1];	/* A value to divide the array */
    first = 0;			/* The leftmost array index */
    last = count-1;		/* The rightmost array index */

    while (varr[last] > divider) /* Can't use >= unless array bounds checked! */
      last--;
    while (varr[first] < divider)
      first++;

    while (first < last) {
      temp = varr[first];
      varr[first] = varr[last];
      varr[last] = temp;

      first++; last--;

      while (varr[last] > divider)
	last--;
      while (varr[first] < divider)
	first++;
    }

    /* Recurse on the half containing k */
    if (k >= first) {
      varr += first;
      count -= first;
      k -= first;
    } else {
      count = first;
    }

  }

  return varr[0];
}

/*****************************************************************************
 * given an array of 'numval' doubles, calculate the clipped mean; use 'niter'
 * iterations in which we discard values more than 'nsigma' stdevs from
 * the mean after each iteration.
 *
 * if clipCorrectFlag == 1, adjust the sigmas to correct for truncation,
 * assuming that the underlying distribution is a gaussian (see Newberg
 * and Yanny 1997)
 *
 * Used by "shRegClippedMeanFind", below. 
 *
 * If we can't calculate mean and sigma, formally, return 1.0 and print
 * a DEBUG message.
 * 
 */

static double 
clippedmeanfind
   (
   double *values,              /* array of values for which we calc mean */
   int numval,                  /* number of values */
   int niter,                   /* number of iterations */
   double nsigma,               /* ignore points more than 'nsigma' stdevs */
                                /*    from the mean */
   int clipCorrectFlag          /* if this = 1 apply sigma clipping correction */
   )
{
   int i, it, num;
   double sum, sumsq;
   double val, minval, maxval;
   double mn, sig;
   double clipfactor;

   it = 0;
   minval = maxval = 0;
   mn = 0.0;
   sig = 0.0;

/* calculate the correction factor for sigma due to clipping */
/* no correction will be made if nsigma == 0 */

   if ( nsigma > 0 ) {
     clipfactor = sqrt(1.0 - nsigma*exp(-nsigma*nsigma/2.0)/
                  (sqrt(2.0*M_PI)*(0.5-0.5*smaDERFC(nsigma/sqrt(2.0)))));
   } else {
     clipfactor = 1;
   }

   while (it < niter) {
      sum = sumsq = num = 0;
      for (i = 0; i < numval; i++) {
	 val = values[i];
	 if ((it > 0) && ((val < minval) || (val > maxval))) {
	    continue;
	 }
	 sum += val;
	 sumsq += val*val;
	 num++;	    
      }
      if (num > 1) {
	 mn = sum/num;
	 sig = sqrt((sumsq - num*mn*mn)/((double)num - 1));
      }
      else {
	 shDebug(2, "clippedmeanfind: too few pixels, mean = sig = 1.0");
	 mn = 1.0;
	 sig = 1.0;
	 break;
      }

/* apply the correction to sigma due to clipping */
      if ( clipCorrectFlag == 1 ) {
        sig /= clipfactor;
      }

      minval = mn - nsigma*sig;
      maxval = mn + nsigma*sig;
      it++;
   }

   return(mn);
}


/*****************************************************************************
 * given an array of 'numval' doubles, calculate the clipped mean; use 'niter'
 * iterations in which we discard values more than 'nsigma' stdevs from
 * the mean after each iteration.
 *
 * if clipCorrectFlag == 1, adjust the sigmas to correct for truncation,
 * assuming that the underlying distribution is a gaussian (see Newberg
 * and Yanny 1997)
 *
 * Used by "shRegClippedMeanFind", below. 
 *
 * If we can't calculate mean and sigma, formally, return 1.0 and print
 * a DEBUG message.
 *
 *
 * This is an exact copy of the routine above, but it handles an array
 * of U16s, rather than doubles.
 * 
 */

static U16 
clippedmeanfindU16
   (
   U16 *values,                 /* array of values for which we calc mean */
   int numval,                  /* number of values */
   int niter,                   /* number of iterations */
   double nsigma,               /* ignore points more than 'nsigma' stdevs */
                                /*    from the mean */
   int clipCorrectFlag          /* if this = 1 apply clipping sigma correction */
   )
{
   int i, it, num;
   U16 ret;
   double sum, sumsq;
   double val, minval, maxval;
   double mn, sig;
   double clipfactor;

   it = 0;
   minval = maxval = 0;
   mn = 0.0;
   sig = 0.0;

/* calculate the correction factor for sigma due to clipping */
/* no correction will be made if nsigma == 0 */

   if ( nsigma > 0 ) {
     clipfactor = sqrt(1.0 - nsigma*exp(-nsigma*nsigma/2.0)/
                (sqrt(2.0*M_PI)*(0.5-2.0*smaDERFC(nsigma/sqrt(2.0)))));
   } else {
     clipfactor = 1;
   }

   while (it < niter) {
      sum = sumsq = num = 0;
      for (i = 0; i < numval; i++) {
	 val = values[i];
	 if ((it > 0) && ((val < minval) || (val > maxval))) {
	    continue;
	 }
	 sum += val;
	 sumsq += val*val;
	 num++;	    
      }
      if (num > 1) {
	 mn = sum/num;
	 sig = sqrt((sumsq - num*mn*mn)/((double)num - 1));
      }
      else {
	 shDebug(2, "clippedmeanfindU16: too few pixels, mean = sig = 1.0");
	 mn = 1.0;
	 sig = 1.0;
	 break;
      }

/* apply the clipping correction to sigma */
      if ( clipCorrectFlag == 1 ) {
        sig /= clipfactor;
      }

      minval = mn - nsigma*sig;
      maxval = mn + nsigma*sig;
      it++;
   }

   if (mn < (double)U16_MIN) {
      return(U16_MIN);
   }
   else if (mn > (double)U16_MAX) {
      return(U16_MAX);
   } else {
      ret = (U16) (mn >= 0.0 ? mn + 0.5 : -(-mn + 0.5));
      return(ret);
   }
}

/*******************************************************************
 *
 *      Find the Standard Deviation
 *
 *                              written: november 13 1993
 *                                       james annis
 *
 ******************************************************************/

static void find_stddev( double numbers[], int number_of_numbers, 
                         double *Fmean, double *Fstddev )
{
/* declare variables */
        double mean, stddev, temp;
        int i;

/* be paranoid about integer to float conversion   */
        double N = number_of_numbers;

	if(number_of_numbers<=0) {
	*Fmean = 0;
	*Fstddev=0;
	return;
	}


/* calculate the mean   */
        mean =0;
        stddev =0 ;
        for (i=0; i< number_of_numbers; i++) {
		mean += numbers[i]; 
		stddev += numbers[i]*numbers[i];
	}
	
/* calculate the std dev   */
	
        mean = mean / N ;

	temp = stddev/N - mean*mean;
	if (temp>0) {
	  stddev = sqrt(stddev/N - mean*mean);
	} else stddev = 0;


/* return the answer   */
        *Fmean = mean;
        *Fstddev = stddev;
        return;
}
   
   



/******************** science functions within this module *******************/

/*<AUTO EXTRACT>

  ROUTINE: atRegClippedStatsFind

  DESCRIPTION:
<HTML>
</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS
</HTML>
</AUTO>*/

RET_CODE atRegClippedStatsFind (const REGION *reg, const double lbound,
	const double ubound, double *mean, double 
	*sigma, double *high, int *hrow, int *hcol, double *low, int *lrow, 
	int *lcol)
{
        int r, c;
        double np,sum2,total, temp;

        /* verify inputs */
        shAssert(reg != NULL);
        shAssert( (reg->nrow + reg->ncol) > 0 );
        shAssert( lbound < ubound );

        np = 0;
        total = 0.0;
        sum2 = 0;
        *low = shRegPixGetAsDbl (reg, 0, 0);
        *lrow = 0;
        *lcol = 0;
        *high = shRegPixGetAsDbl (reg, 0, 0);
        *hrow = 0;
        *hcol = 0;


/*   loop through all rows and pixels, find the sum and sum^2, 
 *   as well as the lowest, highest value */

        for (r=0; r<reg->nrow; r++) {
                for (c=0; c<reg->ncol; c++) {
                        temp = shRegPixGetAsDbl (reg, r, c);
			if ((temp > lbound) && (temp < ubound)) {
                        	total += temp;
	                        sum2 +=  temp*temp;
				np++;

                        	if (temp < *low) {
                                	*low = temp;
                                	*lrow = r;
                                	*lcol = c;
                        	}
                        	else if (temp > *high) {
                                	*high = temp;
					*hrow = r;
                                	*hcol = c;
                        	}
			}
                }
        }

/* Using the sum and sum^2 find the means and standard deviations */
        *mean = total/np;
        *sigma = sqrt(fabs((sum2/np)-((double)*mean*(double)*mean)));

        return (SH_SUCCESS);

}


/*<AUTO EXTRACT>

  ROUTINE: atRegGaussianAdd

  DESCRIPTION:
<HTML>
  Add a 2-D Gaussian profile to an image
</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS
</HTML>
</AUTO>*/

#define epsilon 1e-12

RET_CODE atRegGaussianAdd(REGION *reg, const double rpos, const double cpos, 
                          const double amplitude, const double rsig, 
                          const double csig, const double radius) 
{

	int r,c;
	double dc, dr, pxval;
	int rstart, rend, cstart, cend;

	rstart = (int)floor(rpos-radius);
	if (rstart<0) rstart = 0;

	rend = (int)ceil(rpos+radius);
	if (rend>(reg->nrow-1)) rend = reg->nrow-1;

	cstart = (int)floor(cpos-radius);
	if (cstart<0) cstart = 0;

	cend = (int)ceil(cpos+radius);
	if (cend>(reg->ncol-1)) cend = reg->ncol-1;

	for (r=rstart; r<=rend; r++) {
		for (c=cstart; c<=cend; c++) {

		   dc = (double)c + 0.5 - cpos;
		   dr = (double)r + 0.5 - rpos;
		   if ( (dr*dr+dc*dc) < radius*radius ) { 
			pxval = shRegPixGetAsDbl(reg, r, c) +
				Gaussian(dc, dr, amplitude, csig, rsig);
			shRegPixSetWithDbl(reg, r, c, pxval);
		   }

		}
	}
        return(SH_SUCCESS);
}


/*<AUTO EXTRACT>

  ROUTINE: atRegMap

  DESCRIPTION:
<HTML>
  magnify, rotate around a given point, and shift the SOURCE region (in
  that order) to produce the TARGET region.  The rotation is input in radians.
  For each pixel in the TARGET region, it calculates the corresponding 
  position in the source region.  It then uses bilinear interpolation of 
  the closest four pixels to determine the value to be put in the target 
  pixel.  If the position is within half a pixel of the edge of the source 
  region, it will simply take the value of the closest pixel.  If the pixel 
  is outside the source region, the target region pixel will be set to zero.  
  There is no requirement that the SOURCE and TARGET regions should be the 
  same size.
</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS
</HTML>
</AUTO>*/


RET_CODE atRegMap(const REGION *source, REGION *target, const double rowoff, 
  const double coloff, const double theta, const double rcenter, 
  const double ccenter, const double rowmag, const double colmag) 
{

  int r, c;
  double y1, x1, y2, x2, y3, x3, ysource, xsource;
  double sintheta, costheta;
  int rsource,csource;

  sintheta = sin(M_PI*theta/180.);
  costheta = cos(M_PI*theta/180.);

  for (r=0; r<target->nrow; r++) {
    for (c=0; c<target->ncol; c++) {

      /* Calculate the corresponding pixel in the SOURCE */
      /* The operations are the reverse of shifting the SOURCE to the TARGET */

      /* Shift the center and the image */

      y1 = (double)r + 0.5 - rowoff - rcenter;
      x1 = (double)c + 0.5 - coloff - ccenter;

      /* Rotate */

      y2 = - x1 * sintheta + y1 * costheta;
      x2 = x1 * costheta + y1 * sintheta;

      /* Magnify */

      y3 = y2 / rowmag;
      x3 = x2 / colmag;

      /* Shift the center */

      ysource = y3 + rcenter;
      xsource = x3 + ccenter;

      /* Set with the interpolated pixel value */

      if ( (ysource > 0.5) && (xsource > 0.5) && (ysource < (source->nrow-0.5))
        && (xsource < (source->ncol-0.5)) ) {
        shRegPixSetWithDbl(target, r, c, atRegPixRebin(source, ysource, xsource));
      } else {
	rsource = (int) floor(ysource);
	csource = (int) floor(xsource);
	if ( (rsource >= 0) && (csource >= 0) && (rsource < source->nrow) &&
	  (csource < source->ncol) ) {
          shRegPixSetWithDbl(target, r, c, 
	    shRegPixGetAsDbl(source, rsource, csource));
	} else {
          shRegPixSetWithDbl(target, r, c, 0.0);
	}
      }

    }
  }
  return(SH_SUCCESS);
}


/*<AUTO EXTRACT>

  ROUTINE: atRegModeFind

  DESCRIPTION:
<HTML>
   This histograms all of the pixels in a region and finds the
   most common value.  This is only defined for integer region types
</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS or SH_GENERIC_ERROR
</HTML>
</AUTO>*/

RET_CODE atRegModeFind(const REGION *reg,  double min, double max,
	double *mode) 
{

        HIST *hists;
	unsigned int *line;
        unsigned int r, c, nbins, histmode;
        unsigned int histmin=0;

	*mode = 0;
        shAssert ( (reg->type == TYPE_U8) || (reg->type == TYPE_S8) ||
		 (reg->type == TYPE_U16) || (reg->type == TYPE_S16) ||
		 (reg->type == TYPE_U32) || (reg->type == TYPE_S32));

	/* Somehow convert min and max to histmin and nbins */
	switch (reg->type) {
		case TYPE_U8:	if (min<(double)U8_MIN) min = U8_MIN;
				if (min>=(double)U8_MAX) return SH_GENERIC_ERROR;
				if (max<=(double)U8_MIN) return SH_GENERIC_ERROR;
				if (max>(double)U8_MAX) max = U8_MAX;
				histmin = (unsigned int) min;
				break;
		case TYPE_S8:	if (min<(double)S8_MIN) min = S8_MIN;
				if (min>=(double)S8_MAX) return SH_GENERIC_ERROR;
				if (max<=(double)S8_MIN) return SH_GENERIC_ERROR;
				if (max>(double)S8_MAX) max = S8_MAX;
				histmin = 0x00000080^(unsigned int)
					((signed int) min);
				break;
		case TYPE_U16:	if (min<(double)U16_MIN) min = U16_MIN;
				if (min>=(double)U16_MAX) return SH_GENERIC_ERROR;
				if (max<=(double)U16_MIN) return SH_GENERIC_ERROR;
				if (max>(double)U16_MAX) max=U16_MAX;
				histmin = (unsigned int) min;
				break;
		case TYPE_S16:	if (min<(double)S16_MIN) min = S16_MIN;
				if (min>=(double)S16_MAX) return SH_GENERIC_ERROR;
				if (max<=(double)S16_MIN) return SH_GENERIC_ERROR;
				if (max>(double)S16_MAX) max = S16_MAX;
				histmin = 0x00008000^(unsigned int)
					((signed int) min);
				break;
		case TYPE_U32:	if (min<(double)U32_MIN) min = U32_MIN;
				if (min>=(double)U32_MAX) return SH_GENERIC_ERROR;
				if (max<=(double)U32_MIN) return SH_GENERIC_ERROR;
				if (max>(double)U32_MAX) max=U32_MAX;
				histmin = (unsigned int) min;
				break;
		case TYPE_S32:	if (min<(double)S32_MIN) min = S32_MIN;
				if (min>=(double)S32_MAX) return SH_GENERIC_ERROR;
				if (max<=(double)S32_MIN) return SH_GENERIC_ERROR;
				if (max>(double)S32_MAX) max = S32_MAX;
				histmin = (unsigned int) ((signed int) min);
				break;
		default:	shErrStackPush("region type is unsupported");
	}
	nbins = (unsigned int) (max - min + 1);

        hists = shHistConstructHists (1, histmin, nbins, 1);
	if (hists == 0) return TCL_ERROR;

	line = (unsigned int *)calloc(reg->ncol,sizeof(unsigned int));

/*        shHistInitHists(hists,1);  I don't think this is neccessary
	                             based on my reading of the hists.c
				     code */
        for (r=0; r<reg->nrow; r++) {
		for (c=0; c<reg->ncol; c++) line[c] = atRegPixGetAsU32(reg,r,c);
		shHistAccumulateLine (hists, line, (unsigned int)reg->ncol);
	}
        histmode = shHistFindPeak(hists);
        shHistDestructHists (hists, 1);
	free(line);

	/* Change histmode to mode */
	*mode = (double)histmode + min;

        return (SH_SUCCESS);
}


/*<AUTO EXTRACT>

  ROUTINE: atRegMedianFindAsDbl

  DESCRIPTION:
<HTML>
This finds the median pixel in a region
</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS
</HTML>
</AUTO>*/


RET_CODE atRegMedianFindAsDbl(const REGION *reg, double *median) 
{
	double *line;				/* Array of pixel values */
	register double *p_line;		/* Pointer into array */
        register unsigned int r, c;		/* current row and column */
	register unsigned int nRows, nCols;	/* number of rows and columns */
	int nPixels;				/* number of pixels */

	nRows = reg->nrow;	/* For speed's sake */
	nCols = reg->ncol;	/* For speed's sake */
	nPixels = nRows * nCols;

	shAssert(nPixels > 0);
	line = (double *)shMalloc(nPixels*sizeof(double));

	p_line = line;		/* To prevent needless multiplications */
        for (r=0; r<nRows; r++) {
	  for (c=0; c<nCols; c++) {
	    *(p_line++) = shRegPixGetAsDbl(reg,r,c);
	  }
	}
	*median = indexfind(line, nPixels, (nPixels-1) / 2);

	shFree(line);

        return (SH_SUCCESS);
}


/*<AUTO EXTRACT>

  ROUTINE: atRegStatsFind

  DESCRIPTION:
<HTML>
Finds the mean and sigma on the mean in an image, plus the high and low
values and their positions.
</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS
</HTML>
</AUTO>*/

RET_CODE
atRegStatsFind (const REGION *reg, double *mean, double *sigma,
        double *high, int *hrow, int *hcol, double *low, int *lrow, int *lcol, double *totalOut) 
{
        int r, c;
        double np,sum2,total, temp;

        /* verify inputs */
        shAssert(reg != NULL);
        shAssert( (reg->nrow + reg->ncol) > 0 );

        np = (float)reg->nrow* (float)reg->ncol;
        total = 0.0;
        sum2 = 0;
        *low = shRegPixGetAsDbl (reg, 0, 0);
        *lrow = 0;
        *lcol = 0;
        *high = shRegPixGetAsDbl (reg, 0, 0);
        *hrow = 0;
        *hcol = 0;

/*   loop through all rows and pixels, find the sum and sum^2, 
 *   as well as the lowest, highest value */

        for (r=0; r<reg->nrow; r++) {
                for (c=0; c<reg->ncol; c++) {
                        temp = shRegPixGetAsDbl (reg, r, c);
                        total += temp;
                        sum2 +=  temp*temp;

                        if (temp < *low) {
                                *low = temp;
                                *lrow = r;
                                *lcol = c;
                        }
                        else if (temp > *high) {
                                *high = temp;
                                *hrow = r;
                                *hcol = c;
                        }
                }
        }
/* Using the sum and sum^2 find the means and standard deviations */
        *mean = total/np;
        *sigma = sqrt(fabs((sum2/np)-((double)*mean*(double)*mean)));
	*totalOut = total;
        return(SH_SUCCESS);
}






/*<AUTO EXTRACT>

  ROUTINE: atRegConvolve

  DESCRIPTION:
<HTML>
   Convolve reg with template to produce target.  Normally, the target will
   be the same size as the input region, and the template will be a small
   region with an odd number of rows and columns.
</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS
</HTML>
</AUTO>*/


RET_CODE atRegConvolve(const REGION * const reg,
		   const REGION * const tmplt,
		   REGION * const target) 
{

	int r,c;		/* Position in target */
	int rtemp, ctemp;	/* Position in template */
	double targetval, tempsum, tempweight; /* Computing target value */
	int rstart, rend, cstart, cend;	/* Applicable range in template */
	int tmpltWidth, tmpltHeight; /* Size of template */
	int tmpltHalfWidth, tmpltHalfHeight; /* Half Size of template */
	int rAdjusted, cAdjusted; /* Target position less half template size */
	int rSource;		/* Position in source region */

	tmpltHeight = tmplt->nrow;
	tmpltWidth = tmplt->ncol;
	tmpltHalfWidth = tmpltWidth/2;
	tmpltHalfHeight = tmpltHeight/2;

	for (r=0; r<target->nrow; r++) {
	  rstart = tmpltHalfHeight-r;
	  rend = rstart+target->nrow;
	  if (rstart<0) rstart = 0;
	  if (rend>tmpltHeight) rend = tmpltHeight;
	  rAdjusted = r-tmpltHalfHeight;

	  for (c=0; c<target->ncol; c++) {
	    cstart = tmpltHalfWidth-c;
	    cend = cstart+target->ncol;
	    if (cstart<0) cstart = 0;
	    if (cend>tmpltWidth) cend = tmpltWidth;
	    cAdjusted = c-tmpltHalfWidth;

	    targetval = 0.0;
	    tempsum = 0.0;

	    for (rtemp=rstart; rtemp<rend; rtemp++) {
	      rSource = rAdjusted+rtemp;
	      for (ctemp=cstart; ctemp<cend; ctemp++) {
		tempweight = shRegPixGetAsDbl(tmplt,rtemp,ctemp);
		targetval += tempweight *
		  shRegPixGetAsDbl(reg, rSource, cAdjusted+ctemp);
		tempsum += tempweight;
	      }
	    }

	    shRegPixSetWithDbl(target, r, c, targetval/tempsum);
	  }
	}
    return(SH_SUCCESS);
}


/*<AUTO EXTRACT>

  ROUTINE: atRegSkyFind

  DESCRIPTION:
<HTML>
   Find the sky background in an image by fitting a Gaussian to the
   peak of a histogram of pixel values
</HTML>

  RETURN VALUES:
<HTML>
	Double sky
</HTML>
</AUTO>*/


double atRegSkyFind (const REGION *reg) 
{

        HIST *hists;
	unsigned int *line;
        unsigned int r, c, binspersigma, nsigma, histmode;
	unsigned int n1,n2,n3;
	double mean,sigma,high,low,total;
	double h1, h2, h3, histsky, sky;
	int hrow, hcol, lrow, lcol;

	nsigma = 20;		/* sigma each side of the mean */
	binspersigma = 4;

	line = (unsigned int *)calloc(reg->ncol,sizeof(unsigned int));

	/* Find the mean and sigma */
	atRegStatsFind (reg, &mean, &sigma, &high, &hrow, &hcol, &low, &lrow, &lcol, &total);
	atRegClippedStatsFind(reg, mean-sigma, mean+sigma, &mean, &sigma, &high, &hrow, &hcol, &low, &lrow, &lcol);
	if (sigma<=0) sky = shRegPixGetAsDbl(reg,0,0);
	else {
	/* Histogram nsigma sigma each side of the mean with 
		bin width sigma/binspersigma 				*/
        hists = shHistConstructHists (1, 0, 2*nsigma*binspersigma, 1);
        shHistInitHists(hists,1);
        for (r=0; r<reg->nrow; r++) {
		for (c=0; c<reg->ncol; c++) 
			line[c] = (unsigned int) floor(binspersigma*
                                                    ((shRegPixGetAsDbl(reg,r,c)-mean)/sigma + nsigma));
		shHistAccumulateLine (hists, line, (unsigned int)reg->ncol);
	}

	/* Find the peak bin */
        histmode = shHistFindPeak(hists);

/* Fit a Gaussian to the peak plus its adjacent bins */
	n1 = hists->bins[histmode-1];
	n2 = hists->bins[histmode];
	n3 = hists->bins[histmode+1];
	if ((n1 > 0) && (n2 > 0) && (n3 > 0)) {
	h1 = log ( (double) n1);
	h2 = log ( (double) n2);
	h3 = log ( (double) n3);
	histsky = (double)histmode + 0.5 + (h3-h1)/(2*(2*h2-h1-h3));
      } else {histsky = (double)histmode;}

        shHistDestructHists (hists, 1);

	/* Convert histsky to region sky */

	sky = (histsky/binspersigma - nsigma)*sigma + mean;
      }

	return(sky);

}


/* Brad Holden 7/15/93 */

/*<AUTO EXTRACT>

  ROUTINE: atRegBin

  DESCRIPTION:
<HTML>
A routine to bin an image into larger pixels.  Input is the region to be
binned, the target region, and the x and y binning factor.  If the image size
modulo one of the binning factors is not zero, the extra pixels are thrown 
away.  These pixels are always the ones with the high row and column numbers.
The output region has one pixel for every summed over group of pixels. 
If the value of 'normalize' is 1, then divide the sum in each superpixel
by the number of original pixels that comprise it.
</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS
</HTML>
</AUTO>*/

RET_CODE atRegBin(const REGION *reg, REGION *regtarget, const int rowfactr, 
	      const int colfactr, const int normalize) 
{

  int row, col;
  int rowcount, colcount;
  int target_row, target_col;
  double temp_sum;

  /*  row and col are the coordinates of the upper left-hand corner of the
      rowfactr by colfactr region that will be summed up into the new pixel.
      rowcount and colcount are the pixels coordinates inside the rowfactr by
      colfactr region that is being summed over.  The variables target_row
      and target_col the appropriate pixel coordinates of the target that 
      the summed over region will be put into.

      So, this all works by breaking the input region into rowfactr by 
      colfactr subregions, summing over them and then putting the sum into
      one pixel in the output region.  After one region is summed over, the
      next region is down in the row direction till the edge is reached, then
      I go back to the beginning of the row but down by colfactr. */

  row = col = 0;
  target_row = target_col = 0;

  /*  This loops till the edges of the input region are reached. */

  while( row < reg->nrow && col < reg->ncol) {
    temp_sum = 0;

    /*  Here I sum over the subregion. Bypass regPixGetasDouble for 
        the most common cases. */

    switch (reg->type) {
      case TYPE_FL32:
        for(rowcount = 0; rowcount < rowfactr; rowcount++) {
          for(colcount = 0; colcount < colfactr; colcount++) {
	    temp_sum += reg->rows_fl32 [rowcount + row] [colcount + col];
          }
        }    
        break;
      case TYPE_U16:
        for(rowcount = 0; rowcount < rowfactr; rowcount++) {
          for(colcount = 0; colcount < colfactr; colcount++) {
	    temp_sum += reg->rows_u16 [rowcount + row] [colcount + col];
          }
        }    
      break;
      default:
        for(rowcount = 0; rowcount < rowfactr; rowcount++) {
          for(colcount = 0; colcount < colfactr; colcount++) {
	    temp_sum += shRegPixGetAsDbl(reg, rowcount + row, colcount + col);
          }
        }    
    } /*end switch*/

    /* If the user asked us to normalize, do so */
    if (normalize == 1) {
      temp_sum /= (rowfactr*colfactr);
    }

    /* set the pixel in the output region. Optimise the case
       where the output region is a float */
    switch (reg->type) {
      case TYPE_FL32:
        regtarget->rows_fl32[target_row][target_col] = temp_sum;
        break;
      default: 
        shRegPixSetWithDbl(regtarget, target_row, target_col, temp_sum); 
    }    

  /*  Once a subregion is down, I move to the next row in target and the
      next rowfactr by colfactr subregion in the input region.  */
  
    target_row++;
    row += rowfactr;
    
    /*  If I have gotten to the edge of the row, ie there are less then 
	a rowfactr number of pixels at the edge, I move over to the next 
	column in the output and the next colfactr wide strip in the input. */
    
    if( (reg->nrow - row) < rowfactr ) {
      row = 0;
      target_row = 0;
      col += colfactr;
      target_col++;
    }
    
    /* Once I have gotten to the edge in columns, I set these variables to
       the size of the input region to end the loop. */
    
    if( (reg->ncol - col) < colfactr ) {
      col = reg->ncol;
      row = reg->nrow;
    }
  } /*end while*/

  return(SH_SUCCESS);
}

/* Brad Holden 7/13/93 */

/*<AUTO EXTRACT>

  ROUTINE: atRegMedianFind

  DESCRIPTION:
<HTML>
Finds the pixel by pixel median of the number nReg regions.  Each
pixel from a region is put in a array, the median of the array is
found and recorded.  Each region should be the same size.

<P>
We check for the special case that all the input regions are
type TYPE_U16; in that case, we do not use "shRegPixGetAsDbl",
but instead access the pixel data as type U16 directly.

</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS
</HTML>
</AUTO>*/

RET_CODE atRegMedianFind (
  const REGION * const * const apRegSource,   /* Array of pointers to REGIONs */
  const int nReg,			      /* Number of Regions */
  REGION * const pRegTarget)		      /* Pointer to target REGION */
{
  register const REGION * const *ppRegSource;	/* pointer into apRegSource */
  register const REGION * const *ppRegLast;	/* &apRegSource[nReg - 1] */
  register double *aPixValues;			/* array of pix values */
  register double *pPixValue;			/* pointer into aPixValues */
  register int row, col;			/* row and column of an image */
  register int iPixMedian;			/* half the number of regions */
  register int nRows, nCols;
  int i, u16_flag;
  register U16 *aU16PixValues;                  /* array of U16 pix values */
  register U16 *pU16PixValue;                   /* pointer into aU16PixValues */
  register U16 medU16;                          /* median value for some pix */


  /* Allocate space to do our work */
  aPixValues = (double *)shMalloc(nReg * sizeof(double));
  if (aPixValues == NULL ) {
    shErrStackPush("Out of memory!");
    exit(-1);
  }
  aU16PixValues = (U16 *) shMalloc(nReg*sizeof(U16));
  if (aU16PixValues == NULL) {
    shErrStackPush("Out of memory!");
    exit(1);
  }
  
  /* Precompute some values to prevent multiple recomputations */
  ppRegLast = &apRegSource[nReg - 1];
  nRows = pRegTarget->nrow;
  nCols = pRegTarget->ncol;
  iPixMedian = (nReg-1)/2;

  /* Set "u16_flag" to 1 if all REGIONs are type TYPE_U16 */
  u16_flag = 1;
  for (i = 0; i < nReg; i++) {
    if (apRegSource[i]->type != TYPE_U16) {
      u16_flag = 0;
      break;
    }
  }
  
  /* this is the case when NOT all regions are TYPE_U16; slow */
  if (u16_flag == 0) {
    for(row = 0; row < nRows; row++) {
      for(col = 0; col < nCols; col++) {
        /* Get the value at (row, column) from every region and find median */
        /* This code executed very often!  Speed optimization is important. */
  
        pPixValue = aPixValues;	/* To prevent needless array lookups */
        ppRegSource = apRegSource; /* To prevent needless array lookups */
  
        do {
  	  *(pPixValue++) = shRegPixGetAsDbl(*(ppRegSource++), row, col);
        } while (ppRegSource <= ppRegLast);
        shRegPixSetWithDbl(pRegTarget, row, col, 
  			 indexfind(aPixValues, nReg, iPixMedian));
      }
    }
  }
  else {
    /* and this is the case when all regions ARE TYPE_U16; faster */
    for (row = 0; row < nRows; row++) {
      for (col = 0; col < nCols; col++) {
        pU16PixValue = aU16PixValues;
        ppRegSource = apRegSource;
        do {
          *(pU16PixValue++) = (*(ppRegSource++))->rows_u16[row][col];
        } while (ppRegSource <= ppRegLast);
        medU16 = indexfindU16(aU16PixValues, nReg, iPixMedian);
        pRegTarget->rows_u16[row][col] = medU16;
      }
    }
  }

  shFree(aPixValues);
  shFree(aU16PixValues);
  return(SH_SUCCESS);
}


/******************************************************************************
 * <AUTO EXTRACT>
 *
 * ROUTINE: atRegClippedMeanFind
 *
 * DESCRIPTION:
 * 
 * Finds the pixel-by-pixel mean from a number 'nReg' regions.  The mean
 * can be calculated iteratively, with pixel values more than 'nsigma'
 * sigma away from the previous solution ignored and a new mean found. 
 * The resulting set of pixel values is placed in the output region.
 *
 * If clipping correction is requested, the sigma used to clip the data
 * is corrected (increased) from the effects of truncation by assuming
 * that the underlying distribution is a gaussian.  In other words, if
 * a gaussian is edited at N sigma, the wings will be lost and the sigma
 * determined from the remaining data will be an underestimate of the
 * true sigma.  The '-correct' switch compensates for this if 'nsigma >= 1.5'.
 * (see Newberg and Yanny 1997).
 * 
 * All regions must be of the same size and type.  The output region 
 * defaults to the first region specified in the list of regions.  If 
 * 'niter' == 1 no editing of the data occurs, and a simple mean of the
 * for each pixel is written into the output region.
 * 
 * We check for the special case that all the input regions are
 * type TYPE_U16; in that case, we do not use "shRegPixGetAsDbl",
 * but instead access the pixel data as type U16 directly.
 *
 * RETURN VALUES:
 * 	SH_SUCCESS
 * </AUTO>
 */

RET_CODE
atRegClippedMeanFind 
   (
   const REGION * const * const apRegSource,  /* Array of pointers to REGIONs */
   const int nReg,			      /* Number of Regions */
   const int niter,                           /* # of iters in calc clipped mean */
   const double nsigma,                       /* discard values this many stdevs */
                                              /*   from mean in each iteration */
   REGION * const pRegTarget,		      /* Pointer to output REGION */
   const int clipCorrectFlag                  /* flag to apply sigma clipping correction */
   )
{

  register const REGION * const *ppRegSource;	/* pointer into apRegSource */
  register const REGION * const *ppRegLast;	/* &apRegSource[nReg - 1] */
  register double *aPixValues;			/* array of pix values */
  register double *pPixValue;			/* pointer into aPixValues */
  register int row, col;			/* row and column of an image */
  register int nRows, nCols;
  int i, u16_flag;
  register U16 *aU16PixValues;                  /* array of U16 pix values */
  register U16 *pU16PixValue;                   /* pointer into aU16PixValues */
  register U16 medU16;                          /* median value for some pix */

  /* verify inputs 
   * allow nsigma to be anything in the degenerate case that niter == 1
   * to maintain compatibility with previous versions 
   */

  shAssert ( apRegSource != NULL );
  shAssert ( nReg > 0 );
  shAssert ( niter >= 0 );
  shAssert ( pRegTarget != NULL );

  if ( niter != 1 ) { 
    shAssert ( nsigma > 0.0 ); 
  }

  for ( i = 0; i < nReg; i++) {
    shAssert( (apRegSource[i]->nrow == apRegSource[0]->nrow) &&
              (apRegSource[i]->ncol == apRegSource[0]->ncol) &&
              (apRegSource[i]->type == apRegSource[0]->type ) );
  }

  /* Precompute some values to prevent multiple recomputations */
  ppRegLast = &apRegSource[nReg - 1];
  nRows = pRegTarget->nrow;
  nCols = pRegTarget->ncol;

  /* Set "u16_flag" to 1 if all REGIONs are type TYPE_U16 
     and verify that each region actually has pixels */
  u16_flag = 1;
  for (i = 0; i < nReg; i++) {
    if (apRegSource[i]->type != TYPE_U16) {
      u16_flag = 0;
      break;
    }
    shAssert( (apRegSource[i]->nrow * apRegSource[i]->ncol) > 0 );
  }
  /* this is the case when NOT all regions are TYPE_U16; slow */
  if (u16_flag == 0) {

    /* Allocate space to do our work */
    /* remember if shMalloc fails this calls exit, so no error check needed */
    aPixValues = (double *)shMalloc(nReg * sizeof(double));

    for(row = 0; row < nRows; row++) {
      for(col = 0; col < nCols; col++) {

        /* 
         * Get the value at (row, column) from every region and find the 
         * clipped mean. This code executed very often!  
         * Speed optimization is important. 
         */
        pPixValue = aPixValues;	/* To prevent needless array lookups */
        ppRegSource = apRegSource; /* To prevent needless array lookups */

        do {
  	  *(pPixValue++) = shRegPixGetAsDbl(*(ppRegSource++), row, col);
        } while (ppRegSource <= ppRegLast);

        shRegPixSetWithDbl(pRegTarget, row, col, 
  	        clippedmeanfind(aPixValues, nReg, niter, nsigma, clipCorrectFlag));
      }
    }
    shFree(aPixValues);
  }
  else {
    /* and this is the case when all regions ARE TYPE_U16; faster */

    /* Allocate space to do our work */
    aU16PixValues = (U16 *) shMalloc(nReg*sizeof(U16));

    for (row = 0; row < nRows; row++) {
      for (col = 0; col < nCols; col++) {
        pU16PixValue = aU16PixValues;
        ppRegSource = apRegSource;
        do {
          *(pU16PixValue++) = (*(ppRegSource++))->rows_u16[row][col];
        } while (ppRegSource <= ppRegLast);

        medU16 = clippedmeanfindU16(aU16PixValues, nReg, 
                        niter, nsigma, clipCorrectFlag);

        pRegTarget->rows_u16[row][col] = medU16;
      }
    }
  shFree(aU16PixValues);
  }
return(SH_SUCCESS);
}
   
/*****************************************************************************
 *
 * <AUTO EXTRACT>
 *
 * ROUTINE: atRegMedianFindByColumn
 * 
 * DESCRIPTION: 
 * Given a M-row x N-col REGION, and an empty 1-row x N-col REGION,
 * fill the 1xN region with values which are the median value of the 
 * pixels in each column of the original.
 *
 * Return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 *
 * </AUTO>
 */

RET_CODE
atRegMedianFindByColumn
   (
   REGION *input,          /* I: MxN region from which we calculate medians */
   REGION *output          /* O: 1xN region into which we place medians */
   )
{
   int row, col, n, index;
   double median, *array;

   /* make some sanity checks on the input regions */
   shAssert(input != NULL);
   shAssert(output != NULL);

   if (output->ncol != input->ncol) {
      shError("atRegMedianFindByColumn: input and output 'ncol' don't match");
      return(SH_GENERIC_ERROR);
   }
   if (output->nrow != 1) {
      shError("atRegMedianFindByColumn: output REGION does not have nrow = 1");
      return(SH_GENERIC_ERROR);
   }
   
   n = input->nrow;
   if ((array = (double *) shMalloc(n*sizeof(double))) == NULL) {
      shError("atRegMedianFindByColumn: can't shMalloc for %d doubles", n);
      return(SH_GENERIC_ERROR);
   }

   /*
    * we have special-case code for U16 and FL32 only, to support MT.  
    * We should include special cases for all pixel types sometime.  
    * MWR 8/27/96 
    */
   index = (input->nrow - 1)/2;
   if (input->type == TYPE_FL32 && output->type == TYPE_FL32) {
      for (col = 0; col < input->ncol; col++) {
         for (row = 0; row < input->nrow; row++) {
            array[row] = input->rows_fl32[row][col];
         }
         /* now that 'array' is filled with values, find median */
         if (n == 1) {
	    median = array[0];
         }
         else {
            median = indexfind(array, n, index);
         }
         output->rows_fl32[0][col] = median;
      }
   }
   else if (input->type == TYPE_U16 && output->type == TYPE_U16) {
      for (col = 0; col < input->ncol; col++) {
         for (row = 0; row < input->nrow; row++) {
            array[row] = input->rows_u16[row][col];
         }
         /* now that 'array' is filled with values, find median */
         if (n == 1) {
	    median = array[0];
         }
         else {
            median = indexfind(array, n, index);
         }
         output->rows_u16[0][col] = median;
      }
   }
   else {
      /* handle the general case */

      for (col = 0; col < input->ncol; col++) {
         for (row = 0; row < input->nrow; row++) {
	    array[row] = shRegPixGetAsDbl(input, row, col);
         }
         /* now that 'array' is filled with values, find median */
         if (n == 1) {
	    median = array[0];
         }
         else {
            median = indexfind(array, n, index);
         }
         shRegPixSetWithDbl(output, 0, col, median);
      }
   }
   shFree(array);

   return(SH_SUCCESS);
}

/*****************************************************************************
 *
 * <AUTO EXTRACT>
 *
 * ROUTINE: atRegMedianFindByRow
 * 
 * DESCRIPTION: 
 * Given a N-row x M-col REGION, and an empty N-row x 1-col REGION,
 * fill the Nx1 region with values which are the median value of the 
 * pixels in each row of the original.
 *
 * Return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 *
 * </AUTO>
 */

RET_CODE
atRegMedianFindByRow
   (
   REGION *input,          /* I: NxM region from which we calculate medians */
   REGION *output          /* O: Nx1 region into which we place medians */
   )
{
   int row, col, n, index;
   double median, *array;

   /* make some sanity checks on the input regions */
   shAssert(input != NULL);
   shAssert(output != NULL);

   if (output->nrow != input->nrow) {
      shError("atRegMedianFindByRow: input and output 'nrow' don't match");
      return(SH_GENERIC_ERROR);
   }
   if (output->ncol != 1) {
      shError("atRegMedianFindByRow: output REGION does not have ncol = 1");
      return(SH_GENERIC_ERROR);
   }
   
   n = input->ncol;
   if ((array = (double *) shMalloc(n*sizeof(double))) == NULL) {
      shError("atRegMedianFindByCol: can't shMalloc for %d doubles", n);
      return(SH_GENERIC_ERROR);
   }

   index = (input->ncol - 1)/2;

   /*
    * we have special-case code for FL32 and U16 only, to support MT.  
    * We should include special cases for all pixel types sometime.  
    * MWR 8/27/96 
    */
   if (input->type == TYPE_FL32 && output->type == TYPE_FL32) {
      for (row = 0; row < input->nrow; row++) {
         for (col = 0; col < input->ncol; col++) {
            array[col] = input->rows_fl32[row][col];
         }
         /* now that 'array' is filled with values, find median */
         if (n == 1) {
	    median = array[0];
         }
         else {
            median = indexfind(array, n, index);
         }
         output->rows_fl32[row][0] = median;
      }
   }
   else if (input->type == TYPE_U16 && output->type == TYPE_U16) {
      for (row = 0; row < input->nrow; row++) {
         for (col = 0; col < input->ncol; col++) {
            array[col] = input->rows_u16[row][col];
         }
         /* now that 'array' is filled with values, find median */
         if (n == 1) {
	    median = array[0];
         }
         else {
            median = indexfind(array, n, index);
         }
         output->rows_u16[row][0] = median;
      }
   }
   else {
      /* handle the general case */
      
      for (row = 0; row < input->nrow; row++) {
         for (col = 0; col < input->ncol; col++) {
            array[col] = shRegPixGetAsDbl(input, row, col);
         }
         /* now that 'array' is filled with values, find median */
         if (n == 1) {
	    median = array[0];
         }
         else {
            median = indexfind(array, n, index);
         }
         shRegPixSetWithDbl(output, row, 0, median);
      }
   }
   shFree(array);

   return(SH_SUCCESS);
}



/*****************************************************************************
 *
 * <AUTO EXTRACT>
 *
 * ROUTINE: atRegPixValSubstitute
 * 
 * DESCRIPTION: 
 * Given a REGION, a (double) value, and a (double) new value, 
 * replace all instances of pixels having 'value' with 'newvalue'.
 * The value of compare determines whether only pixels with value
 * oldvalue are replaced (compare = 0), or if pixels with values <=
 * oldvalue are replaced (compare = 1), or whether pixels with values
 * >= oldvalue are replaced (all other values of compare)
 *
 * This might be used, for example, to remove all instances of 0.0
 * from a region, prior to dividing by that region.
 *
 * Return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 *
 * </AUTO>
 */

RET_CODE
atRegPixValSubstitute
   (
   REGION *reg,            /* I: region in which we subsitute values */
   double oldval,          /* I: we replace any pixel having this value... */
   double newval,          /* I:   ... by this value */
   int compare		   /* I: 0 - substitute only values equal to oldval
				 1 - substitute values <= oldval
				 others - substitute values >= oldval */
   )
{
   int row, col;

   /* make some sanity checks on the input regions */
   shAssert(reg != NULL);

   if (compare==0) {
     /* Substitute only values equal to oldval */
     for (row = 0; row < reg->nrow; row++) {
        for (col = 0; col < reg->ncol; col++) {
	   if (shRegPixGetAsDbl(reg, row, col) == oldval) {
              shRegPixSetWithDbl(reg, row, col, newval);
           }
        }
     }
   } else {
     if (compare==1) {
       /* Substitute values less than or equal to oldval */
       for (row = 0; row < reg->nrow; row++) {
          for (col = 0; col < reg->ncol; col++) {
	     if (shRegPixGetAsDbl(reg, row, col) <= oldval) {
                shRegPixSetWithDbl(reg, row, col, newval);
             }
          }
       }
     } else {
       /* Substitute values greater than or equal to oldval */
       for (row = 0; row < reg->nrow; row++) {
          for (col = 0; col < reg->ncol; col++) {
	     if (shRegPixGetAsDbl(reg, row, col) >= oldval) {
                shRegPixSetWithDbl(reg, row, col, newval);
             }
          }
       }
     }
   }

   return(SH_SUCCESS);
}

   
/*****************************************************************************
 *
 * <AUTO EXTRACT>
 *
 * ROUTINE: atRegCorrectFL32
 * 
 * DESCRIPTION: 
 * Apply the corrections given in the array CORR to the pixels in REG
 * type type of REG must be TYPE_FL32.
 *
 * Use simple two-point interpolation among the elements of CORR to obtain a
 * correction to multiply pixels bigger than firstC and smaller than lastC.
 *
 * Return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 *
 * </AUTO>
 */

RET_CODE
atRegCorrectFL32
   (
   REGION *reg,           /* I/0: Unsigned 16-bit integer region to correct */
   double *corr,          /* I:   Corrections to multiply to the region */
   int ncorr,		  /* I:   number of corrections */
   float firstC,	  /* I:   first correction is for this pixel value*/
   float lastC		  /* I:   last correction is for this pixel value */
   )
{
   int r;   
   float step = 1;
   if (ncorr > 1 )step = (lastC-firstC)/(ncorr-1) ;

   shAssert(reg->type == TYPE_FL32);
   
   for (r=0; r<reg->nrow; r++) {
      float *cp     = reg->rows_fl32[r];
      float *cppast = &reg->rows_fl32[r][reg->ncol];
      do {
	 float pix = *cp;
	 float steps;		/* number of steps, including fractional steps */
	 int idx;               /* greatest integer smaller that number of steps */
	 if (pix < firstC) continue;
	 if (pix > lastC ) continue;
	 idx = steps = (pix - firstC)/step;
	 if ((float)idx == steps) {
	    *cp = pix * corr[idx];  /* "integral pixel in a float" -- Optimize "common case" */
	 }else{
	    float w = steps - idx;  /* Interpolated case */
	    *cp = pix * (corr[idx] + w*(corr[idx+1]-corr[idx]));    
	 }
      } while (++cp < cppast);
   }
   return(SH_SUCCESS);
} 

/*****************************************************************************
 *
 * <AUTO EXTRACT>
 *
 * ROUTINE: atRegDistortFL32
 * 
 * DESCRIPTION: 
 * Apply the inverse corrections given in the array CORR to the pixels in REG
 * type of REG must be TYPE_FL32.
 *
 * Use simple two-point interpolation among the elements of CORR to obtain a
 * correction to multiply pixels bigger than firstC and smaller than lastC.
 *
 * Return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 *
 * </AUTO>
 */

RET_CODE
atRegDistortFL32
   (
   REGION *reg,           /* I/0: FL32  region to distort */
   double *corr,          /* I:   Corrections to multiply to the region */
   int ncorr,		  /* I:   number of corrections */
   float firstC,	  /* I:   first correction is for this pixel value*/
   float lastC		  /* I:   last correction is for this pixel value */
   )
{
   int r;   
   float step = 1;
   if (ncorr > 1 )step = (lastC-firstC)/(ncorr-1) ;

   shAssert(reg->type == TYPE_FL32);
   
   for (r=0; r<reg->nrow; r++) {
      float *cp     = reg->rows_fl32[r];
      float *cppast = &reg->rows_fl32[r][reg->ncol];
      do {
	 float pix = *cp;
	 float steps;		/* number of steps, including fractional steps */
	 int idx;               /* greatest integer smaller that number of steps */
	 if (pix < firstC) continue;
	 if (pix > lastC ) continue;
	 idx = steps = (pix - firstC)/step;
	 if ((float)idx == steps) {
	    *cp = pix / corr[idx];  /* "integral pixel in a float" -- Optimize "common case" */
	 }else{
	    float w = steps - idx;  /* Interpolated case */
	    *cp = pix / (corr[idx] + w*(corr[idx+1]-corr[idx]));    
	 }
      } while (++cp < cppast);
   }
   return(SH_SUCCESS);
} 

/*****************************************************************************
 *
 * <AUTO EXTRACT>
 *
 * ROUTINE: atRegCorrectINT
 * 
 * DESCRIPTION: 
 * Muliply the corrections given in the array CORR to the pixels in REG.
 * The type of REG may not be TYPE_FL32. The routine treats the type U16
 * as a special optimised case -- regions of other types are handled through
 * regPixGetAsDbl. Fractional pixles are dithered in. The thing is too slow,
 * due to the necessity of manufaturing random numbers. Maybe they should be 
 * cached and re-used. 
 *
 *
 * Return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 *
 * </AUTO>
 */

RET_CODE atRegCorrectINT(
		    REGION *reg , /* I/0: Unsigned 16-bit integer region to 
				    correct */
		    double *corr, /* I:  Corrections to multiply to the 
				     region */
		    int ncorr,    /* I:   number of corrections */
		    int firstC	  /* I:   first correction is for this pixel 
				     value*/
		    ) {
   
  /* Need these for smaRAND calls */
  float runifs;

  float *dev   = alloca ((reg->ncol)*sizeof(float)); 
  /* Uniform deviants to use for dither*/
  int r, c;
  /* use first pixel as the seed.. */
  int seed = shRegPixGetAsDbl (reg, 0, 0) + 120356; 
   
#if 0
   /* The CERNLIB way, iffed out */
   smaRluxgo (4, seed, 0, 0);             /* will make this repeatable  */ 
#endif
   /* The SLATEC way */
   runifs = fabs(seed/98.7);
   smaRAND(runifs);
   /* end of the SLATEC way */

   corr -= firstC;                     /* We can 0-index now */
   
   for (r=0; r<reg->nrow; r++) {
      int dither, ipix;
      float *d = dev;
#if 0
      /* The CERNLIB way, iffed out */
      smaRanlux (dev, reg->ncol);
#endif
      /* The SLATEC way */
      for(c=0; c<reg->ncol; c++) {
	dev[c] = smaRAND(0.0);
      }
      /* end of the SLATEC way */
      if (reg->type == TYPE_U16) {
	 unsigned short int *cp     =  reg->rows_u16[r];
	 unsigned short int *cppast = &reg->rows_u16[r][reg->ncol];
	 do {
	    float  fract;
	    if (*cp < firstC) continue;
	    if (*cp > firstC + ncorr) continue;
	    fract = (float)*cp * corr[*cp];
	    ipix = fract;
	    fract -= ipix;
	    shAssert (0.0 <= fract && fract < 1.0);
	    dither =  (*d++ < fract ? 1 : 0 );
	    *cp = ipix + dither;      
	 } while (++cp < cppast);
      }else{ /* klunky. slow case for everyone else */ 
	int c;
	for (c=0; c<reg->ncol; c++) {
	    double  fract, pix;
	    pix = shRegPixGetAsDbl (reg, r, c);
	    if (pix < firstC) continue;
	    if (pix > firstC + ncorr) continue;
	    fract = pix * corr[(int)pix]; /* safe to index, as all integers 
					     have exact doubles */
	    ipix = fract;
	    fract -= ipix;
	    shAssert (0.0 <= fract && fract <= 1.0);
	    dither =  (*d++ < fract ? 1 : 0 );
	    ipix += dither;
            shRegPixSetWithDbl (reg, r, c, ipix);
	}
      }
   }
   return(SH_SUCCESS);
}

/*****************************************************************************
 *
 * <AUTO EXTRACT>
 *
 * ROUTINE: atRegCorrectQuick
 * 
 * DESCRIPTION: 
 * Muliply the corrections given in the array CORR to the pixels in REG.
 * The type of REG must be TYPE_U16. 
 * Fractional pixels are dithered in. 
 *
 * The difference between this and the "atRegCorrectINT" routine is that
 * "atRegCorrectINT" generates a random number for every pixel in the 
 * REGION, while "atRegCorrectQuick" re-uses a smaller set of random numbers.
 * 
 * We make a pair of tables, each 65536 elements long:
 *
 *    output[i]     contains the calculated U16 output value appropriate 
 *                      for an input value of 'i'; that is,
 *                         output[i] = integer part of (corrected value of i)
 * 
 *    dither[i]     contains the fractional part of the correction for
 *                      an input value of 'i'; that is,
 *                         dither[i] = (corrected value of i) - output[i]
 *
 * We use the 'dither' table, in conjunction with a set of random numbers
 * uniformly distributed between 0.0 and 1.0, to add an additional 1
 * to the output value for _some_ of the input values.
 *
 * The correction factors 'corr[]' are assumed to be given at uniform
 * steps between 'firstC' and 'lastC'.  That is, given 
 *    firstC =  100
 *    lastC  = 1000
 *    ncorr  =   10
 * we assume that
 *                   corr[0] applies at input value 100
 *                   corr[1] applies at input value 200
 *                   corr[2] applies at input value 300
 *                            ...
 *                   corr[8] applies at input value 900
 *                   corr[9] applies at input value 1000
 *
 *
 * Return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 *
 * </AUTO>
 */

#undef DEBUG

static U16 regCorrect_output[65536];
static float regCorrect_dither[65536];

RET_CODE
atRegCorrectQuick(
		      REGION *reg,  /* I/0: Unsigned 16-bit integer 
				      region to correct */
		      double *corr, /* I:   Corrections to multiply to 
				       the region */
		      int ncorr,    /* I:   number of corrections */
		      int firstC,   /* I:   first correction is for this 
				       pixel value*/
		      int lastC     /* I:   last correction is for this 
				       pixel value */
		      ) {
  /* Need these for smaRAND calls */
  float runifs;

  int r, c;
  float step = 1;
  float outval, corfactor, dcor, w;
  int i, pixval, start, end;
  /* Uniform deviants to use for dither */
  float *dev = alloca (2*(reg->ncol)*sizeof(float)); 
  /* use first pixel as the seed.. */
  int seed = shRegPixGetAsDbl (reg, 0, 0) + 120356; 

/* check inputs */
  shAssert(reg != NULL);
  shAssert(reg->type == TYPE_U16);
  shAssert ( (reg->nrow + reg->ncol) > 0 );
  shAssert(corr != NULL);
  shAssert(ncorr >= 0);
  shAssert(lastC >= firstC);
  
#if 0
  /* The CERNLIB way, iffed out */
  smaRluxgo (4, seed, 0, 0);             /* will make this repeatable  */ 
#endif
  /* The SLATEC way */
  runifs = abs(seed/98.7);
  smaRAND(runifs);
  /* end of the SLATEC way */
  
  
  /* first, fill the tables of 'output' and 'dither' values */
  if (ncorr > 1) {
    step = (lastC - firstC)/(ncorr - 1);
  }
#ifdef DEBUG
  printf("atRegCorrectQuick: filling elems 0 to %d with i and 0.0\n", firstC);
#endif
  for (i = 0; i < firstC; i++) {
    regCorrect_output[i] = i;
    regCorrect_dither[i] = 0.0;
  }
  start = firstC;
  for (i = 0; i < ncorr - 1; i++, start += step) {
    end = start + step;
    corfactor = corr[i];
    dcor = (corr[i + 1] - corr[i])/step;
    
#ifdef DEBUG
    printf("atRegCorrectQuick: number %2d, start %5d, end %5d, corf %5.2f\n",
	   i, start, end, corfactor);
#endif
    for (pixval = start; pixval < end; pixval++) {
      w = pixval - start;
      outval = pixval*(corfactor + w*dcor);
      /* this is the integer portion ... */
      regCorrect_output[pixval] = (U16) outval;
      /*   and now the fractional portion */
      regCorrect_dither[pixval] = outval - (U16) outval;
#ifdef DEBUG
      if (pixval < start + 5 || pixval > end - 5) {
	printf("     input %5d   outval %8.2f  output %5d  dither %5.2f\n",
	       pixval, outval, regCorrect_output[pixval],
	       regCorrect_dither[pixval]);
      }
#endif
      
    }
  }
#ifdef DEBUG
  printf("atRegCorrectQuick: filling elems %d to 65536 with i and 0.0",
	 lastC);
#endif
  for (i = lastC; i < 65536; i++) {
    regCorrect_output[i] = i;
    regCorrect_dither[i] = 0.0;
  }
  
  /* 
   * having filled the lookup tables, we can now loop through the
   * region, one row at a time, and replace each pixel with the
   * output table's value, plus an extra bit if the random number 'd'
   * is less than the fraction portion of the corrected value.
   */
  for(c=0; c<reg->ncol*2; c++) {
    dev[c] = smaRAND(0.0);
  }
  for (r = 0; r < reg->nrow; r++) {
    
    /* 
     * we have to pick the spot into which to start walking through
     * the array "dev" of random numbers.  The array is 2*ncol long,
     * so if we pick a random number between 0 and (ncol-1), we can
     * start at that point and move forward "ncol" elements safely.
     */
    float *d = dev + (int)(dev[(r % reg->ncol)]*reg->ncol);
    shAssert(d >= dev);
    shAssert((d - dev) < reg->ncol);
    {
      unsigned short int *cp     = reg->rows_u16[r];
      unsigned short int *cppast = &reg->rows_u16[r][reg->ncol];
      do {
	pixval = regCorrect_output[*cp];
	if (*d < regCorrect_dither[*cp]) {
	  pixval++;
	}
	d++;
	*cp = pixval;
      } while (++cp < cppast);
    }
  }
  return(SH_SUCCESS);
}


/*****************************************************************************
 *
 * <AUTO EXTRACT>
 *
 * ROUTINE: atRegSigmaClip
 * 
 * DESCRIPTION: 
 *
 *	Find the mean and sigma of the pixels in a region, but only use
 * the pixel values within sigma_clip sigma of the mean to calculate these
 * numbers.  The routine will iterate number_of_iterations times.
 *
 * return: SH_SUCCESS          if all goes well
 *
 * </AUTO>
 */

RET_CODE atRegSigmaClip(
                REGION *regPtr,                 /* the data */
                double sigma_clip,              /* how many sigma */
                int number_of_iterations,       /* how many times */
                double *sigma_mean,             /* the sigma-clipped  mean */
                double *sigma_stddev            /* the sigma-clipped sigma */
        )

{
/* declare variables */
        int r, c, ncols, nrows;
        int i, j;
        int number_of_numbers;
        int new_counter = 0;
        int new_loader = 0;
        double *new_numbers;
        double stddev, clip;
        double mean = 0;
  /*      int returnValue = SH_SUCCESS;   not needed  */

/* error check */
        nrows = regPtr->nrow;
        ncols = regPtr->ncol;
        number_of_numbers = nrows * ncols;
	new_numbers = (double *) shMalloc(number_of_numbers * sizeof(double) );

/* fill numbers array */
        i = 0;
        for (r=0; r <= nrows-1; r++) {
         for (c=0; c <= ncols-1; c++) {
                new_numbers[i] = shRegPixGetAsDbl(regPtr, r, c);
                i++;
        } }

/*
 *                      Here the work is done
 */

/* do the iteration */
        for (i=1; i<=number_of_iterations; i++) {

/* calculate the mean and stddev, noting sigma clipping point */
                find_stddev(new_numbers, number_of_numbers, &mean, &stddev);
                clip = sigma_clip * stddev;
/* find those points inside the sigma clip and store them */
                new_counter =0;
                new_loader  =0;
                for (j=0; j < number_of_numbers; j++) {
                     if (fabs(new_numbers[j]-mean) <= clip)  {
                       if(new_loader != new_counter) {
                          new_numbers[new_loader] = new_numbers[new_counter];
                       }
                       new_loader++;
                     } 
                     new_counter++;
                }

/* go back for another iteration */
                number_of_numbers = new_loader;
        }

/*  return the answer   */

/* calculate the mean and stddev, noting sigma clipping point */
                find_stddev(new_numbers, number_of_numbers, &mean, &stddev);
        
        *sigma_mean = mean;
        *sigma_stddev = stddev;
	shFree (new_numbers);

        return(SH_SUCCESS);
}


/*****************************************************************************
 *
 * <AUTO EXTRACT>
 *
 * ROUTINE: atRegPixRangeSubstitute
 * 
 * DESCRIPTION: 
 * Given a REGION, two (double) values, and a (double) new value, 
 * replace all instances of pixels with value inside range with 'newvalue'.
 *
 * Return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 *
 * </AUTO>
 */

RET_CODE
atRegPixRangeSubstitute
   (
   REGION *reg,            /* I: region in which we subsitute values */
   double minval,          /* I: we replace any pixel greater than this value... */
   double maxval,          /* I: and less than or equal to this value... */
   double newval          /* I:   ... by this value */
   )
{
   int row, col;
   double curVal;

   /* make some sanity checks on the input regions */
   shAssert(reg != NULL);

   
   /* Substitute values less than or equal to oldval */
   for (row = 0; row < reg->nrow; row++) {
     for (col = 0; col < reg->ncol; col++) {
       curVal = shRegPixGetAsDbl(reg, row, col);
       if ( curVal > minval && curVal <= maxval) {
	 shRegPixSetWithDbl(reg, row, col, newval);
       }
     }
   }
   
   return(SH_SUCCESS);
}

   

/*****************************************************************************
 *
 * <AUTO EXTRACT>
 *
 * ROUTINE: atRegMedianSmooth
 * 
 * DESCRIPTION: 
 * Smooth a REGION using an nxm median filter.
 *
 * Return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 *
 * </AUTO>
 */

RET_CODE
atRegMedianSmooth
   (
   REGION *input,          
   int m,		/* number of rows to include in filter */
   int n,		/* number of cols to include in filter */
   REGION *output       
   )
{
   int i,j, row, col, count, n_delta, m_delta;
   double median, *array=NULL;
   U16 medianU16, *arrayU16=NULL;

   /* make some sanity checks on the input regions */
   shAssert(input != NULL);
   shAssert(output != NULL);

   if (output->ncol != input->ncol) {
      shError("atRegMedianSmooth: input and output 'ncol' don't match");
      return(SH_GENERIC_ERROR);
   }
   if (!(m & 01) || !(n & 01) ) {
      shError("atRegMedianSmooth: filter must have odd sizes");
      return(SH_GENERIC_ERROR);
   }
   
   if ( !(input->type == TYPE_U16 && output->type == TYPE_U16) &&
	!(input->type == TYPE_FL32 && output->type == TYPE_FL32) ) {
      shError("atRegMedianSmooth: unsupported image type or conversion");
      return(SH_GENERIC_ERROR);
   }

   if (input->type == TYPE_U16 && output->type == TYPE_U16) {
      if ((arrayU16 = (U16 *) shMalloc(m*n*sizeof(U16))) == NULL) {
         shError("atRegMedianSmooth: can't shMalloc for %d U16s", m*n);
         return(SH_GENERIC_ERROR);
      }
   } else {
      if ((array = (double *) shMalloc(m*n*sizeof(double))) == NULL) {
         shError("atRegMedianSmooth: can't shMalloc for %d doubles", m*n);
         return(SH_GENERIC_ERROR);
      }
   }

   /*
    * we have special-case code for U16 and FL32 only. 
    */
   m_delta = (m -1)/2; /* since we know these are odd, this tells us how many */
   n_delta = (n -1)/2; /* pixels around center to work */
   if (input->type == TYPE_FL32 && output->type == TYPE_FL32) {
      for (col = 0; col < input->ncol; col++) {
         for (row = 0; row < input->nrow; row++) {
   	    count = 0;
	    for ( i = col - n_delta; i <= col + n_delta; i++) {
	   	for ( j = row - m_delta; j <= row + m_delta; j++) {
			if (i < 0 || i >= input->ncol) { continue; }
			if (j < 0 || j >= input->nrow) { continue; }
			array[count] = input->rows_fl32[j][i];
			count++;
	    	}
	    }
            /* now that 'array' is filled with values, find median */
            if (n == 1) {
	       median = array[0];
            } else {
	       median = at_find_median(array, count);
            }
            output->rows_fl32[row][col] = median;
	}
      }
   } else { /* is U16  */
      for (col = 0; col < input->ncol; col++) {
         for (row = 0; row < input->nrow; row++) {
   	    count = 0;
	    for ( i = col - n_delta; i <= col + n_delta; i++) {
	   	for ( j = row - m_delta; j <= row + m_delta; j++) {
			if (i < 0 || i >= input->ncol) { continue; }
			if (j < 0 || j >= input->nrow) { continue; }
			arrayU16[count] = input->rows_u16[j][i];
			count++;
	    	}
	    }
            /* now that 'array' is filled with values, find median */
            if (n == 1) {
	       medianU16 = arrayU16[0];
            } else {
	       medianU16 = at_find_medianU16(arrayU16, count);
            }
            output->rows_u16[row][col] = medianU16;
	 }
      }
   } 
   if (input->type == TYPE_U16 && output->type == TYPE_U16) {
   	shFree(arrayU16);
   } else {
   	shFree(array);
   }

   return(SH_SUCCESS);
}


