/*
 * Code to deal with Bright Objects
 */
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <math.h>
#include "dervish.h"
#include "atConversions.h"		/* for M_PI */
#include "phMathUtils.h"
#include "phBrightObjects.h"
#include "phExtract.h"
#include "phMath.h"			/* for erf */

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
 * File-static globals for fitting functions
 */
static int i0;				/* first index in profile to use */
static double area[NANN];		/* areas of annuli */
static const float *r = NULL;		/* radii for annuli */
static double prof[NANN];		/* sky-subtracted profile */
static double sig[NANN];		/* s.d. of prof[] */
static double core[NANN];		/* psf values in annuli */
static double wing[NANN];		/* power-law-wing values in annuli */

/*****************************************************************************/
/*
 * Return the index of the smallest annulus that doesn't contain
 * saturated pixels
 */
static int
first_good_index(const OBJECT1 *obj1, const OBJMASK *sat)
{
   float cmin, cmax;			/* min, max saturated columns */
   int i0;				/* index of smallest good annulus */
   int i;
   float rad_min;			/* minimum usable radius */

   if(!(obj1->flags & OBJECT1_SATUR)) {
      return(0);
   }

   shAssert(sat != NULL);

   if(r == NULL) {			/* r's not yet looked up */
      r = phProfileGeometry()->radii;
      for(i = 0;i < NANN;i++) {
	 area[i] = M_PI*(pow(r[i+1],2) - pow(r[i],2));
      }
   }

   cmin = fabs(sat->col0 + sat->cmin - obj1->colc);
   cmax = fabs(sat->col0 + sat->cmax - obj1->colc + 0.999);
   
   rad_min = (cmax < cmin) ? cmin : cmax;
   
   for(i = 1;i <= obj1->nprof;i++) {
      if(r[i] > rad_min) {
	 break;
      }
   }
   i0 = i - 1;

   return(i0);
}

/*****************************************************************************/
/*
 * return chisq for a fit to a star. Note the use of the array ind[] to
 * indicate _which_ n parameters are to be fit.
 */
enum {
   I0_1 = 0,				/* narrow central intensity */
   I0_2,				/* wide central intensity */
   I0_P,				/* power law central intensity */
   SIG_1,				/* narrow sigma */
   SIG_2,				/* wide sigma */
   SIG_P,				/* "sigma" of power law */
   BETA,				/* power law index */
   NPAR					/* number of parameters */
};

static double p[NPAR];			/* parameters of model */
static int ind[NPAR];			/* map from param[] to p[] */
static int median_profile;		/* we are using the median profile */

static void
fit_prof(int m,				/* number of data points */
	 int n,				/* number of parameters */
	 double *param,			/* parameters being varied */
	 double *resid,			/* residuals/sigma */
	 int *flg)			/* flag; unused */
{
   int i;

   shAssert(*flg >= 0);

   for(i = 0;i < n;i++) {
      p[ind[i]] = param[i];
   }
   
/*
 * limit wing parameters to some reasonable range
 */
    
   if(p[BETA] <= 0) {
      p[BETA] = 0.0001;	  /* avoid non-negative beta */
   }  
   if(p[BETA] > 10) {
      p[BETA] = 10.0;	  /* avoid large beta */
   }  
   if(p[BETA] - 2 == 0) {
      p[BETA] = 2.0001;	  /* avoid divide by zero */
   }
   if(p[SIG_P] <= 0) {
      p[SIG_P] = 0.0001;  /* avoid non-negative sigmaP */
   }  
   if(p[SIG_P] > 100) {
      p[SIG_P] = 100.0;	  /* avoid large sigmaP */
   }     
   if(p[I0_P] < 0) {
      p[I0_P] = 0.0;      /* avoid negative p0 */
   }  
   if(p[I0_P] > 10.0) {
      p[I0_P] = 10.0;      /* avoid unreasonably large p0 */
   }  

/*
 * find mean value in annuli for core and wings
 */
   if(median_profile) {
      float r_ms;	             /* (mean-square radius)^2 for annulus */
      for(i = 0;i < NANN;i++) {
	 r_ms = (pow(r[i],2) + pow(r[i + 1],2))/2;
	 core[i] = p[I0_1]*exp(-0.5*r_ms*pow(1/p[SIG_1],2)) +
	           p[I0_2]*exp(-0.5*r_ms*pow(1/p[SIG_2],2)); 
         /* remove beta from denom.: */
	 /* wing[i] = p[I0_P]*pow(1 + r_ms / pow(p[SIG_P],2)/p[BETA],-p[BETA]/2); */
            wing[i] = p[I0_P]*pow(1 + r_ms / pow(p[SIG_P],2),-p[BETA]/2);  
      }
   } else {
      for(i = 0;i < NANN;i++) {
	 core[i] = 2*M_PI*
	   (p[I0_1]*pow(p[SIG_1],2)*(1 - exp(-0.5*pow(r[i+1]/p[SIG_1],2))) +
	    p[I0_2]*pow(p[SIG_2],2)*(1 - exp(-0.5*pow(r[i+1]/p[SIG_2],2))));
           /* remove beta from denom.: */
	   wing[i] = 2*p[I0_P]*M_PI*pow(p[SIG_P],2)/(p[BETA] - 2)*
	           (1 - pow(1 + pow(r[i + 1]/p[SIG_P],2),1 - p[BETA]/2));
      }
      
      for(i = NANN - 1;i > 0;i--) {
	 wing[i] = (wing[i] - wing[i - 1])/area[i];
	 core[i] = (core[i] - core[i - 1])/area[i];
      }
   }
/*
 * and actually evaluate residuals
 */
   for(i = 0;i < m;i++) {
      resid[i] = (prof[i+i0] - core[i+i0] - wing[i+i0])/sig[i+i0];
   }

}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Given a bright star's OBJECT1 that's been run through phMeasureObjc,
 * fit the PSF, including two Gaussians and a power law component
 *
 * If fparams is non-NULL, information in the frame mask is used to
 * reject pixels contaminated by saturation
 *
 * The default is to fit a single Gaussian.
 */
