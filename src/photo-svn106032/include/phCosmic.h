#if !defined(PHCOSMIC_H)
#define PHCOSMIC_H

#include "phDgpsf.h"
#include "phVariablePsf.h"

int phFindCR(REGION *reg, float sky, float bkgd, float skyErr,const DGPSF *psf,
	     float e_per_dn, float min_sigma, int min_e, float c3, float c3_2,
	     int keep, const PSF_BASIS *psfBasis,
	     float *adopted_cond3_fac2);     

#endif
