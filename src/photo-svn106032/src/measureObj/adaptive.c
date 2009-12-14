/*
 * Measure adaptive moments.
 *
 * Originally provided by Phil Fischer, based on code from Tim McKay's
 * group.  Error calculations by Dave Johnston.  Major reworking by RHL
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "dervish.h"
#include "phMeasureObj.h"
#include "phObjectCenter.h"
#include "phMathUtils.h"		/* here so as not to re#def M_SQRT */
 
/*****************************************************************************/
/*
 * Decide on the bounding box for the region to examine while calculating
 * the adaptive moments
 */
static void
set_amom_bbox(int *ix1, int *ix2,	/* bounding box */
	      int *iy1, int *iy2,	/*              to consider */
	      int nrow, int ncol,	/* size of region */
	      float xcen, float ycen,	/* centre of object */
	      double sigma11_w,		/* quadratic moments of the */
	      double sigma12_w,		/*         weighting function; */
	      double sigma22_w,		/*                    xx, xy, and yy */
	      float petroRad)		/* object's Petrosian radius */
{
   float rad = 4.*sqrt(((sigma11_w > sigma22_w) ? sigma11_w : sigma22_w));

   shAssert(sigma12_w == sigma12_w);	/* pretend to use this argument */

   if(rad > 2*petroRad) {
      rad = 2*petroRad;
   }

   *ix1 = xcen - rad - 0.5;
   *ix1 = (*ix1 < 0) ? 0 : *ix1;
   *iy1 = ycen - rad - 0.5;
   *iy1 = (*iy1 < 0) ? 0 : *iy1;
     
   *ix2 = xcen + rad + 0.5;
   if(*ix2 >= ncol) {
     *ix2 = ncol - 1;
   }
   *iy2 = ycen + rad + 0.5;
   if(*iy2 >= nrow) {
     *iy2 = nrow - 1;
   }
}   

/*****************************************************************************/
/*
 * Calculate weighted moments of an object up to 2nd order
 */