DGPSF *
phStarParamsFind(const COMP_PROFILE *comp_prof, /* composite profile to fit */
		 const DGPSF *psf,	/* input values of parameters */
		 float profmin,		/* minimum profile value to consider */
		 int medflg,		/* use median not mean profile? */
		 int fit_g2,		/* fit a second Gaussian component */
		 int fit_p,		/* fit a power-law component */
		 int fit_sigmap,	/* fit "sigma" for power law */
		 int fit_beta,		/* fit index of power law */
		 float *chisq)		/* reduced chisq of fit */
{
   double norm = -1e10;			/* sqrt(chisq) of fit */
   int i;
   int nfit;				/* number of parameters to fit */
   int nfunc;				/* number of function evals used */
   int nprof;				/* number of profile points */
   double params[NPAR];			/* parameters to fit */
   int stat;				/* status return from LM */
   double tol;				/* desired tolerance */
   double sumsq_tol;			/* quit when norm reaches this value*/

   shAssert(comp_prof->nprof > 0);
   
   median_profile = medflg;

   if(r == NULL) {
      r = phProfileGeometry()->radii;
      for(i = 0;i < NANN;i++) {
	 area[i] = M_PI*(pow(r[i+1],2) - pow(r[i],2));
      }
   }
/*
 * copy object's profile into globals
 */
   nprof =comp_prof->nprof;
   for(i = 0;i < nprof;i++) {
      prof[i] = medflg ? comp_prof->med[i] : comp_prof->mean[i];
      sig[i] = comp_prof->sig[i];
   }
   
   i0 = 0;  /* all points are good since checked in CompProfAdd */

/*
 * And only consider those parts of the profile with a level of
 * more than profmin counts, so as to reduce our sensitivity to the
 * not-very-well-determined value of the sky
 */
   for(i = 0;i < nprof;i++) {
      if(prof[i] < profmin) {
	 nprof = i;
	 break;
      }
   }
/*
 * initial values for parameters
 */
   p[I0_1] = prof[0]/(1 + psf->b + psf->p0); /* narrow peak */
   p[I0_2] = psf->b*p[I0_1];		/* wide peak */
   p[I0_P] = psf->p0*p[I0_1];		/* peak value of powerlaw wing */
   p[SIG_1] = (psf->sigmax1 + psf->sigmay1)/2; /* narrow width */
   p[SIG_2] = (psf->sigmax2 + psf->sigmay2)/2; /* wide width */
   p[SIG_P] = psf->sigmap;		/* "sigma" for powerlaw */
   p[BETA] = psf->beta;			/* powerlaw index */
/*
 * select which parameters to fit. We put the ones that we want into the
 * first nfit places in the params[] array, and indicate which they are
 * in the ind[] array (which is used in fit_prof())
 */
   nfit = 0;
   ind[nfit++] = I0_1;
   ind[nfit++] = SIG_1;
   if(fit_g2) {
      ind[nfit++] = I0_2;
      ind[nfit++] = SIG_2;
   }
   if(fit_p) {
      ind[nfit++] = I0_P;
   }
   if(fit_sigmap) {
      ind[nfit++] = SIG_P;
   }
   if(fit_beta) {
      ind[nfit++] = BETA;
   }
   shAssert(nfit <= NPAR);

   for(i = 0;i < nfit;i++) {
      params[i] = p[ind[i]];
   }
/*
 * OK, set tolerances and call minimiser
 */
   sumsq_tol = nprof - i0 - nfit;
   if(sumsq_tol <= 0) {
      shErrStackPush("phStarParamsFind: "
		     "Two few non-saturated points in radial profile");
      return(NULL);
   }
   tol = 1e-6;
   stat = phLMMinimise(fit_prof,nprof - i0,nfit,params,&norm,&nfunc,
						      tol, sumsq_tol, 0, NULL);
   *chisq = norm*norm/(nprof - nfit);	/* chisq per degree of freedom */

   if(fit_g2) {
     if(params[SIG_2] < params[SIG_1]) { /* make params[{SIG,I0}_1] narrow */
       float tmp;
       tmp = params[SIG_1]; params[SIG_1] = params[SIG_2]; params[SIG_2] = tmp;
       tmp = params[I0_1]; params[I0_1] = params[I0_2]; params[I0_2] = tmp;
     }
   }

   if(stat < 0) {			/* user exit */
      shFatal("phStarParamsFind: user exit");
   }
   if(stat == 0) {			/* improper input params */
      return(NULL);
   } else {
      if(stat & ~07) {
	 shErrStackPush("phStarParamsFind: LM Minimiser returns %d",stat);
	 return(NULL);
      } else {
	 DGPSF *opsf = phDgpsfNew();

/*       Minimizer sometimes returns negative sigmas, but as we only care about
 *       the variances, take absolute value
 */
         p[SIG_1] = fabs(p[SIG_1]);
         p[SIG_2] = fabs(p[SIG_2]);
         p[SIG_P] = fabs(p[SIG_P]); 

/*       For convenience, let's guarantee that sigma1 is smaller
 */   
         if (p[SIG_1] > p[SIG_2]) {
             float tmp = p[SIG_1];
 	     p[SIG_1] = p[SIG_2];
 	     p[SIG_2] = tmp;
             tmp = p[I0_1];
 	     p[I0_1] = p[I0_2];
 	     p[I0_2] = tmp;
         }

	 opsf->a = p[I0_1] + p[I0_2] + p[I0_P];
	 opsf->sigmax1 = opsf->sigmay1 = p[SIG_1];
	 opsf->sigmax2 = opsf->sigmay2 = p[SIG_2];
         if (p[I0_1] > 0) {
	     opsf->b = p[I0_2]/p[I0_1];
         } else {
	     opsf->b = 0.0;
         }
         if (p[BETA] > 0) {
             if (p[I0_1] > 0) {
	         opsf->p0 = p[I0_P]/p[I0_1];
             } else {
	         opsf->p0 = 0.0;
             }
             /* as fit_prof has beta removed, put it back here */
	     opsf->sigmap = p[SIG_P]/sqrt(p[BETA]);
	     opsf->beta = p[BETA];
         } else {
	     opsf->p0 = 0;
	     opsf->sigmap = 1.0;
	     opsf->beta = 0;
         }
	 opsf->chisq = *chisq;

	 return(opsf);
      }
   }
}

