#if !defined(PHOFFSET_H)
#define PHOFFSET_H
#include "phFramestat.h"

void
phTransShift(TRANS *trans,		/* TRANS to tweak */
	     float drow,		/* amount to add in row */
	     float dcol);		/*                  and column */

void
phOffsetDo(const FIELDPARAMS *fiparams,	/* describe field, incl. astrometry */
	   float row0,			/* input row */
	   float col0,			/* input column */
	   int band0,			/* which band are row0, col0 in? */
	   int band1,			/* band drow, dcol are required for */
	   int relativeErrors,		/* calc. errors for relative offsets */
	   const float *mag,		/* magnitudes in all bands, or NULL */
	   const float *magErr,		/* errors in mag, or NULL */
	   float *drow,			/* amount to add to get output col */
	   float *drowErr,		/* error in drow, or NULL */
	   float *dcol,			/* amount to add to get output row */
	   float *dcolErr);		/* error in dcol, or NULL */

void
phAstromOffsetsFind(const CHAIN *objects, /* list of objects */
		    int ncolor,		/* number of colours in OBJCs */
		    FIELDPARAMS *fiparams, /* TRANS structs etc. */
		    FIELDSTAT *fieldstat); /* summary of field */

#endif