static int
calcmom(const REGION *data,		/* the data */
	float xcen, float ycen,		/* centre of object */
	int ix1, int ix2, int iy1, int iy2, /* bounding box to consider */
	float bkgd,			/* data's background level */
	int interpflag,			/* interpolate within pixels? */
	double w11, double w12, double w22, /* weights */
	double *psum, double *psumx, double *psumy, /* sum w*I, sum [xy]*w*I */
	double *psumxx, double *psumxy, double *psumyy, /* sum [xy]^2*w*I */
	double *psums4)			/* sum w*I*weight^2 or NULL */
{   
   int i,j;
   PIX *row;				/* pointer to a row of data */
   float x, y, x2, xy, y2;		/* x, y, x^2, xy, y^2 */
   float xl,xh,yl,yh;
   float expon;
   float tmod,ymod;
   float X,Y;				/* sub-pixel interpolated [xy] */
   float weight;
   float tmp;
   double sum,sumx,sumy,sumxx,sumyy,sumxy,sums4;
#define RECALC_W 0			/* estimate sigmaXX_w within BBox? */
#if RECALC_W
   double wsum, wsumxx, wsumxy, wsumyy;

   wsum = wsumxx = wsumxy = wsumyy = 0;
#endif

   shAssert(w11 >= 0);			/* i.e. it was set */
   if(fabs(w11) > 1e6 || fabs(w12) > 1e6 || fabs(w22) > 1e6) {
      return(-1);
   }

   sum = sumx = sumy = sumxx = sumxy = sumyy = sums4 = 0;
   
   for(i = iy1; i <= iy2; i++) {
      row = data->ROWS[i];
      y = i-ycen;
      y2 = y*y;
      yl = y - 0.375;
      yh = y + 0.375;
      for(j = ix1;j <= ix2; j++) {
	 x = j - xcen;
	 if(interpflag) {
	    xl = x - 0.375;
	    xh = x + 0.375;
	    
	    expon = xl*xl*w11 + yl*yl*w22 + 2.*xl*yl*w12;
	    tmp = xh*xh*w11 + yh*yh*w22 + 2.*xh*yh*w12;
	    expon = (expon > tmp) ? expon : tmp;
	    tmp = xl*xl*w11 + yh*yh*w22 + 2.*xl*yh*w12;
	    expon = (expon > tmp) ? expon : tmp;
	    tmp = xh*xh*w11 + yl*yl*w22 + 2.*xh*yl*w12;
	    expon = (expon > tmp) ? expon : tmp;

	    if(expon <= 9.0) {
	       tmod = row[j] - bkgd;
	       for(Y = yl;Y <= yh;Y+=0.25) {
		  y2 = Y*Y;
		  for(X = xl;X <= xh;X+=0.25) {
		     x2 = X*X;
		     xy = X*Y;
		     expon = x2*w11 + 2*xy*w12 + y2*w22;
		     weight = exp(-0.5*expon);

		     ymod = tmod*weight;
		     sum += ymod;
		     sumx += ymod*(X+xcen);
		     sumy += ymod*(Y+ycen);
#if RECALC_W
		     wsum += weight;

		     tmp = x2*weight;
		     wsumxx += tmp;
		     sumxx += tmod*tmp;
		     
		     tmp = xy*weight;
		     wsumxy += tmp;
		     sumxy += tmod*tmp;
		     
		     tmp = y2*weight;
		     wsumyy += tmp;
		     sumyy += tmod*tmp;
#else
		     sumxx += x2*ymod;
		     sumxy += xy*ymod;
		     sumyy += y2*ymod;
#endif
		     sums4 += expon*expon*ymod;
		  }
	       }
	    }
	 } else {
	    x2 = x*x;
	    xy = x*y;
	    expon = x2*w11 + 2*xy*w12 + y2*w22;

	    if(expon <= 9.0) {
	       weight = exp(-0.5*expon);
	       tmod = row[j] - bkgd;
	       ymod = tmod*weight;
	       sum += ymod;
	       sumx += ymod*j;
	       sumy += ymod*i;
#if RECALC_W
	       wsum += weight;

	       tmp = x2*weight;
	       wsumxx += tmp;
	       sumxx += tmod*tmp;
	       
	       tmp = xy*weight;
	       wsumxy += tmp;
	       sumxy += tmod*tmp;
	       
	       tmp = y2*weight;
	       wsumyy += tmp;
	       sumyy += tmod*tmp;
#else
	       sumxx += x2*ymod;
	       sumxy += xy*ymod;
	       sumyy += y2*ymod;
#endif
	       sums4 += expon*expon*ymod;
	    }
	 }
      }
   }

   *psum = sum; *psumx = sumx; *psumy = sumy;
   *psumxx = sumxx; *psumxy = sumxy; *psumyy = sumyy;
   if(psums4 != NULL) { *psums4 = sums4; }

#if RECALC_W
   if(wsum > 0) {
      double det = w11*w22 - w12*w12;
      wsumxx /= wsum; wsumxy /= wsum; wsumyy /= wsum;
      printf("%g %g %g  %g %g %g\n", w22/det, -w12/det, w11/det,
	     wsumxx, wsumxy, wsumyy);
   }
#endif

   return((sum > 0) ? 0 : -1);
}

#define MAXIT 100
#define XINTERP 0.25			/* I.e. 0.5*0.5 */
#define XINTERP2 (XINTERP*XINTERP)
#define TOL1 0.001
#define TOL2 0.01

/*
 * Workhorse for adaptive moments
 *
 * Note: this routine assumes that the _centre_ of a pixel is (0.0, 0.0)
 * which is not the SDSS convention. Caveat Lector.
 */
