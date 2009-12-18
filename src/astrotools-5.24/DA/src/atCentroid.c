#if defined(VXWORKS) || !defined(ASTROLINE)
/* <AUTO>
  FILE: atCentroid
<HTML>
  Gunn's routines to calculate centroids
  With Heidi's enhancements to calculate widths
</HTML>
  </AUTO>
*/
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
 * NOTE: The decision of whether the center of a pixel is i.000 or i.500
 *	is quite arbitrary, and I can't believe it is worth discussing
 *	in the comments.  If you cared so much, you should have attended
 *	the meeting in which it was discussed.  I guess meetings _are_
 *	important, if you care what is done on the project, huh?
 *
 *
 * 5/96 Heidi Newberg	changed order of lines so that ast_makegnring is
 *				always called before smsize is set
 *			This routine used to assume that the input sigma
 *				was exactly equal to the image sigma.
 *				I replaced all occurrances of sigma_x2
 *				by sigma_x2/2., and all occurrances of
 *				sigma_y2 with sigma_y2/2.  The new quantities
 *				are inputSigma*inputSigma+imageSigma*imageSigma.
 *				The returned sigsqx is the sigma in the x
 *				direction in the convolved region.
 * 5/96 Heidi Newberg	In the last revision, the sigma of a Gaussian
 *			cross section along x=0 and y=0 of the
 *				convolved bivariate Gaussian were returned.
 *				Now, in addition to these widths, the
 *				sigmas of the cross sections y=x and y=-x
 *				are returned.
 * 6/96 Heidi Newberg	Added routine to calculate a,b,pa from widths
 */
#include <math.h>

#if defined(VXWORKS)  /* online da computers  */
#include <stdio.h>
#include "trace.h"
#define U16  unsigned short int
#define M_PI 3.14159265358979323846
#define shAssert(ex) {if (!(ex)){TRACE(0,"atCentroid: Assertion failed: %s, line %d", #ex, __LINE__,0,0); return -1*__LINE__;}}

#elif defined(NOASTROTOOLS) /* standalone compilation; astrotools 
				not available */
#include <assert.h>
#define shAssert(ex) assert(ex)
#define U16  unsigned short int
#define M_PI 3.14159265358979323846

#else                /* normal astrotools compilation */
#include <dervish.h>
#include "atConversions.h"      /* for M_PI */

#endif

#include "atGaussianWidth.h"	/* prototypes for atGaussianWidth */
#include "atCentroid.h"		/* prototypes for atCentroid */

/* 
 * DXF and DYF are the offsets needed to go from natural floating coordinates
 * in which an object CENTERED in pixel i j has floating coordinates
 * i.0000 j.0000 to the insane ones adopted by the SDSS, in which an
 * object centered on the LL corner of a pixel (and therefore has its light
 * equally shared with pixels i-1,j i-1,j-1 and i,j-1) has zero fractional 
 * part.
 * NOTE: See note above.
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
 * and subtracts the sky.
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
 * loop implementation.
 */
static int 
ast_lgausmth(const U16 **p, int x, int y, int sky)
{
    int ret;
    const int sky4 = sky << 2;
    const int sky8 = sky << 3;

#if !defined(VXWORKS)                   /* normal astrotools compilation */
    shAssert(sigmaring >= 0);		/* i.e. ast_makegnring() was called */
#endif

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

    if(sig_gnring <= 1) return ((ret + 256) >> 9);

    ret += g40*(p[y  ][x+4] + p[y  ][x-4] + p[y+4][x  ] + p[y-4][x  ] - sky4) +
           g41*(p[y+1][x+4] + p[y+4][x+1] + p[y+1][x-4] + p[y+4][x-1] +
                p[y-1][x+4] + p[y-4][x+1] + p[y-1][x-4] + p[y-4][x-1] - sky8) +
           g32*(p[y+2][x+3] + p[y+3][x+2] + p[y+2][x-3] + p[y+3][x-2] +
                p[y-2][x+3] + p[y-3][x+2] + p[y-2][x-3] + p[y-3][x-2] - sky8);

    if(sig_gnring <= 2) return ((ret + 256) >> 9);

    ret += g50*(p[y  ][x+5] + p[y  ][x-5] + p[y+5][x  ] + p[y-5][x  ] - sky4) +
           g51*(p[y+1][x+5] + p[y+5][x+1] + p[y+1][x-5] + p[y+5][x-1] +
                p[y-1][x+5] + p[y-5][x+1] + p[y-1][x-5] + p[y-5][x-1] - sky8) +
           g42*(p[y+2][x+4] + p[y+4][x+2] + p[y+2][x-4] + p[y+4][x-2] +
                p[y-2][x+4] + p[y-4][x+2] + p[y-2][x-4] + p[y-4][x-2] - sky8) +
           g43*(p[y+3][x+4] + p[y+4][x+3] + p[y+3][x-4] + p[y+4][x-3] +
                p[y-3][x+4] + p[y-4][x+3] + p[y-3][x-4] + p[y-4][x-3] - sky8) +
           g33*(p[y+3][x+3] + p[y+3][x-3] + p[y-3][x+3] + p[y-3][x-3] - sky4);

    if(sig_gnring <= 3) return ((ret + 256) >> 9);

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

    if(sig_gnring <= 4) return ((ret + 256) >> 9);           

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

    return ((ret + 256) >> 9);
    /* 
     * note that if sigma > 2.1, we still evaluate the filter, but do not
     * include the outer parts; it is easy to add more rings if this is
     * deemed desirable, but I think not. We should not be trying to work
     * in this kind of seeing anyhow.
     */
}

