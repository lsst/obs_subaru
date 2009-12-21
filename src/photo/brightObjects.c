/*
 * Code to deal with Bright Objects
 */
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <math.h>
#include "dervish.h"
#include "phMathUtils.h"
#include "phDgpsf.h"

#define smaErf erf
/*
 * This macro defines the _marginal_ profile to be used;
 * AMP_G1, AMP_G2, and AMP_P are the marginal amplitudes of the small
 * Gaussian, the large Gaussian, and the power law (r^-3) component.
 * The small Gaussian has variance sigma1^2, the large Gaussian has sigma2^2.
 * All profiles are centred at zc.
 */
#define MPROF(AMP_G1,AMP_G2,AMP_P,Z) ((AMP_G1)*exp(-pow((Z - zc)/sigma1,2)) + \
		 (AMP_G2)*exp(-pow((Z - zc)/sigma2,2)) + \
		 (AMP_P)/(1 + pow((Z - zc)/sigma2,2)/3))

/*
 * Parameters of the PSF
 */
static float alpha;			/* 2-d amplitude of small Gaussian */
static float sigma1, sigma2;		/* standard deviations for PSF */
static float lambda;			/* fraction of power in power law */
static float amp_g1, amp_g2, amp_p;	/* amplitudes for marginal PSF */

static float *marginal; static int zmin, zmax; /* used by func */
static double func(double zc, const void *user);


/*****************************************************************************/
/*
 * uses globals marginal, zmin, zmax
 */
static double
func(double zc,
     const void *user)			/* NOTUSED */
{
   float I0;				/* amplitude of model */
   float sum, sumI, sumM;
   int z;
/*
 * estimate amplitude
 */
   sumI = sumM = 0;
   for(z = zmin;z <= zmax;z++) {
      if(marginal[z] >= -1e5) {
	 sumI += marginal[z];
	 sumM += MPROF(amp_g1,amp_g2,amp_p,z);
      }
   }
   if(sumM == 0) {
      return(1e20);
   } else {
      I0 = sumI/sumM;
   }
/*
 * and sum of residuals
 */
   sum = 0.0;
   for(z = zmin;z <= zmax;z++) {
      if(marginal[z] >= -1e5) {
	 sum += pow(marginal[z] - I0*MPROF(amp_g1,amp_g2,amp_p,z),2);
      }
   }

   return(sum);
}