/*****************************************************************************/
/*
 * Create/Destroy a WINGOBJECT
 */
WINGOBJECT *
phWingobjectNew(void)
{
   WINGOBJECT *ss = shMalloc(sizeof(WINGOBJECT));
   memset(ss, sizeof(WINGOBJECT), '\0');

   return(ss);
}

void
phWingobjectDel(WINGOBJECT *ss)
{
   shFree(ss);
}

/************************************************************************************************************/
/*
 * Create/destroy a OBJECT_MODEL
 */
OBJECT_MODEL *
phObjectModelNew(void)
{
   int i;
   OBJECT_MODEL *mo = shMalloc(sizeof(OBJECT_MODEL));
   mo->id[0] = mo->id[1] = 0;
   mo->ncomp = 0;
   
   for(i = 0; i < MODEL_NCOMP; i++) {
      mo->type[i] = VALUE_IS_BAD;

      mo->weight[i] = mo->counts[i] = mo->r_e[i] =
	  mo->ab[i] = mo->phi[i] = VALUE_IS_BAD;
      mo->spare1[i] = mo->spare2[i] = VALUE_IS_BAD;
   }

   return mo;
}

void
phObjectModelDel(OBJECT_MODEL *mo)
{
    shFree(mo);
}

/*****************************************************************************/
/*
 * This is horrible!  I want to store a pointer in an OBJECT_MODEL, but
 * write out the ID.  I'd use a long (or long long) but dervish v8_22 refuses
 * to write the resulting struct.  Therefore we make the ID field a
 * 2-byte array and use these functions to set/get it.  Yuck.
 */
void
phObjectModelIdSetFromPtr(OBJECT_MODEL *om, const OBJECT1 *obj1)
{
    shAssert(om != NULL && obj1 != NULL);

    shAssert(sizeof(om->id) >= sizeof(obj1));
    memcpy(om->id, (void *)&obj1, sizeof(obj1));
}

OBJECT1 *
phObjectModelPtrGetFromId(OBJECT_MODEL *om)
{
    OBJECT1 *obj1 = NULL;
    
    shAssert(om != NULL);
    shAssert(om->id[0] != 0);
    if (sizeof(obj1) > sizeof(om->id[0])) {
	shAssert(om->id[1] != 0);
    }

    shAssert(sizeof(om->id) >= sizeof(obj1));
    memcpy((void *)&obj1, om->id, sizeof(obj1));

    return obj1;
}

/*****************************************************************************/
/*
 * Here's the workhorse routine to calculate a star's wings, which are
 * represented as a power-law with a parabolic core:
 *
 *   if(r > rtrans) {
 *      wing = p0*(1 + (r/sigmap)^2/beta)^{-beta/2}
 *   } else {
 *      wing = A*p0*(1 - (r/alpha)^2)
 *   }
 * where A and alpha are chosen to match derivative and amplitude at rtrans:
 *   alpha^2 = 2*sigmap^2 + rtrans^2*(2/beta + 1)
 *   A = (1 + (rtrans/sigmap)^2/beta)^{-beta/2}/(1 - (rtrans/alpha)^2)
 */
