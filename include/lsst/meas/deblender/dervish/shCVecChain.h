#if !defined(SHCVECCHAIN_H)
#define SHCVECCHAIN_H

#include "shChain.h"
#include "shCVecExpr.h"
#include "region.h"
#if !defined(NOTCL)
#include "shCHg.h"
#endif


/*
 * Additional operations
 */
VECTOR *shVFromChain(CHAIN *chain, char *member);
VECTOR **shVectorsFromChain(const CHAIN *chain, int nvec, char **members);

#if !defined(NOTCL)
RET_CODE shVPlot(
		  PGSTATE *pgstate, /* where to plot it */
		  VECTOR *vectorX,       /* vector to use for x */
		  VECTOR *vectorXErr,       /* vector to use for x error */
		  VECTOR *vectorY,       /* vector to use for y */
		  VECTOR *vectorYErr,       /* vector to use for y error*/
		  VECTOR *vectorMask,    /* mask to pick only part of data */
		  VECTOR *vectorSymbol,  /* vector for symbol */
		  VECTOR *vectorColor,   /* vector for color */
		  double xminIn,    /* min value for x */
		  double xmaxIn,    /* max value for x */
		  double yminIn,    /* min value for y */
		  double ymaxIn,    /* max value for y */
		  int xyOptMask /* XMINOPT | XMAXOPT, etc., to use above 
				   values, not calculated ones */ 
		  );
#endif

VECTOR_TYPE shVExtreme(
		       VECTOR *vector, /* the vector for an extreme */
		       VECTOR *vMask, /* use only this part of vector */
		       char *minORmax /* set to min or max to get result */
		       );
VECTOR_TYPE shVLimit(
		     VECTOR *vector, /* the vector for a limit */
		     VECTOR *vMask, /* use only this part of vector */
		     char *minORmax /* set to min or max to get result */
		     );

RET_CODE shVNameSet(
		    VECTOR *vector, /* the vector to set */
		    char* name /* what to name it */
		    );
char *shVNameGet(
		    VECTOR *vector /* the vector to get the name from */
		    );

RET_CODE shVStatistics(
		       VECTOR* vector, /* IN:  the vector to evaluate */
		       VECTOR* vMask, /* IN:  the vector to evaluate */
		       char* operation, /* IN:  either "mean" or "sigma" */
		       VECTOR_TYPE* result /* OUT:  the answer */
		       );

int VECTOR_TYPECompare(const void *d1, const void *d2);

VECTOR_TYPE  shVMedian(
		   VECTOR *vector, /* the VECTOR to return the median of */
		   VECTOR *vMask
		 );

RET_CODE shVSigmaClip(VECTOR *vector, VECTOR *vMask, 
		       VECTOR_TYPE sigmaClip, 
		       unsigned int nIter,
		       VECTOR_TYPE *mean, VECTOR_TYPE *sqrtVar);

CHAIN  *shVToChain(VECTOR* vector, CHAIN *chain,
		    char *member);

VECTOR *
shVectorGetFromRegion(const REGION *reg, /* the region with the data */
                      int row0, int row1, /* row range, inclusive */
                      int col0, int col1, /* column range, inclusive */
                      int dirn, /* 0 for squash horizontally, 1 for vertical*/
                      int use_median);  /* use a median? */
RET_CODE
shVectorSetInRegion(const REGION *reg, /* the region with the data */
					  VECTOR *vec, /* the vector to insert */
                      int row, 
                      int col,
                      int dirn); /* 0 to insert row, 1 to insert col */

#if !defined(NOTCL)
RET_CODE shHgFillFromV(HG *hg, VECTOR *vector, VECTOR *vMask);

HG *shHgNewFromV(
		 VECTOR *vector, /* the VECTOR that will supply values */
		 VECTOR *vMask, /* what part of the vector to use */
		 VECTOR *vWeight, /* weight vector */
		 unsigned int nbin, /* the number of bins to create in HG */
		 VECTOR_TYPE min, /* min calculated if >= max */
                 VECTOR_TYPE max, /* max */
                 char *name       /* name to give hg, vector name otherwise */
                 );
#endif
#endif



