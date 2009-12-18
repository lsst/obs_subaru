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
#include "phOffset.h"

static void
set_psfMags(const OBJC *objc, const FIELDPARAMS *fiparams,
	    float *const psfMags, float *const psfMagsErr)
	;

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

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Set the canonical centre for an OBJC. If it's detected in the
 * canonical colour, that's easy; if it isn't choose the band with
 * the largest peak counts. A (fibre?) magnitude would be better,
 * but one isn't available at the colour-dependent centres
 */
void
phObjcCenterCalc(OBJC *objc,		/* the object in question */
		 const FIELDPARAMS *fparams, /* properties of field */
		 int use_color)		/* use colour information? */
{
   int best;				/* index of band with best centre */
   int canonical_color;			/* canonical colour */
   float drow, dcol;			/* convert to reference colour */
   float drowErr, dcolErr;		/* errors in drow, dcol */
   OBJECT1 *obj1;
   int i;
   float psfMags[NCOLOR];		/* PSF magnitudes in all bands */
   float psfMagsErr[NCOLOR];		/* errors in psfMags */

   shAssert(objc != NULL && fparams != NULL);

   if(use_color) {
      set_psfMags(objc, fparams, psfMags, psfMagsErr);
   }

   canonical_color = fparams->ref_band_index;
   
   if(objc->flags3 & OBJECT3_HAS_CENTER) { /* the objc->position is correct */
      return;
   }

   obj1 = objc->color[canonical_color];
   if(obj1 != NULL && (obj1->flags & OBJECT1_DETECTED) &&
		      !(obj1->flags & OBJECT1_SATUR)) { /* easy! */
      best = canonical_color;
   } else {				/* we have to find the most significant
					   of the other bands */
      int peak = -100000;		/* largest peak seen */

      best = -1;
      for(i = 0;i < objc->ncolor;i++) {
	 obj1 = objc->color[i];

	 if(obj1 == NULL ||
	    (obj1->peaks == NULL || obj1->peaks->npeak == 0) ||
	    (obj1->flags & OBJECT1_SATUR)) {
	    continue;
	 }

	 if(obj1->peaks->peaks[0]->peak > peak) {
	    best = i;
	    peak = obj1->peaks->peaks[0]->peak;
	 }
      }
      if(best == -1) {			/* all peaks must be saturated;
					   so relax that condition */
	 for(i = 0;i < objc->ncolor;i++) {
	    obj1 = objc->color[i];
	    
	    if(obj1 == NULL || obj1->peaks == NULL || obj1->peaks->npeak == 0){
	       continue;
	    }
	    if(obj1->peaks->peaks[0]->peak > peak) {
	       best = i;
	       peak = obj1->peaks->peaks[0]->peak;
	    }
	 }
      }
      shAssert(best >= 0);		/* must have been seen in some band */
      
      obj1 = objc->color[best];
   }
/*
 * set the OBJC centre from the chosen band
 */
   objc->colc = obj1->colc;
   objc->colcErr = obj1->colcErr;
   objc->rowc = obj1->rowc;
   objc->rowcErr = obj1->rowcErr;
/*
 * convert coordinate system to the canonical band
 */
   phOffsetDo(fparams, objc->rowc, objc->colc, 
	      best, fparams->ref_band_index, 0,
	      (use_color ? psfMags : NULL),
	      (use_color ? psfMagsErr : NULL), &drow,&drowErr, &dcol,&dcolErr);
   objc->colc += dcol;
   objc->colcErr = sqrt(pow(objc->colcErr,2) + pow(dcolErr,2));
   objc->rowc += drow;
   objc->rowcErr = sqrt(pow(objc->rowcErr,2) + pow(drowErr,2));

   objc->flags3 |= OBJECT3_HAS_CENTER;
/*
 * set the bit which indicates which band is canonical.  This may have
 * already been set for some other band (e.g. for children which were
 * not originally detected in the canonical band, but for which the deblender
 * determines that they were detectable)
 *
 * We cannot simply use obj1, as a saturated detection is acceptable.
 */
   for(i = 0;i < objc->ncolor;i++) {
      if(objc->color[i] != NULL) {
	 objc->color[i]->flags2 &= ~OBJECT2_CANONICAL_BAND;
      }
   }

   if(objc->color[canonical_color] != NULL &&
      (objc->color[canonical_color]->flags & OBJECT1_DETECTED)) {
      obj1 = objc->color[canonical_color];
   }
   obj1->flags2 |= OBJECT2_CANONICAL_BAND;
}

/*****************************************************************************/
/*
 * Estimate magnitudes in each band to get the astrometric errors right.
 *
 * The astrotools routines to handle transformations don't know enough
 * to handle the case where we are processing less than 5 bands, so set
 * all of them to non-floatingpoint-trap representations
 */
static void
set_psfMags(const OBJC *objc, const FIELDPARAMS *fiparams,
	    float *const psfMags, float *const psfMagsErr)
{
   int c;
   
   for(c = 0; c < NCOLOR; c++) {
      psfMags[c] = 30; psfMagsErr[c] = 1e10;
   }

   for(c = 0; c < objc->ncolor; c++) {
      const float cts = (objc->color[c] == NULL) ?
						-1 : objc->color[c]->psfCounts;
      if(cts <= 0) {
	 psfMags[c] = 30; psfMagsErr[c] = 1e10;
      } else {
	 psfMags[c] = 20 - 2.5*log10(cts/fiparams->frame[c].flux20);
	 psfMagsErr[c] = 2.5/log(10)*objc->color[c]->psfCountsErr/cts;
      }
   }
}