static int
subtract_star(BINREGION *wingreg,	/* add wings into these BINREGIONs */
	      const WINGOBJECT *star_wing,	/* describe the wings */
	      const FRAMEPARAMS *fparams, /* info on offsets etc. */
	      float nsigma)		/* mark pixels where we subtracted more
					   than nsigma*skysigma as NOTCHECKED*/
{
   float A, ialpha2;			/* coefficients for parabolic core */
   float ibss;				/* == 1/(beta*sigmap^2) */
   float beta;				/* == star_wing->beta */
   int bin_fac;				/* amount wingreg is binned by */
   int c,r;				/* column and row counters */
   int colc, rowc;			/* centre of the star */
   int cmin, cmax, rmin, rmax;		/* range of column and row number where
					   (int)(scaled star wing) is > 0 */
   int cc, rr2;				/* (col - colc), (row - rowc)^2 */
   float r2;				/* == cc*cc + rr2 */
   SPANMASK *datamask;			/* mask for entire frame in this band*/
   int rad_max;				/* maximum radius to follow halo to */
   float rtrans;			/* == star_wing->rtrans */
   float rtrans2;			/* == rtrans*rtrans */
   S32 *rows;				/* == wingreg->reg->rows_s32[r] */
   float p0;				/* == star_wing->p0 */
   float sigmap;			/* == star_wing->sigmap */
   float skyErr;			/* sigma in sky at the star */
   float sp0;				/* scaled version of p0 */
   float wing;				/* a value in the power law wing */

   shAssert(star_wing != NULL);

   rowc = star_wing->rowc;
   colc = star_wing->colc;
   p0 = star_wing->p0;
   beta = star_wing->beta;
   rtrans = star_wing->rtrans;
   sigmap = star_wing->sigmap;

   shAssert(wingreg != NULL);
   bin_fac = wingreg->bin_col;

   shAssert(fparams != NULL && fparams->data != NULL);
   datamask = (SPANMASK *)fparams->data->mask;
/*
 * Fixup the core of the subtracted model
 *
 * calculate the parameters of the parabolic core that match at rtrans
 */
   rtrans2 = rtrans*rtrans;
   ialpha2 = 1/(2*pow(sigmap,2) + rtrans2*(2/beta + 1));
   A = pow(1 + rtrans2/pow(sigmap,2)/beta,-beta/2)/(1 - rtrans2*ialpha2);
/*
 * See over how large an area we have to calculate this star; once the scaled
 * value drops below 0.5, it'll be truncated to zero anyway
 */
   rad_max = sigmap*sqrt(beta*(pow(2*p0*(1 << wingreg->shift),2/beta) - 1));
   if(rad_max < 1) {			/* too faint to bother subtracting */
      return(-1);
   }
   
   rmin = (rowc - rad_max)/bin_fac - 1;
   rmax = (rowc + rad_max)/bin_fac + 1;
   rmin = (rmin < 0) ? 0 : rmin;
   rmax = (rmax >= wingreg->reg->nrow) ? wingreg->reg->nrow - 1 : rmax;

   cmin = (colc - rad_max)/bin_fac - 1;
   cmax = (colc + rad_max)/bin_fac + 1;
   cmin = (cmin < 0) ? 0 : cmin;
   cmax = (cmax >= wingreg->reg->ncol) ? wingreg->reg->ncol - 1 : cmax;
/*
 * calculate the wings, and add them to the BINREGION
 */
   ibss = 1/(beta*sigmap*sigmap);	/* move constants out of the loop */
   sp0 = p0*(1 << wingreg->shift);
   A *= sp0;
   for(r = rmin;r <= rmax;r++) {
      rr2 = pow(bin_fac*r - rowc,2) + 0.5;
      rows = wingreg->reg->rows_s32[r];
      cc = bin_fac*cmin - colc;
      for(c = cmin;c <= cmax;c++) {
	 r2 = rr2 + cc*cc;
	 if(r2 < rtrans2) {
	    wing = A*(1 - r2*ialpha2);
	 } else {
	    wing = sp0*pow(1 + r2*ibss,-beta/2);
	 }

	 rows[c] += wing + 0.5;
	 cc += bin_fac;
      }
   }
/*
 * Now deal with the consequences of that subtraction for the masks.
 *
 * We'll label all pixels where we subtracted more than 1 skysigma
 * as SUBTRACTED
 *
 * For the NOTCHECKED region, we consider two criteria; where the wings
 * were more than 10 skysigma, and where the wings increased the variance
 * by more than 50%; we adopt the one giving the _larger_ region
 */
   skyErr = phBinregionInterpolate(fparams->skyErr,rowc,colc);

   if(p0 > skyErr) {
      float thresh, thresh2;
      
      thresh = skyErr;
      rad_max = sigmap*sqrt(beta*(pow(p0/thresh,2/beta) - 1));
      if(rad_max < 2*bin_fac) {		/* not reasonably interpolated */
	 rad_max = 2*bin_fac;
      }
      if(rad_max < rtrans) {
	 rad_max = rtrans;
      }
      phObjmaskAddToSpanmask(phObjmaskFromCircle(rowc, colc, rad_max),
			     datamask, S_MASK_SUBTRACTED);
/*
 * the core of that subtraction is not trustworthy
 */
      thresh = nsigma*skyErr;
      thresh2 = 0.5*phGain(fparams, rowc, colc)*skyErr*skyErr;
      if(thresh2 < thresh) {
	 thresh = thresh2;
      }
      if(thresh > p0) {
	 rad_max = 0;
      } else {
	 rad_max = sigmap*sqrt(beta*(pow(p0/thresh,2/beta) - 1));
      }
      if(rad_max < rtrans) {
	 rad_max = rtrans;
      }
      if(rad_max < 2*bin_fac) {	/* not reasonably interpolated */
	 rad_max = 2*bin_fac;
      }
      rad_max += 5;			/* allow for centering errors */
      phObjmaskAddToSpanmask(phObjmaskFromCircle(rowc, colc, rad_max),
			     datamask, S_MASK_NOTCHECKED);
   }

   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Add the power-law wing of a star into the given BINREGION sky.
 *
 * The amplitude of the wing is set by the star's PSF counts (if it's
 * saturated these will be estimated from the profile, assuming that
 * a complete stellar profile is provided in cprof. This will usually
 * be done using mean profiles, but it medflg is true the median profile
 * will be employed. If r_max is positive, only radial points inside
 * r_max will be used in the match)
 *
 * The form true wing is specified by beta, sigmap, and frac, but we modify
 * this function near the centre, replacing it by a paraboloid that is matched
 * in value and slope at rtrans
 */
