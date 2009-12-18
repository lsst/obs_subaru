/*
 * <INTRO>
 * Carry out near-optimal convolutions
 *
 * Convolve an input region, <in>, with a separable filter to produce <out>.
 * The filter is specified by the two float arrays filter_r (size: filtsize_r)
 * and filter filter_c (size: filtsize_c). The filter_r array specifies the
 * filter to be applied in the row direction (it extends across columns);
 * the filter_c array is applied up the columns (it extends across rows)
 */
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <alloca.h>
#include "dervish.h"
#include "phSkyUtils.h"
#include "phUtils.h"
#include "phConsts.h"

#define CSHIFT 14			/* amount to shift smoothing filters */

/*****************************************************************************/
/*
 * The actual working functions
 */
static void convolve_lut(REGION *out, const REGION *in, REGION *scr,
			int filtsize_r, int filtsize_c,
			float *filter_r, float *filter_c, int lshift);
static void convolve_mult(REGION *out, const REGION *in, REGION *scr,
			 int filtsize_r, int filtsize_c,
			 float *filter_r, float *filter_c, int lshift);
static void convolve_mult_fl32(REGION *out, const REGION *in, REGION *scr,
			       int filtsize_r, int filtsize_c,
			       const float *filter_r, const float *filter_c,
			       float scale);

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Convolve an input region, <in>, with a separable filter to produce <out>.
 * The filter is specified by the two float arrays filter_r (size: filtsize_r)
 * and filter filter_c (size: filtsize_c). The filter_r array specifies the
 * filter to be applied in the row direction (it extends across columns);
 * the filter_c array is applied up the columns (it extends across rows).
 *
 * Note that this definition of row filter can be confusing.
 *
 * The regions are assumed to all be U16 or FL32 (in which case only
 * CONVOLVE_MULT is supported); this is asserted by the code.
 * The filter is assumed to be normalisable for U16 regions
 *
 * We use a number of different algorithms, depending on the size of the
 * filter and region to be convolved; you can either ask the code to
 * choose, or explicitly choose one yourself
 *
 * Return value is SH_GENERIC_ERROR if the requested algorithm is unknown
 * or unsupported, otherwise it's SH_SUCCESS
 */
