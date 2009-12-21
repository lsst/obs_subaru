/*
 * Code to centroid stars for astrometry, using the 2-D gaussian least-
 * squares technique, implemented by finding the maximum in the image
 * smoothed with a gaussian by quartic interpolation.
 * 
 * jeg 960204
 *
 * !!!!!!!!!!!!!!!!!!!!!! NB !!!!!!!!!!!!!!!!!!!!!
 * These routines work in the proper floating coordinate system,
 * in which if a star is CENTERED in pixel i,j it has coordinates
 * (i.0000,j.0000). This is NOT the system adopted by the SDSS, for
 * reasons I will never understand. YOU MUST ADD 0.5 to the float
 * coordinates returned by ast_findcen to comply with the survey
 * conventions.
 */
#include <math.h>
#include <dervish.h>
#include "atConversions.h"		/* for M_PI */
#include "phConsts.h"
#include "phObjectCenter.h"		/* prototype for phObjectCenterFind */
#define CHECK_PHOTO_CENTROID 0
#if CHECK_PHOTO_CENTROID
#  include "atCentroid.h"		/* prototypes for atCentroid */
#endif

/* 
 * DXF and DYF are the offsets needed to go from natural floating coordinates
 * in which an object CENTERED in pixel i j has floating coordinates
 * i.0000 j.0000 to the insane ones adopted by the SDSS, in which an
 * object centered on the LL corner of a pixel (and therefore has its light
 * equally shared with pixels i-1,j i-1,j-1 and i,j-1) has zero fractional 
 * part.
 */
#define DXF  0.5
#define DYF  0.5
                
/* 
 * The gaussian filter is evaluated explicitly for speed, to minimize the
 * number of multiplications. The core of the filter is all that is
 * used for the width sigma of the smoothing gaussian small enough; successive
 * rings are added for larger sigma. In general, the filter is cut off at
 * r = 3.5*sigma. We have variables enough at this cutoff only for 
 * sigma <= 2.2, which corresponds
 * to about 2 arcsecond seeing for the SDSS. One could add rings, but
 * one has to ask why one is attempting to do astrometry under such
 * conditions. In any case, for larger sigma the code still works, but
 * the cutoff is no longer at 3.5*sigma, and cleary for bad enough
 * seeing will cease to work at all.
 */

/*
 * # of 'rings'-- break points are sigma = 1, 1.2, 1.5, 1.8, 2.2 
 * corresp # of multiplies to eval filter= 8,  11,  16,  22,  29
 */
static int sig_gnring; 

static int g00,g01,g11,g20,g21,g30,g31,g22,   /* ring1,the core, for sig <= 1*/
           g40,g41,g32,                       /* ring2, for sigma <= 1.2 */
           g50,g51,g42,g43,g33,               /* ring3, for sigma <= 1.5 */
           g60,g61,g62,g52,g53,g44,           /* ring4, for sigma <= 1.8 */
           g70,g71,g72,g63,g64,g54,g55;       /* ring5, for sigma <= 2.2 */

static double sigmaring = -1;		/* sigma for these ring arrays */
static int sig_ncring = 0;		/* 1 + outer radius of ring array; 
					   sig_gnring + 3 */

/*****************************************************************************/
/*
 * This function evaluates exp(-s*(x*x+y*y)) for r = s*(x*x+y*y)<6.125,
 * scales it by 512 and returns its truncated integer value; it returns
 * zero for r > 6.125. s is, for a conventionally represented gaussian,
 * 1/(2*sigma^2), so r < 6.125 corresponds to sqrt(x^2 + y^2) < 3.5*sigma.
 */
static int
gauring(int x,int y,double s)   /* s is 1/(2*sigma^2) */
{
    double r = ((double)(x*x+y*y))*s;
    if(r > 6.125) return 0;   /* 6.125 = 3.5*3.5/2 */
    return (int)(512.0 * (exp(-r)));
}

