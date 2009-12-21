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

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * 
 * Estimate an object's velocity from its position in the bands it's
 * detected in.
 * 
 * Calculate the reduced chi^2 for the fit; there are 2*(ndetect - 2) degrees
 * of freedom, so a negative chi^2 is returned for ndetect <= 2
 *
 * Return OBJECT2 bits describing problems, if any; set OBJC.{row,col}v(Err)?
 */
int
phVelocityFind(OBJC *objc,		/* OBJC whose velocity is desired */
	       const FIELDPARAMS *fiparams, /* info about the frame */
	       float *row,		/* fitted row positions, or NULL */
	       float *rowErr,		/* errors in row, or NULL */
	       float *col,		/* fitted column positions, or NULL */
	       float *colErr,		/* errors in col, or NULL */
	       float *rchisq)		/* reduced chi^2, or NULL */
{
   int c;
   float chisq;				/* chisq for the fit */
   float rchisq_s = 0;			/* storage for rchisq, if needed */
   const float chisq_max = 3.0;		/* worst acceptable reduced chi^2 */
   float colc[NCOLOR], colcErr[NCOLOR];	/* column position and error */
   float covar;				/* covariance of pos and velocity */
   float drow, dcol;			/* offsets from reference colour */
   float drowErr, dcolErr;		/* errors in drow, dcol */
   float errMin = 1e-2;			/* minimum possible positional error */
   int flags2 = 0;			/* OBJECT2 bits indicating problems */
   float psfMags[NCOLOR];		/* PSF magnitudes in all bands */
   float psfMagsErr[NCOLOR];		/* errors in psfMags */
   const OBJECT1 *obj1;			/* == objc->colors[c] */
   int ndetect = 0;			/* number of bands with detections */
   const float nsig = 2;		/* XXX to be considered as moving */
   float pos, posErr;			/* estimated position at t==0 + error*/
   float rowc[NCOLOR], rowcErr[NCOLOR];	/* row position and error */
   float t[NCOLOR];			/* "time" of detection, measured
					   in frames not seconds */
   
   shAssert(objc != NULL && fiparams != NULL);

   if(rchisq == NULL) {
      rchisq = &rchisq_s;
   }

   set_psfMags(objc, fiparams, psfMags, psfMagsErr);
/*
 * estimate position for each band in canonical band's coordinate system
 */
   for(c = 0; c < objc->ncolor; c++) {
      if((obj1 = objc->color[c]) != NULL && obj1->flags & OBJECT1_DETECTED) {
#if 0
	 if(obj1->flags & OBJECT1_CANONICAL_CENTER) {
	    continue;
	 }
#endif

	 if((obj1->flags  & OBJECT1_PEAKCENTER) ||
	    (obj1->flags2 & OBJECT2_INTERP_CENTER)) { /* don't use position */
	    continue;
	 }
	 
	 t[ndetect] = fiparams->frame[c].dframe;
	 phOffsetDo(fiparams, obj1->rowc, obj1->colc,
		    c, fiparams->ref_band_index,
		    1, psfMags, psfMagsErr, &drow, &drowErr, &dcol, &dcolErr);

	 rowc[ndetect] = obj1->rowc + drow;
	 rowcErr[ndetect] = sqrt(pow(obj1->rowcErr,2) + pow(drowErr,2) +
				 pow(errMin,2));
	 colc[ndetect] = obj1->colc + dcol;
	 colcErr[ndetect] = sqrt(pow(obj1->colcErr,2) + pow(dcolErr,2) +
				 pow(errMin,2));
	 ndetect++;
      }
   }

   if(ndetect == 0) {
      flags2 |= OBJECT2_TOO_FEW_GOOD_DETECTIONS;
      *rchisq = -1;
      return(flags2);
   }
/*
 * Fit row data
 */
   chisq = phLinearFit(t, rowc, rowcErr, ndetect, &pos, &posErr, &covar,
		       &objc->rowv, &objc->rowvErr);
/*
 * Estimate row, rowErr, etc.
 */
   for(c = 0; c < objc->ncolor; c++) {
      const int dframe = fiparams->frame[c].dframe;
      if(row != NULL) {
	 row[c] = pos + objc->rowv*dframe;
      }
      if(rowErr != NULL) {
	 if(objc->rowvErr < 0) {
	    rowErr[c] = ERROR_IS_BAD;
	 } else {
	    rowErr[c] = sqrt(pow(posErr,2) + 2*dframe*covar +
						 pow(dframe*objc->rowvErr, 2));
	 }
      }
   }
/*
 * and now column
 */
   chisq += phLinearFit(t, colc, colcErr, ndetect, &pos, &posErr, &covar,
			&objc->colv, &objc->colvErr);
/*
 * Estimate col, colErr, etc.
 */
   for(c = 0; c < objc->ncolor; c++) {
      const int dframe = fiparams->frame[c].dframe;
      if(col != NULL) {
	 col[c] = pos + objc->colv*dframe;
      }
      if(colErr != NULL) {
	 if(objc->colvErr < 0) {
	    colErr[c] = ERROR_IS_BAD;
	 } else {
	    colErr[c] = sqrt(pow(posErr,2) + 2*dframe*covar +
						 pow(dframe*objc->colvErr, 2));
	 }
      }
   }
/*
 * Calculate the reduced chi^2
 */
   if(ndetect <= 2) {
      flags2 |= OBJECT2_TOO_FEW_GOOD_DETECTIONS;
      if(chisq > 5e-4) {
	 shError("phVelocityFind: chisq = %.2g (OBJC id = %d)",
		 chisq, objc->id);
      }
      *rchisq = -1;
   } else {
      *rchisq = chisq/(2*(ndetect - 2));

      if(*rchisq > chisq_max) {
	 flags2 |= OBJECT2_BAD_MOVING_FIT;
      }
   }
/*
 * Is the velocity is consistent with zero?
 */
   if(pow(objc->rowv, 2) + pow(objc->colv, 2) <
		   nsig*nsig*(pow(objc->rowvErr, 2) + pow(objc->colvErr, 2))) {
      flags2 |= OBJECT2_STATIONARY;      
   }

   return(flags2);
}
