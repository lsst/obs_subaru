/*
 * <AUTO>
 *
 * DESCRIPTION:
 * Code to transforming coordinates between bands
 *
 * </AUTO>
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "dervish.h"
#include "phOffset.h"
#include "phConsts.h"
#include "phObjc.h"
#include "phUtils.h"

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Change a TRANS struct by an offset in row and column
 *
 * This routine exposes (part of) the insides of a TRANS to photo, but
 * seems to be needed as it's photo's job to tweak the astrometric offsets
 */
void
phTransShift(TRANS *trans,		/* TRANS to tweak */
	     float drow,		/* amount to add in row */
	     float dcol)		/*                  and column */
{
   trans->a += trans->b*drow + trans->c*dcol;
   trans->d += trans->e*drow + trans->f*dcol;
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Given input coordinates, and a FIELDPARAMS structure, calculate the amount
 * (drow, dcol) by which the input coords must be changed to yield
 * properly transformed coords. To go from band 0 to band 2, say something
 * like:
 *   phOffsetDo(fiparams, row, col, 0, 2,
 *				     0, mag, magErr, &drow, NULL, &dcol, NULL);
 *   newrow = row + drow;
 *   newcol = col + dcol;
 * If you don't have any colour information, mag and magErr may be NULL.
 *
 * Note that band0 and band1 are ints not chars; they are indices into
 * fiparams->filters
 *
 * The relativeErrors flag tells phOffsetDo() that we are interested in
 * transforming to the canonical band, and that only the astrometric errors
 * going _to_ (mu, nu) should be included; the transformation back to
 * (row, col) will be the same for all bands.  If this flag is not set,
 * the errors in band0 == band1 will be set to 0
 */
void
phOffsetDo(const FIELDPARAMS *fiparams,	/* describe field, incl. astrometry */
	   float row0,			/* input row */
	   float col0,			/* input column */
	   int band0,			/* which band are row0, col0 in? */
	   int band1,			/* band drow, dcol are required for */
	   int relativeErrors,		/* transforming to canonical? */
	   const float *mag,		/* magnitudes in all bands, or NULL */
	   const float *magErr,		/* errors in mag, or NULL */
	   float *drow,			/* amount to add to get output col */
	   float *drowErr,		/* error in drow, or NULL */
	   float *dcol,			/* amount to add to get output row */
	   float *dcolErr)		/* error in dcol, or NULL */
{
   char cband0, cband1;			/* filternames corresponding to band[01] */
   int colBinning;			/* == fiparams->frame[]->colBinning */
   double mu, muErr, nu, nuErr;		/* great circle coordinates + errors */
   int ret;
   double col0_for_trans = col0;	/* The value of col0 for the asTrans
					   (allowing for binning problems) */
   double row1, col1;			/* row, col in band 1 */
   double row1Err, col1Err;		/* errors in row1, col1 */

   shAssert(fiparams != NULL && drow != NULL && dcol != NULL);
   shAssert(band0 >= 0 && band0 < fiparams->ncolor);
   shAssert(band1 >= 0 && band1 < fiparams->ncolor);
   colBinning = fiparams->frame[band0].colBinning;
   shAssert(colBinning > 0);
   shAssert(colBinning == fiparams->frame[band1].colBinning)

   *drow = *dcol = 0;
   if(drowErr != 0) {
      *drowErr = 0;			/* not zero for relative errors */
   }
   if(dcolErr != 0) {
      *dcolErr = 0;
   }

   if(band0 == band1 && !relativeErrors) {
      return;
   }
/*
 * Astrom is picky about band names that aren't in ugrizolts are permitted
 */
   cband0 = fiparams->filters[band0];
   cband1 = fiparams->filters[band1];
   if(strchr("ugriz", cband0) == NULL) { cband0 = 'r'; }
   if(strchr("ugriz", cband1) == NULL) { cband1 = 'r'; }
/*
 * Allow for problems in binned scans where the positions of objects in the
 * right side of 2-amp chips are shifted
 */
   col0_for_trans += (col0 <= 0.5*fiparams->ncol) ?
				      fiparams->frame[band0].astrom_dcol_left :
				      fiparams->frame[band0].astrom_dcol_right;

   shAssert(fiparams->frame[band0].toGCC);

   atTransApply(fiparams->frame[band0].toGCC, cband0,
		row0, 0, col0_for_trans, 0, mag, magErr,
		&mu, &muErr, &nu, &nuErr);
   if((row0 == mu && col0_for_trans == nu) || /* The identity */
      (fiparams->frame[band0].toGCC->a > 0.1 && fiparams->frame[band0].toGCC->f > 0.1)) { /* Not real data */
      /* We'll run into trouble with wrapping at mu == 180 */
      row1 = row0; col1 = col0_for_trans;
   } else {
      ret = atTransInverseApply(fiparams->frame[band1].toGCC, cband1, 
				mu, muErr, nu, nuErr, mag, magErr,
				&row1, &row1Err, &col1, &col1Err);
      shAssert(ret >= 0);
   }

   col1 -= (col1 <= 0.5*fiparams->ncol) ?
				      fiparams->frame[band1].astrom_dcol_left :
				      fiparams->frame[band1].astrom_dcol_right;
/*
 * If we want to use the offsets to look at the _relative_ position in bands,
 * we don't want to include the errors in fiparams->frame[band1].toGCC, so
 * estimate the errors by direct differencing
 */
   if(relativeErrors && (drowErr != NULL || dcolErr != NULL)) {
      double row1Drow, col1Dcol;	/* transform of [mn]u + [mn]uErr */
      double row1_forErr, col1_forErr;	/* usually == row1/col1, but not when avoiding mu==180 wrap */
      ret = atTransInverseApply(fiparams->frame[band1].toGCC, cband1, 
			     mu + muErr, 0, nu + nuErr, 0, mag, magErr,
			     &row1Drow, NULL, &col1Dcol, NULL);
      shAssert(ret >= 0);

      ret = atTransInverseApply(fiparams->frame[band1].toGCC, cband1, 
				mu, muErr, nu, nuErr, mag, magErr,
				&row1_forErr, &row1Err, &col1_forErr, &col1Err);
      shAssert(ret >= 0);
      
      row1Err = fabs(row1Drow - row1_forErr);
      col1Err = fabs(col1Dcol - col1_forErr);
   }

   *drow = row1 - row0;
   *dcol = col1 - col0;
   if(drowErr != NULL) {
      *drowErr = row1Err;
   }
   if(dcolErr != NULL) {
      *dcolErr = col1Err;
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Given a chain of merged detected objects, estimate the offsets between
 * the different bands.  We have already used the best estimate TRANS that
 * the astrom pipeline can provide, but we now have positions for many
 * more stars, so we should be able to do better.
 *
 * Restrict ourselves to a simple shift in row and column, and assume
 * that all higher order effects have been already satisfactorily handled
 */
void
phAstromOffsetsFind(const CHAIN *objects, /* list of objects */
		    int ncolor,		/* number of colours in OBJCs */
		    FIELDPARAMS *fiparams, /* TRANS structs etc. */
		    FIELDSTAT *fieldstat) /* summary of field */
{
   U16 *arr[2*NCOLOR];			/* the offset values */
   int c;				/* counter in colour */
   int clip = 1;			/* should we clip histograms? */
   float delta_row, delta_col;		/* offset from expected position */
   float drow, dcol;			/* {row,col} offsets to ref. band */
   int i;
   int n[NCOLOR];			/* number of points */
   int nobj;				/* number of objects in objects */
   const OBJC *objc = NULL;		/* an object from the chain */
   OBJECT1 *obj1;			/* == objc->color[] */
   int ref_band;			/* == fiparams->ref_band_index */
   float rowc, colc;			/* center in reference band */
   float *rowOffset, *colOffset;	/* == fieldstat->{row,col}Offset */

   shAssert(ncolor >= 1);
   shAssert(objects != NULL && objects->type == shTypeGetFromName("OBJC"));
   shAssert(fiparams != NULL);
   shAssert(fieldstat != NULL);   
   nobj = objects->nElements;
   ref_band = fiparams->ref_band_index;
   rowOffset = fieldstat->rowOffset;
   colOffset = fieldstat->colOffset;
   
   if(ncolor == 1 || nobj == 0) {
      fieldstat->rowOffset[0] = fieldstat->colOffset[0] = 0;
      return;
   }
/*
 * allocate space
 */
   for(c = 0; c < ncolor; c++) {
      n[c] = 0;
      if(c == ref_band) {
	 arr[2*c] = arr[2*c + 1] = NULL;
      } else {
	 arr[2*c] = shMalloc(2*nobj*sizeof(U16));
	 arr[2*c + 1] = arr[2*c] + nobj;
      }
   }
/*
 * unpack OBJCs, and set arr arrays to 15000 + 1000*delta_{row,col},
 * where delta_{row,col} is the distance between the row/column centre
 * of the reference band (suitably transformed) and the measured centre
 *
 * phQuartilesGetFromArray only works in U16 as I write this, hence
 * the mapping to 15000 + 1000*delta
 */
   for(i = 0; i < nobj; i++) {
      objc = shChainElementGetByPos(objects, i);
      obj1 = objc->color[ref_band];
      if(obj1 == NULL ||
	 !(obj1->flags & OBJECT1_DETECTED) ||
	 (obj1->flags & (OBJECT1_SATUR | OBJECT1_PEAKCENTER))) {
	 continue;			/* no good reference centre */
      }
      rowc = obj1->rowc; colc = obj1->colc;
      
      for(c = 0; c < ncolor; c++) {
	 if(c != ref_band) {
	    obj1 = objc->color[c];
	    if(obj1 == NULL ||
	       !(obj1->flags & OBJECT1_DETECTED) ||
	       (obj1->flags & (OBJECT1_SATUR | OBJECT1_PEAKCENTER))) {
	       continue;		/* bad centre in this band */
	    }
	    if(obj1->rowcErr >= 0 && obj1->rowcErr < 1 &&
	       obj1->colcErr >= 0 && obj1->colcErr < 1) {
	       phOffsetDo(fiparams, obj1->rowc, obj1->colc, c, ref_band,
			  1, NULL, NULL, &drow, NULL, &dcol, NULL);
	       delta_row = (obj1->rowc + drow - rowc)*1000 + 15000;
	       delta_col = (obj1->colc + dcol - colc)*1000 + 15000;
	       if(delta_row >= 0 && delta_row <= MAX_U16 &&
		  delta_col >= 0 && delta_col <= MAX_U16) {
		  int j = n[c]++;
		  arr[2*c][j] = delta_row + 0.5;
		  arr[2*c + 1][j] = delta_col + 0.5;
	       }
	    }
	 }
      }
   }
   shAssert(objc != NULL && objc->ncolor == ncolor); /* only check last one */
/*
 * find median offsets, if we have enough stars
 */
   for(c = 0; c < ncolor; c++) {
      shAssert(n[c] <= nobj);
      if(c == ref_band || fiparams->astrom_tweak_n_min < 0 ||
	 n[c] < fiparams->astrom_tweak_n_min) {
	 rowOffset[c] = colOffset[c] = 0;
      } else {
	 rowOffset[c] = phQuartilesGetFromArray(arr[2*c], TYPE_PIX, n[c],
						clip, NULL, NULL, NULL) + 0.5;
	 rowOffset[c] -= 15000; rowOffset[c] /= 1000;
	 colOffset[c] = phQuartilesGetFromArray(arr[2*c + 1], TYPE_PIX, n[c],
						clip, NULL, NULL, NULL) + 0.5;
	 colOffset[c] -= 15000; colOffset[c] /= 1000;
      }
   }
/*
 * correct the TRANS structures by those offsets
 */
   for(c = 0; c < ncolor; c++) {
      if(c != ref_band) {
	 phTransShift((TRANS *)fiparams->frame[c].toGCC,
						 -rowOffset[c], -colOffset[c]);
      }
   }
/*
 * cleanup
 */
   for(c = 0; c < ncolor; c++) {
      shFree(arr[2*c]);
   }
}
