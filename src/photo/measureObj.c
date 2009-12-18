/*
 * This is the Measure Objects code
 *
 * Upon arrival in this module, an OBJC is expected to have at least one
 * non-NULL OBJECT1. All coordinates in the OBJECT1s are taken to be in
 * their band's native coordinate system
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "dervish.h"

#include "phObjc.h"

static RANDOM *randoms;			/* random numbers */

/*****************************************************************************/
/*
 * Two procedures that are only needed as one of them needs access to the
 * random numbers in randoms
 */
void
phRemoveAtlasImage(const OBJC *objc,
		   const FIELDPARAMS *fiparams)
{
   int c;
   float sigma;				/* background s.d. */

   shAssert(objc != NULL && objc->aimage != NULL && fiparams != NULL);
   
   for(c = 0;c < objc->ncolor;c++) {
      shAssert(fiparams->frame[c].data != NULL);

      if(objc->aimage->pix[c] != NULL) {
	 shAssert(objc->color[c] != NULL);
	 const float gain = phGain(&fiparams->frame[c], objc->color[c]->rowc, objc->color[c]->colc);
	 const float dark_variance = phDarkVariance(&fiparams->frame[c], objc->color[c]->rowc, objc->color[c]->colc);

	 sigma = sqrt(objc->color[c]->sky/gain + dark_variance);
	 phRegionSetValFromAtlasImage(objc->aimage, c, 1,
				      (REGION *)fiparams->frame[c].data,
				      SOFT_BIAS + fiparams->frame[c].bkgd,
				      sigma, randoms, 0, 0);
      }
   }
}

void
phInsertAtlasImage(const OBJC *objc,
		   const FIELDPARAMS *fiparams)
{
   int c;

   shAssert(objc != NULL && objc->aimage != NULL && fiparams != NULL);
   
   for(c = 0; c < objc->ncolor; c++) {
      shAssert(fiparams->frame[c].data != NULL);

      phRegionSetFromAtlasImage(objc->aimage, c,
				(REGION *)fiparams->frame[c].data, 0, 0, 0, '\0', 0);
   }
}