int
phConvolve(REGION *out,			/* output region; can be same as in */
	   const REGION *in,		/* region to smooth */
	   REGION *scr,			/* scratch region */
	   int filtsize_r,		/* column- and row- size of filters */
	   int filtsize_c,
	   float *filter_r,		/* column- and row- filters */
	   float *filter_c,		/*  !read header comment about this! */
	   int lshift,			/* how much to << the answer */
	   CONVOLVE_ALG alg)		/* algorithm to use */
{
   shAssert(lshift >= 0);
   
   if(alg == CONVOLVE_ANY) {		/* choose best algorithm */
      if(in->type == TYPE_FL32) {
	 alg = CONVOLVE_MULT;
      } else if(in->ncol*in->nrow > 1.5*MAX_U16) {
	 alg = CONVOLVE_LUT;
      } else {
	 alg = CONVOLVE_MULT;
      }
   }
   
   if(alg == CONVOLVE_LUT) {
      (void)convolve_lut(out,in,scr,
			 filtsize_r,filtsize_c,filter_r,filter_c,lshift);
   } else if(alg == CONVOLVE_MULT) {
      convolve_mult(out,in,scr,filtsize_r,filtsize_c,filter_r,filter_c,lshift);
   } else if(alg == CONVOLVE_PLUT) {
      shErrStackPush("phConvolve: CONVOLVE_PLUT is not yet supported");
      return(SH_GENERIC_ERROR);
   } else {
      shErrStackPush("phConvolve: unknown algorithm %d",(int)alg);
      return(SH_GENERIC_ERROR);
   }
/*
 * Copy the regions around the edge that are half a filter wide, if in != out
 */
   if(in != out) {			/* we have somehing to do */
      int c,r;

      if(lshift == 0) {
	 if(in->type == TYPE_U16) {
	    if(out->type == TYPE_U16) {
	       for(r = 0;r < filtsize_c/2;r++) {
		  memcpy((void *)out->rows_u16[r], in->rows_u16[r],
							 in->ncol*sizeof(U16));
	       }
	       for(;r < in->nrow - filtsize_c/2;r++) {
		  memcpy((void *)out->rows_u16[r],in->rows_u16[r],
			 filtsize_r/2*sizeof(U16));
		  memcpy((void *)&out->rows_u16[r][in->ncol - filtsize_r/2],
			 (const void *)&in->rows_u16[r][in->ncol-filtsize_r/2],
			 filtsize_r/2*sizeof(U16));
	       }
	       for(;r < in->nrow;r++) {
		  memcpy((void *)out->rows_u16[r], in->rows_u16[r],
							 in->ncol*sizeof(U16));
	       }
	    } else if(out->type == TYPE_FL32) {
	       U16 *iptr;
	       FL32 *optr;

	       for(r = 0;r < filtsize_c/2;r++) {
		  iptr = in->rows_u16[r];
		  optr = out->rows_fl32[r];
		  for(c = in->ncol - 1; c >= 0; c--) {
		     optr[c] = iptr[c];
		  }
	       }
	       for(;r < in->nrow - filtsize_c/2;r++) {
		  iptr = in->rows_u16[r];
		  optr = out->rows_fl32[r];
		  for(c = filtsize_r/2 - 1; c >= 0; c--) {
		     optr[c] = iptr[c];
		  }
		  
		  iptr = &in->rows_u16[r][in->ncol - filtsize_r/2];
		  optr = &out->rows_fl32[r][in->ncol - filtsize_r/2];
		  for(c = filtsize_r/2 - 1; c >= 0; c--) {
		     optr[c] = iptr[c];
		  }
	       }
	       for(;r < in->nrow;r++) {
		  iptr = in->rows_u16[r];
		  optr = out->rows_fl32[r];
		  for(c = in->ncol - 1; c >= 0; c--) {
		     optr[c] = iptr[c];
		  }
	       }
	    } else {			/* an assertion should have failed */
	       shFatal("phConvolve(): impossible type combination");
	    }
	 } else if(in->type == TYPE_FL32) {
	    for(r = 0;r < filtsize_c/2;r++) {
	       memcpy((void *)out->rows_fl32[r],in->rows_fl32[r],
		      in->ncol*sizeof(FL32));
	    }
	    for(;r < in->nrow - filtsize_c/2;r++) {
	       memcpy((void *)out->rows_fl32[r],in->rows_fl32[r],
		      filtsize_r/2*sizeof(FL32));
	       memcpy((void *)&out->rows_fl32[r][in->ncol - filtsize_r/2],
		      (const void *)&in->rows_fl32[r][in->ncol - filtsize_r/2],
		      filtsize_r/2*sizeof(FL32));
	    }
	    for(;r < in->nrow;r++) {
	       memcpy((void *)out->rows_fl32[r],in->rows_fl32[r],
		      in->ncol*sizeof(FL32));
	    }
	 } else {
	    shFatal("phConvolve: You cannot get here");
	 }
      } else {
	 if(in->type == TYPE_U16) {
	    if(out->type == TYPE_U16) {
	       int ncol = in->ncol;		/* unalias */
	       U16 *orow, *irow;
	       
	       for(r = 0;r < filtsize_c/2;r++) {
		  orow = out->rows_u16[r];
		  irow = in->rows_u16[r];
		  for(c = 0;c < ncol;c++) {
		     orow[c] = irow[c] << lshift;
		  }
	       }
	       for(;r < in->nrow - filtsize_c/2;r++) {
		  orow = out->rows_u16[r];
		  irow = in->rows_u16[r];
		  for(c = 0;c < filtsize_r/2;c++) {
		     orow[c] = irow[c] << lshift;
		  }
		  
		  orow = &out->rows_u16[r][in->ncol - filtsize_r/2];
		  irow = &in->rows_u16[r][in->ncol - filtsize_r/2];
		  for(c = 0;c < filtsize_r/2;c++) {
		     orow[c] = irow[c] << lshift;
		  }
	       }
	       for(;r < in->nrow;r++) {
		  orow = out->rows_u16[r];
		  irow = in->rows_u16[r];
		  for(c = 0;c < ncol;c++) {
		     orow[c] = irow[c] << lshift;
		  }
	       }
	    } else {			/* an assertion should have failed */
	       shFatal("phConvolve(): impossible type combination");
	    }
	 } else if(in->type == TYPE_FL32) {
	    if(out->type == TYPE_FL32) {
	       int ncol = in->ncol;		/* unalias */
	       FL32 *orow, *irow;
	       const float scale = (1 << lshift);
	       
	       for(r = 0;r < filtsize_c/2;r++) {
		  orow = out->rows_fl32[r];
		  irow = in->rows_fl32[r];
		  for(c = 0;c < ncol;c++) {
		     orow[c] = irow[c]*scale;
		  }
	       }
	       for(;r < in->nrow - filtsize_c/2;r++) {
		  orow = out->rows_fl32[r];
		  irow = in->rows_fl32[r];
		  for(c = 0;c < filtsize_r/2;c++) {
		     orow[c] = irow[c]*scale;
		  }
		  
		  orow = &out->rows_fl32[r][in->ncol - filtsize_r/2];
		  irow = &in->rows_fl32[r][in->ncol - filtsize_r/2];
		  for(c = 0;c < filtsize_r/2;c++) {
		     orow[c] = irow[c]*scale;
		  }
	       }
	       for(;r < in->nrow;r++) {
		  orow = out->rows_fl32[r];
		  irow = in->rows_fl32[r];
		  for(c = 0;c < ncol;c++) {
		     orow[c] = irow[c]*scale;
		  }
	       }
	    } else {			/* an assertion should have failed */
	       shFatal("phConvolve(): impossible type combination");
	    }
	 } else {
	    shFatal("phConvolve: You cannot get here either");
	 }
      }
   }

   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Like phConvolve, but for the special case where the filter is Gaussian
 */
int
phConvolveWithGaussian(
	 REGION *out,			/* output region; can be same as in */
	 const REGION *in,		/* region to smooth */
	 REGION *scr,			/* scratch region */
	 int filtsize,			/* column- and row- size of filters */
	 float sigma,			/* sigma in pixels for Gaussian */
	 int lshift,			/* how many bits to << the answer */
	 CONVOLVE_ALG alg)		/* algorithm to use */
{
   float *filter;			/* PSF smoothing filter to use */
   int i;
   float sum;

   shAssert(sigma > 0.0 && lshift >= 0);
      
   filter = alloca(filtsize*sizeof(float));
   sum = 0;
   for(i = 0;i != filtsize;i++) {
      filter[i] = exp(-pow((i - filtsize/2)/sigma,2)/2);
      sum += filter[i];
   }
   for(i = 0;i != filtsize;i++) {
      filter[i] /= sum;
   }

   return(phConvolve(out,in,scr,filtsize,filtsize,filter,filter,lshift,alg));
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Convolve a region in place with the Laplacian of a Gaussian; the
 * argument list is identical to that of phConvolveWithGaussian,
 * except that an extra scratch buffer is required.
 *
 * Note that for a Gaussian (and only for a Gaussian),
 *
 *	del^2(G) = 1/sigma dG/dsigma
 *
 * N.B. This function is not currently used, and is not guaranteed to 
 * be normalised correctly
 */
int
phConvolveWithLaplacian(REGION *out,	/* output region; can be same as in */
			REGION *in,	/* region to smooth */
			REGION *scr,	/* scratch region */
			REGION *scr2,	/* second scratch region */
			int filtsize,	/* column- and row- size of filters */
			float sigma,	/* sigma in pixels for Gaussian */
			int lshift,	/* how much to << the answer */
			CONVOLVE_ALG alg) /* algorithm to use */
{
   float a = 1000;			/* add this constant to del^2 */
   float b;				/* coeff for linear combination */
   const float lambda = 0.05;		/* estimate the derivative dG/dsigma
					   using sigma +- lambda */
   
   if(phConvolveWithGaussian(scr2,in,scr,filtsize,sigma*(1+lambda),lshift,alg)
							       != SH_SUCCESS) {
      return(SH_GENERIC_ERROR);
   }
   if(phConvolveWithGaussian(out,in,scr,filtsize,sigma*(1-lambda),lshift,alg)
							       != SH_SUCCESS) {
      return(SH_GENERIC_ERROR);
   }

   b = (1 << lshift)/(2*lambda*sigma);
   if(shRegIntLincom(out, scr2, a, b, -b, LINCOM_EXACT) == NULL) {
      return(SH_GENERIC_ERROR);
   } else {
      return(SH_SUCCESS);
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Like phConvolve, but for the special case where the filter is the
 * product of powers of row and column times a Gaussian
 *
 * The regions are required to be U16 or FL32
 */
int
phConvolveWithGaussianPowers(REGION *out, /* output region; can be same as in*/
			     const REGION *in, /* region to smooth */
			     REGION *scr, /* scratch region */
			     int filtsize, /* size of filters */
			     float sigma, /* sigma in pixels for Gaussian */
			     int ir,	/* order in row direction */
			     int ic)	/* order in column direction */
{
   float delta;				/* == (z - centre_z)/sigma */
   float *filter_r, *filter_c;		/* PSF smoothing filters to use */
   int i;
   float isigma;			/* == 1/sigma */

   shAssert(sigma > 0.0);
   isigma = 1/sigma;
      
   filter_r = alloca(2*filtsize*sizeof(float));
   filter_c = filter_r + filtsize;
   
   for(i = 0;i != filtsize;i++) {
      delta = (i - filtsize/2)*isigma;
      filter_r[i] = pow(delta, ir)*exp(-0.5*delta*delta);
   }

   for(i = 0;i != filtsize;i++) {
      delta = (i - filtsize/2)*isigma;
      filter_c[i] = pow(delta, ic)*exp(-0.5*delta*delta);
   }

   return(phConvolve(out, in, scr, filtsize, filtsize, filter_c, filter_r,
		     0, CONVOLVE_MULT));
}

/*****************************************************************************/
/*
 * Here beginneth the various convolution implementations
 */
/*****************************************************************************/
/*
 * The slowest part of this operation is doing the multiplies, but it is
 * clearly true that for small filters and images that are almost all very
 * close to sky, the number of discrete values to be multiplied is small,
 * so we can simply build a look up table for them. These lookup tables
 * are returned as the value of the functions, along with the filter values
 *
 * We know that once we look up a given pixel in the lookup table for a given
 * filter coefficient, we will look up all of the others too. This means that
 * we can gain on cache efficiency by grouping the looked-up values together.
 * This is done by allocating the LUT in units of LUT_ENTRY, and offseting
 * the bases of the LUTs for different values of the filter. For example,
 * if N_BLOCK is 4, and the first 4 filter values are (a,b,c,d), the layout
 * of the LUT in memory is:
 *
 *  |a*0:b*0:c*0:d*0|a*1:b*1:c*1:d*1|a*1:b*1:c*1:d*1|...
 *  ^    ^   ^   ^
 *  |    |   |   |
 * lut0  |   |   |
 *      lut1 |	 |
 *          lut2 |
 *              lut3
 * where lut2 is lut[2]; the lut corresponding to filter[2]. This means
 * that value 1*b is calculable as lut[2][1].i0 (it's also lut[0][1].tmp[2],
 * but you never need or want to call it that).
 *
 * If there are more than N_BLOCK distinct coefficients, we have to allocate
 * another 65536*NBLOCK*sizeof(int) bytes of lookup table. We offset these
 * block by some arbitrary amount (currently 1000by) to avoid cache conflicts.
 */
typedef struct {
   int i0;
   int tmp[3];
} LUT_ENTRY;
#define N_BLOCK (sizeof(LUT_ENTRY)/sizeof(int))

static LUT_ENTRY *lut_s = NULL;	/* storage for look up tables */
static int nlut_s = 0;			/* number of LUTs allocated */

static void convolve_lut_rows(const REGION *,
			      REGION *, int, LUT_ENTRY **, int lshift);
static void convolve_lut_columns(const REGION *,
				 REGION *, int, int, LUT_ENTRY **, int lshift);

/*****************************************************************************/
/*
 * We get terrible memory fragmentation if we don't allocate these LUTs
 * once per run
 */
void
phInitConvolveLUT(int nlut)
{
   if(nlut_s >= nlut) {			/* nothing to do */
      return;
   }

   shFree(lut_s);
   lut_s = shMalloc(nlut*((MAX_U16 + 1)*sizeof(LUT_ENTRY) + 1000));
   nlut_s = nlut;
}

void
phFiniConvolveLUT(void)
{
   shFree(lut_s); lut_s = NULL;
   nlut_s = 0;
}

/*
 * convert a filter from float to int, normalised to (1<<CSHIFT)
 */
static LUT_ENTRY**
set_filter_lut(
	   int *filter,			/* output filter */
	   float *filter_in,		/* input filter */
	   int filtsize			/* size of filter */
	   )
{
   int delta;				/* discrepancy in normalisation */
   int i,j;
   LUT_ENTRY **lut;			/* look up tables */
   int sum = 0;				/* sum of int filter */
   float sum_in = 0.0;			/* sum of filter_in */

   for(i = 0;i != filtsize;i++) {
      filter[i] = (1 << CSHIFT)*filter_in[i] + 0.5;
      sum_in += filter_in[i];
      sum += filter[i];
   }
/*
 * adjust the normalisation for rounding errors by adding or subtracting one
 * from the central values of the filter. If the discrepancy is even add the
 * extra value to the central value.
 */
   delta = sum_in*(1 << CSHIFT) - sum;

   if(delta == 0) {
      ;
   } else if(delta > 0) {
      filter[filtsize/2]++; delta--;
      if(delta%2 == 1) {
	 filter[filtsize/2]++; delta--;
      }
      for(i = 1;delta > 0 && i <= filtsize/2;i++, delta -= 2) {
	 filter[filtsize/2 + i]++;
	 filter[filtsize/2 - i]++;
      }
   } else {
      filter[filtsize/2]--; delta++;
      if((-delta)%2 == 1) {
	 filter[filtsize/2]++; delta--;
      }
      for(i = 1;delta < 0 && i <= filtsize/2;i++, delta += 2) {
	 filter[filtsize/2 + i]--;
	 filter[filtsize/2 - i]--;
      }
   }

   shAssert(delta == 0);
/*
 * Now build the LUTs. The malloc'd address is &lut[-1]
 */
   lut = shMalloc((filtsize + 1)*sizeof(int *));
   lut++;
/*
 * Is the filter symmetrical? If so, we only need allocate and fill half
 * of the LUTs, saving memory and cache hits
 */
   for(i = 0;i < filtsize/2;i++) {
      if(filter[i] != filter[filtsize - 1 - i]) {
	 break;
      }
   }
/*
 * allocate the LUTs and fill them up
 */
   if(i == filtsize/2) {		/* filter is symmetrical */
      int nlut = ((filtsize/2 + 1) + (N_BLOCK - 1))/N_BLOCK;

      if(nlut > nlut_s) {
	 phInitConvolveLUT(nlut);
      }
      lut[-1] = lut_s;
      for(i = 0;i <= filtsize/2;i += N_BLOCK) {
	 lut[i] = lut[-1] + i/N_BLOCK*((MAX_U16 + 1) + 1000/sizeof(LUT_ENTRY));
	 lut[filtsize - 1 - i] = lut[i];

	 for(j = 0;j < N_BLOCK && i + j <= filtsize/2;j++) {
	    int k;
	    LUT_ENTRY *lutptr;
	    int val = filter[i + j];

	    if(j > 0) {
	       lut[i + j] = (void *)((int *)lut[i] + j);
	       lut[filtsize - 1 - (i + j)] = lut[i + j];
	    }

	    lutptr = lut[i + j];
	    for(k = 0;k <= MAX_U16;k++) {
	       lutptr[k].i0 = k*val;
	    }
	 }
      }
   } else {
      int nlut = (filtsize + (N_BLOCK - 1))/N_BLOCK;

      if(nlut > nlut_s) {
	 phInitConvolveLUT(nlut);
      }
      lut[-1] = lut_s; 

      for(i = 0;i < filtsize;i += N_BLOCK) {
	 lut[i] = lut[-1] + i/N_BLOCK*((MAX_U16 + 1) + 1000/sizeof(LUT_ENTRY));

	 for(j = 0;j < N_BLOCK && i + j < filtsize;j++) {
	    int k;
	    LUT_ENTRY *lutptr;
	    int val = filter[i + j];

	    if(j > 0) {
	       lut[i + j] = (void *)((int *)lut[i] + j);
	    }

	    lutptr = lut[i + j];
	    for(k = 0;k <= MAX_U16;k++) {
	       lutptr[k].i0 = k*val;
	    }
	 }
      }
   }

   return(lut);
}

static void
free_lut(LUT_ENTRY **lut)
{
   lut--;
   shFree(lut);
}


static void
convolve_lut(REGION *out,		/* output region; can be same as in */
	     const REGION *in,		/* region to smooth */
	     REGION *scr,		/* scratch region */
	     int filtsize_r,		/* column- and row- size of filters */
	     int filtsize_c,
	     float *filter_r,		/* column- and row- filters */
	     float *filter_c,
	     int lshift)		/* how much to << the answer */
{
   int *filter = alloca(((filtsize_r > filtsize_c) ?
			 filtsize_r : filtsize_c)*sizeof(int));
   LUT_ENTRY **lut;				/* LUTs for multiplies */
   int lshift2 = lshift/2;		/* half the desired lshift */

   shAssert(in->type == TYPE_U16);
   shAssert(out->type == TYPE_U16);
   shAssert(in->nrow == out->nrow && in->ncol == out->ncol);
   shAssert(scr->type == TYPE_U16);
   shAssert(in->nrow == scr->nrow && in->ncol == scr->ncol);
   shAssert(in->rows_u16[0] != scr->rows_u16[0]);
   shAssert(scr->rows_u16[0] != out->rows_u16[0]);
   shAssert(filtsize_r%2 == 1 && filtsize_c%2 == 1);
/*
 * Set up int filter, and do the rows
 */
   lut = set_filter_lut(filter,filter_r,filtsize_r);
   convolve_lut_rows(in,scr,filtsize_r,lut,lshift2);
/*
 * Set up other int filter, and do the columns. We may be able to reuse
 * the same filter
 */
   if(filtsize_c != filtsize_r) {
      free_lut(lut); lut = NULL;
   } else {
      int i;
      
      for(i = 0;i < filtsize_c;i++) {
	 if(filter_c[i] != filter_r[i]) {
	    break;
	 }
      }
      if(i != filtsize_c) {		/* we broke out; filters differ */
	 free_lut(lut); lut = NULL;
      }
   }
   
   if(lut == NULL) {
      lut = set_filter_lut(filter,filter_c,filtsize_c);
   }
   convolve_lut_columns(scr,out,filtsize_r,filtsize_c,lut,lshift - lshift2);
   free_lut(lut);
}

/*
 * convolve across the rows
 */
static void
convolve_lut_rows(const REGION *in,	/* region to smooth */
		  REGION *out,		/* result */
		  int filtsize_r,	/* size of filter */
		  LUT_ENTRY **lut,	/* multiplication LUT */
		  int lshift)		/* how much to << answer */
{
   int cmax;				/* max value of column counter, c */
   int cmax4;				/* cmax s.t. we process 4n columns */
   int i;
   const int half = 1 << (CSHIFT - lshift - 1);	/* a half,
						   when >> (CSHIFT-lshift) */
   unsigned short *iptr;		/* pointer to in */
   unsigned short *optr;		/* pointer to out */
   unsigned short *opend;		/* end of range of optr */
   int r;				/* row counter */
   unsigned int tmp0, tmp1, tmp2, tmp3;
   LUT_ENTRY *lutptr;			/* == lut[i] */

   cmax = in->ncol - filtsize_r/2;
   cmax4 = filtsize_r/2 + 4*(int)((cmax - filtsize_r/2)/4);

   for(r = 0;r != in->nrow;r++) {
      iptr = in->rows_u16[r];
      optr = out->rows_u16[r];
      opend = optr + cmax4;
/*
 * copy first and last half-filter-widths explicitly
 */
      memcpy((void *)optr,
	     (const void *)iptr,
	     filtsize_r/2*sizeof(unsigned short));
      memcpy((void *)&optr[in->nrow - filtsize_r/2],
	     (const void *)&iptr[in->nrow - filtsize_r/2],
	     filtsize_r/2*sizeof(unsigned short));
      optr += filtsize_r/2;
/*
 * do columns in groups of 4
 */
      while(optr != opend) {
	 tmp0 = tmp1 = tmp2 = tmp3 = half;
	 i = 0;
	 do {
	    lutptr = lut[i];

	    tmp0 += lutptr[iptr[i]].i0;
	    tmp1 += lutptr[iptr[i + 1]].i0;
	    tmp2 += lutptr[iptr[i + 2]].i0;
	    tmp3 += lutptr[iptr[i + 3]].i0;
	    i++;
	 } while(i != filtsize_r);
	 tmp0 >>= CSHIFT - lshift;
	 optr[0] = (tmp0 > MAX_U16) ?
	   ((tmp0 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp0;
	 tmp1 >>= CSHIFT - lshift;
	 optr[1] = (tmp1 > MAX_U16) ?
	   ((tmp1 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp1;
	 tmp2 >>= CSHIFT - lshift;
	 optr[2] = (tmp2 > MAX_U16) ?
	   ((tmp2 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp2;
	 tmp3 >>= CSHIFT - lshift;
	 optr[3] = (tmp3 > MAX_U16) ?
	   ((tmp3 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp3;
	 iptr += 4;
	 optr += 4;
      }
/*
 * and process the leftover columns one at a time
 */
      while(optr < out->rows_u16[r] + cmax) {
	 tmp0 = half;
	 i = 0;
	 do {
	    tmp0 += lut[i][iptr[i]].i0;
	    i++;
	 } while(i != filtsize_r);
	 tmp0 >>= CSHIFT - lshift;
	 optr[0] = (tmp0 > MAX_U16) ?
	   ((tmp0 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp0;
	 iptr++;
	 optr++;
      }
   }
}

/*
 * convolve across the rows
 *
 * Use array indexes rather than pointers. Do four columns at a time
 */
static void
convolve_lut_columns(const REGION *in,	/* region to smooth */
		     REGION *out,	/* result */
		     int filtsize_r,	/* column- and row- size of filters */
		     int filtsize_c,
		     LUT_ENTRY **lut,	/* multiplication LUT */
		     int lshift)	/* how much to << answer */
{
   int i;
   unsigned short **iptr,*optr;		/* pointers to in and out */
   int c,r;				/* column and row counters */
   int cmax;				/* maximum column to process */
   int cmax4;				/* maximum value of column counter
					   that is a multiple of 4 and < ncol*/
   const int half = 1 << (CSHIFT - lshift - 1);	/* a half,
						   when >> CSHIFT - lshift */
   LUT_ENTRY *lutptr;			/* == lut[i] */
   unsigned int tmp0,tmp1,tmp2,tmp3;

   cmax = in->ncol - filtsize_r/2;
   cmax4 = filtsize_r/2 + 4*(int)((cmax - filtsize_r/2)/4);

   for(r = filtsize_c/2;r != in->nrow - filtsize_c/2;r++) {
      iptr = in->rows_u16 + (r - filtsize_c/2);
      optr = out->rows_u16[r];

      c = filtsize_r/2;
      while(c != cmax4) {
	 tmp0 = tmp1 = tmp2 = tmp3 = half;
	 i = 0;
	 do {
	    lutptr = lut[i];

	    tmp0 += lutptr[iptr[i][c]].i0;
	    tmp1 += lutptr[iptr[i][c+1]].i0;
	    tmp2 += lutptr[iptr[i][c+2]].i0;
	    tmp3 += lutptr[iptr[i][c+3]].i0;
	    i++;
	 } while(i != filtsize_c);

	 tmp0 >>= CSHIFT - lshift;
	 optr[c++] = (tmp0 > MAX_U16) ?
	   ((tmp0 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp0;
	 tmp1 >>= CSHIFT - lshift;
	 optr[c++] = (tmp1 > MAX_U16) ?
	   ((tmp1 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp1;
	 tmp2 >>= CSHIFT - lshift;
	 optr[c++] = (tmp2 > MAX_U16) ?
	   ((tmp2 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp2;
	 tmp3 >>= CSHIFT - lshift;
	 optr[c++] = (tmp3 > MAX_U16) ?
	   ((tmp3 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp3;
      }

      while(c < cmax) {
	 tmp0 = half;
	 i = 0;
	 do {
	    tmp0 += lut[i][iptr[i][c]].i0;
	    i++;
	 } while(i != filtsize_c);
	 tmp0 >>= CSHIFT - lshift;
	 optr[c++] = (tmp0 > MAX_U16) ?
	   ((tmp0 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp0;
      }
   }
}

/*****************************************************************************/
/*
 * Code an optimal convolution, using multiplies rather than LUTs
 */
static void convolve_mult_rows(const REGION *, REGION *, int *, int, int);
static void convolve_mult_columns(const REGION *, REGION *, int *,int,int,int);

/*
 * convert a filter from float to int, normalised to (1<<CSHIFT)
 */
static void
set_filter_mult(int *filter, const float *filter_in, int filtsize)
{
   int delta;				/* discrepancy in normalisation */
   int i;
   int sum = 0;				/* sum of int filter */
   float sum_in = 0.0;			/* sum of filter_in */
   
   for(i = 0;i != filtsize;i++) {
      filter[i] = (1 << CSHIFT)*filter_in[i] + 0.5;
      sum_in += filter_in[i];
      sum += filter[i];
   }
/*
 * adjust the normalisation for rounding errors by adding or subtracting one
 * from the central values of the filter. If the discrepancy is even add the
 * extra value to the central value.
 */
   sum_in *= (1 << CSHIFT);
   delta = (int)((sum_in < 0) ? -(-sum_in + 0.5) : (sum_in + 0.5)) - sum;

   if(delta == 0) {
      return;
   } else if(delta > 0) {
      if(delta%2 == 1) {
	 filter[filtsize/2]++; delta--;
      }
      for(i = 1;delta > 0 && i <= filtsize/2;i++, delta -= 2) {
	 filter[filtsize/2 + i]++;
	 filter[filtsize/2 - i]++;
      }
   } else {
      if((-delta)%2 == 1) {
	 filter[filtsize/2]--; delta++;
      }
      for(i = 1;delta < 0 && i <= filtsize/2;i++, delta += 2) {
	 filter[filtsize/2 + i]--;
	 filter[filtsize/2 - i]--;
      }
   }

   shAssert(delta == 0);
#if !defined(NDEBUG)
   for(i = sum = 0;i != filtsize;i++) {
      sum += filter[i];
   }
   delta = (int)((sum_in < 0) ? -(-sum_in + 0.5) : (sum_in + 0.5)) - sum;
   shAssert(delta == 0);
#endif
}

/*****************************************************************************/
/*
 * convolve using multiplies rather than LUTs. First the U16 version
 */
static void
convolve_mult(REGION *out,		/* output region; can be same as in */
	      const REGION *in,		/* region to smooth */
	      REGION *scr,		/* scratch region */
	      int filtsize_r,		/* column- and row- size of filters */
	      int filtsize_c,
	      float *filter_r,		/* column- and row- filters */
	      float *filter_c,
	      int lshift)		/* how much to << the answer */
{
   int *filter = alloca(((filtsize_r > filtsize_c) ?
			 filtsize_r : filtsize_c)*sizeof(int));
   int lshift2 = lshift/2;		/* half the desired lshift */

   if(in->type == TYPE_FL32 || out->type == TYPE_FL32) {
      float scale = (1 << lshift);
      convolve_mult_fl32(out, in, scr,
			 filtsize_r, filtsize_c, filter_r, filter_c, scale);
      return;
   }
   
   shAssert(in->type == TYPE_U16);
   shAssert(in->type == scr->type && in->type == out->type);
   shAssert(in->nrow == scr->nrow && in->ncol == scr->ncol);
   shAssert(in->nrow == out->nrow && in->ncol == out->ncol);
   shAssert(in->rows_u16[0] != scr->rows_u16[0]);
   shAssert(scr->rows_u16[0] != out->rows_u16[0]);
   shAssert(filtsize_r%2 == 1 && filtsize_c%2 == 1);
/*
 * Set up int filter, and do the rows
 */
   set_filter_mult(filter,filter_r,filtsize_r);
   convolve_mult_rows(in,scr,filter,filtsize_r,lshift2);
/*
 * Set up other int filter, and do the columns
 */
   set_filter_mult(filter,filter_c,filtsize_c);
   convolve_mult_columns(scr,out,filter,filtsize_r,filtsize_c,lshift-lshift2);
}

static void
convolve_mult_rows(const REGION *in,	/* region to smooth */
		   REGION *out,		/* result */
		   int *filter,		/* column filter */
		   int filtsize_r,	/* column- and row- size of filters */
		   int lshift)		/* how much to << answer */
{
   int cmax;				/* max value of column counter, c */
   int cmax4;				/* cmax s.t. we process 4n columns */
   const int half = 1 << (CSHIFT - lshift - 1);	/* a half,
						   when >> CSHIFT - lshift */
   int i;
   unsigned short *iptr;		/* pointer to in */
   unsigned short *optr;		/* pointer to out */
   unsigned short *opend;		/* end of range of optr */
   int r;				/* row counter */
   unsigned int tmp0, tmp1, tmp2, tmp3;
   
   cmax = in->ncol - filtsize_r/2;
   cmax4 = filtsize_r/2 + 4*(int)((cmax - filtsize_r/2)/4);

   for(r = 0;r != in->nrow;r++) {
      iptr = in->rows_u16[r];
      optr = out->rows_u16[r];
      opend = optr + cmax4;
/*
 * copy first and last half-filter-widths explicitly
 */
      memcpy((void *)optr,
	     (const void *)iptr,
	     filtsize_r/2*sizeof(unsigned short));
      memcpy((void *)&optr[in->ncol - filtsize_r/2],
	     (const void *)&iptr[in->ncol - filtsize_r/2],
	     filtsize_r/2*sizeof(unsigned short));
      optr += filtsize_r/2;
/*
 * do columns in groups of 4
 */
      while(optr != opend) {
	 tmp0 = tmp1 = tmp2 = tmp3 = half;
	 i = 0;
	 do {
	    tmp0 += filter[i]*iptr[i];
	    tmp1 += filter[i]*iptr[i + 1];
	    tmp2 += filter[i]*iptr[i + 2];
	    tmp3 += filter[i]*iptr[i + 3];
	    i++;
	 } while(i != filtsize_r);
	 tmp0 >>= CSHIFT - lshift;
	 optr[0] = (tmp0 > MAX_U16) ?
	   ((tmp0 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp0;
	 tmp1 >>= CSHIFT - lshift;
	 optr[1] = (tmp1 > MAX_U16) ? 
	   ((tmp1 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp1;
	 tmp2 >>= CSHIFT - lshift;
	 optr[2] = (tmp2 > MAX_U16) ? 
	   ((tmp2 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp2;
	 tmp3 >>= CSHIFT - lshift;
	 optr[3] = (tmp3 > MAX_U16) ? 
	   ((tmp3 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp3;
	 iptr += 4;
	 optr += 4;
      }
/*
 * and process the leftover columns one at a time
 */
      while(optr < out->rows_u16[r] + cmax) {
	 tmp0 = half;
	 i = 0;
	 do {
	    tmp0 += filter[i]*iptr[i];
	    i++;
	 } while(i != filtsize_r);
	 tmp0 >>= CSHIFT - lshift;
	 optr[0] = (tmp0 > MAX_U16) ?
	   ((tmp0 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp0;
	 iptr++;
	 optr++;
      }
   }
}

/*
 * Use array indexes rather than pointers. Do four columns at a time
 */
static void
convolve_mult_columns(const REGION *in,	/* region to smooth */
		      REGION *out,	/* result */
		      int *filter,	/* filters */
		      int filtsize_r,	/* column- and row- size of filters */
		      int filtsize_c,
		      int lshift)	/* how much to << answer */
{
   int *fptr;				/* pointer to filter */
   int i;
   unsigned short **iptr,*optr;		/* pointers to in and out */
   int c,r;				/* column and row counters */
   int cmax;				/* maximum column to process */
   int cmax4;				/* maximum value of column counter
					   that is a multiple of 4 and < ncol*/
   const int half = 1 << (CSHIFT - lshift - 1);	/* a half,
						   when >> CSHIFT - lshift */
   unsigned int tmp0,tmp1,tmp2,tmp3;
   int u,l;

   cmax = in->ncol - filtsize_r/2;
   cmax4 = filtsize_r/2 + 4*(int)((cmax - filtsize_r/2)/4);

   l = 0;				/* upper and lower range in column */
   u = filtsize_c;			/*     that we are summing over */
   fptr = filter;
   for(r = filtsize_c/2;r != in->nrow - filtsize_c/2;r++) {
      iptr = in->rows_u16;
      optr = out->rows_u16[r];

      c = filtsize_r/2;
      while(c != cmax4) {
	 tmp0 = tmp1 = tmp2 = tmp3 = half;
	 i = l;
	 do {
	    tmp0 += fptr[i]*iptr[i][c];
	    tmp1 += fptr[i]*iptr[i][c+1];
	    tmp2 += fptr[i]*iptr[i][c+2];
	    tmp3 += fptr[i]*iptr[i][c+3];
	    i++;
	 } while(i != u);

	 tmp0 >>= CSHIFT - lshift;
	 optr[c++] = (tmp0 > MAX_U16) ?
	   ((tmp0 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp0;
	 tmp1 >>= CSHIFT - lshift;
	 optr[c++] = (tmp1 > MAX_U16) ?
	   ((tmp1 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp1;
	 tmp2 >>= CSHIFT - lshift;
	 optr[c++] = (tmp2 > MAX_U16) ?
	   ((tmp2 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp2;
	 tmp3 >>= CSHIFT - lshift;
	 optr[c++] = (tmp3 > MAX_U16) ?
	   ((tmp3 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp3;
      }

      while(c < cmax) {
	 tmp0 = half;
	 i = l;
	 do {
	    tmp0 += fptr[i]*iptr[i][c];
	    i++;
	 } while(i != u);
	 tmp0 >>= CSHIFT - lshift;
	 optr[c++] = (tmp0 > MAX_U16) ?
	   ((tmp0 & (S32_SIGN_BIT >> (CSHIFT - lshift))) ? 0 : MAX_U16) : tmp0;
      }

      u++; l++; fptr--;
   }
}

/*****************************************************************************/
/*
 * now the convolution-using-multiplies for FL32 regions
 */
static void
convolve_mult_rows_fl32(const REGION *in, REGION *out, const float *filter,
			int filtsize_r);
static void
convolve_mult_columns_fl32(const REGION *in, REGION *out, const float *filter,
			   int filtsize_r, int filtsize_c);

static void
convolve_mult_fl32(REGION *out,		/* output region; can be same as in */
		   const REGION *in,	/* region to smooth */
		   REGION *scr,		/* scratch region */
		   int filtsize_r,	/* column- and row- size of filters */
		   int filtsize_c,
		   const float *filter_r, /* column- and row- filters */
		   const float *filter_c,
		   float scale)		/* how much to scale result up */
{
   REGION *tmp = NULL;

   shAssert(in != NULL && out != NULL && scr != NULL);

   if(in->type == TYPE_U16) {		/* make an FL32 copy */
      tmp = shRegNew("", in->nrow, in->ncol, TYPE_FL32);
      shRegIntCopy(tmp, in);
      in = tmp;
   }

   shAssert(in->type == TYPE_FL32);
   shAssert(in->type == scr->type && in->type == out->type);
   shAssert(in->nrow == scr->nrow && in->ncol == scr->ncol);
   shAssert(in->nrow == out->nrow && in->ncol == out->ncol);
   shAssert(in->rows_fl32[0] != scr->rows_fl32[0]);
   shAssert(scr->rows_fl32[0] != out->rows_fl32[0]);
   shAssert(filtsize_r%2 == 1 && filtsize_c%2 == 1);
#if FLOATING_PHOTO && PHOTO_U16_COMPAT
/*
 * For testing, convert that filter to int and back so as to give
 * exactly the same results as the U16 version.  Actually it doesn't,
 * as the scr region is also FL32 not U16, but we fix that below
 */
   {
      int *filter = alloca(((filtsize_r > filtsize_c) ?
			    filtsize_r : filtsize_c)*sizeof(int));
      int i;
      
      set_filter_mult(filter,filter_r,filtsize_r);
      for(i = 0; i < filtsize_r; i++) {
	 ((float *)filter_r)[i] = filter[i]/(float)(1 << CSHIFT);
      }

      set_filter_mult(filter,filter_c,filtsize_c);
      for(i = 0; i < filtsize_c; i++) {
	 ((float *)filter_c)[i] = filter[i]/(float)(1 << CSHIFT);
      }
   }
#endif
   convolve_mult_rows_fl32(in, scr, filter_r, filtsize_r);
#if FLOATING_PHOTO && PHOTO_U16_COMPAT
   {
      int i, j;
      PIX *row;
      int nrow = scr->nrow, ncol = scr->ncol;
      for(i = 0; i < nrow; i++) {
	 row = scr->ROWS[i];
	 for(j = 0; j < ncol; j++) {
	    row[j] = (int)(row[j] + 0.5);
	 }
      }
   }
#endif
/*
 * Only scale in column, not row too.  In case filter_c == filter_r,
 * only modify filter _after_ doing the row convolution
 */
   if(scale != 1.0) {
      int i;
      for(i = 0; i < filtsize_c; i++) {
	 ((float *)filter_c)[i] *= scale;
      }
   }
   
   convolve_mult_columns_fl32(scr, out, filter_c, filtsize_r, filtsize_c);
#if FLOATING_PHOTO && PHOTO_U16_COMPAT
   {
      int i, j;
      PIX *row;
      int nrow = out->nrow, ncol = out->ncol;
      for(i = 0; i < nrow; i++) {
	 row = out->ROWS[i];
	 for(j = 0; j < ncol; j++) {
	    row[j] = (int)(row[j] + 0.5);
	 }
      }
   }
#endif

   shRegDel(tmp);
}

static void
convolve_mult_rows_fl32(const REGION *in, /* region to smooth */
			REGION *out,	/* result */
			const float *filter, /* column filter */
			int filtsize_r)	/* size of filter */
{
   int cmax;				/* max value of column counter, c */
   int cmax4;				/* cmax s.t. we process 4n columns */
   int i;
   FL32 *iptr;				/* pointer to in */
   FL32 *optr;				/* pointer to out */
   FL32 *opend;				/* end of range of optr */
   int r;				/* row counter */
   FL32 tmp0, tmp1, tmp2, tmp3;
   
   cmax = in->ncol - filtsize_r/2;
   cmax4 = filtsize_r/2 + 4*(int)((cmax - filtsize_r/2)/4);

   for(r = 0;r != in->nrow;r++) {
      iptr = in->rows_fl32[r];
      optr = out->rows_fl32[r];
      opend = optr + cmax4;
/*
 * copy first and last half-filter-widths explicitly
 */
      memcpy((void *)optr, (const void *)iptr, filtsize_r/2*sizeof(FL32));
      memcpy((void *)&optr[in->nrow - filtsize_r/2],
	     (const void *)&iptr[in->nrow - filtsize_r/2],
	     filtsize_r/2*sizeof(FL32));
      optr += filtsize_r/2;
/*
 * do columns in groups of 4
 */
       while(optr != opend) {
	 tmp0 = tmp1 = tmp2 = tmp3 = 0.0;
	 i = 0;
	 do {
	    tmp0 += filter[i]*iptr[i];
	    tmp1 += filter[i]*iptr[i + 1];
	    tmp2 += filter[i]*iptr[i + 2];
	    tmp3 += filter[i]*iptr[i + 3];
	    i++;
	 } while(i != filtsize_r);
	 optr[0] = tmp0;
	 optr[1] = tmp1;
	 optr[2] = tmp2;
	 optr[3] = tmp3;
	 iptr += 4;
	 optr += 4;
      }
/*
 * and process the leftover columns one at a time
 */
      while(optr < out->rows_fl32[r] + cmax) {
	 tmp0 = 0.0;
	 i = 0;
	 do {
	    tmp0 += filter[i]*iptr[i];
	    i++;
	 } while(i != filtsize_r);
	 optr[0] = tmp0;
	 iptr++;
	 optr++;
      }
   }
}

/*
 * Use array indexes rather than pointers. Do four columns at a time
 */
static void
convolve_mult_columns_fl32(const REGION *in, /* region to smooth */
			   REGION *out,	/* result */
			   const float *filter, /* filter */
			   int filtsize_r, /* column- and */
			   int filtsize_c) /*         row- size of filters  */
{
   const float *fptr;			/* pointer to filter */
   int i;
   FL32 **iptr,*optr;			/* pointers to in and out */
   int c,r;				/* column and row counters */
   int cmax;				/* maximum column to process */
   int cmax4;				/* maximum value of column counter
					   that is a multiple of 4 and < ncol*/
   FL32 tmp0,tmp1,tmp2,tmp3;
   int u,l;

   cmax = in->ncol - filtsize_r/2;
   cmax4 = filtsize_r/2 + 4*(int)((cmax - filtsize_r/2)/4);

   l = 0;				/* upper and lower range in column */
   u = filtsize_c;			/*     that we are summing over */
   fptr = filter;
   for(r = filtsize_c/2;r != in->nrow - filtsize_c/2;r++) {
      iptr = in->rows_fl32;
      optr = out->rows_fl32[r];

      c = filtsize_r/2;
      while(c != cmax4) {
	 tmp0 = tmp1 = tmp2 = tmp3 = 0.0;
	 i = l;
	 do {
	    tmp0 += fptr[i]*iptr[i][c];
	    tmp1 += fptr[i]*iptr[i][c+1];
	    tmp2 += fptr[i]*iptr[i][c+2];
	    tmp3 += fptr[i]*iptr[i][c+3];
	    i++;
	 } while(i != u);

	 optr[c++] = tmp0;
	 optr[c++] = tmp1;
	 optr[c++] = tmp2;
	 optr[c++] = tmp3;
      }

      while(c < cmax) {
	 tmp0 = 0.0;
	 i = l;
	 do {
	    tmp0 += fptr[i]*iptr[i][c];
	    i++;
	 } while(i != u);
	 optr[c++] = tmp0;	 
      }

      u++; l++; fptr--;
   }
}
