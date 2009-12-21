/*
 * Find a median smoothed image
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "dervish.h"
#include "phHist.h"
#include "phSkyUtils.h"
#include "phUtils.h"
#include "phConsts.h"
#include "phObjects.h"
#include "phMathUtils.h"

#define USE_CUMULATIVE 1		/* if true, use the cumulative code */

#define NQUANTILES 3			/* number of quantiles wanted; in fact
					   minor code changes are required
					   to get more than the 3 quartiles */
#define NSIGMA_HIST 10			/* number of sigma in sky histogram
					   to include in measured histogram */

static void fill_histogram(PHHIST *h, PIX **ptr,
			   int col0, int row0, int dc, int dr, int use_mean );
#if 0
static void print_histogram(PHHIST *h);
#endif

static int get_quartiles(PHHIST *h0, PHHIST *h1, PHHIST *h2,
			 PHHIST *h3, float *perc_u, float *perc );
static int get_quartiles_c(PHHIST *h0, PHHIST *h1, PHHIST *h2,
			   PHHIST *h3, PHHIST_CUMUL *chist,
			   float *perc_u, float *perc );
static int get_quartiles_work(PHHIST *h0,
			      int n, float *perc, int sum0, int i0 );
static int get_quartiles_work_c(PHHIST_CUMUL *chist,
				int n, float *perc, int i0 );
static void shift_to_pixel_centres(REGION *out, int ncol, int nrow,
				   int dc, int dr, int min);


/*****************************************************************************/

static int
get_center_width(int use_mean,
		 PHHIST *h0, PHHIST *h1, PHHIST *h2, PHHIST *h3,
		 PHHIST_CUMUL *chist,			/* cumulative histogram of pixels */
		 const int filtsize_r, const int filtsize_c, 
		 float *center, float *width)
{
    int i;
    int npix;				/* number of usable pixels */
    float quartiles_u[NQUANTILES];	/* unclipped quartiles of histogram */
    float quartiles[NQUANTILES];	/* clipped quartiles of histogram */

    if(use_mean) {
	float s0 = 0, s1 = 0, s2 = 0, s3 = 0;
	unsigned int n = 0;
	float sigma;

	/* filtsize_[rc] == 1 => don't use previous line/column */
	if (h0 != NULL && filtsize_r > 1) {
	    s0 = h0->sum; n++;
	}
	if (h1 != NULL && filtsize_r > 1 && filtsize_c > 1) {
	    s1 = h1->sum; n++;
	}
	if (h2 != NULL) {
	    s2 = h2->sum; n++;
	}
	if (h3 != NULL && filtsize_c > 1) {
	    s3 = h3->sum; n++;
	}
	
	npix = 1;			/* value isn't used, so 1's good enough */
	
	quartiles[1] = (s0 + s1 + s2 + s3)/n;
	sigma = (s0*s0 + s1*s1 + s2*s2 + s3*s3)/n - quartiles[1]*quartiles[1];

	if(n > 1) {
	    sigma = sqrt(n/(float)(n - 1)*sigma); /* debias, and convert to s.d. */
	}

	quartiles[0] = quartiles[1] - sigma;
	quartiles[2] = quartiles[1] + sigma;
	for(i = 0;i <= 2;i++) {
	    int total_number =
		h0 ? h0->total_number :
		h1 ? h1->total_number : 
		h2 ? h2->total_number : 
		h3 ? h3->total_number : 0;

	    quartiles[i] /= total_number;
	    quartiles_u[i] = quartiles[i];
	}
    } else {
	if(filtsize_r == 1) {	/* do NOT use previous line */
	    h0 = h1 = NULL;
	}
	if(filtsize_c == 1) {	/* do NOT use previous column */
	    h0 = h2 = NULL;
	}
#if USE_CUMULATIVE
	npix = get_quartiles_c(h0, h1, h2, h3,
			       chist, quartiles_u, quartiles);
#else
	npix = get_quartiles(h0, h1, h2, h3, quartiles_u, quartiles);
#endif
    }

    if(npix <= 0) {
	*center = *width = MAX_U16;
    } else {
	float iqr = quartiles[2] - quartiles[0];
	*center = quartiles[1];
	*width = FLT2PIX(IQR_TO_SIGMA*iqr/sqrt(npix));
    }
    
    return npix;
}

/*****************************************************************************/
/*
 * <EXTRACT AUTO>
 *
 * Calculate the median-smoothed image <out> from an input region, <in>.
 * The filter to apply is of size filtsize_r*filtsize_c where filtsize_r
 * is the size of the filter used when smoothing in the row direction
 * (it's the number of columns), and filtsize_c is the dimension of the
 * filter along the columns (it's the number of rows)
 *
 * The resulting REGION contains the values of the medians smoothed in
 * filtsize_r*filtsize_c boxes, centred on
 *   ((i+1)*filtsize_r/2, (j+1)*filtsize_c/2)
 * centres, i.e. with overlapping filters (unless use_mean is true, in
 * which case _means_ not medians are used)
 *
 * In other words, if shift_to_centres is _false_, the position
 *     (n + 0.5, m + 0.5)
 * (i.e. the centre of a binned pixel for an even-sized filter) in the binned
 * image corresponds to
 *     (n*filtsize_r/2, m*filtsize_c/2)
 * in the input image (i.e. at the corner of a pixel if the filter's even).
 *
 * Put another way, if shift_to_centres is _false_ the unbinned position
 * corresponding to (rc, cc) in the binned image is
 *     ((rc - 0.5)*filtsize_r/2, (cc - 0.5)*filtsize_c/2)
 *
 * If no fatal errors occur, the output region is always returned.
 *
 * For a Gaussian the standard deviation is IQR_TO_SIGMA*(interquartile range)
 * (n.b. erfc(1/(sqrt(2)*2*IQR_TO_SIGMA) = 0.5)
 */