static int
get_moments(const REGION *data,		/* the data to process */
	    float bkgd,			/* background level */
	    float petroRad,		/* object's Petrosian radius */
	    float xcen, float ycen,	/* centre of object */
	    float shiftmax,		/* max allowed centroid shift */
	    float *A,			/* amplitude of best-fit Gaussian;
					   or NULL */
	    double *Mxx, double *Mxy, double *Myy, /* quadratic moments */
	    double *Mxy4,		/* higher order moment used for
					   PSF correction */
	    int *flags2)		/* value of flags2 */
{
   float amp_w = 0;			/* amplitude of best-fit Gaussian */
   float d;				/* weighted size of object */
   double det_w;			/* determinant of sigmaXX_w matrix */
   float e1, e2;			/* current and old values of */
   float e1_old, e2_old;		/*        shape parameters e1 and e2 */
   int iter;                            /* iteration number */
   int interpflag;			/* interpolate finer than a pixel? */
   int ix1,ix2,iy1,iy2;                 /* corners of the box being analyzed */
   float sigma11_ow_old;		/* previous version of sigma11_ow */
   double sum;				/* sum of intensity*weight */
   double sumx, sumy;			/* sum ((int)[xy])*intensity*weight */
   double sumxx, sumxy, sumyy;		/* sum {x^2,xy,y^2}*intensity*weight */
   double sums4;			/* sum intensity*weight*exponent^2 */
   float sigma11_ow, sigma12_ow,	/* quadratic moments of */
	 sigma22_ow;			/*     weight*object; xx, xy, and yy */
   double sigma11_w, sigma12_w,		/* quadratic moments of the */
          sigma22_w;			/*     weighting fcn; xx, xy, and yy */
   double w11, w12, w22;		/* current weights for moments */
   const float xcen0 = xcen;		/* initial centre */
   const float ycen0 = ycen;		/*                of object */

   sigma11_w = sigma22_w = 1.5;
   sigma12_w = 0;

   w11 = w12 = w22 = -1;		/* always set when iter == 0 */
   e1_old = e2_old = sigma11_ow_old = 1.e6;
   
   interpflag = 0;
   for(iter = 0; iter < MAXIT; iter++) {
      set_amom_bbox(&ix1, &ix2, &iy1, &iy2, data->nrow, data->ncol, xcen, ycen,
		    sigma11_w, sigma12_w, sigma22_w, petroRad);

      det_w = sigma11_w*sigma22_w - sigma12_w*sigma12_w;
#if 0					/* this form is numerically unstable
					   on my G4 powerbook */
      shAssert(det_w >= 0.0);
#else
      shAssert(sigma11_w*sigma22_w >= sigma12_w*sigma12_w - EPSILON_f);
#endif
      if(det_w < EPSILON_f) {		/* a suitably small number */
	 *flags2 |= OBJECT2_AMOMENT_UNWEIGHTED;
	 break;
      }

      {
	 const double ow11 = w11;	/* old */
	 const double ow12 = w12;	/*     values */
	 const double ow22 = w22;	/*            of w11, w12, w22 */

	 w11 = sigma22_w/det_w; w12 = -sigma12_w/det_w; w22 = sigma11_w/det_w;

	 if(sigma11_w < XINTERP || sigma22_w < XINTERP || det_w < XINTERP2) {
	    if(interpflag == 0) {
	       interpflag = 1;		/* N.b.: stays set for this object */
	       if(iter > 0) {
		  sigma11_ow_old = 1.e6; /* force at least one more iteration*/
		  w11 = ow11; w12 = ow12; w22 = ow22;
		  iter--;		/* we didn't update wXX */
	       }
	    }
	 }
      }

      if(calcmom(data, xcen, ycen, ix1, ix2, iy1, iy2, bkgd, interpflag,
		 w11, w12, w22,
		 &sum, &sumx, &sumy, &sumxx, &sumxy, &sumyy, &sums4) < 0) {
	 *flags2 |= OBJECT2_AMOMENT_UNWEIGHTED;
	 break;
      }

      amp_w = sum/(M_PI*sqrt(det_w));
#if 0
/*
 * Find new centre
 *
 * This is only needed if we update the centre; if we use the
 * obj1->{row,col}c we've already done the work
*/
      xcen = sumx/sum;
      ycen = sumy/sum;
#endif
      if(fabs(sumx/sum - (xcen0 - 0.5)) > shiftmax || 
	 fabs(sumy/sum - (ycen0 - 0.5)) > shiftmax) {
	 *Mxx = sumx/sum - (xcen0 - 0.5);
	 *Myy = sumy/sum - (ycen0 - 0.5);
	 *flags2 |= OBJECT2_AMOMENT_SHIFT;
	 return(-1);
      }
/*
 * OK, we have the centre. Proceed to find the second moments.
 */
      sigma11_ow = sumxx/sum;
      sigma22_ow = sumyy/sum;
      sigma12_ow = sumxy/sum;

      if(sigma11_ow <= 0 || sigma22_ow <= 0) {
	 *flags2 |= OBJECT2_AMOMENT_UNWEIGHTED;
	 break;
      }

      d = sigma11_ow + sigma22_ow;
      e1 = (sigma11_ow - sigma22_ow)/d;
      e2 = 2.*sigma12_ow/d;
/*
 * Did we converge?
 */
      if(iter > 0 &&
	 fabs(e1 - e1_old) < TOL1 && fabs(e2 - e2_old) < TOL1 &&
				fabs(sigma11_ow/sigma11_ow_old - 1.) < TOL2 ) {
	 break;				/* yes; we converged */
      }

      e1_old = e1; e2_old = e2; sigma11_ow_old = sigma11_ow;
/*
 * Didn't converge, calculate new values for weighting function
 *
 * The product of two Gaussians is a Gaussian:
 * <x^2 exp(-a x^2 - 2bxy - cy^2) exp(-Ax^2 - 2Bxy - Cy^2)> = 
 *                            <x^2 exp(-(a + A) x^2 - 2(b + B)xy - (c + C)y^2)>
 * i.e. the inverses of the covariances matrices add.
 *
 * We know sigmaXX_ow and sigmaXX_w, the covariances of the weighted object
 * and of the weights themselves.  We can estimate the object's covariance as
 *   sigmaXX_ow^-1 - sigmaXX_w^-1
 * and, as we want to find a set of weights with the _same_ covariance as the
 * object we take this to be the an estimate of our correct weights.
 *
 * N.b. This assumes that the object is roughly Gaussian.
 * Consider the object:
 *   O == delta(x + p) + delta(x - p)
 * the covariance of the weighted object is equal to that of the unweighted
 * object, and this prescription fails badly.  If we detect this, we set
 * the OBJECT2_AMOMENT_UNWEIGHTED bit, and calculate the UNweighted moments
 * instead.
 */
      {
	 double det_n;			/* determinant of nXX matrix */
	 double det_ow;			/* determinant of sigmaXX_ow matrix */
	 float n11, n12, n22;		/* elements of inverse of
					   next guess at weighting function */
	 float ow11, ow12, ow22;	/* elements of inverse of sigmaXX_ow */

	 det_ow = sigma11_ow*sigma22_ow - sigma12_ow*sigma12_ow;

	 if(det_ow <= 0) {
	    *flags2 |= OBJECT2_AMOMENT_UNWEIGHTED;
	    break;
	 }
	 
	 ow11 =  sigma22_ow/det_ow;
	 ow12 = -sigma12_ow/det_ow;
	 ow22 =  sigma11_ow/det_ow;

	 n11 = ow11 - w11;
	 n12 = ow12 - w12;
	 n22 = ow22 - w22;
	 det_n = n11*n22 - n12*n12;

	 if(det_n <= 0) {		/* product-of-Gaussians
							   assumption failed */
	    *flags2 |= OBJECT2_AMOMENT_UNWEIGHTED;
	    break;
	 }
      
	 sigma11_w = n22/det_n;
	 sigma12_w = -n12/det_n;
	 sigma22_w = n11/det_n;
      }

      if(sigma11_w <= 0 || sigma22_w <= 0) {
	 *flags2 |= OBJECT2_AMOMENT_UNWEIGHTED;
	 break;
      }
   }

   if(iter == MAXIT) {
      *flags2 |= OBJECT2_AMOMENT_MAXITER | OBJECT2_AMOMENT_UNWEIGHTED;
   }

   if(sumxx + sumyy == 0.0) {
      *flags2 |= OBJECT2_AMOMENT_UNWEIGHTED;
   }
/*
 * Problems; try calculating the un-weighted moments
 */
   if(*flags2 & OBJECT2_AMOMENT_UNWEIGHTED) {
      w11 = w22 = w12 = 0;
      if(calcmom(data, xcen, ycen, ix1, ix2, iy1, iy2, bkgd, interpflag,
		 w11, w12, w22,
		 &sum, &sumx, &sumy, &sumxx, &sumxy, &sumyy, NULL) < 0 ||
								    sum <= 0) {
	 return(-1);
      }

      sigma11_w = sumxx/sum;		/* estimate of object moments */
      sigma12_w = sumxy/sum;		/*   usually, object == weight */
      sigma22_w = sumyy/sum;		/*      at this point */
   }

   if(A != NULL) { *A = amp_w; }
   *Mxx = sigma11_w;
   *Mxy = sigma12_w;
   *Myy = sigma22_w;
   *Mxy4 = sums4/sum;

   return(0);
}

