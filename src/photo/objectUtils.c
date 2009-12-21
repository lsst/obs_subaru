#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <alloca.h>
#include "dervish.h"
#include "phObjects.h"
#include "phMaskbittype.h"
#include "phExtract.h"
#include "phSkyUtils.h"
#include "phObjectCenter.h"
#include "phConsts.h"
#include "phBrightObjects.h"

static void
fit_peak_center(PEAK *peak,		/* the peak in question */
		const REGION *in,	/* region containing peak */
		const FRAMEPARAMS *fparams, /* gain etc. */
		int binmax,		/* maximum allowed binning */
		int use_mean,		/* use mean not median? */
		CENTROID_FLAGS cflags)	/* control the centroiding */
	;

static void
object_center_fit(PEAK *peak,		/* PEAK to fit centre of */
		  int *flags,		/* object's flags */
		  int *flags2,		/* object's flags2 */
		  const OBJMASK *objmask, /* pixels in object */
		  float *rowc, float *rowcErr, /* returned row, */
		  float *colc, float *colcErr, /*    column, */
		  const REGION *in,	/* region containing obj */
		  const FRAMEPARAMS *fparams, /* gain etc. */
		  int binmax,		/* maximum allowed binning */
		  int use_mean,		/* use mean not median? */
		  CENTROID_FLAGS cflags) /* control the centroiding */
	;

/*****************************************************************************/
/*
 * Do the work of finding the maximum
 */
#define FIND_MAX(TYPE, ROWS_PTR) \
   TYPE **rows = reg->ROWS_PTR, *row; \
   \
   if(rowc == NULL && colc == NULL) { \
      for(r = row0; r <= row1; r++) { \
         row = rows[r]; \
         for(c = col0; c <= col1; c++) { \
	    if(row[c] > max) { \
	       max = row[c]; rmax = r; cmax = c; \
	    } \
	 } \
      } \
   } else { \
      double val; \
      \
      shAssert(rowc != NULL && colc != NULL); \
      \
      for(r = row0; r <= row1; r++) { \
         row = rows[r]; \
         for(c = col0; c <= col1; c++) { \
            val = row[c]; \
	    if(val > max) { \
	       max = row[c]; rmax = r; cmax = c; \
	    } \
	    val -= bkgd; \
	    sum += val; \
	    sumr += r*val; \
	    sumc += c*val; \
	 } \
      } \
   }


/*
 * <AUTO EXTRACT>
 *
 * Like phObjectCenterFit (q.v.), but for an PEAK
 */
