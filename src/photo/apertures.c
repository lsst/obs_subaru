/*
 * Use coefficients calculated by make_photom_coeffs to perform aperture
 * photometry
 */
#include <math.h>
#include "dervish.h"
#include "alloca.h"
#include "phExtract.h"

/*
 * here are the coefficients
 */
#include "photom_coeffs.c"

/*****************************************************************************/
/*
 * A utility function to calculate the (clipped) mean and error from the
 * values in an annulus. If sig is NULL, the input array sigma[] may also
 * be NULL
 */
static float
mean_in_annulus(const float *val_in,	/* values to have their mean found */
		const float *sigma_in,	/* sigmas of bins */
		const float *npix_in,	/* number of pixels in each bin */
		int n,			/* number of values to consider */
		int clip,		/* how hard to clip the mean (>= 0) */
		int local_mean,		/* use a local mean for sigma? */
		float *sig		/* `sigma' for the returned value */
		)
{
   float *val;				/* val_in[] valid bins */
   int i,j;
   int nbin;				/* the number of valid cells */
   float *npix;				/* npix_in[] in valid bins */
   float mean;				/* the desired value */
   float *sigma;			/* sigma_in[] for valid bins */
   float norm;				/* normalisaton; sum(npix) */
   float sum;				/* a sum of values */
   float var;				/* estimated variance of mean */

   val = alloca(3*n*sizeof(float));
   npix = val + n;
   sigma = npix + n;
/*
 * Count acceptable pixels, and copy npix_in to npix
 */
   nbin = 0; var = 0;
   for(i = 0;i < n;i++) {
      if((npix[i] = npix_in[i]) > 0) {
	 nbin++;
      }
   }

   if(nbin <= 2*clip) {			/* no points after clipping */
      mean = 65535;
      if(sig != NULL) {
	 *sig = 1e10;
      }
      return(mean);
   }
/*
 * If we have to clip, find the values that are to be deleted. Note that
 * we cannot simply sort val, as we are going to calculate the variance
 * based on the order of val[]
 */
   if(clip > 0) {
      float *arr;			/* sorted copy of val_in[] */
      float tmp;
      int top;				/* clip the top or bottom of range? */

      arr = alloca(n*sizeof(float));

      nbin = 0;
      for(i = 0;i < n;i++) {		/* sort input values into arr[] */
         if(npix[i] > 0) {
            tmp = val_in[i];
            for(j = nbin - 1;j >= 0 && arr[j] > tmp;j--) {
               arr[j + 1] = arr[j];
            }
            arr[j + 1] = tmp;
            nbin++;
         }
      }
/*
 * set npix to -1 for the values we want clipped
 */
      for(top = 0;top <= 1;top++) {
	 for(j = 0;j < clip;j++) {
	    tmp = (top) ? arr[nbin - j - 1] : arr[j]; /* value to clip */
	    for(i = 0;i < n;i++) {
	       if(npix[i] > 0) {
		  if(val_in[i] == tmp) { /* clip this value */
		     npix[i] = -1;
		     break;
		  }
	       }
	    }
	 }
      }
   }      
/*
 * copy values into local copies of arrays, only copying values for which
 * npix > 0
 */
   nbin = 0;
   for(i = 0;i < n;i++) {
      if(npix[i] > 0) {
	 val[nbin] = val_in[i];
	 npix[nbin] = npix[i];
	 sigma[nbin] = (sigma_in == NULL) ? 0 : sigma_in[i];
	 nbin++;
      }
   }
/*
 * At this point, val[], npix[], and sigma[] are set to the input values
 * that had npix >= 0, and weren't clipped
 */
   mean = norm = 0;
   for(i = 0;i < nbin;i++) {
      mean += val[i]*npix[i];
      norm += npix[i];
   }
   shAssert(norm != 0);
   mean /= norm;

   if(sig == NULL) {      
      return(mean);
   }
      
   if(nbin == 1) {			/* only one bin left */
      *sig = sigma[0];
   } else {
      if(local_mean) {
/*
 * use the mean of the neighbouring points as an estimate of the mean
 */
	 float lmean;			/* local mean */
	 float nm, vm, np, vp;		/* {npix,val}[i-1], {npix,val}[i+1] */

	 sum = 0;
	 for(i = 0;i < nbin;i++) {
	    if(i == 0) {
	       nm = npix[nbin - 1]; vm = val[nbin - 1];
	       np = npix[i + 1]; vp = val[i + 1];
	    } else if(i == nbin - 1) {
	       nm = npix[i - 1]; vm = val[i - 1];
	       np = npix[0]; vp = val[0];
	    } else {
	       nm = npix[i - 1]; vm = val[i - 1];
	       np = npix[i + 1]; vp = val[i + 1];
	    }
	    lmean = (nm*vm + np*vp)/(nm + np);
	    sum += npix[i]*pow(val[i] - lmean,2);
	 }
	 
	 var = sum/norm;
	 var *= 4.0f/9.0f;		/* debias estimate */
      } else {
/*
 * use a single global value
 */
	 float norm2;			/* sum(npix^2) */
      
	 sum = norm2 = 0;
	 for(i = 0;i < nbin;i++) {
	    sum += npix[i]*pow(val[i] - mean,2);
	    norm2 += pow(npix[i],2);
	 }
	 
	 var = sum/norm;
	 var /= (1 - norm2/pow(norm,2)); /* debias estimate */
      }
   }
   *sig = sqrt(var/nbin);		/* s.d. of mean */

   if(clip) {
      ;					/* XXX correction for clipping */
   }

   return(mean);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Return the mean of the means in annulus ann of the extracted profile prof;
 * the clip largest and smallest means are rejected
 *
 * If sig is not NULL, return an estimate of the noise in the aperture
 */
float
phProfileMean(
	      const CELL_STATS *cstats, /* the radial profile */
	      int ann,			/* which annulus? */
	      int clip,			/* how hard to clip the mean (>= 0) */
	      int local_mean,		/* use a local mean for sigma? */
	      float *sig)		/* estimate of noise */
{
   float area;				/* number of pixels in a cell */
   int cell;				/* index of a cell in the annulus */
   float flux;				/* total flux in the annulus */
   int i;
   int ncell;				/* the number of valid cells */
   float mean[NSEC], sigma[NSEC], npix[NSEC];
   float profMean;			/* the desired value */

   if(ann == 0) {
      if(sig != NULL) {
	 *sig = 0.0;			/* we have no estimate of the noise */
      }
      return(cstats->cells[0].mean);
   }

   ncell = 0;
   flux = 0.0;
   for(i = 0;i < NSEC;i++) {
      cell = (ann - 1)*NSEC + i + 1;
   
      area = (cell < cstats->ncell &&
	      cstats->cells[cell].flg & EXTRACT_EDGE) ? 
						  0 : cstats->cells[cell].area;
      if(area > 0) {
	 npix[ncell] = area;
	 mean[ncell] = cstats->cells[cell].mean;

	 flux += area*mean[ncell];

	 if(sig != NULL) {
	    sigma[ncell] = cstats->cells[cell].sig;
	 }
	 ncell++;
      }
   }
/*
 * if we are not clipping, we must ensure that the identity
 *	counts = sum (mean_i*area_i)
 * holds, where the sum is over complete annuli, and area is the canonical
 * area in (e.g.) the CELL_STATS structure. It is in general not true that
 * the sum of the areas of the cells exactly equals area_i, so be careful
 */
   if(clip || ann >= cstats->nannuli_c) {
      profMean = (ncell == 0) ? 0 :
	 mean_in_annulus(mean, sigma, npix, ncell, clip, local_mean, sig);
   } else {
      if(sig != NULL) {
	 (void)mean_in_annulus(mean, sigma, npix, ncell, clip, local_mean,sig);
      }
      profMean = flux/cstats->area[ann];
   }

   return(profMean);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Return the mean of the medians in annulus ann of the extracted profile prof;
 * the clip largest and smallest medians are rejected
 *
 * If sig is non-NULL, also calculate an estimate of the standard deviation
 * of the value; this is based on the assumption that the values sampled
 * belong to a Gaussian distribution, and properly allows for the clipping
 * on that assumption.
 */
float
phProfileMedian(
		const CELL_STATS *cstats, /* the radial profile */
		int ann,		/* which annulus? */
		int clip,		/* how hard to clip the mean of
					   the medians (>= 0) */
		int local_mean,		/* use a local mean for sigma? */
		float *sig		/* `sigma' for the returned value */
		)
{
   int cell;				/* index of a cell in the annulus */
   int i;
   int ncell;				/* the number of valid cells */
   int ntot;				/* number of pixels in a cell
					   (or 1/2 pixels in inner part) */
					   
   float med[NSEC], sigma[NSEC], npix[NSEC];

   if(ann == 0) {
      if(sig != NULL) {
	 *sig = 0.0;			/* we have no estimate of the noise */
      }
      return(cstats->cells[0].qt[1]);
   }

   ncell = 0;
   for(i = 0;i < NSEC;i++) {
      cell = (ann - 1)*NSEC + i + 1;
   
      ntot = (cstats->cells[cell].flg & EXTRACT_EDGE) ? 
						  0 : cstats->cells[cell].ntot;
      if(ntot > 0) {
	 npix[ncell] = ntot;
	 med[ncell] = cstats->cells[cell].qt[1];

	 if(sig != NULL) {
	    sigma[ncell] = cstats->cells[cell].sig;
	 }
	 ncell++;
      }
   }

   return(mean_in_annulus(med, sigma, npix, ncell, clip, local_mean, sig));
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Find the mean in an aperture, calculated using sinc apertures if
 * the coefficients are available for the desired aperture
 *
 * Returns 0 if the mean was calculated, -1 otherwise
 */
int
phProfileMeanFromSincApertures(const CELL_STATS *cstats, /* radial profile */
			       int ann,	/* which annulus? */
			       int clip, /* how hard to clip the mean (>= 0) */
			       int local_mean, /* use a local mean for sigma?*/
			       float *ret_mean, /* measured mean */
			       float *sig) /* estimate of noise */
{
   float ans;				/* the desired answer */
   int cell;				/* index of a cell in the annulus */
   int i;
   int ncell;				/* the number of valid cells */
   float mean[NSEC], sigma[NSEC], npix[NSEC];
   const REGION *reg = cstats->syncreg;
   int rowc = reg->nrow/2;
   int colc = reg->ncol/2;

   if(ann == 0) {
      if(sig != NULL) {
	 *sig = 0.0;			/* we have no estimate of the noise */
      }
      phSincApertureEval(cstats->syncreg,0,0,rowc,colc,&ans);
      *ret_mean = ans/coefficients[0].sum;

      return(0);
   }

   ncell = 0;
   for(i = 0;i < NSEC;i++) {
      cell = (ann - 1)*NSEC + i + 1;
   
      if(phSincApertureEval(cstats->syncreg,cell,0,rowc,colc,&ans) != 0) {
	 return(-1);
      } else {
	 npix[ncell] = coefficients[cell].sum;
	 mean[ncell] = ans/npix[ncell];
      }

      if(sig != NULL) {
	 sigma[ncell] = cstats->cells[cell].sig;
      }
      ncell++;
   }

   *ret_mean = mean_in_annulus(mean, sigma, npix, ncell, clip, local_mean,sig);

   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Evaluate the flux in a given aperture coeffs for the given region. The
 * object is assumed to be at the centre of the specified pixel
 *
 * Return 0 if all is well, else -1.
 *
 * See also phSincApertureEval
 */
int
phSincApertureEvalFromCoeffs(const REGION *reg,	/* input data */
			 const COEFF *coeffs,	/* the aperture coefficients */
			 int bkgd,	/* the REGION's background level */
			 int rowc, int colc,	/* central pixel of object
						   (ignoring any row0, col0 in
						   the REGION) */
			 float *val)		/* the answer */
{
   int i,j;
   PIX *rows;
   float sum = 0.0;

   shAssert(reg != NULL && reg->type == TYPE_PIX);

   if(rowc + coeffs->rmin < 0 ||
      rowc + coeffs->rmax >= reg->nrow ||
      colc + coeffs->cmin < 0 ||
      colc + coeffs->cmax >= reg->ncol) {
      return(-1);
   }
/*
 * Calculate sum of image with coefficients, row by row
 */
   for(i = 0;i <= coeffs->rmax - coeffs->rmin;i++) {
      rows = &reg->ROWS[rowc + coeffs->rmin + i][colc + coeffs->run[i].col0];
      for(j = 0;j < coeffs->run[i].n;j++) {
	 sum += rows[j]*coeffs->run[i].coeffs[j];
      }
   }

   *val = sum - bkgd*coeffs->sum;

   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Like phSincApertureEvalFromCoeffs, but specify aperture by number not by
 * passing a pointer. These numbers are the cells as used in phExtract...
 */
int
phSincApertureEval(const REGION *reg,	/* input data */
		   int c,		/* the desired aperture */
		   int bkgd,		/* the REGION's background level */
		   int rowc, int colc,	/* central pixel of object */
		   float *val)		/* the answer */
{
   if(c < 0 || c >= sizeof(coefficients)/sizeof(coefficients[0])) {
      *val = 0;
      return(-1);
   }

   return(phSincApertureEvalFromCoeffs(reg, &coefficients[c],
				       bkgd, rowc, colc,val));
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return the mean flux within a cell, specified as a cell number as used
 * by phExtractProfile
 */
int
phSincApertureMean(const REGION *reg,	/* input data */
		   int c,		/* the desired aperture */
		   int bkgd,		/* the REGION's background level */
		   int rowc, int colc,	/* central pixel of object */
		   float *val,		/* the answer */
		   float *area)		/* number of pixels involved */
{
   if(phSincApertureEval(reg, c, bkgd, rowc, colc, val) < 0) {
      *area = 0;
      return(-1);
   }

   *area = coefficients[c].sum;
   *val /= coefficients[c].sum;

   return(0);
}
