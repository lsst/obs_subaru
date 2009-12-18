

/* <AUTO>
  FILE: atObject
<HTML>
  Routines to calculate quantities about astronomical objects
</HTML>
  </AUTO>
*/
/***************************************************************
 *
 *  C routine to calculate quantities about astronomical objects
 *
 **************************************************************/

#include "atConversions.h" /* for M_PI */
#include "dervish.h"
#include "atObject.h"
#include "atCentroid.h"
#include "atGaussianWidth.h"
#include "atRegSci.h"
#include "slalib.h"

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
 * If sigma <= 0, the image is assumed to be already smoothed with the PSF.
 *
 * Any of dxc, dyc, sigsqx, sigsqy, sigsqp, sigsqm and peak may be NULL
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
 * Return codes:
 *    0         OK
 *   -1         Centre isn't locally highest pixel (this is set after hunting
 *              for a maximum and failing after FINDERR tries)
 *   -2         Object is too close to the edge
 *   -3         Object has vanishing second derivative
 *   -4         Sky value is set too high
 */
int
atObjectCenterFind(const REGION *reg,   /* region containing object */
                   int x0, int y0,      /* initial estimate of centre */
                   int sky,             /* level of background */
                   float sigma,         /* sigma of Gaussian to smooth image
                                           with (ignore if <= 0) */
                   float dark_variance, /* variance of the background */
                   float gain,          /* gain of amplifiers */
                   float *xc, float *yc, /* centre of image */
                   float *dxc, float *dyc, /* errors in xc, yc */
                   float *peak,         /* peak value */
                   float *sigsqx, float *sigsqy, /* estimates of the width^2
                                                    of the object in x and y */
                   float *sigsqp, float *sigsqm)
{
   int ncol, nrow;                      /* == reg->{ncol,nrow} */
   U16 **rows;                          /* == reg->rows */
   int row0, col0;                      /* == reg->{row,col}0 */
   int ret;                             /* return code */

   shAssert(reg != NULL && reg->type == TYPE_U16);
   rows = reg->rows;
   row0 = reg->row0; col0 = reg->col0;
   nrow = reg->nrow; ncol = reg->ncol;
/*
 * allow for origin of region
 */
   x0 -= col0; y0 -= row0;

   ret = atObjectCenterFind_Ptr((const U16 **)rows, nrow, ncol, x0, y0,
                                sky, sigma, dark_variance, gain,
                                xc, yc, dxc, dyc, peak, sigsqx, sigsqy,
                                sigsqp, sigsqm);

   *xc += col0; *yc += row0;

   return(ret);
}


/*<AUTO EXTRACT>

  ROUTINE: atCentroidFind

  DESCRIPTION:
<HTML>
	This routine will calculate the centroid of an object in a REGION.
It is a wrapper around <A HREF=atObject.html#atObjectCenterFind>atObjectCenterFind</A>.  It additionally finds the position angle and sigmas in the
major and minor axis directions of a bivariate Gaussian fit to the center
of the image using <A HREF=atCentroid.html#atEllipseFind>atEllipseFind</A>.
</HTML>

  RETURN VALUES:
<HTML>
<listing>
		<0>	OK
		<-1>	center isn't locally highest pixel
		<-2>	object is too close to the edge
		<-3>	object has vanishing second derivative
		<-4>	sky is too high
		<-10>	Zero sigma - peak and counts not calculated
		<-11>	Infinite sigma
</listing>
</HTML>
</AUTO>*/

