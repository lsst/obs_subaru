/*
 * Find all the peaks in an OBJECT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <alloca.h>
#include "dervish.h"
#include "phObjects.h"

/*****************************************************************************/

#define NSIZE 2				/* size of box for naive search
					   is 2*NSIZE + 1 */
#define NSADDLE 3			/* max number of pixels to seach for
					   a saddle point */

static int
check_saddlepoint(const REGION *reg,	/* region containing PEAKS */
		  const PEAK *peak,	/* the peak in question */
		  const OBJMASK *om,	/* the object containing the peaks */
		  int delta,		/* amount peak must exceed the
					   saddlepoint value */
		  int npeak_max)	/* maximum number of peaks */
{
   int npix[2*NSIZE + 1][2*NSIZE + 1] = { /* number of points to check */
      { 1,        2,         3,         2,         1 },
      { 2,        0,         0,         0,         2 },
      { 3,        0,         0,         0,         2 },
      { 2,        0,         0,         0,         2 },
      { 1,        2,         3,         2,         1 }};
   int rarr[2*NSIZE + 1][2*NSIZE + 1][NSADDLE] = { /* row coords to check */
      {{1      }, {1, 1   }, {1, 1, 1}, {1, 1   }, {1      }},
      {{1, 2   }, {0      }, {0      }, {0      }, {1, 2   }},
      {{1, 2, 3}, {0      }, {0      }, {0      }, {1, 2, 3}},
      {{2, 3   }, {0      }, {0      }, {0      }, {2, 3   }},
      {{3      }, {3, 3   }, {3, 3, 3}, {3, 3   }, {3      }}};
   int carr[2*NSIZE + 1][2*NSIZE + 1][NSADDLE] = { /* column coords to check */
      {{1      }, {1, 2   }, {1, 2, 3}, {2, 3   }, {3      }},
      {{1, 1   }, {0      }, {0      }, {0      }, {3, 3   }},
      {{1, 1, 1}, {0      }, {0      }, {0      }, {3, 3, 3}},
      {{1, 1   }, {0      }, {0      }, {0      }, {3, 3   }},
      {{1      }, {1, 2   }, {1, 2, 3}, {2, 3   }, {3      }}};
   int i;
   int r, c;				/* row and column counters */
   int *rr, *cc;			/* == {rarr,carr}[r][c] */
   int rp, cp, pp;			/* position and intensity of peak */
   PIX **rows, *row;			/* == reg->ROWS, reg->ROWS[r] */

   shAssert(reg != NULL && reg->type == TYPE_PIX);
   shAssert(peak != NULL);
   rows = reg->ROWS;
   rp = peak->rpeak; cp = peak->cpeak;
   pp = rows[rp][cp];
/*
 * first try a naive search in the vicinity of the peak
 */
   if(rp < NSIZE || rp >= reg->nrow - NSIZE ||
      cp < NSIZE || cp >= reg->ncol - NSIZE) {
      ;					/* too close to edge */
   } else {
      for(r = 0;r < (2*NSIZE + 1);r++) {
	 row = &rows[rp - NSIZE + r][cp - NSIZE];
	 for(c = 0;c < (2*NSIZE + 1);c++) {
	    if(row[c] > pp) {
	       rr = rarr[r][c];
	       cc = carr[r][c];
	       for(i = npix[r][c] - 1;i >= 0;i--) {
		  if(rows[rr[i] + rp - NSIZE][cc[i] + cp - NSIZE] >=
								  pp - delta) {
		     return(1);
		  }
	       }
	    }
	 }
      }
   }
/*
 * The naive approach failed; we'll have to run the object finder
 */
   if(om != NULL) {			/* run object finder within object */
      unsigned short levels = pp - delta;
      int nobj;				/* number of objects detected */
      CHAIN *objs;			/* all objects in om above levels */
      OBJECT *obj;			/* an object on the objs list */
      int ret = 0;			/* return code from routine */

      objs = phObjectsFindInObjmask(reg, om, 1, &levels, -1, 0, npeak_max);
      shAssert(objs != NULL);
      nobj = objs->nElements;

      for(i = 0;i < nobj;i++) {
	 obj = shChainElementRemByPos(objs,HEAD);
	 if(phPixIntersectObjmask(obj->sv[0],cp,rp)) { /* the right object */
	    if(rp != obj->peaks->peaks[0]->rpeak ||
					   cp != obj->peaks->peaks[0]->cpeak) {
	       ret = 1;
	       phObjectDel(obj);
	       break;
	    }
	 }
	 phObjectDel(obj);
      }

      for(;i < nobj;i++) {
	 phObjectDel(shChainElementRemByPos(objs, HEAD));
      }

      shChainDel(objs);

      return(ret);
   }

   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Cull some of a list of PEAKs. Specifically:
 *  1/ Those with a peak value < threshold
 *  2/ Each peak (except the first) has a saddle point joining it to
 *     a higher peak; if the value at that saddle point is >= (peak - delta),
 *     cull the peak.
 *
 * When dealing with saturated peaks, the "peak" position may not be at
 * a maximum of the intensity.  We therefore determine for each peak
 * the contour delta below it's peak value, and require that the brightest
 * peak within this contour is not culled.  In the simple case of a single
 * object, this reduces to ensuring that the brightest peak is kept.
 *
 * If delta is negative, it's taken to be a multiple of the local pixel
 * standard deviation, as calculated using the gain, sky_var, and neff
 * values.
 */
void
phPeaksCull(const REGION *reg,		/* region containing PEAKS */
	    PEAKS *peaks,		/* the peaks in question */
	    const OBJMASK *om,		/* the object containing the peaks */
	    int threshold,		/* minimum acceptable peak value */
	    float delta,		/* by how much peaks must exceed
					   the saddlepoint value */
	    float gain,			/* gain of chip */
	    float sky_var,		/* total variance/pixel of sky */
	    float neff)			/* effective area due to smoothing */
{
   int cull;				/* should we cull this peak? */
   float *deltas = NULL;		/* values of delta for each peak */
   int i, j;
   float igain_neff;			/* == 1/(gain*neff) */

   shAssert(peaks != NULL);
   shAssert(delta >= 0 || (gain > 0 && neff > 0 && sky_var >= 0));
   if(peaks->npeak > 0) {
     deltas = alloca(peaks->npeak*sizeof(float));
   }

   if(delta >= 0) {
      igain_neff = 0;			/* NOTUSED */
   } else {
      igain_neff = 1/(gain*neff);
   }
   
#if 0
   if(peaks->npeak > 0) {
      peaks->peaks[0]->flags |= PEAK_REAL;	/* never cull first PEAK */
   }
#else
   for(i = 0; i < peaks->npeak; i++) {	/* calculate values of delta */
     if(delta >= 0) {			
       deltas[i] = delta;
     } else {
       float pix_var = sky_var + peaks->peaks[i]->peak*igain_neff; /*variance*/
       deltas[i] = -delta*sqrt(pix_var);
     }
   }
   /*
    * For each peak, find mask of detected pixels above (peak - deltas[i])
    * and keep the first peak that lies in that mask
    */
   {
       int nreal = 0;
       for(i = 0; i < peaks->npeak; i++) {
	   OBJMASK *mask =
	       phObjmaskFindInObjmask(reg, om,
				      peaks->peaks[i]->rpeak,
				      peaks->peaks[i]->cpeak,
				      peaks->peaks[i]->peak - deltas[i] + SOFT_BIAS);
	   if(mask != NULL) {
	       for(j = 0; j < peaks->npeak; j++) {
		   if(phPixIntersectObjmask(mask,
					    peaks->peaks[j]->cpeak,
					    peaks->peaks[j]->rpeak)) {
		       nreal++;
		       peaks->peaks[j]->flags |= PEAK_REAL;	/* never cull first PEAK */
		       break;
		   }
	       }
	   }
	   
	   phObjmaskDel(mask);
       }

       if(nreal == 0) {
	   peaks->peaks[0]->flags |= PEAK_REAL;	/* keep first PEAK */
       }
   }
#endif
   
   for(i = 0;i < peaks->npeak;i++) {	
      if(peaks->peaks[i]->flags & PEAK_REAL) { /* it's already passed muster */
	 continue;
      }

      cull = 0;
      if(peaks->peaks[i]->peak < threshold) {
	 cull = 1;
      } else if (!(peaks->peaks[i]->flags & PEAK_SATUR)) { /* saddlepoint check fails for interpolated saturated peaks */
	 cull = check_saddlepoint(reg, peaks->peaks[i], om,
				  deltas[i], 2*peaks->npeak);
      }

      if(cull) {
	 phPeakDel(peaks->peaks[i]); peaks->peaks[i] = NULL;
      } else {
	 peaks->peaks[i]->flags |= PEAK_REAL;
      }
   }
/*
 * remove deleted peaks from the list
 */
   for(i = j = 0; i < peaks->npeak; i++) {
      if(peaks->peaks[i] != NULL) {
	 peaks->peaks[j++] = peaks->peaks[i];
      }
   }
   for(i = j; i < peaks->npeak; i++) {
     peaks->peaks[i] = NULL;
   }
   peaks->npeak = j;
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Cull the peaks in an OBJECT chain; see phPeaksCull for more information
 * on which peaks are deleted.
 *
 * If delta is positive, neff and fparams is not used and fparams must be NULL;
 * if it's negative its absolute value is taken to be the number of local sigma
 */
void
phObjectChainPeaksCull(const REGION *reg, /* REGION that objects live in */
		       const CHAIN *objects, /* the chain of OBJECTs to cull */
		       int threshold,	/* threshold for acceptable peaks */
		       float delta,	/* amount peaks must exceed
					   their saddle points */
		       const FRAMEPARAMS *fparams, /* gain etc. of chip */
		       float neff)	/* n_{effective} of any smoothing */
{
   CURSOR_T curs;			/* cursor for objs */
   OBJECT *obj = NULL;			/* OBJECT with peaks to cull */

   shAssert(objects != NULL && objects->type == shTypeGetFromName("OBJECT"));
   shAssert(reg != NULL && reg->type == TYPE_PIX);

   if(delta < 0) {			/* it's really a number of s.d. */
      shAssert(fparams != NULL && fparams->sky != NULL && neff > 0);
   } else {
      shAssert(fparams == NULL);	/* it's not used */
      neff = 1;				/* not used either */
   }

   curs = shChainCursorNew(objects);
   while((obj = shChainWalk(objects,curs,NEXT)) != NULL) {
	const float gain = phGain(fparams, obj->rowc, obj->colc); shAssert(gain > 0);
	const float dark_variance = phDarkVariance(fparams, obj->rowc, obj->colc); shAssert(dark_variance >= 0);
	const float sky = phBinregionInterpolate(fparams->sky, obj->rowc, obj->colc);
	float sky_variance = (dark_variance + sky/gain)/neff; /* variance/pixel of sky */

	if (sky_variance < 0) {			/* sky must be -ve; measure objects
						   will fail */
	     sky_variance = 0;
	     obj->flags |= OBJECT1_BADSKY;
	}
	
	phPeaksCull(reg,obj->peaks,obj->sv[0], threshold, delta, gain, sky_variance, neff);
   }

   shChainCursorDel(objects,curs);
}