BINREGION *
phMedianSmooth(const REGION *in,	/* input region */
	       int filtsize_c,		/* size of filter */
	       int filtsize_r,
	       int coarse,		/* how coarsely to sample */
	       int minval, int maxval,	/* {min,max} pixel values to include
					   in histograms */
	       int shift,		/* how many bits to shift
					   output REGION (-ve: S32 output but shift == 0) */
	       int shift_to_centers,	/* shift returned region by 0.5,0.5? */
	       BINREGION *bskysig,	/* BINREGION to return sky sigma;
					   may be NULL*/
	       int use_mean,		/* use a mean NOT a median */
	       PIXDATATYPE regType)	/* type of desired REGION */
{
   int c,r;				/* column and row counters */
   PHHIST_CUMUL *chist;			/* cumulative histogram of pixels */
   float center, width;			/* location/scale of histogram (e.g. mean/s.d.) */
   int dr,dc;				/* step size along rows and cols*/
   int drp, dcp;			/* dr and dc, allowing for partial
					   areas at the edge of the REGION */
   PHHIST **histograms[2];		/* histograms of areas of image */
   PHHIST *histograms_s;		/* storage for histograms[] */
   int *histograms_ss;			/* storage for histograms[]->value */
   PHHIST *h0, *h1, *h2, *h3;		/* the current histograms of interest */
   int i;
   int nr,nc;				/* no. of steps along rows and cols*/
   int npix = 0;			/* number of pixels involved in
					   estimating mean and quartiles */
   int nval;				/* no. of pixel values in histogram */
   REGION *out;				/* output region */
   REGION *skysig;			/* sky sigma region */
   float median;			/* median of histogram */
   float scale = (shift >= 0 ? (1 << shift) : 1/(float)(1 << (-shift)));
					/* how much to scale output REGION */
   float iqr,sigma;			/* estimated semi-interquartile range
					   and sigma of histogram */

   if (regType == (PIXDATATYPE)0) {	/* type of desired REGION */
       regType = (shift == 0) ? TYPE_PIX : TYPE_S32;
   }

   shAssert(in->type == TYPE_PIX);
   shAssert(filtsize_r == 1 || (filtsize_r >= 2 && filtsize_r%2 == 0));
   shAssert(filtsize_c == 1 || (filtsize_c >= 2 && filtsize_c%2 == 0));
/*
 * The shift-to-centre code was written assuming that shift != 0 (and thus
 * that the output region is S32); this is not a fundamental restriction,
 * but I don't feel like lifting it until there is a demonstrated need
 */
   shAssert(shift == 0 || shift_to_centers);
/*
 * If we are really using medians (as controlled by use_mean), we must
 * first find out the range of pixel values that we must keep to calculate
 * the three quartiles of the image. We'll do this by finding the quartiles
 * of the image coarsely sampled, and then multiplying by suitable fudge
 * factors
 *
 * phRegStatsFromQuartiles can only fail of region isn't of type U16, and
 * we know that it is before we get here.
 */
   if(use_mean) {
      nval = 1;
      median = sigma = 0;		/* not really used in this case */
   } else {
     int use_all_pixels = (minval <= 0 || maxval <= 0) ? 1 : 0;
     
     if(phRegStatsFromQuartiles(in,coarse,0,NULL,&median,NULL,&iqr,NULL,NULL,
				NULL,NULL,NULL,NULL,NULL) != SH_SUCCESS) {
	 shFatal("phMedianSmooth: impossible error at line %d",__LINE__);
      }
      sigma = IQR_TO_SIGMA*iqr;
      if(use_all_pixels) {
	 PEAKS *peaks = phPeaksNew(1);	/* peak of object */
	 PEAK *peak;			/* == peaks->peaks[0] */

	 phObjectPeakFind(in, NULL, peaks);
	 shAssert(peaks->npeak == 1);
	 peak = peaks->peaks[0];
	 nval = in->ROWS[peak->rpeak][peak->cpeak] + 2*NSIGMA_HIST*sigma + 2;
	 phPeaksDel(peaks);
      } else {
	 nval = (int)(5*NSIGMA_HIST*sigma + 2); /* cast is required by KAP */
      }
/*
 * obey {min,max}val, if positive
 */
      if(minval <= 0) {
	 minval = median - NSIGMA_HIST*sigma;
	 if(minval < 0) { minval = 0; }
      }
      if(maxval > 0) {
	 if(nval < maxval - minval + 1 && maxval > minval) {
	    nval = maxval - minval + 1;
	 }
      }
   }
/*
 * Time to create the output REGION
 */
   dc = (filtsize_c == 1) ? 1 : filtsize_c/2;
   dr = (filtsize_r == 1) ? 1 : filtsize_r/2;
   nc = in->ncol/dc;
   if(nc*dc < in->ncol) nc++;
   nr = in->nrow/dr;
   if(nr*dr < in->nrow) nr++;

   shAssert(nc > 1 && nr > 1);

   out = shRegNew("median values",nr + 1,nc + 1, regType);
   skysig = shRegNew("errors in median values",nr + 1,nc + 1, regType);
/*
 * Now create the histograms that we need.	    ....................
 * In order to avoid binning each part of	    ....................
 * the image 4 times we'll keep the histograms	    XXXXXXXXX...........
 * for areas that are filtsize_c/2*filtsize_r/2,    xxxxxxxXXXXXXXXXXXXX
 * and add these together in fours. This means	    xxxxxxxxxxxxxxxxxxxx
 * that when we've processed all the areas	    xxxxxxxxxxxxxxxxxxxx
 * marked X or x, we need to store the histograms   xxxxxxxxxxxxxxxxxxxx
 * for all the Xs; a total of nc + 2 histograms
 *
 * For convenience, we actually allocate pointers to two whole rows of
 * histograms, but only populate at most nc + 2 of them. In fact, we
 * allocate an extra 2 pointers at the start of each row to simplify
 * the bookkeeping. That way we can always get an unused hist from the
 * other histogram, two to the left of the current area.
 */
   histograms[0] = shMalloc(2*(nc + 2)*sizeof(PHHIST *));
   histograms[0] += 2;
   histograms[1] = histograms[0] + nc;
   histograms[1] += 2;
   histograms_s = shMalloc((nc + 2)*sizeof(PHHIST));
   histograms_ss = shMalloc((nc + 2)*nval*sizeof(int));

   for(i = 0;i < nc + 2;i++) {
      histograms[1][i - 2] = &histograms_s[i];
      histograms[1][i - 2]->min_bin = minval;
      histograms[1][i - 2]->num_bin = nval;
      histograms[1][i - 2]->total_number = 0;
      histograms[1][i - 2]->value = &histograms_ss[i*nval];
   }
/*
 * We also need a cumulative histogram; this has to be int not
 * unsigned short so we can't fake it in place with a regular hist structure
 */
   chist = shMalloc(sizeof(PHHIST_CUMUL));
   chist->cvalue = shMalloc((nval + 1)*sizeof(int));
   chist->cvalue++;			/* create chist[-1] */
   chist->num_bin = nval;
   chist->min_bin = histograms[1][0]->min_bin;
/*
 * Finally we can almost start processing the region. Fill the first row
 * of the histogram specially, and then go through the rest of the region
 * calculating the sub-histograms. As each group of four in a square is
 * completed we can find a point in the median-smoothed image
 */
   histograms[0][nc - 1] = histograms[1][-1];
   histograms[0][nc - 2] = histograms[1][-2];
   histograms[1][-1] = histograms[1][-2] = NULL;

   for(r = 0;r < nr;r++) {
      histograms[(r + 1)%2][-2] = histograms[r%2][nc - 2];
      histograms[(r + 1)%2][-1] = histograms[r%2][nc - 1];
      histograms[r%2][nc - 2] = histograms[r%2][nc - 1] = NULL;

      drp = (r < nr - 1) ? dr : in->nrow - r*dr;
      for(c = 0;c < nc;c++) {
	 histograms[r%2][c] = histograms[(r + 1)%2][c - 2];
	 histograms[(r + 1)%2][c - 2] = NULL;
	 dcp = (c < nc - 1) ? dc : in->ncol - c*dc;
	 fill_histogram(histograms[r%2][c], in->ROWS,
					       c*dc, r*dr, dcp, drp, use_mean);
	 if(c == 0) {
	     h0 = (r == 0) ? NULL : histograms[(r + 1)%2][c];
	     h1 = NULL;
	     h2 = histograms[(r    )%2][c];
	     h3 = NULL;
	 } else {
	     h0 = (r == 0) ? NULL : histograms[(r + 1)%2][c];
	     h1 = (r == 0) ? NULL : histograms[(r + 1)%2][c - 1];
	     h2 = histograms[(r    )%2][c];
	     h3 = histograms[(r    )%2][c - 1];
	 }

	 npix = get_center_width(use_mean, h0, h1, h2, h3, chist,
				 filtsize_r, filtsize_c, &center, &width);

	 if(out->ROWS != NULL) {
	    if(npix <= 0) {
	       out->ROWS[r][c] = median;
	       skysig->ROWS[r][c] = MAX_U16;
	    } else {
	       out->ROWS[r][c] = FLT2PIX(center);
	       skysig->ROWS[r][c] = FLT2PIX(width);
	    }
	 } else {
	    if(npix <= 0) {
	       out->rows_s32[r][c] = FLT2PIX(scale*median);
	       skysig->rows_s32[r][c] = FLT2PIX(scale*MAX_U16);
	    } else {
	       out->rows_s32[r][c] = FLT2PIX(scale*center);
	       skysig->rows_s32[r][c] = FLT2PIX(scale*width);
	    }
	 }
      }
   }
/*
 * We allocated an extra row at the top and an extra column at the right;
 * these are used for interpolating and are initialised to 0
 */
   for(c = 0;c <= nc;c++) {
      if(out->ROWS != NULL) {
	 out->ROWS[nr][c] = 0;
	 skysig->ROWS[nr][c] = 0;
      } else {
	 out->rows_s32[nr][c] = 0;
	 skysig->rows_s32[nr][c] = 0;
      }
   }
   for(r = 0;r <= nr;r++) {
      if(out->ROWS != NULL) {
	 out->ROWS[r][nc] = 0;
	 skysig->ROWS[r][nc] = 0;
      } else {
	 out->rows_s32[r][nc] = 0;
	 skysig->rows_s32[r][nc] = 0;
      }
   }
/*
 * out's values are on pixel corners, because the filter was taken to
 * have even size. We have to resample onto pixel centres to make
 * interpolation efficient, and we can take the opportunity to extrapolate
 * to the edges of the frame at the same time
 *
 * We allocated us an extra row and column at the top and right, to make
 * interpolation easy. Note that these will, in general, correspond to
 * pixels beyond the boundaries of the original image
 */
   if(shift_to_centers) {
      shift_to_pixel_centres(out, in->ncol, in->nrow, dc, dr, 0);
      shAssert(out->rows_s32[1] ==
	       out->rows_s32[0] + out->ncol); /* data array is contiguous */
      shift_to_pixel_centres(skysig, in->ncol, in->nrow, dc, dr,
			     phQuartilesGetFromArray(skysig->rows_s32[0],
						     TYPE_S32,
						     skysig->nrow*skysig->ncol,
						     0, NULL, NULL, NULL));
   }

   shFree(&chist->cvalue[-1]);
   shFree(chist);
   shFree(histograms_ss);
   histograms[0] -= 2;			/* we +2'd after allocation */
   shFree(histograms[0]);
   shFree(histograms_s);
/*
 * pack up the answers
 */
   {
      BINREGION *bout = phBinregionNew();
      bout->reg = out;
      bout->bin_row = dr;
      bout->bin_col = dc;
      bout->shift = shift;

      if(bskysig == NULL) {
	 shRegDel(skysig);
      } else {
	 bskysig->reg = skysig;
	 bskysig->bin_row = dr;
	 bskysig->bin_col = dc;
	 bskysig->shift = shift;
      }
      
      return(bout);
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return Statistics of a region, specifically the mean, median, sigma, SIQR,
 * [clipped] {q1,median,q3,sigma}, and {minimum,maximum} pixel values.
 * If you've asked for one of the [clipped] {q1,median,q3}, the histogram
 * phHistogram is filled out as a side effect.
 *
 * The parameter clip is ignored, and the clipped sigma is estimated from the
 * (clipped) IQR
 *
 * Only every <coarse> pixel is examined.
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if REGION is not U16
 *
 */
static int phHistogramN = 0;		/* no. of pixels in phHistogram */
static int phHistogram[MAX_U16 + 1];	/* histogram found by
					   phRegStatsFromQuartiles */

int
phRegStatsFromQuartiles(const REGION *reg, /* the region in question */
			int coarse,      /* how coarse is the sampling? */
			float clip,      /* how many sigma to clip at */
			float *pmean,    /* O: calculated mean */
			float *pmedian,  /* O: calculated median */
			float *psigma,   /* O: calculated standard deviation */
			float *piqr,     /* O: calculated IQR */
			float *pq1,	 /* O: lower quartile */
			float *pq3,	 /* O: upper quartile */
			float *pcmedian, /* O: clipped median */
			float *pcsigma,	 /* O: clipped sigma */
			PIX *pminpix,	 /* O: minimum and ... */
			PIX *pmaxpix,    /* O:  maximum pixel values */
			float *pcounts)	 /* O: total counts in reg */
{
   int c, r;
   int nrow,ncol;
   PIX *rows;
   float quartiles_u[NQUANTILES];	/* unclipped quartiles of histogram */
   float quartiles[NQUANTILES];		/* clipped quartiles of histogram */
   float val;

   shAssert(reg != NULL);
   shAssert(coarse > 0);
     
   ncol = reg->ncol; nrow = reg->nrow;
   
   if(reg->type != TYPE_PIX) {
      if((reg->type == TYPE_FL32) &&
	 (pmean == NULL && pmedian == NULL && psigma == NULL && piqr == NULL &&
	  pq1 == NULL && pq3 == NULL && pcmedian == NULL && pcsigma == NULL &&
	  pminpix == NULL && pmaxpix == NULL)) {
	 
	 if(pcounts != NULL) {
	    double sum = 0;
	    float *rows_fl32;
	    for(r = 0; r < nrow; r++) {
	       rows_fl32 = reg->rows_fl32[r];
	       for(c = 0; c < ncol; c++) {
		  sum += rows_fl32[c];
	       }
	    }
	    *pcounts = sum;
	 }

	 return(SH_SUCCESS);	 
      }

      shError("phRegStatsFromQuartiles: Region %s has type %d, not %d",
					       reg->name, reg->type, TYPE_PIX);
      return(SH_GENERIC_ERROR);
   }

   if(pmean != NULL || psigma != NULL || pminpix != NULL || pmaxpix != NULL ||
      (pcounts != NULL && coarse == 1)) {
      double sum;
      PIX min = MAX_U16, max = 0;
      int npix = ((nrow - 1)/coarse + 1)*((ncol - 1)/coarse + 1);

      sum = 0;
      for(r = 0;r < nrow;r += coarse) {
	 rows = reg->ROWS[r];
	 if(pmean != NULL || psigma != NULL ||
					    (coarse == 1 && pcounts != NULL)) {
	    for(c = 0;c < ncol;c += coarse) {
	       sum += rows[c];
	    }
	 }
	 if(pminpix != NULL || pmaxpix != NULL) {
	    for(c = 0;c < ncol;c += coarse) {
	       val = rows[c];
	       if(val < min) {
		  min = val;
	       }
	       if(val > max) {
		  max = val;
	       }
	    }
	 }
      }
      if(pmean != NULL) *pmean = sum/npix;
      if(pminpix != NULL) *pminpix = min;
      if(pmaxpix != NULL) *pmaxpix = max;
      if(pcounts != NULL && coarse == 1) *pcounts = sum;
      
      if(psigma != NULL) {
	 double mean = sum/npix;
	 sum = 0.0;
	 for(r = 0;r < nrow;r += coarse) {
	    rows = reg->ROWS[r];
	    for(c = 0;c < ncol;c += coarse) {
	       val = rows[c] - mean;
	       sum += val*val;
	    }
	 }
	 *psigma = sqrt(sum/npix);
      }
   }

   if(pmedian != NULL || piqr != NULL || pq1 != NULL || pq3 != NULL ||
					 pcmedian != NULL || pcsigma != NULL) {
/*
 * Find the quartiles of the image, suitably coarsely sampled.
 */
      int nr,nc;			/* no. of steps along rows and cols*/
      int *scr = phHistogram;		/* a static extern */

      if(pcmedian != NULL || pcsigma != NULL) {
	 shAssert(clip < 0.0);		/* as its value's ignored */
      }

      nr = reg->nrow/coarse + (reg->nrow%coarse == 0 ? 0 : 1);
      nc = reg->ncol/coarse + (reg->ncol%coarse == 0 ? 0 : 1);
      
      memset((void *)scr,'\0',(MAX_U16 + 1)*sizeof(int));
      shAssert(scr[0] == 0);		/* i.e. 0 is \0\0\0\0 */
      
      {
	 PIX *ptr, *pend;
	 int ival;
	 
	 for(r = 0;r < reg->nrow;r += coarse) {
	    ptr = reg->ROWS[r];
	    pend = ptr + nc*coarse;
	    for(;ptr != pend;ptr += coarse) {
	       ival = PIX2INT(*ptr);
	       if(ival > MAX_U16) {
		  ival = MAX_U16;
	       }
	       scr[ival]++;
	    }
	 }
      }
      
      {
	 PHHIST h;
	 
	 h.value = scr; h.min_bin = 0; h.num_bin = MAX_U16;
	 phHistogramN = h.total_number = nr*nc;
	 (void)get_quartiles(&h,NULL,NULL,NULL, quartiles_u, quartiles);
      }

      if(pq1 != NULL) {
	 *pq1 = quartiles_u[0];
      }
      if(pmedian != NULL) {
	 *pmedian = quartiles_u[1];
      }
      if(pq3 != NULL) {
	 *pq3 = quartiles_u[2];
      }
      if(piqr != NULL) {
	 *piqr = (quartiles_u[2] - quartiles_u[0]);
      }
      if(pcmedian != NULL) {
	 *pcmedian = quartiles[1];
      }
      if(pcsigma != NULL) {
	 *pcsigma = IQR_TO_SIGMA*(quartiles[2] - quartiles[0]);
      }
   }

   if(pcounts != NULL && coarse != 0) { /* we need counts, and not just
					      at some coarsness */
      double sum = 0;

      for(r = 0;r < nrow;r++) {
	 rows = reg->ROWS[r];
	 for(c = 0;c < ncol;c++) {
	    sum += rows[c];
	 }
      }
      *pcounts = sum;
   }

   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * Fill out a PHHIST from a portion of an image.
 */
static void
fill_histogram(PHHIST *h,		/* output histogram */
	       PIX **iptr,		/* image pointer */
	       int col0, int row0,	/* starting value of col and row */
	       int dc, int dr,		/* size in column and row directions */
	       int use_mean)		/* prepare for mean NOT median */
{
   int r;
   register int *hist;
   PIX *ptr, *pend;
   register unsigned int first, last;
   
   h->total_number = dc*dr;
   if(dc == 0) {			/* unlikely, but we can shorten the
					   inner loop if we know dc >= 1*/
      h->sum = 0;
      return;
   }

   if(use_mean) {
      float sum = 0;
/*
 * we don't really need a histogram, just the sum of all the pixels.
 */
      for(r = row0;r != row0 + dr;r++) {
	 ptr = &iptr[r][col0];
	 pend = ptr + dc;
	 do {
	    sum += *ptr++;
	 } while(ptr != pend);
      }
      h->sum = sum;

      return;
   }
/*
 * OK, we really want to prepare for a median. To work.
 */
   memset((void *)h->value,'\0',h->num_bin*sizeof(int));
   shAssert(h->value[0] == 0);		/* i.e. 0 is \0\0 */

   first = h->min_bin;
   last = h->min_bin + h->num_bin - 1;
   hist = h->value - first;		/* we don't want to subtract first
					   in the inner loop */
   
   for(r = row0;r != row0 + dr;r++) {
      ptr = &iptr[r][col0];
      pend = ptr + dc;
      do {
	 if(*ptr >= last || *ptr <= first) {
	    if(*ptr >= last) {
	       hist[last]++;
	    } else {
	       hist[first]++;
	    }
	 } else {
	    hist[PIX2INT(*ptr)]++;
	 }
	 ptr++;
      } while(ptr != pend);
   }
}


#if 0
/*
 * This version uses the lut to fill the histogram array. It uses
 * fewer cycles than fill_histogram() (about 12.7 per pixel v. 14.5
 * on an R4400), but it's less cache efficient
 *
 * N.b. use_mean is not supported
 */
static void
fill_histogram(
	       PHHIST *h,		/* output histogram */
	       unsigned short *lut,	/* lut to use */
	       unsigned short **iptr,	/* image pointer */
	       int col0, int row0,	/* starting value of col and row */
	       int dc, int dr		/* size in column and row directions */
	       )
{
   int c,r;
   int *hist = h->value;
   unsigned short *ptr, *pend;
   
   memset((void *)h->value,'\0',h->num_bin*sizeof(int));
   shAssert(h->value[0] == 0);		/* i.e. 0 is \0\0 */

   h->total_number = dc*dr;
   for(r = row0;r != row0 + dr;r++) {
      ptr = &iptr[r][col0];
      pend = ptr + dc;
      for(;ptr != pend;ptr++) {
	 hist[lut[*ptr]]++;
      }
   }
}
#endif

#if 0
/*****************************************************************************/
/*
 * print a PHHIST
 */
static void
print_histogram(PHHIST *h)
{
   int i;
   int sum = 0;

   shErrStackPush("size = %d, range = %d:%d\n",
	   h->num_bin,h->min_bin,h->min_bin + h->num_bin - 1);
   for(i = 0;i < h->num_bin;i++) {
      shErrStackPush("%d %d\n",h->min_bin + i,h->value[i]);
      sum += h->value[i];
   }
   shErrStackPush("sum = %d\n\n",sum);
}
#endif

/*****************************************************************************/
/*
 * Find the quartiles of 1 or 4 hists; the latter case is equivalent to
 * adding them all together and using the former code, but it's faster as
 * we can stop adding when all the desired quartiles are found
 *
 * The value 2.3263 for NSIGMA_CLIP corresponds to the first and last
 * percentiles for a Gaussian; 1.28155 would be the tenth and ninetieth
 *
 * If we clip the histogram at NSIGMA_CLIP we have to correct the resulting IQR
 * (or sigma) by multiplying by FIX_IQR. For an NSIGMA_CLIP of 2.32634, the
 * correction is a factor of 1.02376 == 0.758915/IQR_TO_SIGMA, where
 *    erfc(1/(2*sqrt(2)*0.758915)) = 0.5*(1 + erfc(2.32634/sqrt(2)))
 * For NSIGMA_CLIP of 1.28155, FIX_IQR is 0.95347/IQR_TO_SIGMA == 1.28621 
 */
#if 1
#define NSIGMA_CLIP 2.32634		/* how many sigma to clip at */
#define FIX_IQR 1.02376			/* correction to sigma due to trimming*/
#define MEDIAN_CLIP_BIAS 0.060		/* For a Poisson process, median =
					   clipped_median + MEDIAN_CLIP_BIAS */
#else
#define NSIGMA_CLIP 1.28155		/* how many sigma to clip at */
#define FIX_IQR 1.28621			/* correction to sigma due to trimming*/
#define MEDIAN_CLIP_BIAS 0.120		/* For a Poisson process, median =
					   clipped_median + MEDIAN_CLIP_BIAS */
#endif

#define NLEV 3				/* number of values to return */

static int
get_quartiles(PHHIST *h0,		/* histograms of pixel values */
	      PHHIST *h1,		/* may */
	      PHHIST *h2,		/*    be */
	      PHHIST *h3,		/*      NULL */
	      float *perc_u,		/* the derived quartiles; output */
	      float *perc)		/* clipped quartiles; output */
{
   int clip;				/* (int)fclip */
   float fclip;				/* where to clip the histogram */
   int i;
   int n;				/* number of points histogrammed */
   float sigma;				/* estimated sigma of histogram */
   int size;				/* == h0->num_bin */
   int sum;				/* sum of pixels processed */
   int sumi;				/* the next term in the sum */
/*
 * find a non-NULL PHHIST
 */
   if(h0 == NULL) {
      if(h1 == NULL) {
	 if(h2 == NULL) {
	    if(h3 == NULL) {
	       shFatal("get_quartiles: at least one PHHIST must be non-NULL");
	    }
	    h0 = h3; h3 = NULL;
	 } else {
	    h0 = h2; h2 = NULL;
	 }
      } else {
	 h0 = h1; h1 = NULL;
      }
   }

   size = h0->num_bin;

   shAssert(h1 == NULL || (h1->num_bin == size && h0->min_bin == h1->min_bin));
   shAssert(h2 == NULL || (h2->num_bin == size && h0->min_bin == h2->min_bin));
   shAssert(h3 == NULL || (h3->num_bin == size && h0->min_bin == h3->min_bin));
/*
 * Add all the histograms into h0
 */
   n = h0->total_number;
   if(h1 != NULL) {
      n += h1->total_number;
      for(i = 0;i < size;i++) {
	 h0->value[i] += h1->value[i];
      }
   }
   if(h2 != NULL) {
      n += h2->total_number;
      for(i = 0;i < size;i++) {
	 h0->value[i] += h2->value[i];
      }
   }
   if(h3 != NULL) {
      n += h3->total_number;
      for(i = 0;i < size;i++) {
	 h0->value[i] += h3->value[i];
      }
   }
/*
 * find the quartiles of the untrimmed histogram
 */
   if(get_quartiles_work(h0,n,perc_u,0,0) < 0) {
      return(-1);
   }

   if(perc == NULL) {
      return(n);
   }
/*
 * Now trim the histogram. Throw away everything outside NSIGMA_CLIP sigma
 * from the median
 */
   sigma = IQR_TO_SIGMA*(perc_u[2] - perc_u[0]);
/*
 * We'll need to deal specially with the bin which is split when we trim
 * the bottom of the histogram (some of its pixels still ought to be included).
 * It's easiest to do this if we trim the upper wing first, where all that
 * we need to know is how many pixels are being rejected.
 *
 * Remember that the 100th bin is taken to run from 99.5 to 100.5, so add
 * 0.5 to the clip values.
 *
 * Now that all that's said, do it: clip upper wing
 */
   fclip = (perc_u[1] + NSIGMA_CLIP*sigma) + 0.5 - h0->min_bin;
   clip = (int)fclip;
   sum = sumi = 0;
   if(clip == size - 1) {
      sum = h0->value[size - 1];
   } else if(clip < size - 1) {
      for(i = size - 1;i >= clip;i--) {
	 sumi = h0->value[i];
	 sum += sumi;
      }
      sumi = (fclip - clip)*sumi;
      sum -= sumi;
   }

   n -= sum;
/*
 * and clip the lower wing
 */
   fclip = (perc_u[1] - NSIGMA_CLIP*sigma) + 0.5 - h0->min_bin;
   clip = (int)fclip;
   sum = sumi = 0;
   if(clip <= 0) {
      i = 0;
      sum = h0->value[0];
   } else {
      for(i = 0;i <= clip;i++) {
	 sumi = h0->value[i];
	 sum += sumi;
      }
      sumi = (1 - (fclip - clip))*sumi;
      sum -= sumi;
   }

   n -= sum;
/*
 * Did we clip everything?
 */
   if(n == 0) {
      for(i = 0; i < 3; i++) {
	 perc[i] = perc_u[i];
      }

      return(n);
   }
/*
 * Now we're ready to find the trimmed quartiles. Deal properly with the
 * bin that was split when we trimmed the histogram
 */
   if(get_quartiles_work(h0,n,perc,sumi,i) < 0) {
      return(-1);
   }
/*
 * Correct the interquartile range because we've trimmed at NSIGMA_CLIP sigma
 */
   perc[0] = perc[1] - FIX_IQR*(perc[1] - perc[0]);
   perc[2] = perc[1] + FIX_IQR*(perc[2] - perc[1]);

   return(n);
}

static int
get_quartiles_c(
	      PHHIST *h0,		/* histograms of pixel values */
	      PHHIST *h1,		/* may */
	      PHHIST *h2,		/*    be */
	      PHHIST *h3,		/*      NULL */
	      PHHIST_CUMUL *ch,		/* cumulative histogram */
	      float *perc_u,		/* the unclipped derived quartiles */
	      float *perc)		/* the clipped derived quartiles */
{
   unsigned int *chist = ch->cvalue;	/* unpack to ensure no aliasing */
   int clip;				/* (int)fclip */
   float fclip;				/* where to clip the histogram */
   int i;
   int n;				/* number of points histogrammed */
   float sigma;				/* estimated sigma of histogram */
   int num_bin, min_bin;			/* == h?->num_bin, h?->min_bin */
   int sum;				/* sum of pixels processed */

/*
 * find a non-NULL PHHIST
 */
   if(h0 == NULL) {
      if(h1 == NULL) {
	 if(h2 == NULL) {
	    if(h3 == NULL) {
	       shFatal("get_quartiles: at least one PHHIST must be non-NULL");
	    }
	    h0 = h3; h3 = NULL;
	 } else {
	    h0 = h2; h2 = NULL;
	 }
      } else {
	 h0 = h1; h1 = NULL;
      }
   }

   num_bin =
       h0 ? h0->num_bin :
       h1 ? h1->num_bin :
       h2 ? h2->num_bin :
       h3 ? h3->num_bin : 0;
   min_bin =
       h0 ? h0->min_bin :
       h1 ? h1->min_bin :
       h2 ? h2->min_bin :
       h3 ? h3->min_bin : 0;

   shAssert(h0 == NULL || (h0->num_bin == num_bin && min_bin == h0->min_bin));
   shAssert(h1 == NULL || (h1->num_bin == num_bin && min_bin == h1->min_bin));
   shAssert(h2 == NULL || (h2->num_bin == num_bin && min_bin == h2->min_bin));
   shAssert(h3 == NULL || (h3->num_bin == num_bin && min_bin == h3->min_bin));
/*
 * Add all the histograms into chist; we need to use the cumulative
 * histogram at least 3 times so this is an efficient thing to do.
 */
   chist[-1] = 0;

   n = h0->total_number;
   for(i = 0;i < num_bin;i++) {
      chist[i] = chist[i - 1] + h0->value[i];
   }
   if(h1 != NULL) {
      int sum = 0;
      n += h1->total_number;
      for(i = 0;i < num_bin;i++) {
	 sum += h1->value[i];
	 chist[i] += sum;
      }
   }
   if(h2 != NULL) {
      int sum = 0;
      n += h2->total_number;
      for(i = 0;i < num_bin;i++) {
	 sum += h2->value[i];
	 chist[i] += sum;
      }
   }
   if(h3 != NULL) {
      int sum = 0;
      n += h3->total_number;
      for(i = 0;i < num_bin;i++) {
	 sum += h3->value[i];
	 chist[i] += sum;
      }
   }
   shAssert(chist[num_bin - 1] == n);
/*
 * find the quartiles of the untrimmed histogram
 */
   if(get_quartiles_work_c(ch,n,perc_u,0) < 0) {
      return(-1);
   }

   if(perc == NULL) {
      return(n);
   }
/*
 * Now trim the histogram. Throw away everything outside NSIGMA_CLIP sigma
 * from the median
 */
   sigma = IQR_TO_SIGMA*(perc_u[2] - perc_u[0]);
/*
 * We'll need to deal specially with the bin which is split when we trim
 * the bottom of the histogram (some of its pixels still ought to be included).
 * It's easiest to do this if we trim the upper wing first, where all that
 * we need to know is how many pixels are being rejected. We also need to
 * know the total number of pixels, and this is modified after trimming at the
 * top
 *
 * Remember that the 100th bin is taken to run from 99.5 to 100.5, so add
 * 0.5 to the clip values.
 *
 * Note that the first and last bins are special; they contain all the pixels
 * that didn't fit in the histogram. This means that we cannot interpolate
 * into them, so if clip lies in one of them, clip it all.
 *
 * Now that all that's said, do it: clip upper wing
 */
   fclip = (perc_u[1] + NSIGMA_CLIP*sigma) + 0.5 - ch->min_bin;
   clip = (int)fclip;
   if(clip >= num_bin) {
      sum = 0;
   } else if(clip == num_bin - 1) {	/* last bin is special */
      sum = n - chist[num_bin - 2];
   } else {
      sum = n - chist[clip];
      sum += (1 - (fclip - clip))*(chist[clip] - chist[clip - 1]);
   }

   n -= sum;
/*
 * and clip the lower wing.
 */
   fclip = (perc_u[1] - NSIGMA_CLIP*sigma) + 0.5 - ch->min_bin;
   clip = (int)fclip;
   if(clip < 0) {
      clip = 0;
   } else if(clip <= 1) {		/* first bin is special */
      sum = chist[0];
   } else {
      sum = chist[clip - 1];
      sum += (fclip - clip)*(chist[clip] - chist[clip - 1]);
   }
   chist[-1] = sum;

   n -= sum;
/*
 * Now we're ready to find the trimmed quartiles. Deal properly with the
 * bin that was split when we trimmed the histogram
 */
   if(get_quartiles_work_c(ch,n,perc,clip) < 0) {
      return(-1);
   }
/*
 * Correct the interquartile range because we've trimmed at NSIGMA_CLIP sigma
 */
   perc[0] = perc[1] - FIX_IQR*(perc[1] - perc[0]);
   perc[2] = perc[1] + FIX_IQR*(perc[2] - perc[1]);

   return(n);
}

/*****************************************************************************/
/*
 * Now the two routines that find the quartiles given the histograms
 *
 * There are two versions, depending on whether we use the cumulative
 * histogram or the differential ones. Using the cumulative one is slightly
 * faster; if we restricted ourselves to filters with fewer than 2^16 pixels
 * it would be slightly faster again, and not need any extra storage (we
 * could cumulate into h0 rather than needing chist).
 */

static int
get_quartiles_work_c(
		     PHHIST_CUMUL *ch,	/* cumulative histogram of pixels */
		     int n,		/* number of points histogrammed */
		     float *quartiles,	/* the derived quartiles; output */
		     int i0		/* starting value of i */
		     )
{
   unsigned int *chist = ch->cvalue;	/* unpack to ensure no aliasing */
   static float levs[NLEV] = {		/* desired quartiles */
      0.25, 0.50, 0.75
   };
   int ilevs[NLEV];			/* number of points below quartiles */
   int ilevel;				/* (int)level */
   int i,j,l;
   int size;				/* unpacked to tell the compiler that
					   there are no aliasing problems */
   int sum;				/* sum of pixels processed */
   int sumi;				/* the next term in the sum */

   ilevs[0] = (int)(levs[0]*n) + chist[-1];
   ilevs[1] = (int)(levs[1]*n) + chist[-1];
   ilevs[2] = (int)(levs[2]*n) + chist[-1];
   
   size = ch->num_bin;
   i = i0;
   for(l = 0;l < NLEV;l++) {
      ilevel = ilevs[l];
      if(i == 0) {
	 if(chist[0] >= ilevel) {
	    if(l == 0 && ch->min_bin != 0) {
	       shErrStackPush("Lower intensity limit of histogram "
			      "is too high, (%d > %d)", chist[0], ilevel);
	       shError("Lower intensity limit of histogram is too high "
		       "(%d > %d)", chist[0], ilevel);
	    }
	    quartiles[l] = ch->min_bin;
	    continue;
	 }
      }

      for(;i < size;i++) {
	 if(chist[i] > ilevel) {
/*
 * assume that all pixel values are equally likely; this isn't true, and if
 * we worked a little harder we could correct using the shape of the histogram.
 *
 * Each bin is assumed to cover (i - 0.5, i + 0.5) which is I think correct;
 * consider a symmetrical histogram so the median lies in the middle of the
 * central bin. The median should equal the mean, and this convention achieves
 * this.
 *
 * If there are missing values in the data, more than one of the values
 * of chist may be the same, so we have to look backwards to find the
 * to find the value i0 corresponding to the pixel intensities which last
 * contributed to the histogram. i1 is the first pixel intensity above
 * the desired quantile
 */
	    {
	       float i0, i1;

	       if(chist[i-1] > n*levs[l]) {
		  for(j = i - 1;j >= 0 && chist[j] == chist[i-1];j--) continue;
		  j++;
	       } else {
		  j = i - 1;
	       }
	       i0 = ch->min_bin + j + 0.5;
	    
	       i1 = ch->min_bin + i + 0.5;
	       
	       sum = chist[i - 1] - chist[-1];
	       sumi = chist[i] - chist[-1];

	       quartiles[l] = i0 + (i1 - i0)*(n*levs[l] - sum)/(sumi - sum);
	    }
	    break;
	 }
      }
   }

   return(0);
}

static int
get_quartiles_work(
		   PHHIST *h0,	/* histogram of pixel values */
		   int n,		/* number of points histogrammed */
		   float *quartiles,	/* the derived quartiles; output */
		   int sum0,		/* number of pixels already accounted
					   for */
		   int i0		/* starting value of i */
		   )
{
   int bad_value = 0;			/* number of values we failed to get */
   static float levs[NLEV] = {		/* desired quartiles */
      0.25, 0.50, 0.75
   };
   int ilevs[NLEV];			/* number of points below quartiles */
   int ilevel;				/* (int)level */
   int i,l;
   int size;				/* unpacked to tell the compiler that
					   there are no aliasing problems */
   int sum;				/* sum of pixels processed */
   int sumi;				/* the next term in the sum */

   ilevs[0] = levs[0]*n;
   ilevs[1] = levs[1]*n;
   ilevs[2] = levs[2]*n;
   
   size = h0->num_bin;
   sum = sum0;
   i = i0;
   for(l = 0;l < NLEV;l++) {
      ilevel = ilevs[l];
      if(i == 0) {
	 sumi = h0->value[0];
	 if(sumi >= ilevel) {
	    if(h0->min_bin == 0) {
	       shAssert(i == 0);
	       quartiles[l] = ilevel/(float)sumi - 0.5;
	    } else {
	       bad_value++;
	       if(l == 0) {
		  shErrStackPush("Lower intensity limit "
				 "of histogram is too high");
		  shError("Lower intensity limit of histogram is too high");
	       }
	       
	       quartiles[l] = h0->min_bin + i + 0.5;
	    }

	    continue;
	 }
      }

      for(;i < size;i++) {
	 sumi = h0->value[i];
	 sum += sumi;
	 if(sum > ilevel) {
	    sum -= sumi;
/*
 * assume that all pixel values are equally likely; this isn't true, and if
 * we worked a little harder we could correct using the shape of the histogram.
 *
 * Each bin is assumed to cover (i - 0.5, i + 0.5) which is I think correct;
 * consider a symmetrical histogram so the median lies in the middle of the
 * central bin. The median should equal the mean, and this convention achieves
 * this.
 */
	    quartiles[l] = h0->min_bin + (i - 1) + 0.5 + (n*levs[l] -sum)/sumi;
	    break;
	 }
      }
   }
#if 0 && !defined(NDEBUG)
/*
 * check that all is well. This shouldn't be in production code!
 */
   for(;i < size;i++) {
      sumi = h0->value[i];
      sum += sumi;
   }
   shAssert(sum == n);
#endif

#if 0
   return(bad_value > 0 ? -1 : 0);
#else
   return(0);
#endif
}

/*****************************************************************************/
/*
 * Resample a binned image, with values given at the corners of pixels, to
 * one with values at pixel centres. At the same time, extrapolate as needed
 * to fill the dummy rows at the right and top of the image
 */
static void
shift_to_pixel_centres(REGION *out,	/* region to shift */
		       int ncol, int nrow, /* size of input region */
		       int dc, int dr,	/* size of bins */
		       int min)		/* minimum allowable value for
					   extrapolated pixels */
{
   float cellsize;			/* size of last cell */
   float f;				/* correction factor for last pixel */
   float facc, facr;			/* interpolating factors */
   int sr,sc;				/* row and col counters in smooth */
   int nc, nr;				/* unliased from out->n{col,row} */
   int val;				/* an interpolated value */

   shAssert(out != NULL && out->type == TYPE_S32);
   shAssert(out->nrow >= 3 && out->ncol >= 3);

   nc = out->ncol;
   nr = out->nrow;
/*
 * if the bins don't fit evenly into the input region, the last bins
 * will be smaller than dc*dr, and their effective centres will be displaced
 * correspondingly. This is a problem if there are gradients in the data,
 * so fix them now.
 *
 * The last cell only has dimension cellsize, so its effective row centre
 * is displaced downwards by (dr + cellsize)/2, so the gradient at the edge
 * is (val[nr - 2] - val[nr - 3])/((dr + cellsize)/2) and the correction is
 * this times (dr - cellsize)/2
 */
   if(nc >= 4 && ncol%dc != 0) {
      cellsize = (nc - 1)*dc - ncol;	/* nc - 1 as there's a fake left cell*/
      f = (dc - cellsize)/(dc + cellsize);
      shAssert(f > 0);
      for(sr = 0;sr < nr;sr++) {
	 val = out->rows_s32[sr][nc - 2] +
	   f*(out->rows_s32[sr][nc - 2] - out->rows_s32[sr][nr - 3]);
	 out->rows_s32[sr][nc - 2] = (val > min) ? val : min;
      }
   }
   if(nr >= 4 && nrow%dr != 0) {
      cellsize = (nr - 1)*dr - nrow;	/* nr - 1 as there's a fake top cell */
      f = (dr - cellsize)/(dr + cellsize);
      shAssert(f > 0);
      for(sc = 0;sc < nc;sc++) {
	 val = out->rows_s32[nr - 2][sc] +
	   f*(out->rows_s32[nr - 2][sc] - out->rows_s32[nr - 3][sc]);
	 out->rows_s32[nr - 2][sc] = (val > min) ? val : min;
      }
   }
/*
 * Fill the edge rows and columns of out, extrapolating as needed,
 * still at pixel corners. First fill the array out to its full width, then
 * extrapolate to the first and last rows
 */
   if(nr == 3) {
      for(sc = 0;sc < nc;sc++) {
	  out->rows_s32[2][sc] = out->rows_s32[1][sc];
      }
   } else {
      for(sr = 0;sr < nr - 1;sr++) {
	 val = out->rows_s32[sr][nc - 2] +
			(out->rows_s32[sr][nc - 2] - out->rows_s32[sr][nc - 3]);
	 out->rows_s32[sr][nc - 1] = (val > min) ? val : min;
      }
   }

   if(nc == 3) {
      for(sr = 0;sr < nr;sr++) {
	  out->rows_s32[sr][2] = out->rows_s32[sr][1];
      }
   } else {
      for(sc = 0;sc < nc;sc++) {
	 val = out->rows_s32[nr - 2][sc] +
			(out->rows_s32[nr - 2][sc] - out->rows_s32[nr - 3][sc]);
	 out->rows_s32[nr - 1][sc] = (val > min) ? val : min;
      }
   }
/*
 * we have the full grid on pixel corners but we need it at pixel centres.
 * Shift it by (0.5,0.5).
 */
   facc = 1/(2*(float)dc);
   facr = 1/(2*(float)dr);

   for(sr = 0;sr < nr - 1;sr++) {	/* the main body. Interpolated, so
					 don't impose "min" restriction */
      for(sc = 0;sc < nc - 1;sc++) {
	 out->rows_s32[sr][sc] +=
	   facr*(out->rows_s32[sr + 1][sc] - out->rows_s32[sr][sc]) +
		      facc*(out->rows_s32[sr][sc + 1] - out->rows_s32[sr][sc]);
      }
   }
   
   for(sr = 0;sr < nr - 1;sr++) {		/* and the right */
      val = out->rows_s32[sr][nc - 1] +
	facr*(out->rows_s32[sr + 1][nc-1] - out->rows_s32[sr][nc-1]) +
		      facc*(out->rows_s32[sr][nc-1] - out->rows_s32[sr][nc-2]);
      out->rows_s32[sr][nc - 1] = (val > min) ? val : min;
   }
   for(sc = 0;sc < nc - 1;sc++) {		/* and the top */
      val = out->rows_s32[nr - 1][sc] +
	facr*(out->rows_s32[nr-1][sc] - out->rows_s32[nr-2][sc]) +
		  facc*(out->rows_s32[nr-1][sc + 1] - out->rows_s32[nr-1][sc]);
      out->rows_s32[nr - 1][sc] = (val > min) ? val : min;
   }
   val = out->rows_s32[nr - 1][nc - 1] +		/* and the top right */
     facr*(out->rows_s32[nr - 1][nc-1] - out->rows_s32[nr-2][nc-1]) +
		facc*(out->rows_s32[nr - 1][nc-1] - out->rows_s32[nr-1][nc-2]);
   out->rows_s32[nr - 1][nc - 1] = (val > min) ? val : min;
}

/*****************************************************************************/
/*
 * Debias an estimate of sky
 */
void
phSkyEstimateDebias(BINREGION *breg,	/* BINREGION with smoothed sky */
		    int clipped,	/* were sky values clipped at
					   NSIGMA_CLIP sigma? */
		    float gain1, float gain2) /* gains of two halves of chip */
{
   REGION *skyreg;			/* the sky region */
   float debias;			/* how much to debias data */
   int i, j;
   int val;				/* value to add to skyreg */
   
   shAssert(breg != NULL && breg->reg != NULL);
   skyreg = breg->reg;
   shAssert(skyreg->type == TYPE_S32);

   if(gain2 <= 0) {			/* single amp chip */
      gain2 = gain1;
   }
   shAssert(gain1 > 0 && gain2 > 0);

   if(skyreg->ncol == 1) {
      gain1 = gain2 = 0.5*(gain1 + gain2);
   }

   debias = 1.0/6.0;			/* mean - median for a Poisson */
   if(clipped) {
      debias += MEDIAN_CLIP_BIAS;	/* allow for clipping */
   }

   for(j = 0; j < skyreg->ncol/2; j++) {
      val = debias/gain1*(1 << breg->shift) + 0.5;
      for(i = 0; i < skyreg->nrow; i++) {
	 skyreg->rows_s32[i][j] += val;
      }
   }
   
   for(; j < skyreg->ncol; j++) {
      val = debias/gain2*(1 << breg->shift) + 0.5;
      for(i = 0; i < skyreg->nrow; i++) {
	 skyreg->rows_s32[i][j] += val;
      }
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * given a filled out phHistogram, see comments above phRegStatsFromQuartiles,
 * return a 65536-point LUT to convert U16 numbers to U8 via histogram
 * equalisation
 */
REGION *
phLUTByHistogramEqualize(int min,
			 int max)
{
   int i, j;
   const int *const hist = phHistogram;
   REGION *lut_reg = shRegNew("Histeq LUT", 1, MAX_U16 + 1, TYPE_U8);
   U8 *const lut = lut_reg->rows_u8[0];
   int npix;				/* number of pixels in (min,max) */
   int sum;				/* cumulative value of histogram */
   int target;

   shAssert(phHistogramN > 0);
/*
 * Handle min and max
 */
   if(min < 0) min = 0;
   if(max == 0 || max > MAX_U16) max = MAX_U16;

   sum = 0;
   for(i = 0; i < min; i++) {
      sum += hist[i];
   }
   for(i = max; i <= MAX_U16; i++) {
      sum += hist[i];
   }

   npix = phHistogramN - sum;	/* number of pixels to use */
   
   if(npix <= 0) {
      shError("Range %d -- %d has no pixels; ignoring range", min, max);
      npix = phHistogramN;	/* number of pixels to use */
      min = 0; max = MAX_U16;
   }
/*
 * Build LUT
 */
   sum = 0;
   for(i = 0; i <= min; i++) {
      lut[i] = 0;
   }
   for(j = 0; j < 256 && i <= max; j++) {
      target = npix*j/255.0 + 0.5;
      for(; i <= MAX_U16; i++) {
	 lut[i] = j;
	 if(sum > target) {
	    break;
	 }
	 sum += hist[i];
      }
   }
   for(; i <= MAX_U16; i++) {
      lut[i] = 255;
   }

   return(lut_reg);
}


REGION *
phLUTByAnalyticStretch(int min,	  /* value to map to 0 */
		       int max,	  /* value to map to 255 */
		       int type,  /* type of stretch: 0 = linear (default) */
                                  /*                  1 = sqrt   */
                                  /*                  2 = log    */
                                  /*                  3 = asinh  */
                                  /*                  4 = sqrt(asinh)  */
                                  /*                  5 = histeq  */
		       float param)	/* parameter for stretch */
{
   int i;
   REGION *lut_reg = shRegNew("linear LUT", 1, MAX_U16 + 1, TYPE_U8);
   U8 *const lut = lut_reg->rows_u8[0];
   float slope;				/* slope of DN:uchar line */
   enum lut_types {
      LUT_LINEAR =  0,
      LUT_SQRT =    1,
      LUT_LOG =     2,
      LUT_ASINH =   3,
      LUT_SQRTA =   4,
      LUT_HISTEQ =  5
   };

   lut_reg->row0 = min; lut_reg->col0 = max;

   if(min < max) {
      switch(type) {
       case LUT_LINEAR:
	 slope = 255.0/(max - min);
	 break;
       case LUT_SQRT:
	 slope = 255.0/sqrt(max - min);
	 break;  
       case LUT_LOG:
	 slope = 255.0/log((float)max/min);
	 break;        
       case LUT_ASINH:
	 slope = 255.0/asinh_ph((float)(max - min)/param);
	 break;        
       case LUT_SQRTA:
	 slope = 255.0/sqrt(asinh_ph((float)(max - min)/param));
	 break;
       case LUT_HISTEQ:
	 return(phLUTByHistogramEqualize(min, max));
       default:
	 shErrStackPush("phLUTByAnalyticStretch: unknown type of stretch %d",
			type);
	 shRegDel(lut_reg);
	 return(NULL);
	 break;				/* NOTREACHED */
      }
      
       for(i = 0; i <= MAX_U16; i++) {
	 if(i <= min) {
	    lut[i] = 0;
	 } else if(i >= max) {
	    while(i <= MAX_U16) {
	       lut[i++] = 255;
	    }
	    break;
	 } else {
            switch(type) {
	     case LUT_LINEAR:
	       lut[i] = (i - min)*slope;
	       break;
	     case LUT_SQRT:
	       lut[i] = sqrt(i - min)*slope;
	       break;  
	     case LUT_LOG:
	       lut[i] = log((float)i/min)*slope;
	       break;  
	     case LUT_ASINH:
	       lut[i] = asinh_ph((i - min)/param)*slope;
	       break;
	     case LUT_SQRTA:
	       lut[i] = sqrt(asinh_ph((i - min)/param))*slope;
	       break;
            }    	 
	 }
       }
   } else {
      shError("phLUTByAnalyticStretch: "
	      "inverted stretches are not yet allowed");
      shRegDel(lut_reg);
      return(NULL);
   }

   return(lut_reg);
}