/*****************************************************************************/
/*
 * Error analysis, courtesy of David Johnston, University of Chicago
 */
/*
 * This function takes the 4 Gaussian parameters A, sigmaXX_w and the
 * sky variance and fills in the Fisher matrix from the least squares fit.
 *
 * Following "Numerical Recipes in C" section 15.5, it ignores the 2nd
 * derivative parts and so the fisher matrix is just a function of these
 * best fit model parameters. The components are calculated analytically.
 */
MAT *
calc_fisher(float A,			/* amplitude */
	    float sigma11_w,
	    float sigma12_w,
	    float sigma22_w,
	    float bkgd_var,
	    MAT *fisher)		/* desired fisher matrix; or NULL */
{
   double D, F, fac;
   
   D = sigma11_w*sigma22_w - sigma12_w*sigma12_w;
   
   if(D <= EPSILON) {
      return(NULL);
   }
/*
 * Create Fisher matrix if necessary
 */
   if(fisher == NULL) {
      fisher = phMatNew(4, 4);
   }
   shAssert(fisher->n == 4 && fisher->m == 4);
/*
 * a normalization factor
 */
   shAssert(bkgd_var > 0.0);
   F = M_PI*sqrt(D)/bkgd_var;
/*
 * Calculate the 10 independent elements of the 4x4 Fisher matrix 
 */
   fac = F*A/(4.0*D);
   fisher->me[0][0] =  F;
   fisher->me[0][1] =  fac*sigma22_w;
   fisher->me[1][0] =  fisher->me[0][1];
   fisher->me[0][2] =  fac*sigma11_w;			
   fisher->me[2][0] =  fisher->me[0][2];
   fisher->me[0][3] = -fac*2*sigma12_w;	
   fisher->me[3][0] =  fisher->me[0][3];
   
   fac = 3.0*F*A*A/(16.0*D*D);
   fisher->me[1][1] =  fac*sigma22_w*sigma22_w;
   fisher->me[2][2] =  fac*sigma11_w*sigma11_w;
   fisher->me[3][3] =  fac*4.0*(sigma12_w*sigma12_w + D/3.0);
   
   fisher->me[1][2] =  fisher->me[3][3]/4.0;
   fisher->me[2][1] =  fisher->me[1][2];
   fisher->me[1][3] =  fac*(-2*sigma22_w*sigma12_w);
   fisher->me[3][1] =  fisher->me[1][3];
   fisher->me[2][3] =  fac*(-2*sigma11_w*sigma12_w);
   fisher->me[3][2] =  fisher->me[2][3];
/*
 * We now have a Fisher matrix.
 * Should check to see if it is positive definite. Ignore for now 
 * or let Robert do this. It could return NULL if it is not
 */
   return(fisher);
}