/*****************************************************************************/
/*
 * this function constructs the array of static filter coefficients; in the
 * DA code this should perhaps be done with a table look-up if all the
 * evaluations of gaussians is too time-consuming; tables for sigma=0.8 to
 * sigma=2.5 or so at intervals of 0.1 should be enough; that is 18 sets of
 * 29 ints or shorts, certainly not an excessive amount of storage.
 */
static void 
ast_makegnring(double sigma)
{
    double s = 1./(2.*sigma*sigma);
    sig_gnring = 1;

    g00 = gauring(0,0,s);
    g01 = gauring(0,1,s);
    g11 = gauring(1,1,s);
    g20 = gauring(2,0,s);
    g21 = gauring(2,1,s);
    g22 = gauring(2,2,s);
    g30 = gauring(3,0,s);
    g31 = gauring(3,1,s);
    
    if(sigma > 1.){
        sig_gnring = 2;
        
        g40 = gauring(4,0,s);        
        g41 = gauring(4,1,s);        
        g32 = gauring(3,2,s);
        if((g40 | g41 | g32) == 0) sig_gnring = 1;        
        
        if(sigma > 1.2 && sig_gnring == 2){ 
            sig_gnring = 3;
            
            g50 = gauring(5,0,s);
            g51 = gauring(5,1,s);
            g42 = gauring(4,2,s);
            g43 = gauring(4,3,s);
            g33 = gauring(3,3,s);
            if((g50 | g51 | g42 | g43 | g33) == 0) sig_gnring = 2;
           
            if(sigma > 1.5 && sig_gnring == 3){
                sig_gnring = 4;
               
                g60 = gauring(6,0,s);               
                g61 = gauring(6,1,s);
                g62 = gauring(6,2,s);
                g52 = gauring(5,2,s);
                g53 = gauring(5,3,s);
                g44 = gauring(4,4,s);
                if((g60 | g61 | g62 | g52 | g53 | g44) == 0) sig_gnring = 3;
               
                if(sigma > 1.8 && sig_gnring == 4){
                    sig_gnring = 5;
                    
                    g70 = gauring(7,0,s);
                    g71 = gauring(7,1,s);
                    g72 = gauring(7,2,s);
                    g63 = gauring(6,3,s);
                    g64 = gauring(6,4,s);
                    g54 = gauring(5,4,s);
                    g55 = gauring(5,5,s);
                    if((g70 | g71 | g72 | g63 | g64 | g54 | g55) == 0) 
                                                                sig_gnring = 4;

                    if(sigma > 2.1 && sig_gnring == 5){
                        sig_gnring = 6;
                    }
                }else{
                    g70 = g71 = g72 = g63 = g64 = g54 = g55 = 0;
                }
            }else{
                g60 = g61 = g62 = g52 = g53 = g44 = 0;
                g70 = g71 = g72 = g63 = g64 = g54 = g55 = 0;
            }
        }else{
            g50 = g51 = g42 = g43 = g33 = 0;
            g60 = g61 = g62 = g52 = g53 = g44 = 0;
            g70 = g71 = g72 = g63 = g64 = g54 = g55 = 0;
        }
    }else{
        g40 = g41 = g32 = 0;
        g50 = g51 = g42 = g43 = g33 = 0;
        g60 = g61 = g62 = g52 = g53 = g44 = 0;
        g70 = g71 = g72 = g63 = g64 = g54 = g55 = 0;
    }
    sig_ncring = sig_gnring + 3;  /* 1+ outer radius of filter */
    sigmaring =  sigma;
}

/*****************************************************************************/
/* 
 * This function evaluates the smoothed image at a single location x,y.
 * and subtracts the sky. The image is scaled up by 2\pi sigma^2 (i.e.
 * the central value of the filter is 1.0 once we allow for the fact that
 * this routine works with pixel values scaled up by 512 == 1<<9)
 *
 * p is the picture; we pass no size information here, since we assume the
 * calling function has performed range checking. Sky is an estimate of
 * the background. It is handled here in a pretty inefficient way, but it
 * is not very costly, and for small filters is actually probably faster
 * than accumulating the weighted sum of the gij's (in makegnring()) and
 * subtracting its multiple of sky from the result, since eight
 * additions are usually a bit faster than one multiply.
 *
 * The code is messy because I (JEG) have unrolled the loop in order to take
 * explicit account of the symmetry of the filter and to ignore its unpopulated
 * corners; this saves nearly a factor of two in execution time over a simple
 * loop implementation. (I'm not convinced of this, RHL)
 */