/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 */
void
phCompositeProfileDel(COMP_PROFILE *cprof) /* COMP_PROFILE to delete */
{
   if(cprof != NULL) {
     shFree(cprof->profs);
   }
   shFree(cprof);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Find the centre of a saturated star
 *
 * In general there's extra charge bled into the (non-saturated) pixels
 * adjoining the end saturated pixel in each column, but these pixels
 * have been identified in correct frames.
 *
 * The algorithm used is to extract the marginal distributions of the
 * intensity in the columns that do not contain saturated pixels, and
 * then fit these marginal distributions by the corresponding marginal
 * distribution of the PSF, which is taken to be a sum of two Gaussians
 * and an r^-3 power law tail. The marginal distribution of the power
 * law isn't quite a power law, but we fake this. Note that as all that
 * we want is a centroid, the exact form of the PSF doesn't matter much,
 * providing it isn't too dissimilar. In particular, the power law wings
 * are required for brightish stars.
 *
 * Return 0 for success, -1 for error
 */
int
phSaturatedStarCentroid(const REGION *reg, /* region containing star */
			const OBJMASK *objmask, /* pixels in bright star */
			const DGPSF *psf, /* the core PSF */
			float sky,	/* sky level */
			float *pxc,	/* centre of object */
			float *pyc)
{
   OBJMASK *bad;			/* bad pixels in object's BBox */
   int cmin, cmax, rmin, rmax;		/* unpacked from objmask */
   float *marginal_x, *marginal_y;	/* marginal distributions of dats */
   const int niter = 100;		/* maximum number of iterations */
   int nsat;				/* number of saturated pixels */
   PIX *row;				/* a row of the region */
   SPANMASK *sm;			/* region mask */
   float sum;				/* sum used in finding marginal_y */
   const float tol = 1e-3;		/* tolerance on centre */
   float val;				/* value of a pixel */
   int x,y;				/* column, row counters */
   int x1, x2, x3, x4;			/* x1--x2 and x3--x4 are good columns*/
   double xc, yc;			/* x- and y- centre of object */
   double xc_sat;			/* x-centre from saturation trails */

   shAssert(reg != NULL && reg->type == TYPE_PIX);
   sm = (SPANMASK *)reg->mask;
   shAssert(sm != NULL && sm->cookie == SPAN_COOKIE);
   shAssert(objmask != NULL);
/*
 * See what PSF they want to use
 */
   if(psf == NULL) {
      alpha = 0.9;
      sigma1 = 1;
      sigma2 = 3;
   } else {
      sigma1 = (psf->sigmax1 + psf->sigmay1)/2;
      sigma2 = (psf->sigmax2 + psf->sigmay2)/2;
      alpha = 1/(1 + psf->b);
   }
   lambda = 0.075;			/* fraction of power in power law */
/*
 * Find which pixels, within the bounding box, are to be used in the fit.
 * We'll only ignore the saturated columns, as the other defects (bad columns,
 * cosmic rays) are well enough interpolated over for this purpose.
 */
#if 0					/* use entire detection mask */
   cmin = objmask->cmin - 3;
   cmax = objmask->cmax + 3;
   rmin = objmask->rmin - 3;
   rmax = objmask->rmax + 3;
#else					/* use region around saturated pixels*/
   {
      OBJMASK *sat =
	phObjmaskGrow(phReturnObjmaskAtRect(sm->masks[S_MASK_SATUR],
					    *pxc - 1, *pyc - 1,
					    *pxc + 1, *pyc + 1), reg, 10);
      phObjmaskAndObjmask(sat, objmask); /* clip to objmask */
      shAssert(sat != NULL);
      
      cmin = sat->cmin;
      cmax = sat->cmax;
      rmin = sat->rmin;
      rmax = sat->rmax;

      phObjmaskDel(sat);
   }
#endif

   if(cmin < reg->col0) cmin = reg->col0;
   else if(cmin > reg->col0 + reg->ncol - 1) cmin = reg->col0 + reg->ncol - 1;
   if(cmax < reg->col0) cmax = reg->col0;
   else if(cmax > reg->col0 + reg->ncol - 1) cmax = reg->col0 + reg->ncol - 1;
   if(rmin < reg->row0) rmin = reg->row0;
   else if(rmin > reg->row0 + reg->nrow - 1) rmin = reg->row0 + reg->nrow - 1;
   if(rmax < reg->row0) rmax = reg->row0;
   else if(rmax > reg->row0 + reg->nrow - 1) rmax = reg->row0 + reg->nrow - 1;
   
   {
      OBJMASK *om = phObjmaskFromRect(cmin,rmin,cmax,rmax);
      int i, n;
      bad = phObjmaskIntersection(om,sm->masks[S_MASK_SATUR]);

      if(bad == NULL) {
/*
 * Clipping the region to the objmask must have trimmed off the saturated
 * region (recall that sat != NULL above).  This can happen when
 * DEBLENDED_AT_EDGE; e.g. PR 5405
 *
 */
	 shError("Failed to find overlap with saturated area at (%.1f, %.1f)",
		 *pyc, *pxc);
	 
	 phObjmaskDel(om);
	 return(-1);
      }
      
      nsat = xc_sat = 0;
      for(i = 0;i < bad->nspan;i++) {
	 n = bad->s[i].x2 - bad->s[i].x1 + 1;
	 nsat += n;
	 xc_sat += n*bad->s[i].x1 + (n - 1)*n/2;
      }
      xc_sat /= nsat;
      phObjmaskDel(om);
   }
/*
 * We are going to do two 1-D fits, so we have to use the same set of
 * columns in all rows (and vice-versa) when collapsing the data to its
 * marginal distribution; find the good columns and then the marginal
 * distributions of the pixels
 */
   if(bad->nspan == 0) {
      x1 = cmin; x2 = cmax;		/* all columns are */
      x3 = cmax + 1; x4 = cmax;		/* no columns */
   } else {
      x1 = cmin; x2 = bad->cmin - 1;	/* left hand good portion */
      x3 = bad->cmax + 1; x4 = cmax;	/* right hand good portion */
   }
   phObjmaskDel(bad);
/*
 * Check that all of those rows and columns are valid. Note that x3,x4
 * can be set to cmax+1, cmax if there are no bad columns
 */
   if(x2 <= x1 + 3 || (x3 <= cmax && x4 <= x3 + 3)) { /* too few good columns*/
      shErrStackPush("phSaturatedStarCentroid: "
		     "Object near row %d--%d, col %d--%d has too few pixels",
		     rmin,rmax,cmin,cmax);
      *pxc = (cmin + cmax)/2.0 + 0.4999; /* so that (int)(xc + 0.5) == xc */
      *pyc = (rmin + rmax)/2.0 + 0.4999;
      return(-1);
   }

   marginal_x = alloca((cmax - cmin + 1)*sizeof(float));
   marginal_x -= cmin;
   marginal_y = alloca((rmax - rmin + 1)*sizeof(float));
   marginal_y -= rmin;

   memset(&marginal_x[cmin],'\0',(cmax - cmin + 1)*sizeof(float));
   memset(&marginal_y[rmin],'\0',(rmax - rmin + 1)*sizeof(float));
   
   for(y = rmin;y <= rmax;y++) {
      row = reg->ROWS[y - reg->row0] - reg->col0;
      sum = 0;
      for(x = x1;x <= x2;x++) {
	 val = row[x];
	 marginal_x[x] += val;
	 sum += val;
      }
      for(x = x3;x <= x4;x++) {
	 val = row[x];
	 marginal_x[x] += val;
	 sum += val;
      }
      marginal_y[y] = sum/((x2 - x1 + 1) + (x4 - x3 + 1)) - sky;
   }
   for(x = x1;x <= x4;x++) {
      marginal_x[x] = marginal_x[x]/(rmax - rmin + 1) - sky;
   }   
   for(x = x2 + 1;x < x3;x++) {
      marginal_x[x] = -1e10;		/* i.e. there's no data */
   }   
/*
 * and now do the minimisations, first in x then y. Because we are using
 * the marginal distributions, we have to be a little carefull about the
 * normalisations of the 3 parts of the PSF.
 *
 * We don't actually do the x-minimisation if there are a lot of saturated
 * pixels, as finding the centroid of the bleed trails works better
 */
   xc = (cmin + cmax)/2.0;
   yc = (rmin + rmax)/2.0;

   amp_g1 = alpha*sqrt(2*M_PI)*sigma1;
   amp_g2 = (1 - alpha)*sqrt(2*M_PI)*sigma2;
   amp_p = (lambda/15)*sqrt(3.0)*sigma2*2;
   if(nsat < 10) {			/* use marginal distribution */
      marginal = marginal_x; zmin = cmin; zmax = cmax; /* pass to func */
      if(phBrentMinimum(&xc, cmin, cmax, func, NULL, niter, tol) < 0) {
	 shErrStackPush("phSaturatedStarCentroid: "
			"phBrentMinimum Failed to converge in x\n");
      }
   } else {				/* use bleed trails */
      xc = xc_sat;
   }

   if(x3 < cmax) {			/* i.e. there are missing columns */
      float ysize = (rmax - rmin)/2;	/* characteristic y value */
      amp_g1 = alpha*sqrt(2*M_PI)*sigma1*
	0.5*((1 - smaErf(fabs(x2-xc)/(sqrt(2.0)*sigma1))) +
	     (1 - smaErf(fabs(x3-xc)/(sqrt(2.0)*sigma1))));
      amp_g2 = (1 - alpha)*sqrt(2*M_PI)*sigma2*
	0.5*((1 - smaErf(fabs(x2-xc)/(sqrt(2.0)*sigma2))) +
	     (1 - smaErf(fabs(x3-xc)/(sqrt(2.0)*sigma2))));
      amp_p = (lambda/15)*sqrt(3.0)*sigma2*
	((1 - sqrt(pow(x2-xc,2)/(3*pow(sigma2,2) + pow(x2-xc,2) +
							      pow(ysize,2)))) +
	 (1 - sqrt(pow(x3-xc,2)/(3*pow(sigma2,2) + pow(x3-xc,2) +
							      pow(ysize,2)))));
   }
   marginal = marginal_y; zmin = rmin; zmax = rmax; /* pass to func */
   if(phBrentMinimum(&yc, rmin, rmax, func, NULL, niter, tol) < 0) {
      shErrStackPush("phSaturatedStarCentroid: "
		     "phBrentMinimum Failed to converge in y\n");
   }
   xc += 0.4999;
   yc += 0.4999;		  /* survey definition of coordinates,
				     i.e. (.5,.5) is centre of a pixel;
				     0.4999 so that (int)(xc + 0.5) == xc */
   *pxc = xc; *pyc = yc;
   
   return(0);
}