/*****************************************************************************/
/*
 * This function computes the covariance of the ellipticities e1 and e2,
 * and the standard deviation of the trace T == sigma11_w + sigma22_w.
 */
int
calc_M_errors(float bkgd_var,		/* background per-pixel variance */
	      float A,			/* amplitude of best-fit Gaussian */
	      float sigma11_w,		/* quadratic moments of the */
	      float sigma12_w,		/*         weighting function; */
	      float sigma22_w,		/*                    xx, xy, and yy */
	      float *Var_e1e1,		/* Errors.  Var(e1) */
	      float *Var_e1e2,		/*          Covar(e1 e2) */
	      float *Var_e2e2,		/*          Var(e2) */
	      float *TErr)		/*          s.d.(T) */
{
   MAT *covar;				/* covariance matrix */
   float T;				/* sigma??'s trace */
   MAT *fisher;				/* Fisher matrix for A, sigmaXX_w */
   int i;
   VEC *lambda;				/* eigen values */
   MAT *Q;				/* eigen vectors */
   float T4;				/* == T^4 */
/*
 * Now for errors. First calculate the Fisher matrix
 */
   fisher = calc_fisher(A, sigma11_w, sigma12_w, sigma22_w, bkgd_var, NULL);
   
   if(fisher == NULL) {			/* no covariances are available */
      return(-1);
   }
/*
 * invert it to get covariance.
 */
   Q = phMatNew(fisher->n, fisher->m);
   lambda = phEigen(fisher, Q, NULL);

   shAssert(Q == Q &&			/* stop compilers from whining */
	    sizeof(lambda->ve[0]) == sizeof(double));
   for(i = 0;i < fisher->n;i++) {
      if(fabs(lambda->ve[i]) < EPSILON) { /* we know lambda->ve[]'s double */
	 shError("Singular covariance matrix");
	 
	 lambda->ve[i] = 0;
	 phMatDel(Q); phVecDel(lambda);
	 phMatDel(fisher);

	 return(-1);
      } else {
	 lambda->ve[i] = 1/lambda->ve[i];
      }
   }
   covar = phEigenInvert(NULL, Q, lambda);

   phMatDel(Q); phVecDel(lambda);
   phMatDel(fisher);
/*
 * And calculate covariance/errors in the interesting quantities
 */
   T  = (sigma11_w + sigma22_w);
   shAssert(T > 0);
   T4 = 4/(T*T*T*T);			/* for our convenience */

   *Var_e1e1 = T4*(covar->me[1][1]*sigma22_w*sigma22_w +
		   covar->me[2][2]*sigma11_w*sigma11_w -
		   2.0*sigma11_w*sigma22_w*covar->me[1][2]);
   *Var_e1e2 = T4*(-1.0*sigma22_w*sigma12_w*covar->me[1][1] +
		   sigma11_w*sigma12_w*covar->me[2][2] +
		   (sigma11_w - sigma22_w)*sigma12_w*covar->me[1][2] +
		   T*(sigma22_w*covar->me[1][3] -sigma11_w*covar->me[2][3]));
   *Var_e2e2 = T4*(sigma12_w*sigma12_w*(covar->me[1][1] + covar->me[2][2] +
					2.0*covar->me[1][2]) -
		   2.0*T*sigma12_w*(covar->me[1][3] + covar->me[2][3]) +
		   T*T*covar->me[3][3]);
   *TErr = covar->me[1][1] + covar->me[2][2];
/*
 * We report all variances as "Err"; i.e. standard deviations not variances
 */
   *Var_e1e1 = sqrt(*Var_e1e1);
   *Var_e1e2 = (*Var_e1e2 < 0) ? -sqrt(-*Var_e1e2) : sqrt(*Var_e1e2);
   *Var_e2e2 = sqrt(*Var_e2e2);
   *TErr = sqrt(*TErr);
/*
 * clean up
 */
   phMatDel(covar);

   return(0);
}

