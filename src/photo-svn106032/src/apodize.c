/*
 * code to handle apodized apertures
 */
#include <math.h>
#include <string.h>
#include <alloca.h>
#include "dervish.h"
#include "atConversions.h"		/* for M_PI, M_SQRT2 */
#include "phExtract.h"
#include "phMath.h"

/*
 * here are the coefficients
 */
#include "apo_coeffs.c"

/*****************************************************************************/
/*
 * calculate a normalised Gaussian, N(dx, sigma^2)
 */
static void
set_gaussian_filter(float *f,		/* filter to set */
		    int nf,		/* filter is f[-nf/2] ... f[nf/2] */
		    float sigma,	/* desired sigma */
		    float dx)		/* centre of Gaussian */
{
   int i;
   float sum = 0;
/*
 * calculate filter...
 */
   for(i = -nf/2; i <= nf/2; i++) {
      f[i] = exp(-0.5*pow((i + dx)/sigma, 2));
      sum += f[i];
   }
/*
 * ... and normalise
 */
   for(i = -nf/2; i <= nf/2; i++) {
      f[i] /= sum;
   }
}
/*****************************************************************************/
/*
 * calculate a normalised sinc filter
 */
static void
set_sinc_filter(float *f,		/* filter to set */
		int nf,			/* filter is f[-nf/2] ... f[nf/2] */
		float dx)		/* amount to shift */
{
   int i;
   float sum = 0;
/*
 * calculate filter...
 */
   for(i = -nf/2; i <= nf/2; i++) {
      if(i - dx == 0.0) {
	 f[i] = M_PI;
      } else {
	 f[i] = sin(M_PI*(i - dx))/(i - dx);
      }
      f[i] *= (1 + cos(M_PI*(i - dx)/(float)(nf/2))); /* cosbell the filter */
      sum += f[i];
   }
/*
 * ... and normalise
 */
   for(i = -nf/2; i <= nf/2; i++) {
      f[i] /= sum;
   }
}

/*****************************************************************************/
/*
 * convolve in with fx in the column direction and fy in the row direction
 *
 * NOTE that this convolution is NOT optimised for speed! It should not be
 * applied unthinkingly to large images
 */
static void
convolve(float **data,
	 float **scr,
	 const float *fx,
	 const float *fy,
	 int ncoeff,
	 int nf)
{
   int i, j, k;
   double sum;
/*
 * clear scratch array
 */
   for(i = -ncoeff/2 - nf; i <= ncoeff/2 + nf; i++) {
      memset(&scr[i][-ncoeff/2 - nf], '\0', (ncoeff + 2*nf)*sizeof(float));
   }
/*
 * convolve in x
 */
   for(i = -ncoeff/2 - nf/2; i <= ncoeff/2 + nf/2; i++) {
      for(j = -ncoeff/2 - nf/2; j <= ncoeff/2 + nf/2; j++) {
	 sum = 0;
	 for(k = -nf/2; k <= nf/2; k++) {
	    sum += data[i][j + k]*fx[k];
	 }
	 scr[i][j] = sum;
      }
   }
/*
 * then in y
 */
   for(i = -ncoeff/2 - nf/2; i <= ncoeff/2 + nf/2; i++) {
      for(j = -ncoeff/2 - nf/2; j <= ncoeff/2 + nf/2; j++) {
	 sum = 0;
	 for(k = -nf/2; k <= nf/2; k++) {
	    sum += scr[i + k][j]*fy[k];
	 }
	 data[i][j] = sum;
      }
   }
}

/*****************************************************************************/
#define DEBUG 0

#if DEBUG
static void
write_flt_array_as_region(float **arr,
			  int n,
			  const char *file)
{
   int i,j;
   REGION *reg = shRegNew("", n, n, TYPE_FL32);

   shAssert(n%2 == 1);

   for(i = -n/2; i <= n/2; i++) {
      for(j = -n/2; j <= n/2; j++) {
	 reg->rows_fl32[i + n/2][j + n/2] = arr[i][j];
      }
   }

   shRegWriteAsFits(reg,(char *)file,STANDARD,2,DEF_NONE,NULL,0);
   shRegDel(reg);
}
#endif

