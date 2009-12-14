#if !defined(PHSTAR1C_H)
#define PHSTAR1C_H

#include "phDgpsf.h"
#include "phCalib1.h"
#include "phCcdpars.h"
#include "phObjc.h"

typedef struct {
  const int id;				/* id for this STAR1C */
  int starid;				/* id for this star */
  int ncolors;				/* dimension of star1[] */
  STAR1_FLAGS flags;			/* Why rejected */ 
  STAR1 **star1;			/* Pointer to the actual star1s */ 
} STAR1C;

STAR1C *phStar1cNew (int ncolors);

void phStar1cDel(STAR1C *star);

RET_CODE
phMakeStar1clist(CHAIN *star1clist, CHAIN *masks, CHAIN *pstamps, int ifilter, int nfilters,
		 int id0, int frame, const REGION *flat, const REGION *bias,
		 const CCDPARS *ccdpars, int bscale, const CALIB1 *cal,  int stamp_size, 
		 int aperture, int psf_def_rad, float cr_min_s, int cr_min_e, float cr_cond3,
		 float cr_cond32, FRAMEPARAMS *fparams, int threshold, float sigma_multi, 
		 float sigma_badsky, float second_peak, int wing, int sym_flag, int regflag);

#endif
