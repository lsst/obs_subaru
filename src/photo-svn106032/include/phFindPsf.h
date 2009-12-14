#if !defined(PHFINDPSF_H)
#define PHFINDPSF_H

#include "phCalib1.h"
#include "phFramestat.h"

RET_CODE
phSelectPsfStars(const CHAIN *starlist, const CHAIN *cframes, CHAIN **frameinfolist, 
                 int ncolor, int icolor, int frame, int first_star, int last_star,  
                 float ncellclip, int nbadcell, float maxdev,
		 float nsigma_sigma, float nsigma_QU, float nsigma_RMS_rad,
                 int niter, int min_starnum);

RET_CODE
phQAforPsfStars(const CHAIN *starlist, PSF_BASIS *KLpsf, CHAIN *cframes,
                CHAIN **frameinfolist, int ncolor, int icolor, int frame, int nrow,
                int ncol, float gain,  float dark_var, float sky, float skyerr, 
                int first_star, int last_star, float q_min, float q_max);

RET_CODE
phMeasurePsf(PSF_BASIS *KLpsf, const CHAIN **winglist, const CHAIN **wingmasks, 
             const CHAIN *cframes, CHAIN **frameinfolist, float gain,  float dark_var, 
             int ncolor, int icolor, float meansky, float meanskyerr, float err_min, 
              float err_max, int frame, int nrow, int ncol, int wing_options);

RET_CODE
phCheckCompiler(float x);

DGPSF *
phKLpsf2dgpsf(PSF_BASIS *KLpsf, int row, int col, float sky, float skyerr, float gain, 
              float dark_var);

RET_CODE
phFilterApCorr(const CHAIN *cframes, CHAIN **frameinfolist, int min_starnum,
               int ncolor, int nave, int n, float a, float b);

RET_CODE
phGetCprof(const CHAIN *starlist, int first, int last, int *badstar, int type,    
          int scaling_type, CELL_PROF *cprof_bar, float *oddness, float *maxchi2);        	

void
phSetBadstarFlagFromChain(const CHAIN *starlist, /* Chain of STAR1cs */
			  const CHAIN *const *frameinfolist, /* clipping info
							      for each band */
			  float ncellclip, /*# of sigma for a cell to be bad*/ 
			  int nbadcell,	/* max. # of bad cells for a star
					   to be good */ 
			  float maxdev,	/* max dev of a cell to condemn all */ 
			  float nsigma_sigma, /* nsigma to clip sigma at */
			  float nsigma_QU, /* nsigma to clip QU at */
			  float nsigma_RMS_rad);/* nsigma to clip RMS rad. at*/

#endif
 