static float
ast_lgausmth(const PIX **p, int x, int y, int sky)
{
    double ret;
    const int sky4 = sky << 2;
    const int sky8 = sky << 3;

    shAssert(sigmaring >= 0);		/* i.e. ast_makegnring() was called */

    ret =  g00*(p[y  ][x  ]                                           - sky ) +
           g01*(p[y  ][x+1] + p[y  ][x-1] + p[y+1][x  ] + p[y-1][x  ] - sky4) +
           g11*(p[y+1][x-1] + p[y+1][x+1] + p[y-1][x-1] + p[y-1][x+1] - sky4) +
           g20*(p[y  ][x+2] + p[y  ][x-2] + p[y-2][x  ] + p[y+2][x  ] - sky4) +
           g21*(p[y+2][x+1] + p[y+1][x+2] + p[y+1][x-2] + p[y+2][x-1] +
                p[y-2][x+1] + p[y-1][x+2] + p[y-1][x-2] + p[y-2][x-1] - sky8) +
           g30*(p[y  ][x+3] + p[y  ][x-3] + p[y+3][x  ] + p[y-3][x  ] - sky4) +
           g31*(p[y+3][x+1] + p[y+1][x+3] + p[y+3][x-1] + p[y+1][x-3] +
                p[y-3][x+1] + p[y-1][x+3] + p[y-3][x-1] + p[y-1][x-3] - sky8) +
           g22*(p[y+2][x+2] + p[y+2][x-2] + p[y-2][x+2] + p[y-2][x-2] - sky4);

    if(sig_gnring <= 1) return (ret/512);

    ret += g40*(p[y  ][x+4] + p[y  ][x-4] + p[y+4][x  ] + p[y-4][x  ] - sky4) +
           g41*(p[y+1][x+4] + p[y+4][x+1] + p[y+1][x-4] + p[y+4][x-1] +
                p[y-1][x+4] + p[y-4][x+1] + p[y-1][x-4] + p[y-4][x-1] - sky8) +
           g32*(p[y+2][x+3] + p[y+3][x+2] + p[y+2][x-3] + p[y+3][x-2] +
                p[y-2][x+3] + p[y-3][x+2] + p[y-2][x-3] + p[y-3][x-2] - sky8);

    if(sig_gnring <= 2) return (ret/512);

    ret += g50*(p[y  ][x+5] + p[y  ][x-5] + p[y+5][x  ] + p[y-5][x  ] - sky4) +
           g51*(p[y+1][x+5] + p[y+5][x+1] + p[y+1][x-5] + p[y+5][x-1] +
                p[y-1][x+5] + p[y-5][x+1] + p[y-1][x-5] + p[y-5][x-1] - sky8) +
           g42*(p[y+2][x+4] + p[y+4][x+2] + p[y+2][x-4] + p[y+4][x-2] +
                p[y-2][x+4] + p[y-4][x+2] + p[y-2][x-4] + p[y-4][x-2] - sky8) +
           g43*(p[y+3][x+4] + p[y+4][x+3] + p[y+3][x-4] + p[y+4][x-3] +
                p[y-3][x+4] + p[y-4][x+3] + p[y-3][x-4] + p[y-4][x-3] - sky8) +
           g33*(p[y+3][x+3] + p[y+3][x-3] + p[y-3][x+3] + p[y-3][x-3] - sky4);

    if(sig_gnring <= 3) return (ret/512);

    ret += g60*(p[y  ][x+6] + p[y  ][x-6] + p[y+6][x  ] + p[y-6][x  ] - sky4) +
           g61*(p[y+1][x+6] + p[y+6][x+1] + p[y+1][x-6] + p[y+6][x-1] +
                p[y-1][x+6] + p[y-6][x+1] + p[y-1][x-6] + p[y-6][x-1] - sky8) +
           g62*(p[y+2][x+6] + p[y+6][x+2] + p[y+2][x-6] + p[y+6][x-2] +
                p[y-2][x+6] + p[y-6][x+2] + p[y-2][x-6] + p[y-6][x-2] - sky8) +
           g52*(p[y+2][x+5] + p[y+5][x+2] + p[y+2][x-5] + p[y+5][x-2] +
                p[y-2][x+5] + p[y-5][x+2] + p[y-2][x-5] + p[y-5][x-2] - sky8) +
           g53*(p[y+3][x+5] + p[y+5][x+3] + p[y+3][x-5] + p[y+5][x-3] +
                p[y-3][x+5] + p[y-5][x+3] + p[y-3][x-5] + p[y-5][x-3] - sky8) +
           g44*(p[y+4][x+4] + p[y+4][x-4] + p[y-4][x+4] + p[y-4][x-4] - sky4);

    if(sig_gnring <= 4) return (ret/512);           

    ret += g70*(p[y  ][x+7] + p[y  ][x-7] + p[y+7][x  ] + p[y-7][x  ] - sky4) +
           g71*(p[y+1][x+7] + p[y+7][x+1] + p[y+1][x-7] + p[y+7][x-1] +
                p[y-1][x+7] + p[y-7][x+1] + p[y-1][x-7] + p[y-7][x-1] - sky8) +
           g72*(p[y+2][x+7] + p[y+7][x+2] + p[y+2][x-7] + p[y+7][x-2] +
                p[y-2][x+7] + p[y-7][x+2] + p[y-2][x-7] + p[y-7][x-2] - sky8) +
           g63*(p[y+3][x+6] + p[y+6][x+3] + p[y+3][x-6] + p[y+6][x-3] +
                p[y-3][x+6] + p[y-6][x+3] + p[y-3][x-6] + p[y-6][x-3] - sky8) +
           g64*(p[y+4][x+6] + p[y+6][x+4] + p[y+4][x-6] + p[y+6][x-4] +
                p[y-4][x+6] + p[y-6][x+4] + p[y-4][x-6] + p[y-6][x-4] - sky8) +
           g54*(p[y+4][x+5] + p[y+5][x+4] + p[y+4][x-5] + p[y+5][x-4] +
                p[y-4][x+5] + p[y-5][x+4] + p[y-4][x-5] + p[y-5][x-4] - sky8) +
           g55*(p[y+5][x+5] + p[y+5][x-5] + p[y-5][x+5] + p[y-5][x-5] - sky4);

    return (ret/512);
    /* 
     * note that if sigma > 2.1, we still evaluate the filter, but do not
     * include the outer parts; it is easy to add more rings if this is
     * deemed desirable, but I think not. We should not be trying to work
     * in this kind of seeing anyhow.
     */
}