int atCentroidFind(REGION *reg, 
	int row0, int col0,
	double dbl_sky, 
	double dbl_sigma, 
	double dbl_dark_variance, 
	double dbl_gain, 
	double minval, double maxval, /* for scaling images to U16 */
	float *fl_xc, float *fl_yc, 
	float *fl_dxc, float *fl_dyc, 
	float *fl_peak, float *fl_counts,
	float *fl_siga, float *fl_sigb,
	float *fl_pa) {

  int sky, answer;
  float sigma, dark_variance, gain, peak, sigsqx, sigsqy;
  float sigsqp, sigsqm;
  double immean,imsigma,usemax,usemin,usefactor,total;
  int hrow,hcol,lrow,lcol;
  int r,c;
  double pix;
  REGION *newreg;

  sky = (int) dbl_sky;
  sigma = (float) dbl_sigma;
  dark_variance = (float) dbl_dark_variance;
  gain = (float) dbl_gain;

  if (minval!=maxval || reg->type!=TYPE_U16) {

    /* Get a new U16 region of the same size reg */
    newreg = shRegNew("", reg->nrow, reg->ncol, TYPE_U16);

    /* Set the pixels to the scaled values */
    if (maxval>minval) {
      usemin = minval; usemax = maxval;
    } else {
      /* Use max and min values in image to scale */
      atRegStatsFind(reg,&immean,&imsigma,&usemax,&hrow,&hcol,&usemin,&lrow,&lcol,&total);
    }
    usefactor = U16_MAX/(usemax-usemin);
    for (r=0; r<reg->nrow; r++) {
      for (c=0; c<reg->ncol; c++) {
         pix = shRegPixGetAsDbl(reg,r,c);
         if (pix>usemax) pix = usemax;
         if (pix<usemin) pix = usemin;
         shRegPixSetWithDbl(newreg,r,c,(pix-usemin)*usefactor);
      }
    }

    sky = (sky-usemin)*usefactor;

    if (sky>65535) {
       *fl_xc=0;
       *fl_yc=0;
       if (fl_dxc != NULL) *fl_dxc = 0;
       if (fl_dyc != NULL) *fl_dyc = 0;
       *fl_peak=0;
       *fl_counts=0;
       *fl_siga=0;
       *fl_sigb=0;
       *fl_pa=0;
       return -4;
    }
  
    answer = atObjectCenterFind(newreg, col0, row0, sky, sigma, 
	dark_variance, gain, fl_xc, fl_yc, fl_dxc, fl_dyc, &peak, 
	&sigsqx, &sigsqy, &sigsqp, &sigsqm);

    peak = peak/usefactor;

    (void)shRegDel(newreg);

  } else {
  
    if (sky>65535) {
       *fl_xc=0;
       *fl_yc=0;
       if (fl_dxc != NULL) *fl_dxc = 0;
       if (fl_dyc != NULL) *fl_dyc = 0;
       *fl_peak=0;
       *fl_counts=0;
       *fl_siga=0;
       *fl_sigb=0;
       *fl_pa=0;
       return -4;
    }
  
    answer = atObjectCenterFind(reg, col0, row0, sky, sigma, 
	dark_variance, gain, fl_xc, fl_yc, fl_dxc, fl_dyc, &peak, 
	&sigsqx, &sigsqy, &sigsqp, &sigsqm);

  }

if (answer==0) {
  /* Calculate the major axis, minor axis, and position angle */
  answer = atEllipseFind(sigsqx,sigsqy,sigsqp,sigsqm,sigma,peak,fl_siga,fl_sigb,fl_pa,fl_peak,fl_counts);
} else {
  *fl_xc = 0;
  *fl_yc = 0;
  if (fl_dxc != NULL) *fl_dxc = 0;
  if (fl_dyc != NULL) *fl_dyc = 0;
  *fl_siga = 0;
  *fl_sigb = 0;
  *fl_pa = 0;
  *fl_peak = 0;
  *fl_counts = 0;
}

  return answer;

}



/*<AUTO EXTRACT>

  ROUTINE: atGaussianWidthFind

  DESCRIPTION:
<HTML>
	This routine will calculate the width of an object in a REGION.
It is a wrapper around <A HREF=atGaussianWidth.html#atSigmaFind>atSigmaFind</A>.
<P>
	The routine will accept a region of any type, but if it is not U16,
the whole region will be scaled and converted to type U16.  Therefore, it is
best to send subregions to save time doing the converting.  The scaling
is linear with minval -> 0 and maxval -> 65535.  If minval=maxval, then
the min and max of the region will be used as the minval and maxval.
</HTML>

  RETURN VALUES:
<HTML>
<listing>
		<0>	OK
		<-1>	Sky=65535
		<-2>    Error in setfsigma (sigma too large)
		<-3>    sigma out of range (inf sharp or inf flat)
		<-4>    Too many iterations
		<-5>    Too close to edge
		<-6>    Error in lgausmom - check sky value
		<-7>	Too flat in atFindFocMom, from atSigmaFind
		<-10>	Zero sigma - peak and counts not calculated
		<-11>	Infinite sigma
</listing>
</HTML>
</AUTO>*/