WINGOBJECT *
phBrightStarWings(OBJECT1 *obj1,	/* the object in question */
		  const COMP_PROFILE *cprof, /* a composite profile */
		  const FRAMEPARAMS *fparams, /* info on offsets etc. */
		  int medflg,		/* use median profile? */
		  float r_max,		/* use only profile to this radius */
		  BINREGION *wingreg,	/* add wings into these BINREGIONs */
		  float beta,		/* power law slope to assume */
		  float sigmap,		/* "sigma" for power law wings */
		  float frac,		/* ratio of central value of wings
					   to PSF counts */
		  float rtrans,		/* transition radius to a parabolic
					   form near the centre */
		  float nsigma)		/* mark pixels where we subtracted more
					   than nsigma*skysigma as NOTCHECKED*/
{
   SPANMASK *datamask;			/* mask for entire frame in this band*/
   float psf_counts;			/* psf counts for this object */
   OBJMASK *sat;                        /* mask of saturated pixels */
   WINGOBJECT *star_wing = NULL;	/* describe subtracted star */

   shAssert(fparams != NULL && fparams->data != NULL &&
	    fparams->data->mask != NULL);
   shAssert(wingreg != NULL && wingreg->reg != NULL);
   shAssert(wingreg->reg->type == TYPE_S32);
   shAssert(wingreg->bin_row == wingreg->bin_col);
   shAssert(obj1 != NULL && obj1->peaks != 0 && obj1->peaks->npeak > 0);
   shAssert(cprof != NULL && cprof->prof_is_set);
   
   datamask = (SPANMASK *)fparams->data->mask;

   star_wing = phWingobjectNew();

   if(obj1->flags & OBJECT1_EDGE) {	/* give up */
      phWingobjectDel(star_wing);
      return(NULL);
   }

   if(obj1->flags & OBJECT1_SATUR) {
      sat = phObjmaskIntersection(obj1->mask,datamask->masks[S_MASK_SATUR]);
      shAssert(sat != NULL);
      psf_counts = phPSFCountsGetFromProfile(obj1,cprof,
                                             first_good_index(obj1,sat),
					     medflg, r_max);
      phObjmaskDel(sat);
      if(psf_counts < -1e10) {
	 phWingobjectDel(star_wing);
	 return(NULL);			/* photo can continue safely */
      }
   } else {
      psf_counts = obj1->psfCounts;
   }

/*
 * Save required book-keeping information
 */
   star_wing->p0 = psf_counts*frac;
   star_wing->rowc = obj1->rowc;
   star_wing->colc = obj1->colc;
   star_wing->beta = beta;
   star_wing->rtrans = rtrans;
   star_wing->sigmap = sigmap;

   if(subtract_star(wingreg, star_wing, fparams, nsigma) < 0) {
      phWingobjectDel(star_wing);
      return(NULL);
   }

   obj1->flags2 |= OBJECT2_SUBTRACTED_WINGS;

   return(star_wing);
}

/*****************************************************************************/
/*
 * Calculate the amplitude of a power law fit to an OBJECT1, based on the radial profile
 * beyond the index iprofMinl
 */
static float
powerLaw_set_from_profile(const float *profile, /* Profile; e.g. OBJECT1.profMean */
			 const float *profileErr, /* errors in profile; e.g. OBJECT1.profErr */
			 int nprof,	/* number of points in profile; e.g. OBJECT1.nprof */
			 int iprofMin, /* first value of profMean[] to use  */
			 float beta /* the power law is r^{-beta}; if 0 use value in DGPSF */
    )
{
    float I0 = 0.0;			/* central amplitude of profile */
    const float *radii = phProfileGeometry()->radii; /* definitions of radii */

    shAssert(fabs(beta - 2.0) > 1e-4);	/* we'll be integrating r^{-beta} 2 pi r; beta == 2 => ln */

    {
	int i;
	double sum_pm = 0.0;		/* sum (profile * model)/sigma^2 */
	double sum_mm = 0.0;		/* sum (model   * model)/sigma^2 */
	for (i = iprofMin; i < nprof - 1; i++) {
	    const float r1 = radii[i];
	    const float r2 = radii[i + 1];
	    const float model = 2*(pow(r1, 2 - beta) - pow(r2, 2 - beta))/((beta - 2)*(r2*r2 - r1*r1));
	    const float sigma2 = pow(profileErr[i], 2);

	    if (sigma2 > 0) {
		sum_pm += profile[i]*model/sigma2;
		sum_mm +=   model*model/sigma2;
	    }
	}

	if (sum_mm == 0.0) {
	    shAssert(sum_pm == 0.0);
	    return 0.0;
	}
	I0 = sum_pm/sum_mm;		/* intensity of model */
    }

    return I0;
}