/*****************************************************************************/
/*
 * Actually calculate the adaptive moments and errors
 */
void
phAdaptiveMomentsCalc(OBJC *objc,
		      int color,
		      const FIELDPARAMS *fiparams )
{
   float amp_w = 0;			/* amplitude of best-fit Gaussian */
   const float bkgd = (FLOATING_PHOTO ? 0 : SOFT_BIAS) +
						   fiparams->frame[color].bkgd;
   float bkgd_var;			/* background per-pixel variance */
   const REGION *data = fiparams->frame[color].data;
   int flags2;				/* flags2 set by get_moments */
   OBJECT1 *obj1 = objc->color[color];
   REGION *psf_reg;			/* the KL-reconstructed PSF */
   int ret;				/* a return code */
   float shiftmax;                      /* max allowed centroid shift */
   double sums4;			/* higher order moment used for
					   PSF correction */
   double sigma11_w,sigma22_w,sigma12_w; /* moments of the weighting fcn */
   float xcen, ycen;			/* centre of object */

   shiftmax = 2*obj1->petroRad;
   if(shiftmax < 2) {
      shiftmax = 2;
   } else if(shiftmax > 10) {
      shiftmax = 10;
   }

   {
      float sky = obj1->sky;
      if(sky < fiparams->frame[color].global_sky) {
	 sky = fiparams->frame[color].global_sky;
      }
      
      bkgd_var = sky/phGain(&fiparams->frame[color], obj1->rowc, obj1->colc) +
	   phDarkVariance(&fiparams->frame[color], obj1->rowc, obj1->colc);
   }

   xcen = obj1->colc - 0.5;
   ycen = obj1->rowc - 0.5;

   sigma11_w = sigma22_w = obj1->petroRad*obj1->petroRad; sigma12_w = 0;
   flags2 = 0;
   ret = get_moments(data, bkgd, obj1->petroRad, xcen, ycen, shiftmax,
		     &amp_w, &sigma11_w, &sigma12_w, &sigma22_w,
		     &sums4, &flags2);
   obj1->flags2 |= flags2;
/*
 * We need to measure the PSF's moments even if we failed on the object
 */
   if(ret < 0) {
      ;					/* failed */
   } else if(flags2 & OBJECT2_AMOMENT_SHIFT) {
      obj1->M_e1 = sigma22_w;		/* save the centroid shifts in row */
      obj1->M_e2 = sigma11_w;		/*       and column (11 == column) */
   } else {
      if(sigma11_w + sigma22_w != 0.0) {
	 obj1->M_e1 = (sigma11_w - sigma22_w)/(sigma11_w + sigma22_w);
	 obj1->M_e2 = 2*sigma12_w/(sigma11_w + sigma22_w);
	 obj1->M_rr_cc = sigma11_w + sigma22_w;

	 obj1->M_cr4 = sums4;
      
	 if((obj1->flags2 & OBJECT2_AMOMENT_UNWEIGHTED) ||
	    calc_M_errors(bkgd_var, amp_w, sigma11_w, sigma12_w, sigma22_w,
			  &obj1->M_e1e1Err, &obj1->M_e1e2Err, &obj1->M_e2e2Err,
			  &obj1->M_rr_ccErr) < 0) {
	    obj1->M_e1e1Err = obj1->M_e1e2Err = obj1->M_e2e2Err =
					       obj1->M_rr_ccErr = ERROR_IS_BAD;
	 }
      }
   }      
/*
 * Now determine moments of PSF
 */
   if(fiparams->frame[color].psfBasis == NULL) {
      shError("No PSF_BASIS for band %d", color);
      return;				/* XXX needs a diagnostic bit? */
   }
   
   shAssert(KLPsf[color] != NULL);
   psf_reg = KLPsf[color]->reg;

   ret = phObjectCenterFind(psf_reg, psf_reg->ncol/2, psf_reg->nrow/2,
			    0.0, fiparams->frame[color].smooth_sigma,
			    0.0, 0.0, 1,
			    &xcen, &ycen, NULL, NULL, NULL, NULL, NULL);
   if(ret != 0) {			/* use peak pixel */
      xcen = psf_reg->ncol/2 + 0.5;
      ycen =  psf_reg->nrow/2 + 0.5;
   }
   xcen -= 0.5; ycen -= 0.5;
#if 0
   xcen = psf_reg->ncol/2 - 0.5;
   ycen = psf_reg->nrow/2 - 0.5;
#endif

   flags2 = 0;
   if(get_moments(psf_reg, (FLOATING_PHOTO ? 0 : SOFT_BIAS),
		  20, xcen, ycen, shiftmax, NULL,
		  &sigma11_w, &sigma12_w, &sigma22_w, &sums4, &flags2) < 0) {
#if 0
      shRegWriteAsFits((REGION *)psf_reg,
			  "bad-psf.fit", STANDARD, 2, DEF_NONE, NULL, 0);
#endif
      shFatal("Failed to measure shape of PSF "
	      "for object at r,c = %.1f,%.1f, band %d: 0x%08x",
	      objc->rowc, objc->colc, color, flags2);
   }
#if 0	
   if(flags2 & OBJECT2_AMOMENT_UNWEIGHTED) {
      obj1->flags2 |= OBJECT2_AMOMENT_UNWEIGHTED_PSF; /* flag doesn't exist */
   }
#endif

   obj1->M_e1_psf = (sigma11_w - sigma22_w)/(sigma11_w + sigma22_w);
   obj1->M_e2_psf = 2*sigma12_w/(sigma11_w + sigma22_w);
   obj1->M_rr_cc_psf = sigma11_w + sigma22_w;

   obj1->M_cr4_psf = sums4;
}

