#if !defined(PHMERGECOLORS_H)
#define PHMERGECOLORS_H

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "dervish.h"
#include "phObjc.h"
#include "phOffset.h"

   /* the new routines for the Merge Colors module */

RET_CODE
phInitMergeColors(int ncolor);

RET_CODE
phFiniMergeColors(void);

RET_CODE
phMergeColors(CHAIN **objects,
	      const FIELDPARAMS *fparams,
	      CHAIN *merged,
	      int cut_atlas_images,
	      int no_restore_ai,	/* don't restore old atlas images */
	      int val,			/* value to replace atlas images by */
	      float sigma,		/* val's standard deviation */
	      RANDOM *rand);		/* random numbers for sigma */
/*
 * merge lists of PEAKs in peaks1; peaks2 is destroyed unless copy is true
 */
int phPeaksMerge(PEAKS *peaks1, PEAKS *peaks2, float drow, float dcol,
		 float eps, int copy, int bitmask);

void phObjcPeaksSetFromObject1s(OBJC *objc, const FIELDPARAMS *fparams);

#endif