/*****************************************************************************/
/*
 * Calculate the amplitude of a power law fit to an OBJECT1, based on the radial profile
 * beyond the index iprofMinl
 */
float
phPowerLawSetFromObject1(const OBJECT1 *obj1, /* the object with a calculated profile */
			 int iprofMin, /* first value of profMean[] to use  */
			 float beta, /* the power law is r^{-beta}; if 0 use value in DGPSF */
			 int use_mean /* use mean, not median, profile XXX */
    )
{
    shAssert(obj1 != NULL);

    return powerLaw_set_from_profile((use_mean ? obj1->profMean : obj1->profMed),
				     obj1->profErr, obj1->nprof, iprofMin, beta);
}

float
phPowerLawSetFromCompositeProfile(const COMP_PROFILE *cprof, /* the composite profile */
                                  int iprofMin, /* first value of profMean[] to use  */
                                  float beta, /* the power law is r^{-beta}; if 0 use value in DGPSF */
                                  int use_mean   /* Use mean, not median, profile */
                        )
{
    shAssert(cprof != NULL);

    return powerLaw_set_from_profile((use_mean ? cprof->mean : cprof->med),
				     cprof->sig, cprof->size, iprofMin, beta);
}

/*****************************************************************************/
/*  
 * <AUTO EXTRACT>
 *
 * Here's the code to generate composite profiles of stars (the problem being
 * that stars bright enough to measure halo properties are saturated, so a
 * simple addition doesn't suffice).
 *
 * First call
 *    phCompositeProfileNew()
 * then
 *    phCompositeProfileAdd()
 * for each star that you want included in the composite. Finally, call
 *    phCompositeProfileFind()
 * to estimate a composite profile from the stars that you ...Added.
 */
COMP_PROFILE *
phCompositeProfileNew(void)
{
   int i;
   COMP_PROFILE *new = shMalloc(sizeof(COMP_PROFILE));

   new->psfCounts = -1;
   for(i = 0;i < NANN;i++) {
      new->mean[i] = new->med[i] = VALUE_IS_BAD;
      new->sig[i] = VALUE_IS_BAD;
   }
   new->size = 10;
   new->profs = shMalloc(new->size*sizeof(ONE_PROFILE));
   new->nprof = new->n = 0;
   new->prof_is_set = 0;

   return(new);
}


/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * copy the content of cprof2 to cprof1 COMP_PROFILE structure.
 *
 */
