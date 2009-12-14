#if !defined(PHVECUTILS_H)
#define PHVECUTILS_H

VECTOR *
phVectorGetFromRegion(const REGION *reg, /* the region with the data */
		      int row0, int row1, /* row range, inclusive */
		      int col0, int col1, /* column range, inclusive */
		      int dirn, /* 0 for squash horizontally, 1 for vertical*/
		      VECTOR **Q,	/* Q[123]; Must be NULL if !use_median;
					   otherwise if not NULL,
					   Q[1] == return value == median */
		      int clip,		/* clip the histogram? */
		      int use_median,	/* use median rather than mean */
		      int nbin);	/* sum together rows/columns in sets
					   of nbin while finding mean/median */

int
phVectorAddToRegionRows(REGION *reg,	/* the REGION... */
			const VECTOR *vector); /* and the VECTOR */
int
phVectorAddToRegionCols(REGION *reg,	/* the REGION... */
			const VECTOR *vector); /* and the VECTOR */

#endif
