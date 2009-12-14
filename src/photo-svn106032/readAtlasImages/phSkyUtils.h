#if !defined(PHSKYUTILS_H)
#define PHSKYUTILS_H

#include "phRandom.h"
/*
 * A type to encapsulate binned data
 */
typedef struct {
   REGION *reg;				/* the binned REGION */
   REGION *parent;			/* may be NULL */
   int bin_row, bin_col;		/* how much <reg> is binned by */
   int shift;				/* number of bits <reg> is scaled up by
					   (if -ve, scaled down --- but note
					   that C doesn't allow "x >>= (-2)")*/
   int row0, col0;			/* origin of binned region in parent */
   int rsize, csize;			/* size of parent */
} BINREGION;				/* pragma SCHEMA */

/*
 * Specify the convolution algorithm to use
 */
typedef enum {
   CONVOLVE_ANY,			/* code's choice */
   CONVOLVE_MULT,			/* do the multiplies explicitly */
   CONVOLVE_LUT,			/* use a LUT */
   CONVOLVE_PLUT			/* use a packed LUT */
} CONVOLVE_ALG;				/* pragma IGNORE */

/*
 * Con/destructors
 */
BINREGION *phBinregionNew(void);
BINREGION *phBinregionNewFromConst(float val, int nrow, int ncol,
				   int bin_row, int bin_col, int shift);
BINREGION *phBinregionNewFromBinregion(const BINREGION *old);
void phBinregionDel(BINREGION *breg);

BINREGION *
phMedianSmooth(const REGION *in,	/* input region */
	       int filtsize_c,		/* size of filter */
	       int filtsize_r,
	       int coarse,		/* how coarsely to sample */
	       int minval, int maxval,	/* {min,max} pixel values to include
					   in histograms */
	       int shift,		/* how many bits to shift
					   output REGION */
	       int shift_to_centers,	/* shift returned region by 0.5,0.5? */
	       BINREGION *skyErr,	/* BINREGION to return sky sigma;
					   may be NULL*/
	       int use_mean);		/* use a mean NOT a median */

void
phSkySubtract(
	      REGION *out,		/* input region */
	      const REGION *in,		/* subtracted region */
	      const BINREGION *smooth,	/* region to subtract; generally
					   smaller than <in> or <out> */
	      RANDOM *rand		/* random numbers; used in dithering */
	      );

void
phSkyEstimateDebias(BINREGION *breg,	/* BINREGION with smoothed sky */
		    int clipped,	/* were sky values clipped at
					   NSIGMA_CLIP sigma? */
		    float gain1, float gain2); /* gains of two halves of chip */

float
phBinregionInterpolate(
		       const BINREGION *sky,	/* the binned data */
		       int row, int col		/* where we want the value */
		       );

void
phInterpolateAndSubtract(
			 REGION *out,	/* output region */
			 const REGION *in, /* input region */
			 const BINREGION *smooth,/* region to interpolate and
						    subtract */
			 RANDOM *rand	/* random numbers; used in dithering */
			 );

void phInitConvolveLUT(int nlut);
void phFiniConvolveLUT(void);
int phConvolve(REGION *out,		/* output region; can be same as in */
	       const REGION *in,	/* region to smooth */
	       REGION *scr,		/* scratch region */
	       int filtsize_r,		/* column- and row- size of filters */
	       int filtsize_c,
	       float *filter_r,		/* column- and row- filters */
	       float *filter_c,
	       int lshift,		/* how much to << answer */
	       CONVOLVE_ALG alg);	/* algorithm to use */

int phConvolveWithGaussian(REGION *out,	/* output region; can be same as in */
			   const REGION *in,	/* region to smooth */
			   REGION *scr,	/* scratch region */
			   int filtsize, /* column- and row- size of filters */
			   float sigma,	/* sigma in pixels for Gaussian */
			   int lshift,	/* how much to << answer */
			   CONVOLVE_ALG alg); /* algorithm to use */
int
phConvolveWithGaussianPowers(REGION *out, /* output region; can be same as in*/
			     const REGION *in, /* region to smooth */
			     REGION *scr, /* scratch region */
			     int filtsize, /* size of filters */
			     float sigma, /* sigma in pixels for Gaussian */
			     int ir,	/* order in row direction */
			     int ic);	/* order in column direction */

int phConvolveWithLaplacian(REGION *out, /* output region; can be same as in */
			    REGION *in,	/* region to smooth */
			    REGION *scr, /* scratch region */
			    REGION *scr2, /* second scratch region */
			    int filtsize, /* column- and row- size of filters*/
			    float sigma, /* sigma in pixels for Gaussian */
			    int lshift,	/* how much to << answer */
			    CONVOLVE_ALG alg); /* algorithm to use */

BINREGION *phRegIntBin(BINREGION *out, const REGION *in,
		       const int col_fac, const int row_fac, int rshift,
		       const int bkgd, const int addcon,
		       const int row0, const int col0, const int slow);

REGION *phBinregionUnBin(const BINREGION *in, RANDOM *rand,
			 const int bkgd, const int addcon);

REGION *
phLUTByHistogramEqualize(int min, int max);
REGION *
phLUTByAnalyticStretch(int min, int max, int type, float param);

#endif