void
phCompositeProfileCopy(COMP_PROFILE *cprof1,   /* target cprof */  
                       COMP_PROFILE *cprof2)   /* source cprof */
{
   shAssert(cprof1 != NULL);
   shAssert(cprof2 != NULL);
   /* copy the content */
   shFree(cprof1->profs);
   *cprof1 = *cprof2;
   cprof1->profs = NULL;
   cprof1->size = 0;

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
 *
 * Add the profile in an OBJECT1 that's been through phMeasureObjc into
 * a pre-existing COMP_PROFILE.
 *
 * Return whether the profile really was added.
 */
int
phCompositeProfileAdd(COMP_PROFILE *cprof,       /* the composite profile */
                      const OBJECT1 *obj1,       /* the object in question */
		      const OBJMASK *satur_mask, /* mask for saturated pixels */
                      int   is_it_wing,          /* 1 for wing star, forces i0>0 */
		      float profmin)	         /* minimum profile value to consider */
{
   int i;
   int n;				/* == cprof->n */
   float prof;				/* point on the OBJECT1's profile */
   int rad_min_wing;                    /* (index of) minimum radius for wing stars */

   shAssert(obj1 != NULL && cprof != NULL);
   shAssert(satur_mask != NULL);

/*
 * See if we have room; if not, make some
 */
   n = cprof->n;
   if(n == cprof->size) {
      cprof->size = 1.4*cprof->size + 1;
      cprof->profs = shRealloc(cprof->profs,cprof->size*sizeof(ONE_PROFILE));
   }
/*
 * For those radial bins where the OBJECT1's profile is missing, or two
 * faint to be interesting, we'll fill it in based on the COMP_PROFILE
 * suitably scaled. First make a copy of the input profile/standard deviation
 */

   for(i = 0;i < obj1->nprof;i++) {
      prof = cprof->profs[n].mean[i] = obj1->profMean[i]; 
      prof = cprof->profs[n].med[i] = obj1->profMed[i];
      if(prof < profmin) {
	 break;
      }
      cprof->profs[n].sig[i] = obj1->profErr[i];
   }
   cprof->profs[n].nprof = i;
   cprof->profs[n].i0 = first_good_index(obj1,satur_mask);
   /*** We don't allow the first rad_min_wing points from the wing stars ***/
   /* This should be moved to psParam.par file XXX */
   rad_min_wing = 4;
   /** well, if the KL PSF doesn't go that far, then back off (PR 1358) **/
   if (cprof->profs[0].nprof <= rad_min_wing) {
       rad_min_wing = cprof->profs[0].nprof - 1;
   }
   if (cprof->profs[n].i0 < rad_min_wing && is_it_wing) {
       cprof->profs[n].i0 = rad_min_wing;
   }
   cprof->profs[n].psfCounts = obj1->psfCounts;
   cprof->profs[n].alpha = 0;

   if(cprof->profs[n].nprof > cprof->profs[n].i0 + 1) { /* new information */
      cprof->n++;
      return 1;
   } else {
      return 0;					/* <= 1 point on profile; useless */
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Given a COMP_PROFILE filled with stellar profiles, derive the composite
 * profile
 */
int
phCompositeProfileFind(COMP_PROFILE *cprof, /* the composite profile */
		       int medflg)	/* use the median profile? */
{
   float **mat;				/* matrix */
   int conn[NANN];			/* are points in the radial profile
					   connected by data?*/
   int i0;				/* starting index for profiles */
   int i;				/* index for radii */
   int j;				/* index for different profiles */
   int k;				/* generic index */
   int n;				/* == cprof->n */
   int nprof;				/* number of radial points */
   int nN;				/* == n + nprof */
   float *p;				/* vector of p_{ij} */
   float *profile;			/* the mean or median radial profile */
   float sum;
   float **w;				/* inverse-variances of profiles */

   shAssert(cprof != NULL);

   if((n = cprof->n) == 0) {
      shErrStackPush("phCompositeProfileFind: n = 0 in COMP_PROFILE");
      return(SH_GENERIC_ERROR);
   }

   /* determine the radial extent od the data */
   i0 = NANN; nprof = 0;
   for(j = 0;j < n;j++) {
      if(cprof->profs[j].nprof > nprof) {
	 nprof = cprof->profs[j].nprof;
      }
      if(cprof->profs[j].i0 < i0) {
	 i0 = cprof->profs[j].i0;
      }
   }

   if(i0 != 0) {
      shErrStackPush("phCompositeProfileFind: Profile doesn't extend to r==0");
      return(SH_GENERIC_ERROR);
   }
/*
 * ensure that the profiles overlap sufficiently to be able to estimate a
 * composite profile. conn[k] == 1 => annulus k is connected to annulus k + 1
 */
   for(i = 0;i < nprof;i++) {
      conn[i] = 0;
   }
   for(j = 0;j < n;j++) {
      for(i = cprof->profs[j].i0;i < cprof->profs[j].nprof - 1;i++) {
	 conn[i] = 1;
      }
   }

   for(i = 0;i < nprof - 1;i++) {
      if(!conn[i]) {
	 shErrStackPush("phCompositeProfileFind: "
			"annulus %d isn't connected to %d",i,i+1);
	 return(SH_GENERIC_ERROR);
      }
   }
/*
 * get the matrices and vector
 */
   nN = n + (nprof - 1);

   mat = alloca((nN + n)*sizeof(float *));
   {					/* alloca weirdness on SGIs needs tmp*/
      float *tmp = alloca((nN*nN + n*nprof)*sizeof(float));
      mat[0] = tmp;
   }
   w = mat + nN;
   w[0] = mat[0] + nN*nN;
   for(k = 0;k < nN;k++) {
      mat[k] = mat[0] + k*nN;
   }
   for(k = 0;k < n;k++) {
      w[k] = w[0] + k*nprof;
   }
   p = alloca(nN*sizeof(float));
/*
 * calculate the inverse-variances of log(prof[])
 */
   for(j = 0;j < n;j++) {
      profile = (medflg) ? cprof->profs[j].med : cprof->profs[j].mean;
      for(i = 0;i < cprof->profs[j].i0;i++) {
	 w[j][i] = 0;
      }
      for(;i < cprof->profs[j].nprof;i++) {
	 w[j][i] = pow(profile[i]/cprof->profs[j].sig[i],2);
      }
      for(;i < nprof;i++) {
	 w[j][i] = 0;
      }
   }
/*
 * and fill them. Note that we are _not_ estimating prof[0] (the first
 * element indexed by i), so don't include it in the matrix
 */
   for(i = 1;i < nprof;i++) {		/* top left block */
      memset(mat[i - 1],'\0',(nprof - 1)*sizeof(float));
      sum = 0;
      for(j = 0;j < n;j++) {
	 sum += w[j][i];
      }
      mat[i - 1][i - 1] = sum;
   }

   for(j = 0;j < n;j++) {		/* bottom right block */
      memset(&mat[nprof - 1 + j][nprof - 1],'\0',n*sizeof(float));
      sum = 0;
      for(i = 0;i < nprof;i++) {
	 sum += w[j][i];
      }
      mat[nprof - 1 + j][nprof - 1 + j] = sum;
   }

   for(i = 1;i < nprof;i++) {		/* upper right and lower left blocks */
      for(j = 0;j < n;j++) {
	 mat[nprof - 1 + j][i - 1] = mat[i - 1][nprof - 1 + j] = w[j][i];
      }
   }

   for(i = 1;i < nprof;i++) {		/* top of vector */
      sum = 0;
      for(j = 0;j < n;j++) {
	 if(w[j][i] > 0) {
	    profile = (medflg) ? cprof->profs[j].med : cprof->profs[j].mean;
	    sum += log(fabs(profile[i]))*w[j][i];
	 }
      }
      p[i - 1] = sum;
   }
   for(j = 0;j < n;j++) {		/* bottom of vector */
      profile = (medflg) ? cprof->profs[j].med : cprof->profs[j].mean;
      sum = 0;
      for(i = 0;i < nprof;i++) {
	 if(w[j][i] > 0) {
	    sum += log(fabs(profile[i]))*w[j][i];
	 }
      }
      p[nprof - 1 + j] = sum;
   }
/*
 * OK, invert the matrix and then solve for our estimates. We need the
 * inverse for error estimates, so we don't solve the matrix equation directly
 */
   if(phMatrixInvert(mat,NULL,nN) != 0) {
      shErrStackPush("phCompositeProfileFind: matrix is singular");
      return(SH_GENERIC_ERROR);
   }

   profile = (medflg) ? cprof->med : cprof->mean;
   cprof->nprof = nprof;
   profile[0] = 1; cprof->sig[0] = 0;
   for(i = 1;i < nprof;i++) {
      sum = 0;
      for(k = 0;k < nN;k++) {
	 sum += mat[i - 1][k]*p[k];
      }
      sum = exp(sum);
      profile[i] = sum;
      cprof->sig[i] = sqrt(mat[i - 1][i - 1])*sum;
   }

   for(j = 0;j < n;j++) {
      sum = 0;
      for(k = 0;k < nN;k++) {
	 sum += mat[nprof - 1 + j][k]*p[k];
      }
      cprof->profs[j].alpha = exp(sum);
   }
/*
 * estimate the PSF counts corresponding to a profile with alpha == 1
 */
   sum = cprof->psfCounts = 0;
   for(j = 0;j < n;j++) {
      if(cprof->profs[j].i0 == 0) {	/* not saturated */
	 sum += cprof->profs[j].alpha;
	 cprof->psfCounts += cprof->profs[j].psfCounts;
      }
   }
   if(sum != 0) {
      cprof->psfCounts /= sum;
   }
/*
 * and now the profile that we didn't explicitly scale (either the median
 * or mean, depending on medflg)
 */
   profile = (medflg == 0) ? cprof->med : cprof->mean;
   for(i = 0;i < nprof;i++) {
      profile[i] = sum = 0;
      for(j = 0;j < n;j++) {
	 if(i >= cprof->profs[j].i0 && i < cprof->profs[j].nprof) {
	    sum += cprof->profs[j].alpha; /* a valid point */
	    profile[i] += (medflg == 0) ?
			      cprof->profs[j].med[i] : cprof->profs[j].mean[i];
	 }
      }
      if(sum != 0) {
	 profile[i] /= sum;
      }
   }

   cprof->prof_is_set = 1;

   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Given an object's profile and a composite profile, calculate the psfCounts
 * for the object. This is done by matching the profiles (so the object can
 * perfectly well be saturated).
 *
 * We could use a technique similar to that used to derive the composite
 * profiles, but let us be lazy and assume that the errors in the composite
 * are small
 */
float
phPSFCountsGetFromProfile(const OBJECT1 *obj1, /* the object */
			  const COMP_PROFILE *cprof, /* the composite */
			  int i0,	/* and the first good radial point */
			  int medflg,	/* use median profile? */
			  float r_max)	/* use only profile to this radius */
{
   int i;
   int nprof;
   float sum_obj, sum_comp;		/* sums of obj1's and cprof's profile*/
   
   shAssert(obj1 != NULL && cprof != NULL && cprof->prof_is_set);
   shAssert(i0 >= 0);

   if(r == NULL) {			/* r's not yet looked up */
      r = phProfileGeometry()->radii;
      for(i = 0;i < NANN;i++) {
	 area[i] = M_PI*(pow(r[i+1],2) - pow(r[i],2));
      }
   }

   nprof = (cprof->nprof < obj1->nprof) ? cprof->nprof : obj1->nprof;
   if(r_max > 0) {			/* maximum radius to use */
      for(i = i0;i < nprof;i++) {
	 if(r[i + 1] > r_max) {
	    break;
	 }
      }
      if(i > i0) {			/* don't entirely trim overlap */
	 nprof = i;
      }
   }

   if(i0 >= nprof) {
      if(cprof->nprof > 0) {
	 shError("phPSFCountsGetFromProfile: "
		 "profiles have no points in common");
      }
      return(-5e10);
   }

   sum_comp = sum_obj = 0;
   for(i = i0;i < nprof;i++) {
      float isig2 = 1/pow(obj1->profErr[i],2); /* inverse variance */
      if(medflg) {
	 sum_obj += obj1->profMed[i]*isig2;
	 sum_comp += cprof->med[i]*isig2;
      } else {
	 sum_obj += obj1->profMean[i]*isig2;
	 sum_comp += cprof->mean[i]*isig2;
      }
   }
   shAssert(sum_comp != 0);

   return(cprof->psfCounts*sum_obj/sum_comp);
}