int atGaussianWidthFind(REGION *reg, 
	int row0, int col0,
	double dbl_sky, 
	double minval, double maxval, /* for scaling images to U16 */
	float *fl_siga, float *fl_sigb, float *fl_pa, float *fl_peak,
	float *fl_counts, float *fl_xf, float *fl_yf) {

  int sky, answer;
  double immean,imsigma,usemax,usemin,usefactor,total;
  double sig=1.2,sigsq,sigsqx,sigsqy,sigsqp,sigsqm;
  int hrow,hcol,lrow,lcol;
  int r,c;
  double pix;
  REGION *newreg;
  GAUSSMOM momentstruct;
  double aconv2, bconv2;
  float denom;

  /* BUG -- include the following line and it works! */
  /* no you don't */
  /* int ifeTest; */ 

  sky = (int) dbl_sky;

  if (minval!=maxval || reg->type!=TYPE_U16) {

    /* Get a new U16 region of the same size reg */
    newreg = shRegNew("", reg->nrow, reg->ncol, TYPE_U16);

    /* Set the pixels to the scaled values */
    if (maxval>minval) {
      usemin = minval; usemax = maxval;
    } else {
      /* Use max and min values in image to scale */
      atRegStatsFind(reg,&immean,&imsigma,&usemax,&hrow,&hcol,&usemin,&lrow,&lcol,&total);
    }
    usefactor = U16_MAX/(usemax-usemin);
    for (r=0; r<reg->nrow; r++) {
      for (c=0; c<reg->ncol; c++) {
         pix = shRegPixGetAsDbl(reg,r,c);
         if (pix>usemax) pix = usemax;
         if (pix<usemin) pix = usemin;
         shRegPixSetWithDbl(newreg,r,c,(pix-usemin)*usefactor);
      }
    }

    sky = (sky-usemin)*usefactor;

    if (sky>65535) {
      *fl_siga = 0;
      *fl_sigb = 0;
      *fl_pa = 0;
      *fl_peak = 0;
      *fl_counts = 0;
      *fl_xf = 0;
      *fl_yf = 0;
       return -1;
    }
  
    answer = atSigmaFind((const U16 **)newreg->rows, newreg->ncol, newreg->nrow, col0, row0, sky, &momentstruct, &sig);

    /* BUG -- you do not need to include the following line for it to work */
    /* ifeTest = (int) momentstruct.g_filval; */
    momentstruct.g_filval = momentstruct.g_filval/usefactor;
      
    (void)shRegDel(newreg);

  } else {
  
    if (sky>65535) {
       *fl_siga = 0;
       *fl_sigb = 0;
       *fl_pa = 0;
       *fl_peak = 0;
       *fl_counts = 0;
       *fl_xf = 0;
       *fl_yf = 0;
       return -1;
    }
  
    answer = atSigmaFind((const U16 **)reg->rows, reg->ncol, reg->nrow, col0, row0, sky, &momentstruct, &sig);

  }

  if (answer==0) {
    sigsq = (sig)*(sig);
    sigsqx = sigsq*(1.+momentstruct.g_xmom)/(1.-momentstruct.g_xmom);
    sigsqy = sigsq*(1.+momentstruct.g_ymom)/(1.-momentstruct.g_ymom);
    sigsqp = sigsq*(1.+momentstruct.g_pmom)/(1.-momentstruct.g_pmom);
    sigsqm = sigsq*(1.+momentstruct.g_mmom)/(1.-momentstruct.g_mmom);
    answer = atEllipseFind(sigsqx,sigsqy,sigsqp,sigsqm,0,momentstruct.g_filval,fl_siga,fl_sigb,fl_pa,fl_peak,fl_counts);
    *fl_pa = -(*fl_pa);
    aconv2 = sigsq*(*fl_siga)*(*fl_siga)/(sigsq+((*fl_siga)*(*fl_siga)));
    bconv2 = sigsq*(*fl_sigb)*(*fl_sigb)/(sigsq+((*fl_sigb)*(*fl_sigb)));
    
    /* Chris S. added this sanity check */
    denom = aconv2*bconv2;
    if (denom > 0) {
      *fl_peak = momentstruct.g_filval/(2.*M_PI*sqrt(denom));
    } else {
      *fl_siga = 0;
      *fl_sigb = 0;
      *fl_pa = 0;
      *fl_peak = 0;
      *fl_counts = 0;
      *fl_xf = 0;
      *fl_yf = 0;
      return -11;
    }

    *fl_counts = 2.*M_PI*(*fl_peak)*(*fl_siga)*(*fl_sigb);
    *fl_xf = momentstruct.g_xf;
    *fl_yf = momentstruct.g_yf;
  } else {
    *fl_siga = 0;
    *fl_sigb = 0;
    *fl_pa = 0;
    *fl_peak = 0;
    *fl_counts = 0;
    *fl_xf = 0;
    *fl_yf = 0;
    return (answer-1);
  }

  return answer;

}

