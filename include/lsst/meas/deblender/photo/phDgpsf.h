#if !defined(PHDGPSF_H)
#define PHDGPSF_H

#include "dervish.h"
#include "atConversions.h"
#include "phExtract.h"

/*****************************************************************************/
/*
 * This is the description of the PSF in terms of indices and weights
 * for the cellfitobj code
 */
#define IORDER_SEEING 4			/* number of seeing coeffs */

typedef struct {
   int i[IORDER_SEEING];		/* indices of models */
   float w[IORDER_SEEING];		/* weights of models */
   float L;				/* likelihood of best fit */
   CELL_PROF residuals;                 /* residuals profile */
} PSF_COEFFS;				/* pragma SCHEMA */

/*****************************************************************************/
/*
 * Describe a composite profile
 */
typedef struct {
   int i0;				/* first good value */
   int nprof;				/* (number of good values) + i0 */
   float psfCounts;			/* psf counts of this composite */
   float mean[NANN];			/* star's mean profile */
   float med[NANN];			/* star's median profile */
   float sig[NANN];			/* profile's standard deviation */
   float alpha;				/* derived scaling wrt composite */
} ONE_PROFILE;				/* pragma SCHEMA */

typedef struct {
   int prof_is_set;			/* the prof[] array is set */
   int n;				/* number of profiles added together */
   int size;				/* dimension of profs[] */
   ONE_PROFILE *profs;			/* the individual profiles */
   float psfCounts;			/* psf counts of prof[] */
   int nprof;				/* number of points in prof[] */
   float mean[NANN];			/* composite mean profile */
   float med[NANN];			/* composite median profile */
   float sig[NANN];			/* prof[]'s standard deviation */
} COMP_PROFILE;				/* pragma SCHEMA */

COMP_PROFILE *phCompositeProfileNew(void);
void phCompositeProfileDel(COMP_PROFILE *prof);

/*****************************************************************************/
/*
 * This struct holds information about a PSF which is represented as
 * a sum of two gaussians, each separable in X and Y, plus a power
 * law part (see phDgpsfNew for details)
 */
typedef struct dgpsf {
   float a;				/* intensity at origin */
   float width;                         /* effective PSF FWHM in arcsec */
   float sigmax1;			/* x-width of narrow Gaussian */
   float sigmax2;			/* x-width of wide Gaussian */
   float sigmay1;			/* y-width of narrow Gaussian */
   float sigmay2;			/* y-width of wide Gaussian */
   float b;				/* ratio wide/narrow at origin */
   float p0;				/* value of power law part at origin */
   float beta;				/* slope of power law */
   float sigmap;			/* width param for power law */
   float chisq;			        /* chisq/dof of fitted parameters */
   float psfCounts;			/* measured PSF counts for psf */
   float sigma1_2G;			/* width of narrow Gaussian from 2-Gaussian fit */
   float sigma2_2G;			/* width of wide Gaussian from 2-Gaussian fit */  
   float b_2G;			        /* width of wide Gaussian from 2-Gaussian fit */                    
   PSF_COEFFS coeffs;
} DGPSF;

DGPSF *phDgpsfNew(void);
void phDgpsfDel(DGPSF *dgpsf);
void phDgpsfCopy(DGPSF *dgpsf1, DGPSF *dgpsf2);

/*
 * functions that deal with the DGPSF structure
 */
void phDgpsfAdd(REGION *reg, const DGPSF *dgpsf, float outer,
		float x0, float y0, float amp);
void
phDgpsfAddInObjmask(REGION *region,	/* add to this region */
		    const OBJMASK *om,	/* within this mask */
		    int drow, int dcol,	/* offset mask by this much */
		    const DGPSF *dgpsf,	/* using this PSF */
		    float outer,	/* maximum size of added psf */
		    float rowc,		/* desired row centre */
		    float colc,		/* desired column centre */
		    float I0);		/* peak value of PSF */
float
phDgpsfSize(const DGPSF *dgpsf,		/* using this PSF */
	    float I0,			/* peak value of PSF */
	    int level);			/* desired level */
float phDgpsfCumulativeFlux(const DGPSF *psf, float r);

#endif