/******************************************************************************/
/*
 * Interface callable from tcl
 */
int
phAdaptiveMomentsGet(const REGION *data, /* region wherein lies the object */
		     float rowc, float colc, /* position of centre */
		     float rad,		/* initial radius for moments */
		     float bkgd,	/* non-subtracted background level */
		     float bkgd_var,	/* background per-pixel variance */
		     float shiftmax,	/* max allowed centroid shift */
		     float *M_e1,	/* desired moments */
		     float *M_e2,
		     float *M_rr_cc,
		     float *M_cr4,	/* 4-th order moment */
		     float *M_e1e1Err,	/* errors in desired moments */
		     float *M_e1e2Err,
		     float *M_e2e2Err,
		     float *M_rr_ccErr,
		     float *amp_w)	/* amplitude of Gaussian */
{
   int flags2;				/* flags2 set by get_moments */
   int ret;				/* a return code */
   double sums4;			/* higher order moment used for
					   PSF correction */
   double sigma11_w,sigma22_w,sigma12_w; /* moments of the weighting fcn */
   const float xcen = colc - 0.5;	/* centre of object; */
   const float ycen = rowc - 0.5;	/*     n.b. violates SDSS convention */
   float dummy[9];			/* Used if any inputs are NULL */
/*
 * Deal with NULL parameters
 */
   ret = 0;
   if(amp_w == NULL) amp_w = &dummy[ret++];
   if(M_e1 == NULL) M_e1 = &dummy[ret++];
   if(M_e2 == NULL) M_e2 = &dummy[ret++];
   if(M_rr_cc == NULL) M_rr_cc = &dummy[ret++];
   if(M_cr4 == NULL) M_cr4 = &dummy[ret++];
   if(M_e1e1Err == NULL) M_e1e1Err = &dummy[ret++];
   if(M_e1e2Err == NULL) M_e1e2Err = &dummy[ret++];
   if(M_e2e2Err == NULL) M_e2e2Err = &dummy[ret++];
   if(M_rr_ccErr == NULL) M_rr_ccErr = &dummy[ret++];
   shAssert(ret <= sizeof(dummy)/sizeof(float));

   sigma11_w = sigma22_w = rad*rad; sigma12_w = 0;
   flags2 = 0;
   *amp_w = 0;
   ret = get_moments(data, bkgd, rad, xcen, ycen, shiftmax, amp_w,
		     &sigma11_w, &sigma12_w, &sigma22_w, &sums4, &flags2);
   if(ret < 0) {
      ;					/* failed */
   } else if(flags2 & OBJECT2_AMOMENT_SHIFT) {
      *M_e1 = sigma22_w;		/* save the centroid shifts in row */
      *M_e2 = sigma11_w;		/*       and column (11 == column) */
   } else {
      if(sigma11_w + sigma22_w != 0.0) {
	 *M_e1 = (sigma11_w - sigma22_w)/(sigma11_w + sigma22_w);
	 *M_e2 = 2*sigma12_w/(sigma11_w + sigma22_w);
	 *M_rr_cc = sigma11_w + sigma22_w;

	 *M_cr4 = sums4;
      
	 if((flags2 & OBJECT2_AMOMENT_UNWEIGHTED) ||
	    calc_M_errors(bkgd_var, *amp_w, sigma11_w, sigma12_w, sigma22_w,
			  M_e1e1Err, M_e1e2Err, M_e2e2Err,
			  M_rr_ccErr) < 0) {
	    *M_e1e1Err = *M_e1e2Err = *M_e2e2Err =
			       *M_rr_ccErr = ERROR_IS_BAD;
	 }
      }
   }      
   
   return(flags2);
}