/*<AUTO EXTRACT>

  ROUTINE: at2ndMoments

  DESCRIPTION:
<HTML>
	This routine will calculate position angle and widths from 2nd
moments, given the centroid of an object and a REGION containing it.
</HTML>

  RETURN VALUES:
<HTML>
</HTML>
</AUTO>*/

int at2ndMoments(
	REGION *reg, 
	double row, double col, double sky, 
	int rmin, int cmin, int rmax, int cmax,
	double *pa, double *a, double *b) {

  double rrsum,rcsum,ccsum,vsum;
  int r,c;
  double value,dr,dc;
  double rr,rc,cc;
  double temp,eigen1,eigen2;

/* initialize some variables */
   rrsum = 0;
   rcsum = 0;
   ccsum = 0;
   vsum = 0;

/* loop over rows and columns */
   for (r=rmin; r<=rmax; r++) {
     for (c=cmin; c<=cmax; c++) {
       value =  shRegPixGetAsDbl(reg, r, c) - sky;
       /* compute second moment sums */
       if (value>0) {
         dr = r+0.5-row;
         dc = c+0.5-col;
         rrsum += value * dr * dr;
         rcsum += value * dr * dc;
         ccsum += value * dc * dc;
         vsum  += value;
       }
     }
   }

   if (vsum==0) {
     *pa = 0;
     *a = 0;
     *b = 0;
     return -3;
   }
/* compute second moments */
   rr = rrsum / vsum;
   rc = rcsum / vsum;
   cc = ccsum / vsum;

/* compute eigenvalues */
   temp = sqrt((rr-cc)*(rr-cc) + 4*rc*rc);
   eigen1 = (cc + rr + temp) / 2.;
   eigen2 = (cc + rr - temp) / 2.;

/* compute the position angle */
   if (rc == 0) {
        if (cc >= rr) {*pa = 0;} else {*pa = 90.;}
   } else {
        *pa = atan( (eigen1 - cc) / rc ) * 180. / M_PI;
   }

/* compute the widths */
   if (eigen1 <= 0) {
      *a = 0;
      *b = 0;
      return -1;
   } 

   *a = sqrt(eigen1);

   if (eigen2 <= 0) {
      *b = 0;
      return -2;
   } 

   *b = sqrt(eigen2);

   return 0;
  
} 

/*<AUTO EXTRACT>

  ROUTINE: atDACentroidFind

  DESCRIPTION:
<HTML>
	This routine will calculate the centroid, widths, and position
angle of an object in a REGION.  It is a wrapper around 
<A HREF=atCentroid.html#atDACentroid>atDACentroid</A>.  This routine
should produce the same results as the DA produces in the gang files.
</HTML>

  RETURN VALUES:
<HTML>
<listing>
		<0>	OK
        <-1>    center isn't locally highest pixel in atObjectCenterFind_Ptr
        <-2>    object is too close to the edge in atObjectCenterFind_Ptr
        <-3>    object has vanishing second derivative in atObjectCenterFind_Ptr
        <-4>    sky is too high in atObjectCenterFind_Ptr
        <-10>   Zero sigma - peak and counts not calculated after atObjectCenterFind_Ptr
        <-11>   Infinite sigma after atObjectCenterFind_Ptr
        <-21>   Error in setfsigma (sigma too large) in atSigmaFind
        <-22>   Sigma out of range (inf sharp or inf flat) in atSigmaFind
        <-23>   Too many iterations in atSigmaFind
        <-24>   Too close to edge in atSigmaFind
        <-25>   Error in lgausmom - check sky value in atSigmaFind
	<-26>	Too flat in atFindFocMom, from atDACentroid
        <-30>   Zero sigma - peak and counts not calculated after atSigmaFind
        <-31>   Infinite sigma after atSigmaFind
        <-40>   Sky >65535
</listing>
</HTML>
</AUTO>*/