/*****************************************************************************/
/* 
 * This function does the gaussian quartic interpolation for the maximum 
 * for three equally spaced values vm,v0,vp, assumed to be at abscissae -1,0,1;
 * the answer is returned as *cen
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
#define NITER 10				/* maximum allowed number of tries to
					   find max in smoothed image */
/*
 * <AUTO EXTRACT>
 *
 * This is atObjectCenterFind unpacked for the DA system (i.e. no REGION)
 * </AUTO>*/
int
atObjectCenterFind_Ptr(const U16 **rows, /* data */
		      int nrow, int ncol, /* size of data array */
		      int x0, int y0,	/* initial estimate of centre */
					/* x is col, and y is row */
		      int sky,		/* level of background */
		      float sigma,	/* sigma of Gaussian to smooth image
					   with (ignore if <= 0) */
		      float dark_variance, /* variance of the background */
		      float gain,	/* gain of amplifiers */
		      float *xc, float *yc, /* center of image */
		      float *dxc, float *dyc, /* errors in xc, yc */
		      float *peak,	/* peak value */
		      float *sigsqx, float *sigsqy, 
		      float *sigsqp, float *sigsqm)
				/* estimates of the width^2 of the 
				convolved object along the lines
				y=0, x=0, y=x, and y=-x */
{
   float val[3][3];			/* sky subbed region around (x0,y0) */
   float vpk;				/* Estimate for real peak value of
					   smoothed image (sky subtracted) */
   float xpk,ypk,mpk,ppk;
   int i,j;
   int xp,yp;
   float d2x, d2y, d2p, d2m;
   float sx, sy, sp, sm;
   float dx0,dy0,dm0,dp0;		/* first approx to center from
					   quadratic interpolation */
   float dx,dy;
   float dx4,dy4;
   float m0x,m1x,m2x;
   float m0y,m1y,m2y;
   float smx,smy,dm2x,dm2y;
   int quartic_bad;			/* is quartic interpolation bad? */
   float sigma_x2;			/* widths^2 of smoothed image in x */
   float sigma_y2;			/*              and y */
   float sigma_p2;			/*		and along y=x */
   float sigma_m2;			/*		and along y=-x */
   float v,vc;
   int niter;				/* number of iterations looking for
					   centre */
   static float osigma = -1;		/* previous value of sigma */
   int smsize;				/* size of smoothing filter */

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

#if defined(VXWORKS)                    /* online da computers  */  
      if(sigmaring < 0) ast_makegnring(2.0); /* hack that needs to be fixed */
#endif

      vc = val[1][1] = (sigma > 0) ?
			     ast_lgausmth(rows,x0,y0,sky) : rows[y0][x0] - sky;
      
      for(i = 0;i < 3;i++) {
	 yp = y0 + i - 1;
	 for(j = 0;j < 3;j++) {
	    if(i == 1 && j == 1) {	/* central pixel is already done */
	       continue;
	    }
	    xp = x0 + j - 1;
	    
	    v = val[i][j] = (sigma > 0) ?
			     ast_lgausmth(rows,xp,yp,sky) : rows[yp][xp] - sky;
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
 * These estimates are done as four separate one-dimensinal quadratic
 * fits.  The one dimensional parabola P(x) = a x^2 + b x + c is fit
 * by three points, P(-1), P(0), and P(1).  The solution is:
 *
 *		c = P(0)
 *		b = (P(1)-P(-1))/2 = s
 *	      -2a = 2 P(0) - P(1) -P(-1) = d2
 *
 * These can be translated to the quadratic approximation to a Gaussian
 * by the equations:
 *
 *		A = pk = c - (b^2 /4a) = P(0) + 0.5 s^2 / d2
 *		x0 = s/d2
 *		sigma^2 = pk / d2
 *
 * We now proceed to calculate these fits for the vertical, horizontal,
 * and diagonal lines through the center of the 3x3 pixel array.
 */
   d2x = 2*val[1][1] - val[1][0] - val[1][2];
   d2y = 2*val[1][1] - val[0][1] - val[2][1];
   d2p = 2*val[1][1] - val[2][2] - val[0][0];
   d2m = 2*val[1][1] - val[0][2] - val[2][0];

   sx =  0.5*(val[1][2] - val[1][0]);
   sy =  0.5*(val[2][1] - val[0][1]);
   sp =  0.5*(val[0][0] - val[2][2]);
   sm =  0.5*(val[2][0] - val[0][2]);

   if(d2x == 0.0 || d2y == 0.0 || d2p == 0.0 || d2m == 0.0) { 
      /* vanishing second derivative */
      return(-3);
   }
   shAssert(d2x > 0.0 && d2y > 0.0 && d2p > 0.0 && d2m > 0.0);
   /* Find quadratic estimate of the centroid.  The plus and minus
	estimates are are (distance along y=x)/sqrt(2) and
	(distance along y=-x)/sqrt(2), respectively */
   dx0 = sx/d2x;
   dy0 = sy/d2y;
   dp0 = sp/d2p;
   dm0 = sm/d2m;
   
   vpk = val[1][1] + 0.5*(sx*dx0 + sy*dy0); /* This is used for the
						error esitmates, though
						I'm not sure why */
   /* These are the fits to the peak along the four separate directions.
	If the object is centered in the center of a pixel, these are all
	the same and equal to the amplitude of the bivariate Gaussian fit
	to the convolved object.  If not, these "peaks" will be lower, 
	since they measure the amplitude of the cross section of the 
	bivariate Gaussian, which does not in general go through the peak 
	of the Gaussian. */
   xpk = val[1][1] + 0.5*(sx*dx0);
   ypk = val[1][1] + 0.5*(sy*dy0);
   mpk = val[1][1] + 0.5*(sm*dm0);
   ppk = val[1][1] + 0.5*(sp*dp0);

   /* We now calculate the sigma^2 in each of the four directions, using
	the fourth order correction.  The fourth order correction is
	rather large, and does not depend on any of the interpolations,
	so we might as well put it in here even if the quadratic
	solution is used.  sigma^2 is guaranteed to be positive if
	d2 was positive, and that is true if the peak pixel is in the
	center. */
   sigma_x2 = xpk/(d2x) - (1 + 6*dx0*dx0)/4;
   sigma_y2 = ypk/(d2y) - (1 + 6*dy0*dy0)/4;
   sigma_p2 = 2.*(ppk/(d2p) - (1 + 6*(dp0*dp0))/4);
   sigma_m2 = 2.*(mpk/(d2m) - (1 + 6*(dm0*dm0))/4);
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
   }
   if (sigma_x2 <= 0 || sigma_y2 <= 0 || sigma_p2 <= 0 || sigma_m2 <= 0) return -4;
/*
 * Now for the errors.
 */
   if(dxc != NULL && dyc != NULL) {
      float q;				/* per-pixel noise */
      float halfsigma_x2 = sigma_x2/2.;
      float halfsigma_y2 = sigma_y2/2.;

      shAssert(vpk > 0 && gain > 0);
      
      q = sqrt((sky + vpk/2)/gain + dark_variance);
      *dxc = q*sqrt(halfsigma_x2)/(vpk*sqrt(2*M_PI))*sqrt(1 - exp(-1/halfsigma_x2));
      *dyc = q*sqrt(halfsigma_y2)/(vpk*sqrt(2*M_PI))*sqrt(1 - exp(-1/halfsigma_y2));
      
      if(!quartic_bad) {
        *dxc *= 1 + AMPAST4/(8*halfsigma_x2);
        *dyc *= 1 + AMPAST4/(8*halfsigma_y2);
      }
   }
/*
 * finally return estimates for the (width of the convolved object)^2 in 
 * the directions y=0, x=0, y=x, and y=-x, and the maximum of the smoothed 
 * peak intensity, if desired.  The peak is a lower limit, and will be
 * exactly the Gaussian fit only if the object is centered on a pixel.
 */
   if(sigsqx != NULL) *sigsqx = sigma_x2;
   if(sigsqy != NULL) *sigsqy = sigma_y2;
   if(sigsqp != NULL) *sigsqp = sigma_p2;
   if(sigsqm != NULL) *sigsqm = sigma_m2;
   if(peak != NULL) *peak = vpk; 
   
   return 0;
}

/*<AUTO EXTRACT>

  ROUTINE: atEllipseFind

  DESCRIPTION:
<HTML>
This routine finds the position angle and sigmas in the
major and minor axis directions of a bivariate Gaussian fit to the center
of the image.  These quantities are calculated as follows:
<P>
First look at a Gaussian that is rotated but centered on x=0, y=0.
It has equation:
<listing>
        G(x,y) = A exp { -x'^2/(2 a^2) - y'^2/(2 b^2) },
</listing>
where
<listing>
        x' = x cos(pa) + y sin(pa),
        y' = - x sin(pa) + y cos(pa),
</listing>
and a and b are the sigmas in the directions of the major and minor axes.
<P>
Let's look at the cross section y=0.  There,
<listing>
        G(x,0) = A exp { -x^2/(2 sigx^2) }
</listing>
where
<listing>
        (1/sigx^2) = ( cos(pa)/a )^2 + ( sin(pa)/b )^2.
We can write a similar equation for x=0:
<listing>
        (1/sigy^2) = ( sin(pa)/a )^2 + ( cos(pa)/b )^2.
</listing>
<P>
To get the sigmas in the y=x and y=-x directions, we must parameterize
so that arc length will be preserved.  We use y=t/sqrt(2) and x=t/sqrt(2)
for the y=x case.  Plugging in this parameterization, we find
<listing>
        (1/sigp^2) = (1 + 2 sin(pa) cos(pa))/(2 a^2)
                        + (1 - 2 sin(pa) cos(pa))/(2 b^2)
</listing>
Similarly for y=-x:
<listing>
        (1/sigm^2) = (1 - 2 sin(pa) cos(pa))/(2 a^2)
                        + (1 + 2 sin(pa) cos(pa))/(2 b^2)
</listing>
<P>
Now we comput the quantites:
<listing>
        1/sigy^2 - 1/sigx^2 = cos(2 pa) ( - 1/a^2 + 1/b^2)
</listing>
and
<listing>
        1/sigp^2 - 1/sigm^2 = sin(2 pa) (1/a^2 - 1/b^2)
</listing>
<P>
It is easy to see from the previous two equations that:
<listing>
        tan(2 pa) = { - (1/sigp^2 - 1/sigm^2) / ( 1/sigy^2 - 1/sigx^2) }
</listing>

To get pa in the right quadrant, use:
<listing>
        2 pa = atan2 { - 1/sigp^2 + 1/sigm^2, 1/sigy^2 - 1/sigx^2 }
</listing>
<P>
We can then invert the linear equations for the sigmas to find a and b.
If sin(pa)^2 < cos(pa)^2, then use the sigx, sigy numbers to calculate
a and b.  Otherwise, use sigp and sigm.
<P>
RETURNS:
<listing>
	<0>	OK
	<-10>	Zero sigma - peak and counts not calculated
	<-11>	Infinite sigma
</listing>

</HTML>
</AUTO>*/
int atEllipseFind(
  float sigsqx, float sigsqy, float sigsqp, float sigsqm, float sigma,
  float convpeak, float *siga, float *sigb, float *pa, float *peak, float *counts) {

  double pmdiff, crdiff;
  double theta, costheta, sintheta;
  double den, num;
  double aconv2, bconv2;

  shAssert((sigsqx>0)&&(sigsqy>0)&&(sigsqp>0)&&(sigsqm>0));

  /* Calculate the major axis, minor axis, and position angle */
  pmdiff = - 1./sigsqp + 1./sigsqm;
  crdiff = 1./sigsqy - 1./sigsqx;
  if ((pmdiff==0)&&(crdiff==0)) {
    theta = 0;
    aconv2 = sigsqx;
    bconv2 = sigsqx;
  } else {
    theta = 0.5*atan2(pmdiff,crdiff);
    costheta = cos(theta);
    sintheta = sin(theta);
    if (crdiff*crdiff>=pmdiff*pmdiff) {
       den = costheta*costheta-sintheta*sintheta;
       num = costheta*costheta/sigsqx-sintheta*sintheta/sigsqy;
       if (num==0) return -11;
       aconv2 = den/num;
       num = costheta*costheta/sigsqy-sintheta*sintheta/sigsqx;
       if (num==0) return -11;
       bconv2 = den/num;
    } else {
       den = 4.*sintheta*costheta;
       num = (1.+2.*sintheta*costheta)/sigsqp+(2.*sintheta*costheta-1.)/sigsqm;
       if (num==0) return -11;
       aconv2 = den/num;
       num = (2.*sintheta*costheta-1.)/sigsqp+(1.+2.*sintheta*costheta)/sigsqm;
       if (num==0) return -11;
       bconv2 = den/num;
    }
  }
  if (aconv2>(sigma*sigma)) {
    *siga = sqrt(aconv2 - sigma*sigma);
  } else *siga = 0;
  if (bconv2>(sigma*sigma)) {
    *sigb = sqrt(bconv2 - sigma*sigma);
  } else *sigb = 0;
  *pa = 180.*theta/3.1415926535;

  if (sigma>0) {
   if (*siga>0&&*sigb>0) {
    *peak = convpeak*sqrt(aconv2*bconv2)/(2*M_PI*sigma*sigma*(*siga)*(*sigb));
   } else {*peak = 0; *counts=0; return -10;}
  } else {
    *peak = convpeak;
  }
  *counts = 2.*M_PI*(*peak)*(*siga)*(*sigb);
  return 0;
}


/*<AUTO EXTRACT>

  ROUTINE: atDACentroid

  DESCRIPTION:

RETURNS:
<listing>
	<0>	OK
	<-1>	center isn't locally highest pixel in atObjectCenterFind_Ptr
	<-2>	object is too close to the edge in atObjectCenterFind_Ptr
	<-3>	object has vanishing second derivative in atObjectCenterFind_Ptr
	<-4>	sky is too high in atObjectCenterFind_Ptr
	<-10>	Zero sigma - peak and counts not calculated after atObjectCenterFind_Ptr
	<-11>	Infinite sigma after atObjectCenterFind_Ptr
	<-21>	Error in setfsigma (sigma too large) in atSigmaFind
	<-22>	Sigma out of range (inf sharp or inf flat) in atSigmaFind
	<-23>	Too many iterations in atSigmaFind
	<-24>	Too close to edge in atSigmaFind
	<-25>	Error in lgausmom - check sky value in atSigmaFind
	<-26>	Too flat in atFindFocMom, from atSigmaFind
	<-30>	Zero sigma - peak and counts not calculated after atSigmaFind
	<-31>	Infinite sigma after atSigmaFind
</listing>

</HTML>
</AUTO>*/
int atDACentroid( const U16 **rows, /* data */
		      int nrow, int ncol, /* size of data array */
		      int row0, int col0,/* initial estimate of centre */
		      int sky,		/* level of background */
		      float sigma,	/* sigma of Gaussian to smooth image
					   with (ignore if <= 0) */
		      float dark_variance, /* variance of the background */
		      float gain,	/* gain of amplifiers */
		      float *rowc, float *colc, /* center of image */
		      float *drowc, float *dcolc, /* errors in rowc, colc */
		      float *peak,	/* peak value */
		      float *counts, float *siga, 
		      float *sigb, float *pa) {

  int status;
  float sigsqx,sigsqy,sigsqp,sigsqm;
  double dsigma;
  GAUSSMOM moment;
  float peakDummy, countsDummy;

  status = atObjectCenterFind_Ptr(rows, nrow, ncol, col0, row0, sky, sigma, 
	dark_variance, gain, colc, rowc, drowc, dcolc, peak, 
	&sigsqx, &sigsqy, &sigsqp, &sigsqm);

        if (status==0) {

          status = atEllipseFind(sigsqx, sigsqy, sigsqp, sigsqm, sigma, 
	  			*peak, siga, sigb, pa, peak, counts);
          if (status==0) {
            dsigma = (double)((*siga)+(*sigb))/2.;
            status = atSigmaFind(rows, ncol, nrow, col0, row0, sky, &moment, &dsigma);
            if (status==0) {
              dsigma *= dsigma;
              sigsqx = dsigma*(1.+moment.g_xmom)/(1.-moment.g_xmom);
              sigsqy = dsigma*(1.+moment.g_ymom)/(1.-moment.g_ymom);
              sigsqp = dsigma*(1.+moment.g_pmom)/(1.-moment.g_pmom);
              sigsqm = dsigma*(1.+moment.g_mmom)/(1.-moment.g_mmom);
              status = atEllipseFind(sigsqx,sigsqy,sigsqp,sigsqm,0.,moment.g_filval, siga, sigb, pa, &peakDummy, &countsDummy);
              (*pa) = -(*pa);
            } /* atSigmaFind */
            if (status!=0) status -= 20;
          } /* 1st atEllipseFind */
        } /* atObjectCenterFind_Ptr */

  return status;

}

#endif