void
phPeakCenterFit(PEAK *peak,		/* the peak in question */
		const REGION *in,	/* region containing peak */
		const OBJMASK *mask,	/* pixels comprising peak's object */
		const FRAMEPARAMS *fparams, /* gain etc. */
		int binmax,		/* maximum allowed binning */
		CENTROID_FLAGS cflags)	/* control the centroiding */
{
   int use_mean = 1;			/* XXX */
   int flags = 0;			/* OBJECT1->flags style flags */
   int flags2 = 0;			/* OBJECT1->flags2 style flags */
   
   shAssert(peak != NULL);
   shAssert(in != NULL && in->type == TYPE_PIX);
   shAssert(fparams != NULL);

   if(peak->flags & PEAK_SATUR) {
      flags |= OBJECT1_SATUR;
   }

   object_center_fit(peak, &flags, &flags2, mask,
		     &peak->rowc, &peak->rowcErr, &peak->colc, &peak->colcErr,
		     in, fparams, binmax, use_mean, cflags);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Find the centre of an OBJECT (see also phObject1CenterFit)
 *
 * If the object contains saturated pixels, use phSaturatedStarCentroid;
 * otherwise find the position of the image's maximum using fit_peak_center
 */
static void
object_center_fit(PEAK *peak,		/* PEAK to fit centre of */
		  int *flags,		/* object's flags */
		  int *flags2,		/* object's flags2 */
		  const OBJMASK *objmask, /* pixels in object */
		  float *rowc, float *rowcErr, /* returned row, */
		  float *colc, float *colcErr, /*    column, */
		  const REGION *in,	/* region containing obj */
		  const FRAMEPARAMS *fparams, /* gain etc. */
		  int binmax,		/* maximum allowed binning */
		  int use_mean,		/* use mean not median? */
		  CENTROID_FLAGS cflags) /* control the centroiding */
{
   shAssert(in != NULL && in->type == TYPE_PIX);
   shAssert(fparams != NULL);
   shAssert(peak != NULL);

   if(*flags & OBJECT1_SATUR) {
      const SPANMASK *sm = (SPANMASK *)(in->mask);
      shAssert(sm->cookie == SPAN_COOKIE);

      if(phRectIntersectMask(sm->masks[S_MASK_SATUR],
			     peak->cpeak - 1,peak->rpeak - 1,
			     peak->cpeak + 1,peak->rpeak + 1)) {
	 peak->flags |= PEAK_SATUR;

	 shAssert(objmask != NULL);
	 *colc = peak->cpeak; *rowc = peak->rpeak;
	 if(phSaturatedStarCentroid(in, objmask, fparams->psf,
				 fparams->bkgd + SOFT_BIAS, colc, rowc) == 0) {
	    *colcErr = *rowcErr = 1;
	 } else {
	    peak->flags |= PEAK_PEAKCENTER;
	    *flags |= OBJECT1_PEAKCENTER;
	    *rowc = peak->rpeak + 0.4999; /* 0.4999 so that */
	    *colc = peak->cpeak + 0.4999; /*     (int)(colc+0.5) == cpeak */
	    
	    *colcErr = *rowcErr = 1;
	 }
	 
	 peak->rowc = *rowc; peak->rowcErr = *rowcErr;
	 peak->colc = *colc; peak->colcErr = *colcErr;

	 peak->rpeak = (int)(peak->rowc + 0.5);
	 peak->cpeak = (int)(peak->colc + 0.5);
	 shAssert(peak->rpeak >= 0 && peak->rpeak < in->nrow &&
		  peak->cpeak >= 0 && peak->cpeak < in->ncol);

	 peak->peak = in->rows[peak->rpeak][peak->cpeak];
	 
	 return;
      }
   }

   fit_peak_center(peak, in, fparams, binmax, use_mean, cflags);
   
   *rowc = peak->rowc; *rowcErr = peak->rowcErr;
   *colc = peak->colc; *colcErr = peak->colcErr;
/*
 * Propagate some PEAK_... flags. Note that we ignore PEAK_BINNED_EDGE,
 * as it can be easily set for perfectly reasonable, but large, objects
 */
   if(peak->flags & PEAK_EDGE) {
      *flags |= OBJECT1_EDGE;
   }
   if(peak->flags & PEAK_PEAKCENTER) {
      *flags |= OBJECT1_PEAKCENTER;
   }
   if(peak->flags & PEAK_BINNED_CENTER) {
      *flags2 |= OBJECT2_BINNED_CENTER;
   }
}

/*
 * <AUTO EXTRACT>
 *
 * Find the highest pixel in a region, only searching the rectangle with
 * corners (row0, col0) and (row1, col1). If row1 and/or col1 are <= 0,
 * take them to be relative to the top right of the REGION
 *
 * If rowc and colc are non-NULL the centroid of the region is returned,
 * assuming a background level of bkgd.  Please specify neither or both.
 *
 * If more than one pixel has the same maximum value, the one found first
 * in a row-by-row search starting at (row0, col0) will be returned
 *
 * Return 0 if all is well; -1 if the region type is not supported
 */
int
phRegIntMaxPixelFind(const REGION *reg,	/* region to search */
		     float bkgd,	/* background level (only used
					 if rowc/colc != NULL) */
		     int row0, int col0, /* LLC of rectangle to search */
		     int row1, int col1, /* URC of rectangle to search */
		     int *rpeak, int *cpeak, /* position of maximum */
		     float *rowc, float *colc) /* region centroid, or NULL */
{
   int r, c;
   int max = -1e9, rmax = 0, cmax = 0;	/* max. pixel, and position of max */
   int ncol,nrow;			/* unpacked from reg */
   double sum = 0, sumr = 0, sumc = 0;	/* sum(I), sum({row,col}*I) */

   shAssert(reg != NULL);
   ncol = reg->ncol; nrow = reg->nrow;
   if(row1 <= 0) { row1 += nrow - 1; }
   if(col1 <= 0) { col1 += ncol - 1; }

   shAssert(row0 >= 0 && row1 >= row0 && row1 < nrow);
   shAssert(col0 >= 0 && col1 >= col0 && col1 < ncol);

   if(reg->type == TYPE_U8) {
      FIND_MAX(U8, rows_u8);
   } else if(reg->type == TYPE_S8) {
      FIND_MAX(S8, rows_s8);
   } else if(reg->type == TYPE_U16) {
      FIND_MAX(U16, rows_u16);
   } else if(reg->type == TYPE_S16) {
      FIND_MAX(S16, rows_s16);
   } else if(reg->type == TYPE_U32) {
      FIND_MAX(U32, rows_u32);
   } else if(reg->type == TYPE_S32) {
      FIND_MAX(S32, rows_s32);
   } else if(reg->type == TYPE_FL32) {
      FIND_MAX(FL32, rows_fl32);
   } else {
      shError("phRegIntMaxPixelFind doesn't handle regions of type %d\n",
	      reg->type);
      return(-1);
   }
   
   *rpeak = rmax; *cpeak = cmax;
   if(rowc != NULL) {
      shAssert(sum != 0);
      *rowc = sumr/sum + 0.5;		/* (0.5, 0.5) is centre of pixel */
      *colc = sumc/sum + 0.5;
   }

   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Find a PEAK's centre
 *
 * It is expected that this routine be used on a _smoothed_ region; in
 * the case that the region has been smoothed with the PSF this is equivalent
 * to a MLE fit for the centre using the PSF, if the peak counts are less
 * than sky. Nevertheless, if the provided FRAMEPARAMS specifies a psf
 * and the object's bright enough (and NO_SMOOTH isn't set) the region will
 * be smoothed as part of centroiding.
 *
 * When the object's peak is in an edge pixel we are unable to fit, so the
 * centre of the peak pixel is found; this is also what we do when the
 * reported peak pixel is not the highest in its vicinity --- something
 * that can happen if some of the pixels are masked
 */
static void
fit_peak_center(PEAK *peak,		/* the peak in question */
		const REGION *in,	/* region containing peak */
		const FRAMEPARAMS *fparams, /* gain etc. */
		int binmax,		/* maximum allowed binning */
		int use_mean,		/* use mean not median? */
		CENTROID_FLAGS cflags)	/* control the centroiding */
{
   BINREGION *binreg = NULL;		/* median smoothed region */
   int binsize;				/* amount region's binned by */
   int binsize_r, binsize_c;		/* row/column binning (<= binsize) */
   int cc, rc;				/* the peak pixel */
   int c0, r0;				/* initial guess for smoothed peak */
   float rowc = 0, colc = 0;		/* latest estimate of rowc etc. */
   float rowcErr = 0, colcErr = 0;	/*       (initialised to placate cc) */
   REGION *in_sub;			/* subregion of in */
   int ret;
   float gain, dark_variance;		/* unpacked */
   int bkgd;				/*       from fparams */
   float peakval = 0;			/* peak value in object */
   float peak_min;			/* min. value of peakval to use
					   statistical not systematic errors */
   const REGION *preg;			/* region containing peak */
   const DGPSF *psf;			/* == fparams->psf */
   float sigma;				/* smoothing sigma to use */
   float sigsqrow = 0, sigsqcol = 0;	/* width^2 in row and column */
   float sky;				/* sky level */
   const SPANMASK *sm;			/* mask associated with in */
   int use_peak = 0;			/* use peak pixel as centre */
   float wfac;				/* critical width */

   shAssert(peak != NULL);
   shAssert(in != NULL && in->type == TYPE_PIX);
   shAssert(fparams != NULL && fparams->sky != NULL);

   psf = fparams->psf;
   bkgd = fparams->bkgd + SOFT_BIAS;
   dark_variance = phDarkVariance(fparams, peak->rpeak, peak->cpeak);
   gain = phGain(fparams, peak->rpeak, peak->cpeak);
   peak_min = fparams->min_peak_statistical_errors;

   sigma = (psf == NULL) ? 0 : psf->sigma1_2G;
   if((cflags & ALWAYS_SMOOTH)) {
      ;
   } else if((cflags & NO_SMOOTH) ||
	     fparams->psf == NULL ||
	     peak->peak > fparams->peak_smooth_thresh) {
      sigma = 0;			/* no smoothing */
   }
   if(cflags & SMOOTHED) {
      shAssert((cflags & NO_SMOOTH) != 0 && fparams->smooth_sigma >= 0);
      sigma = -fparams->smooth_sigma;
   }
#if 0					/* Checking v. SSC ONLY! */
   sigma = 1.2;				/* The SSC's value */
#endif

   wfac = (psf == NULL) ? 1 : psf->width*psf->width;
   wfac *= 1.5;
   
   cc = peak->cpeak; rc = peak->rpeak;

   sm = (SPANMASK *)(in->mask);
   if(sm != NULL) {
      shAssert(sm->cookie == SPAN_COOKIE);
      
      if(phPixIntersectMask(sm->masks[S_MASK_SATUR], cc, rc)) {
	 peak->flags |= PEAK_SATUR;
	 if(peak->rowcErr < 0) {	/* not set yet */
	    peak->flags |= PEAK_PEAKCENTER;
	    peak->colc = cc + 0.4999;	/* 0.4999 so that */
	    peak->rowc = rc + 0.4999;	/*           (int)(colc+0.5)==cc */
	    peak->colcErr = peak->rowcErr = 1;
	 }
	 
	 return;
      }
   }
   
   sky = phBinregionInterpolate(fparams->sky, rc, cc);

   binsize_c = binsize_r = binsize = 1;
   for(;binsize <= binmax;binsize *= 2) {
      if(binsize == 1) {
	 in_sub = NULL;
	 preg = in;
	 c0 = cc; r0 = rc;
      } else {
	 const int nrow_sub = 10 + 5*binsize_r;
	 const int ncol_sub = 10 + 5*binsize_c;
	 phBinregionDel(binreg); binreg = NULL;

	 peak->flags |= PEAK_BINNED_CENTER;
	   
	 in_sub = shSubRegNew("fit_peak_center",in, nrow_sub, ncol_sub,
			      rc - nrow_sub/2, cc - ncol_sub/2, NO_FLAGS);
	 if(in_sub == NULL) {
	    peak->flags |= PEAK_EDGE;	/* too close to edge for a fit */
	    use_peak = 1;
	 
	    break;
	 }

	 binreg = phMedianSmooth(in_sub, binsize_c, binsize_r,
				 1, -1, -1, 0, 0, NULL, use_mean, (PIXDATATYPE)0);
	 
	 preg = binreg->reg;
	 c0 = preg->ncol/2; r0 = preg->nrow/2;
      }
/*
 * Try to find the centre. If we fail, use the highest pixel and set
 * warning flags
 */
      if((ret = phObjectCenterFind(preg, c0, r0, sky, sigma,
				   bkgd, dark_variance, gain,
				   &colc, &rowc, &colcErr, &rowcErr,
				   &peakval, &sigsqcol, &sigsqrow)) < 0) {

	 if(ret == -1) {		/* centre not highest pixel */
	    ret = phObjectCenterFind(preg, c0, r0, sky, 0,
				     bkgd, dark_variance, gain,
				     &colc, &rowc, &colcErr, &rowcErr,
				     &peakval, &sigsqcol, &sigsqrow);
	 }

	 if(ret != 0 || rowcErr < 0 || colcErr < 0) {
	    if(rowcErr < 0 || colcErr < 0) { /* error would have been NaN */
	       use_peak = 1;
	    } else {
	       switch (ret) {
	       case -1:			/* r0, c0 isn't local maximum */
	       case -3:			/* vanishing second derivative */
	       case -4:			/* central pixel is negative */
	       case -5:			/* central pixel is zero */
		  use_peak = 1;
		  break;
	       case -2:			/* too close to edge for a fit */
		  if(binsize == 1) {
		     peak->flags |= PEAK_EDGE;
		     use_peak = 1;
		  } else {
		     peak->flags |= PEAK_BINNED_EDGE;
		  }
		  break;
	       default:
		  shFatal("fit_peak_center: "
			  "unknown return from phObjectCenterFind: %d", ret);
	       }
	    }
	    
	    shRegDel(in_sub);
	    
	    break;
	 }
      }
      
      if(in_sub != NULL) {
	 if(binsize_r == 1) {
	    rowc += in_sub->row0;
	 } else {
	    rowc = in_sub->row0 + (rowc - 0.5)*(binsize_r/2);
	    rowcErr *= binsize/2;
	    sigsqrow *= binsize_r*binsize_r/4;
	 }
	 if(binsize_c == 1) {
	    colc += in_sub->col0;
	 } else {
	    colc = in_sub->col0 + (colc - 0.5)*(binsize_c/2);
	    colcErr *= binsize/2;
	    sigsqcol *= binsize_c*binsize_c/4;
	 }
	 
	 shRegDel(in_sub);
      }
      
      if(sigsqrow < wfac*binsize_r*binsize_r &&
	 sigsqcol < wfac*binsize_c*binsize_c) {
	 if(binsize == 1) {
	    peak->rowc = rowc; peak->rowcErr = rowcErr;
	    peak->colc = colc; peak->colcErr = colcErr;
	 }
#if 1
	 if(binsize > 1 || peakval > peak_min) {
	    break;
	 }
#else
	 break;
#endif
      }
      peak->rowc = rowc; peak->rowcErr = rowcErr;
      peak->colc = colc; peak->colcErr = colcErr;

      if(sigsqrow > wfac*binsize_r*binsize_r) {
	 binsize_r *= 2;
      }
      if(sigsqcol > wfac*binsize_c*binsize_c) {
	 binsize_c *= 2;
      }
   }
   phBinregionDel(binreg);
/*
 * Did we have problems?
 */
   if(use_peak) {
      peak->flags |= PEAK_PEAKCENTER;
      peak->colc = cc + 0.4999; peak->rowc = rc + 0.4999; /* 0.4999 so that
							 (int)(colc+0.5)==cc */
      peak->colcErr = binsize_c;
      peak->rowcErr = binsize_r;

      return;
   }
/*
 * there's a lower limit on the possible error in a position
 */
   {
      const float astrom_floor2 = fparams->astrom_floor*fparams->astrom_floor;
      if(peak->rowcErr > 0) {
	 peak->rowcErr = sqrt(peak->rowcErr*peak->rowcErr + astrom_floor2);
      }
      if(peak->colcErr > 0) {
	 peak->colcErr = sqrt(peak->colcErr*peak->colcErr + astrom_floor2);
      }
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Find the highest pixel in an object, specified as an OBJMASK and REGION;
 * if the OBJMASK is NULL, search the entire region
 *
 * Return 1, unless there are no good pixels in the object, in which
 * case return 0
 */
int
phObjectPeakFind(const REGION *reg,     /* region where object was found */
		 const OBJMASK *om,     /* Mask of pixels in object, or NULL */
                 PEAKS *peaks)		/* the peak of the object */
{
   int i;
   int max, max_row, max_col;		/* maximum in om, and its position */
   PIX *row;
   int x;				/* row counter */

   shAssert(reg != NULL && reg->type == TYPE_PIX && peaks != NULL);
   if(om != NULL && om->npix == 0) {
      return(0);
   }
/*
 * allocate/free enough space to give peaks exactly one PEAK
 */
   phPeaksRenew(peaks,1);
   peaks->npeak = 1;

   max = -1;
   max_row = max_col = 0;		/* pacify compilers */

   if(om == NULL) {			/* search entire region */
      int nrow = reg->nrow, ncol = reg->ncol;
      for(i = 0;i < nrow; i++) {
	 row = reg->ROWS[i];
	 for(x = 0; x < ncol; x++) {
	    if(row[x] > max) {
	       max = row[x];
	       max_row = i;
	       max_col = x;
	    }
	 }
      }
   } else {				/* only search within om */
      for(i = 0;i < om->nspan;i++) {
	 row = reg->ROWS[om->s[i].y];
	 for(x = om->s[i].x1;x <= om->s[i].x2;x++) {
	    if(row[x] > max) {
	       max = row[x];
	       max_row = om->s[i].y;
	       max_col = x;
	    }
	 }
      }
   }
   shAssert(max >= 0);

   peaks->peaks[0]->rpeak = max_row;
   peaks->peaks[0]->cpeak = max_col;
   peaks->peaks[0]->peak = max;

   peaks->peaks[0]->rowc = peaks->peaks[0]->rpeak + 0.4999;
   peaks->peaks[0]->colc = peaks->peaks[0]->cpeak + 0.4999;
   
   return(peaks->npeak);
}