int atDACentroidFind(REGION *reg, 
	int row0, int col0,
	double dbl_sky, 
	double dbl_sigma, 
	double dbl_dark_variance, 
	double dbl_gain, 
	double minval, double maxval, /* for scaling images to U16 */
	float *fl_rowc, float *fl_colc, 
	float *fl_drowc, float *fl_dcolc, 
	float *fl_peak, float *fl_counts,
	float *fl_siga, float *fl_sigb,
	float *fl_pa) {

  int sky, answer;
  float sigma, dark_variance, gain;
  double immean,imsigma,usemax,usemin,usefactor,total;
  int hrow,hcol,lrow,lcol;
  int r,c;
  double pix;
  REGION *newreg;

  sky = (int) dbl_sky;
  sigma = (float) dbl_sigma;
  dark_variance = (float) dbl_dark_variance;
  gain = (float) dbl_gain;

  if (minval!=maxval || reg->type!=TYPE_U16) {

    /* Get a new U16 region of the same size reg */
    newreg = shRegNew("", reg->nrow, reg->ncol, TYPE_U16);

    /* Set the pixels to the scaled values */
    if (maxval>minval) {
      usemin = minval; usemax = maxval;
    } else {
      /* Use max and min values in image to scale */
      atRegStatsFind(reg,&immean,&imsigma,&usemax,&hrow,&hcol,&usemin,&lrow,&lcol,&total);
    }
    usefactor = U16_MAX/(usemax-usemin);
    for (r=0; r<reg->nrow; r++) {
      for (c=0; c<reg->ncol; c++) {
         pix = shRegPixGetAsDbl(reg,r,c);
         if (pix>usemax) pix = usemax;
         if (pix<usemin) pix = usemin;
         shRegPixSetWithDbl(newreg,r,c,(pix-usemin)*usefactor);
      }
    }

    sky = (sky-usemin)*usefactor;

    if (sky>65535) {
       *fl_rowc=0;
       *fl_colc=0;
       if (fl_drowc != NULL) *fl_drowc = 0;
       if (fl_dcolc != NULL) *fl_dcolc = 0;
       *fl_peak=0;
       *fl_counts=0;
       *fl_siga=0;
       *fl_sigb=0;
       *fl_pa=0;
       return -40;
    }
  
    answer = atDACentroid((const U16 **)newreg->rows, newreg->nrow, newreg->ncol,
	row0, col0, sky, sigma, 
	dark_variance, gain, fl_rowc, fl_colc, fl_drowc, fl_dcolc, fl_peak, 
	fl_counts, fl_siga, fl_sigb, fl_pa);

    (*fl_peak) = (*fl_peak)/usefactor;
    (*fl_counts) = (*fl_counts)/usefactor;

    (void)shRegDel(newreg);

  } else {
  
    if (sky>65535) {
       *fl_rowc=0;
       *fl_colc=0;
       if (fl_drowc != NULL) *fl_drowc = 0;
       if (fl_dcolc != NULL) *fl_dcolc = 0;
       *fl_peak=0;
       *fl_counts=0;
       *fl_siga=0;
       *fl_sigb=0;
       *fl_pa=0;
       return -40;
    }
  
    answer = atDACentroid((const U16 **)reg->rows, reg->nrow, reg->ncol,
	row0, col0, sky, sigma, 
	dark_variance, gain, fl_rowc, fl_colc, fl_drowc, fl_dcolc, fl_peak, 
	fl_counts, fl_siga, fl_sigb, fl_pa);

  }

  return answer;

}
