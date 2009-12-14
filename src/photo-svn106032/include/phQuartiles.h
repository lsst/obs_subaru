#if !defined(PHQUARTILES_H)
#define PHQUARTILES_H
#include "dervish.h"
#include "phCcdpars.h"

typedef struct {
   int ntilesPercentiles[4];		/* percentages in ntiles - (25,50,75)*/
                                        /* Use [3] == 0 for mode */
   int nr, nc;				/* nrows and ncolumns in image */
   int tscal;				/* factor by which the data is scaled*/
   REGION *data[4];			/* 4 values for each block of data */
} QUARTILES; /* pragma SCHEMA */

QUARTILES *phQuartilesNew(int nrow, int ncol);
void phQuartilesDel(QUARTILES *quartiles);
RET_CODE phTblcolToQuartiles(TBLCOL *tblcol, QUARTILES *quartiles, int row);
REGION   *phQuartilesToRegion(QUARTILES *quart, CCDPARS *ccdpars);

#endif