/*****************************************************************************/

static REGION *
apodize_coeffs(const COEFF *coeff,	/* coefficients to apodize */
	       float asigma,		/* use an N(d[rc], asigma^2) */
	       float dr, float dc)	/*               apodizing filter */
{
   REGION *acoeff;			/* apodized coefficients */
   float **c, *c_s;			/* augmented array */
   float *fx, *fy;			/* smoothing filters */
   int i,j;
   float max;				/* largest coefficient */
   int ncoeff;				/* number of coeffs in each dimension*/
   int n;				/* size of augmented array */
   int nf;				/* size of smoothing filter */
   int row, col;			/* row/column counters */
   float **scr, *scr_s;			/* scratch array */

   shAssert(coeff != NULL && coeff->cmax == coeff->rmax)
   shAssert(coeff->rmin == -coeff->rmax && coeff->cmin == -coeff->cmax);
   ncoeff = 2*coeff->cmax + 1;

   nf = 1 + 2*(int)(3*asigma + 0.5 + fabs(fabs(dc) > fabs(dr) ? dc : dr));
   if(nf < 9) {
      nf = 9;
   }
   n = ncoeff + 2*nf;
   shAssert(n%2 == 1 && nf%2 == 1);
/*
 * set up augmented array; note that origin of coeff is c[0][0]
 */
   fx = alloca(2*nf*sizeof(float)); fy = fx + nf;
   fx += nf/2; fy += nf/2;

   c = alloca(2*n*sizeof(float *)); scr = c + n;
   c_s = alloca(2*n*n*sizeof(float)); scr_s = c_s + n*n;
   memset(c_s, '\0', 2*n*n*sizeof(float));
   shAssert(c_s[0] == 0.0);		/* check that 0.0 is all bits 0 */
   for(i = 0;i < n; i++) {
      c[i] = c_s + i*n + n/2;
      scr[i] = scr_s + i*n + n/2;
   }
   c += n/2; scr += n/2;
/*
 * fill out c
 */
   for(i = 0;i < ncoeff;i++) {
      row = i + coeff->rmin;
      for(j = 0;j < coeff->run[i].n;j++) {
	 col = j + coeff->run[i].col0;
	 c[row][col] = c[-row][col] = c[row][-col] = c[-row][-col] =
						      coeff->run[i].coeffs[j];
      }
   }
/*
 * set filters and convolve with them
 */
   if(asigma < 0.5) {			/* we can't smooth and shift at once,
					   so do a sinc shift first */
      if(dr != 0.0 && dc != 0.0) {
	 set_sinc_filter(fx, nf, dc);
	 set_sinc_filter(fy, nf, dr);
      
	 convolve(c, scr, fx, fy, ncoeff, nf);
	 dr = dc = 0.0;
      }
   }

   if(asigma > 0.0) {
      set_gaussian_filter(fx, nf, asigma, dc);
      set_gaussian_filter(fy, nf, asigma, dr);
      
      convolve(c, scr, fx, fy, ncoeff, nf);
   }
/*
 * Find the largest coefficient
 */
   max = 0;
   for(i = -ncoeff/2 - nf/2 + 1; i < ncoeff/2 + nf/2; i++) {
      for(j = -ncoeff/2 + nf/2 + 1; j < ncoeff/2 + nf/2; j++) {
	 if(c[i][j] > max) {
	    max = c[i][j];
	 }
      }
   }
   shAssert(max > 0.0);
/*
 * Set the maximum value to 1.0, and pack results into acoeff
 */
   n = ncoeff + 2*(int)(nf/2);		/* all the values that we need */
   acoeff = shRegNew("apodized coeffs", n, n, TYPE_FL32);

   for(i = -ncoeff/2 - nf/2; i <= ncoeff/2 + nf/2; i++) {
      for(j = -ncoeff/2 + nf/2; j <= ncoeff/2 + nf/2; j++) {
	 acoeff->rows_fl32[i + n/2][j + n/2] = c[i][j]/max;
      }
   }
/*
 * write those coeffs for debugging
 */
#if DEBUG
   write_flt_array_as_region(c, ncoeff + 2*nf, "acoeff.fts");
#endif
   return(acoeff);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Evaluate the flux with an apodized aperture
 */
int
phApoApertureEval(const REGION *reg,	/* input data */
		  int ann,		/* the desired annulus */
		  float asigma,		/* desired apodising sigma */
		  float bkgd,		/* the REGION's background level */
		  float rowc, float colc, /* central pixel of object */
		  float *val)		/* the answer */
{
   REGION *acoeff;			/* apodised coefficients */
   float *arow;				/* row pointer in acoeff */
   int hsize;				/* "halfsize"; == acoeff->nrow/2 */
   int i, j;
   int irowc, icolc;			/* central pixel */
   PIX *row;				/* row pointer in reg */
   double sum;

   if(ann < 0 || ann >= sizeof(apo_coeffs)/sizeof(apo_coeffs[0])) {
      shErrStackPush("phApoApertureEval: invalid aperture %d", ann);
      return(-1);
   }
/*
 * apodize and shift coefficients to desired sub-pixel position
 *
 * First find central pixel of aperture (Remember that 20.5 is
 * the centre of pixel 20)
 */
   irowc = (int)rowc; icolc = (int)colc;
   acoeff = apodize_coeffs(&apo_coeffs[ann], asigma,
						   rowc - irowc, colc - icolc);
   shAssert(acoeff->nrow == acoeff->ncol && acoeff->nrow%2 == 1);
/*
 * check that aperture isn't too close to edge
 */
   hsize = acoeff->nrow/2;
   if(irowc - hsize < 0 || irowc + hsize >= reg->nrow ||
      icolc - hsize < 0 || icolc + hsize >= reg->ncol) {
      shErrStackPush("phApoApertureEval: "
		     "aperture is too close to edge of region");
      return(-1);
   }
/*
 * multiply data by those coefficients, and sum result
 */
   sum = 0;
   for(i = -hsize; i <= hsize; i++) {
      arow = &acoeff->rows_fl32[i + hsize][hsize];
      row = &reg->ROWS[i + irowc][icolc];
      for(j = -hsize; j <= hsize; j++) {
	 sum += arow[j]*(row[j] - bkgd);
      }
   }
   
   *val = sum;
   
   shRegDel(acoeff);

   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return the expectation value of the row and column position of an
 * object, evaluated within an apodized aperture. The origin is the
 * origin of the region
 */
int
phApoApertureMomentsEval(const REGION *reg, /* input data NOTUSED */
			 int ann,	/* the desired annulus NOTUSED */
			 float asigma,	/* desired apodising sigma NOTUSED */
			 float bkgd,	/* the REGION's background level NOTUSED */
			 float rowc, float colc, /* central pixel of object NOTUSED */
			 float *m_r, float *m_c) /* <row|col> NOTUSED */
{
   return(-1);
}

/*****************************************************************************/
/*
 * find the area of a circle enclosed within a rectangle
 */
static float
pix_intersection(float r,		/* radius of circle */
		 float x1, float y1,	/* bottom left corner of rectangle */
		 float x2, float y2)	/* top right corner of rectangle */
{
   float area;				/* area of intersection */
/*
 * is area zero?
 */
   if(fabs((x2 - x1)*(y2 - y1)) < 1e-5) {
      return(0.0);
   }   
/*
 * Reduce problem to one where the rectangle is all in the same quadrant
 */
   if(x1*x2 < 0) {			/* crosses y-axis */
      return(pix_intersection(r, 0, y1, x1, y2) +
					   pix_intersection(r, 0, y1, x2, y2));
   }
   if(y1*y2 < 0) {			/* crosses x-axis */
      return(pix_intersection(r, x1, 0, x2, y1) +
					   pix_intersection(r, x1, 0, x2, y2));
   }
/*
 * the rectangle all lies within one quadrant; make it the first quadrant
 */
   if(x1 < 0) x1 = -x1;
   if(x2 < 0) x2 = -x2;
   if(y1 < 0) y1 = -y1;
   if(y2 < 0) y2 = -y2;

   if(x1 > x2) {
      float tmp = x1;
      x1 = x2; x2 = tmp;
   }
   if(y1 > y2) {
      float tmp = y1;
      y1 = y2; y2 = tmp;
   }
/*
 * If (x1, y1) is the origin, we actually have to do some work here, rather
 * than in a recursive call
 */
   if(x1 == 0.0 && y1 == 0.0) {
      float xin, yin;			/* intersection of sides with circle */
      if(x2*x2 + y2*y2 <= r*r) {	/* rectangle lies within circle */
	 return(x2*y2);
      }
      if(x2 > r) x2 = r;
      if(y2 > r) y2 = r;

      xin = sqrt(r*r - y2*y2);		/* x-intersection with top of rect */
      yin = sqrt(r*r - x2*x2);		/* y-intersection with right of rect */

      return(0.5*(x2*yin + y2*xin + r*r*(atan2(y2, xin) - atan2(yin, x2))));
   }
/*
 * It isn't so simple; reduce problem to the one we just solved
 *
 * rectangle intersects circle. Reduce problem to rectangles with a corner
 * at the centre of the circle
 */
   area = pix_intersection(r, 0.0, 0.0, x2, y2);
   area -= pix_intersection(r, 0.0, 0.0, x1, y2);
   area -= pix_intersection(r, 0.0, 0.0, x2, y1);
   area += pix_intersection(r, 0.0, 0.0, x1, y1); /* we subtracted it twice */

   return(area);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * This is a naive routine to calculate the flux within an aperture, based
 * solely on the fraction of each pixels enclosed within the aperture, or
 * on an apodized filter based on the positions of the centre of the pixels
 *
 * It is _not_ to be used in production code --- its only purpose in
 * life is to enable me to do Monte Carlo simulations of the MT photometry
 *
 * If asigma < 0, use a 0/1 weight depending on whether the pixel centre
 * lies in the aperture
 *
 * If asigma == 0.0, use an aperture whose weights are the fraction of the
 * pixel within r pixels of the desired centre
 *
 * If asigma > 0, apodize aperture based on pixel centres, using an
 * N(0,asigma^2) Gaussian
 */
int
phApoApertureEvalNaive(const REGION *reg, /* input data */
		       float r,		/* the aperture's radius */
		       float asigma,	/* desired apodising sigma */
		       float bkgd,	/* the REGION's background level */
		       float rowc, float colc, /* central pixel of object */
		       float *val)	/* desired value */
{
   const int ir = r + 4*fabs(asigma) + 1.0;
   int i, j;
   float rp;				/* radial distance to centre of pixel
					   from centre of aperture */
   double sum;				/* sum of flux */
   float w;				/* pixel's weight */
   float y1, x1;			/* row- and col- LL corner of pixel */

   shAssert(r >= 0.0 && reg != NULL && reg->type == TYPE_PIX);

   sum = 0;
   for(i = rowc - ir; i <= rowc + ir; i++) {
      if(i < 0 || i >= reg->nrow) {
	 continue;
      }
      y1 = i - (rowc - 0.5) - 0.5;
      for(j = colc - ir; j <= colc + ir; j++) {
	 if(j < 0 || j >= reg->ncol) {
	    continue;
	 }
	 x1 = j - (colc - 0.5) - 0.5;

	 rp = sqrt((y1 + 0.5)*(y1 + 0.5) + (x1 + 0.5)*(x1 + 0.5));
	 if(asigma < 0) {
	    w = (rp < r) ? 1 : 0;
	 } else if(asigma == 0.0) {
	    w = pix_intersection(r, x1, y1, x1 + 1.0, y1 + 1.0);
	 } else {
	    w = 0.5*(1 - erf((rp - r)/(M_SQRT2*asigma)));
	 }

	 sum += w*(reg->ROWS[i][j] - bkgd);
      }
   }

   *val = sum;
   
   return(0);
}