/*****************************************************************************/
/* 
 * This function does the gaussian quartic interpolation for the position
 * of the maximum * for three equally spaced values vm,v0,vp, assumed to be at
 * abscissae -1,0,1; the answer is returned as *cen
 *
 * Return 0 is all is well, otherwise 1
 */
#define AMPAST4 1.33			/* amplitude of `4th order' corr
					   compared to theory */

static int
inter4(float vm,float v0,float vp,float *cen)
{
    float sp = v0-vp;
    float sm = v0-vm;
    float d2 = sp + sm;
    float s = 0.5*(vp-vm);

    if(d2 <= 0.0f || v0 <= 0.0f) {
       return(1);
    }
    
    *cen = s/d2*(1. + AMPAST4*sp*sm/(d2*v0));

    return(fabs(*cen) < 1 ? 0 : 1);
}

/*****************************************************************************/
/*
 * Calculate error in centroid
 */
static float
astrom_errors(float gain,		/* CCD's gain */
	      float esky,		/* noise-equivalent sky, including
					   background variance */
	      float A,			/* abs(peak value in raw image) */
	      float tau2,		/* Object is N(0,tau2) */
	      float As,			/* abs(peak value in smoothed image) */
	      float s,			/* slope across central pixel */
	      float d,			/* curvature at central pixel */
	      float sigma,		/* width of smoothing filter */
	      int quartic_bad)		/* was quartic estimate bad? */
{
   const float k = quartic_bad ? 0 : AMPAST4; /* quartic correction coeff */
   const float sigma2 = sigma*sigma;	/* == sigma^2 */
   float sVar, dVar;			/* variances of s and d */
   float xVar;				/* variance of centroid, x */

   if(As == 0.0 || d == 0.0) {
      return(1e3);
   }

   if(sigma == 0) {			/* no smoothing; no covariance */
      sVar = esky/gain/2;		/* sky */
      dVar = 6*esky/gain;

      sVar += 0.5*(A/gain)*exp(-1/(2*tau2));
      dVar += (A/gain)*(4*exp(-1/(2*tau2)) + 2*exp(-1/(2*tau2)));
   } else {				/* smoothed */
      sVar = esky/gain/(8*M_PI*sigma2)*(1 - exp(-1/sigma2));
      dVar = esky/gain/(2*M_PI*sigma2)*
				   (3 - 4*exp(-1/(4*sigma2)) + exp(-1/sigma2));

      sVar += (A/gain)/(12*M_PI*sigma2)*(exp(-1/(3*sigma2)) - exp(-1/sigma2));
      dVar += (A/gain)/(3*M_PI*sigma2)*
				   (2 - 3*exp(-1/(3*sigma2)) + exp(-1/sigma2));
   }

   xVar = sVar*pow(1/d + k/(4*As)*(1 - 12*s*s/(d*d)), 2) +
				 dVar*pow(s/(d*d) - k/(4*As)*8*s*s/(d*d*d), 2);

   return(xVar >= 0 ? sqrt(xVar) : ERROR_IS_BAD);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * This is phObjectCenterFind unpacked for the DA system (i.e. no REGION)
 */
#define NITER 5				/* maximum allowed number of tries to
					   find max in smoothed image */
static int
phObjectCenterFind_Ptr(const PIX **rows, /* data */
		      int nrow, int ncol, /* size of data array */
		      int x0, int y0,	/* initial estimate of centre */
		      float sky,	/* number of counts in the sky */
		      float sigma,	/* sigma of Gaussian to smooth image
					   with (if < 0, assume that the
					   region is already smoothed) */
		      float bkgd,	/* level of background to subtract */
		      float dark_variance, /* variance of the background */
		      float gain,	/* gain of amplifiers */
		      float *xc, float *yc, /* centre of image */
		      float *dxc, float *dyc, /* errors in xc, yc */
		      float *peak,	/* peak value */
		      float *sigsqx, float *sigsqy) /* estimates of the width^2
						    of the object in x and y */
{
   float val[3][3];			/* background subbed region around
					   (x0,y0) */
   float vpk;				/* Estimate for real peak value of
					   smoothed image (sky subtracted) */
   int i,j;
   int xp,yp;
   float d2x, d2y;
   float sx, sy;
   float dx0,dy0;			/* first approx to center from
					   quadratic interpolation */
   float dx,dy;
   float dx4,dy4;
   float m0x,m1x,m2x;
   float m0y,m1y,m2y;
   float smx,smy,dm2x,dm2y;
   int quartic_bad;			/* is quartic interpolation bad? */
   float sigma_x2;			/* widths^2 of smoothed image in x */
   float sigma_y2;			/*                            and y */
   float v,vc;
   int niter;				/* number of iterations looking for
					   centre */
   static float osigma = -1;		/* previous value of sigma */
   int smsize;				/* size of smoothing filter */
   int sign_peak = 1;			/* sign of smoothed peak value */
   
   if(sigma > 0) {
      if(fabs(sigma - osigma) > 1e-4) { /* sigma has changed */
	 ast_makegnring(sigma);
	 osigma = sigma;
      }
      smsize = sig_ncring + 1;
   } else {
      smsize = 1;
   }
/*
 * look for the centre in the smoothed image
 */
   *xc = x0; *yc = y0;			/* ensure that they are not NaNs */
   for(niter = 0;;niter++) {
      if(niter >= NITER) {
	 return(-1);
      }
      if(x0 < smsize || x0 > ncol - smsize - 1 || 
	 y0 < smsize || y0 > nrow - smsize - 1) {
	 return (-2);
      }
/*
 * Go through the 3x3 region around (x0,y0) calculating the values of the
 * smoothed image. The smoothed maximum may not coincide with the unsmoothed,
 * so we may have to iterate a little.
 */
      vc = val[1][1] = (sigma > 0) ?
			   ast_lgausmth(rows,x0,y0,bkgd) : rows[y0][x0] - bkgd;
      
      for(i = 0;i < 3;i++) {
	 yp = y0 + i - 1;
	 for(j = 0;j < 3;j++) {
	    if(i == 1 && j == 1) {	/* central pixel is already done */
	       continue;
	    }
	    xp = x0 + j - 1;
	    
	    v = val[i][j] = (sigma > 0) ?
			   ast_lgausmth(rows,xp,yp,bkgd) : rows[yp][xp] - bkgd;
	    if(v > vc) {
/*
 * missed the maximum--ie the max in the smoothed image is not at the same
 * place as that in the raw image. Move the centre to the new maximum, and
 * try again.
 */
	       x0 += j - 1;
	       y0 += i - 1;
	       goto try_again;
	    }
	 }
      }
      break;				/* we have the maximum */
/*
 * target of a single goto deep in double for-loop above
 */
    try_again:;
   }
/*
 * find a first quadratic estimate
 */
   d2x = 2*val[1][1] - val[1][0] - val[1][2];
   d2y = 2*val[1][1] - val[0][1] - val[2][1];
   sx =  0.5*(val[1][2] - val[1][0]);
   sy =  0.5*(val[2][1] - val[0][1]);
   if(d2x == 0.0 || d2y == 0.0) { /* vanishing second derivative */
      return(-3);
   }
   shAssert(d2x > 0.0 && d2y > 0.0);
   dx0 = sx/d2x;
   dy0 = sy/d2y;			/* first guess */
   
   vpk = val[1][1] + 0.5*(sx*dx0 + sy*dy0);
   if(vpk < 0) {
      sign_peak = -1;
      vpk = -vpk;
   }
/*
 * now evaluate maxima on stripes
 */
   quartic_bad = 0;
   quartic_bad += inter4(val[0][0],val[0][1],val[0][2],&m0x);
   quartic_bad += inter4(val[1][0],val[1][1],val[1][2],&m1x);
   quartic_bad += inter4(val[2][0],val[2][1],val[2][2],&m2x);
   
   quartic_bad += inter4(val[0][0],val[1][0],val[2][0],&m0y);
   quartic_bad += inter4(val[0][1],val[1][1],val[2][1],&m1y);
   quartic_bad += inter4(val[0][2],val[1][2],val[2][2],&m2y);
   
   if(quartic_bad) {			/* >= 1 quartic interpolator is bad */
      *xc = x0 + dx0 + DXF;
      *yc = y0 + dy0 + DYF;
      sigma_x2 = vpk/d2x;		/* widths^2 in x */
      sigma_y2 = vpk/d2y;		/*             and y */
   } else {
      smx = 0.5*(m2x-m0x);
      smy = 0.5*(m2y-m0y);
      dm2x = m1x - 0.5*(m0x+m2x);
      dm2y = m1y - 0.5*(m0y+m2y);      
      dx = m1x + dy0*(smx - dy0*dm2x);	/* first quartic approx */
      dy = m1y + dx0*(smy - dx0*dm2y);
      
      dx4 = m1x + dy*(smx - dy*dm2x);	/* second quartic approx */
      dy4 = m1y + dx*(smy - dx*dm2y);
      
      *xc = x0 + dx4 + DXF;
      *yc = y0 + dy4 + DYF;
      sigma_x2 = vpk/d2x - (1 + 6*dx0*dx0)/4; /* widths^2 in x */
      sigma_y2 = vpk/d2y - (1 + 6*dy0*dy0)/4; /*             and y */
   }

   if(sigma > 0) {
      vpk /= 2*M_PI*sigma*sigma;	/* scaled up by ast_lgausmth() */
      sx  /= 2*M_PI*sigma*sigma;
      sy  /= 2*M_PI*sigma*sigma;
      d2x /= 2*M_PI*sigma*sigma;
      d2y /= 2*M_PI*sigma*sigma;
   }
/*
 * Now for the errors.
 */
   shAssert((dyc == NULL && dxc == NULL) || (dyc != NULL && dxc != NULL));

   if(dxc != NULL) {
      float A;				/* peak of _un_ smoothed object */
      float tau_x2, tau_y2;		/* width^2 of _un_ smoothed object */
      float esky = sky + gain*dark_variance; /* equivalent sky, including
						background variance */

      shAssert(gain > 0);
      
      tau_x2 = sigma_x2; tau_y2 = sigma_y2; 
      tau_x2 -= sigma*sigma;		/* correct for smoothing */
      tau_y2 -= sigma*sigma;

      if(tau_x2 <= sigma*sigma) {	/* problem; sigma_x2 must be bad */
	 tau_x2 = sigma*sigma;
      }
      if(tau_y2 <= sigma*sigma) {	/*          sigma_y2 must be bad */
	 tau_y2 = sigma*sigma;
      }

      A = vpk*sqrt((sigma_x2/tau_x2)*(sigma_y2/tau_y2));

      *dxc = astrom_errors(gain, esky, A, tau_x2,
			   vpk, sx, d2x, fabs(sigma), quartic_bad);
      *dyc = astrom_errors(gain, esky, A, tau_y2,
			   vpk, sy, d2y, fabs(sigma), quartic_bad);
   }
/*
 * finally return estimates for the (width of the unsmoothed object)^2,
 * and the maximum of the smoothed peak intensity, if desired
 */
   if(sigma > 0) {
      sigma_x2 -= sigma*sigma;		/* estimate width of _un_ smoothed */
      sigma_y2 -= sigma*sigma;		/*                          object */

      if(sigma_x2 <= sigma*sigma) {	/* problem; sigma_x2 must be bad */
	 sigma_x2 = sigma*sigma;
      }
      if(sigma_y2 <= sigma*sigma) {	/*          sigma_y2 must be bad */
	 sigma_y2 = sigma*sigma;
      }
   }
   
   if(vpk != 0) {
      shAssert(sigma_x2 > 0 && sigma_y2 > 0);
   }

   if(sigsqx != NULL) {
      *sigsqx = sigma_x2;
   }
   if(sigsqy != NULL) {
      *sigsqy = sigma_y2;
   }
   if(peak != NULL) {
      *peak = vpk;			/* n.b. This is the SMOOTHED peak */
   }

   return((sign_peak < 0) ? -4 : (vpk == 0) ? -5 : 0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Find the astrometric center of an object using a fourth-order interpolation
 * scheme appropriate to Gaussian stars.
 *
 * If sigma, the width of the PSF (taken to be a Gaussian N(0,sigma^2)), is
 * greater than 0, the routine first smooths the input image with this
 * Gaussian. If the initial guess for the centre (x0,y0) of the smoothed image
 * is not correct, the routine iterates, but this is a computationally
 * expensive process); it is in practice always OK to use the maximum in
 * the unsmoothed image for this guess.
 *
 * If sigma <= 0, the image is assumed to be already smoothed with the PSF;
 * in this case the widths will be simply an estimate of the object's sigma^2.
 * If sigma > 0, then the returned widths are sigma^2/2; the presumed
 * width of the unsmoothed object.
 *
 * Any of dxc, dyc, sigsqx, sigsqy, and peak may be NULL
 *
 * If dxc or dyc is NULL the dark variance and gain (and sigma if the image
 * is already smoothed) are not required. If desired, the errors are
 * calculated as follows:
 *
 * The standard deviation in the quadratic interpolator is
 *     (q |sigma|)/(A \sqrt(2pi))*sqrt(1 - exp(-1/sigma^2))
 * where q is the per-pixel noise and A the central amplitude in the smoothed
 * image. For sigma == 1 this is 0.32q/A, rising to 0.40q/A for sigma large
 *
 * We shall take q^2 == (sky + A/2)/gain + dark_variance, where A/2 is an
 * estimate of the noise due to the object itself. The dark variance should
 * not include any contribution from the photon noise in the sky
 *
 * Note that the "bkgd" is simply subtracted from each pixel; it should
 * contain any softbias and any sky value that has _not_ already been
 * subtracted. The "sky" is the number of counts in the sky, and may
 * or may not have been subtracted from the image.
 *
 * Return codes:
 *    0		OK
 *   -1		Centre isn't locally highest pixel (this is set after hunting
 *              for a maximum and failing after NITER tries)
 *   -2         Object is too close to the edge
 *   -3         Object has vanishing second derivative
 *   -4         Object's central pixel (after smoothing) is negative. In
 *		this case the centre's still estimated
 *   -5         Object's central pixel (after smoothing) is zero. This leads
 *              to an estimate of 0 for the sigma_[xy]2, so the error
 *              estimate is meaningless (1000); a centre is still returned
 */
int
phObjectCenterFind(const REGION *reg,	/* region containing object */
		   int x0, int y0,	/* initial estimate of centre */
		   float sky,		/* number of counts in the sky */
		   float sigma,		/* sigma of Gaussian to smooth image
					   with (if < 0, assume that the
					   region is already smoothed) */
		   float bkgd,		/* level of background to subtract */
		   float dark_variance,	/* variance of the background */
		   float gain,		/* gain of amplifiers */
		   float *xc, float *yc, /* centre of image */
		   float *dxc, float *dyc, /* errors in xc, yc */
		   float *peak,		/* peak value */
		   float *sigsqx, float *sigsqy) /* estimates of the width^2
						    of the object in x and y */
{
   int ncol, nrow;			/* == reg->{ncol,nrow} */
   PIX **rows;				/* == reg->ROWS */
   int row0, col0;			/* == reg->{row,col}0 */
   int ret;				/* return code */

   shAssert(reg != NULL && reg->type == TYPE_PIX);
   rows = reg->ROWS;
   row0 = reg->row0; col0 = reg->col0;
   nrow = reg->nrow; ncol = reg->ncol;
/*
 * allow for origin of region
 */
   x0 -= col0; y0 -= row0;

   ret = phObjectCenterFind_Ptr((const PIX **)rows, nrow, ncol, x0, y0,
				sky, sigma, bkgd, dark_variance, gain,
				xc, yc, dxc, dyc, peak, sigsqx, sigsqy);

#if CHECK_PHOTO_CENTROID
/*
 * We cannot use this code as is because it fails to distinguish sky from bkgd;
 * but we can check that we agree with it.
 */
   {
      float tx = *xc, ty = *yc; int tr = ret;

      ret = atObjectCenterFind_Ptr((const PIX **)rows, nrow, ncol, x0, y0,
				   bkgd, sigma, dark_variance, gain,
				   xc, yc, NULL, NULL, NULL, NULL, NULL,
				   NULL, NULL);

      if(ret == 0 && tr == 0 &&
	 (fabs(ty - *yc) > 1e-4 || fabs(tx - *xc) > 1e-4)) {
	 printf("RHL %d:%d %10.4f %10.4f  %10.4f %10.4f\n",
		tr, ret, ty, ty - *yc, tx, tx - *xc); fflush(stdout);
      }
   }
#endif

   *xc += col0; *yc += row0;

   return(ret);
}
