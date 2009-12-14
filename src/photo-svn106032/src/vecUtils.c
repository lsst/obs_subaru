/*
 * Utilities concerned with VECTORs
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include "dervish.h"
#include "phConsts.h"
#include "phUtils.h"
#include "phVecUtils.h"

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * extract some range of rows and columns of a REGION
 * into a VECTOR.
 *
 * The rectangular region with corners (row0, col0) and (row1, col1)
 * is averaged over row (dirn == 0) or over column (dirn == 1). Note
 * that dirn == 1 gives a _vertical_ slice through the data (note that
 * a 1 looks a little like a column...)
 *
 * As a special case, a negative value of row1 will be taken to be
 *   nrow - |row1|
 * and similarily for columns
 *
 * The rows/columns will be averaged together in sets of nbin pixels
 * while calculating the means/medians.  This is useful when estimating
 * values for undersampled data such as bias frames
 */
VECTOR *
phVectorGetFromRegion(const REGION *reg, /* the region with the data */
		      int row0, int row1, /* row range, inclusive */
		      int col0, int col1, /* column range, inclusive */
		      int dirn,		/* 0 for collapse onto a column,
					   1 for collapsing onto a row */
		      VECTOR **Q,	/* Q[123]; Must be NULL if !use_median;
					   otherwise if not NULL,
					   Q[1] == return value == median */
		      int clip,		/* clip the histogram? */
		      int use_median,	/* use median rather than mean */
		      int nbin)		/* sum together rows/columns in sets
					   of nbin while finding mean/median */
{
   int dimen;				/* dimension of vec */
   double i_drow;			/* == 1/(double)(row1 - row0 + 1) */
   double i_dcol;			/* == 1/(double)(col1 - col0 + 1) */
   double i_nbin;			/* == 1/(double)nbin */
   int i, j, k;
   float qt[3];				/* quartiles of row/column */
   double sum;				/* used in filling vector */
   VECTOR *vec;				/* vector to return */
   VECTOR_TYPE *vec_vec;		/* == vec->vec */
   VECTOR_TYPE *Q1, *Q3;		/* == Q[{0,2}]->vec, or NULL */

   shAssert(reg != NULL);
   shAssert(!use_median || reg->type == TYPE_PIX);
   shAssert(nbin > 0);
   i_nbin = 1/(double)nbin;

   if(row0 < 0) row0 = reg->nrow + row0;
   if(row1 < 0) row1 = reg->nrow + row1;
   if(col0 < 0) col0 = reg->ncol + col0;
   if(col1 < 0) col1 = reg->ncol + col1;

   if(row0 > row1 || col0 > col1) {
      shErrStackPush("phVectorGetFromRegion: row/col out of order");
      return(NULL);
   }
   if(row0 < 0 || row1 >= reg->nrow || col0 < 0 || col1 >= reg->ncol) {
      shErrStackPush("phVectorGetFromRegion: row/col out of range");
      return(NULL);
   }

   if(dirn == 0) {
      dimen = col1 - col0 + 1;
   } else if(dirn == 1) {
      dimen = row1 - row0 + 1;
   } else {
      shErrStackPush("phVectorGetFromRegion: illegal value of dirn: %d",dirn);
      return(NULL);
   }
   vec = shVectorNew(dimen); vec_vec = vec->vec;

   if(Q == NULL) {
      Q1 = Q3 = NULL;
   } else {
      shAssert(use_median);
      Q[0] = shVectorNew(dimen); Q1 = Q[0]->vec;
      Q[1] = vec;
      Q[2] = shVectorNew(dimen); Q3 = Q[2]->vec;
   }
/*
 * OK, time to do the work
 */
   if(nbin != 1) {
      if(dirn == 0) {
	 row1 = row0 + nbin*((row1 - row0 + 1)/nbin) - 1;
      } else {
	 col1 = col0 + nbin*((col1 - col0 + 1)/nbin) - 1;
      }
   }
   i_dcol = 1/(double)(col1 - col0 + 1);
   i_drow = 1/(double)(row1 - row0 + 1);
/*
 * The work is _identical_ for all data types, so let's use a macro;
 * this makes it hard to debug, of course....
 */
#define DO_WORK(TYPE) \
   if(dirn == 0) { \
      if(nbin != 1) { \
	 row1 = row0 + nbin*((row1 - row0)/nbin); \
      } \
      if(use_median) { \
	 int nel = (row1 - row0 + 1)/nbin; \
	 TYPE *arr = alloca(nel*sizeof(TYPE)); \
	 memset(arr, '\0', sizeof(TYPE)); shAssert(arr[0] == 0); \
	  \
	 for(j = col0; j <= col1; j++) { \
	    memset(arr, '\0', nel*sizeof(TYPE)); \
	     \
	    if(nbin > 1) { \
	       for(i = 0; i < nel; i++) { \
		  for(k = 0; k < nbin; k++) { \
		     arr[i] += rows[row0 + i*nbin + k][j]; \
		  } \
	       } \
	    } else { \
	       for(i = 0; i < nel; i++) { \
		  arr[i] = rows[i + row0][j]; \
	       } \
	    } \
            \
	    vec_vec[j - col0] = phQuartilesGetFromArray(arr, reg->type, nel, \
							clip, qt, NULL, NULL);\
	    if(Q1 != NULL) { \
	       Q1[j - col0] = qt[0]; \
	       Q3[j - col0] = qt[2]; \
	    } \
            \
	    if(nbin > 1) { \
	       vec_vec[j - col0] *= i_nbin; \
	       if(Q1 != NULL) { \
		  Q1[j - col0] *= i_nbin; \
		  Q3[j - col0] *= i_nbin; \
	       } \
	    } \
	 } \
      } else { \
	 for(j = col0; j <= col1; j++) { \
	    vec_vec[j - col0] = 0; \
	 } \
	 \
	 for(i = row0; i <= row1; i++) { \
	    row = rows[i]; \
	 \
	    for(j = col0; j <= col1; j++) { \
	       vec_vec[j - col0] += row[j]; \
	    } \
	 } \
	 for(j = col0; j <= col1; j++) { \
	    vec_vec[j - col0] = vec->vec[j - col0]*i_drow; \
	 } \
      } \
   } else { \
      int nel = (col1 - col0 + 1)/nbin; \
      TYPE *arr = NULL; \
      if(use_median && nbin > 1) { \
	 arr = alloca(nel*sizeof(TYPE)); \
      } \
      for(i = row0; i <= row1; i++) { \
	 row = &rows[i][col0]; \
	 if(use_median) { \
	    if(nbin == 1) { \
	       arr = row; \
	    } else { \
	       memset(arr, '\0', nel*sizeof(TYPE)); shAssert(arr[0] == 0); \
               \
	       for(j = 0; j < nel; j++) { \
		  for(k = 0; k < nbin; k++) { \
		     arr[j] += row[j*nbin + k]; \
		  } \
	       } \
	    } \
            \
	    vec_vec[i - row0] = phQuartilesGetFromArray(arr, reg->type, nel, \
							clip, qt, NULL, NULL);\
            \
	    if(Q1 != NULL) { \
	       Q1[i - row0] = qt[0]; \
	       Q3[i - row0] = qt[2]; \
	    } \
	     \
	    if(nbin > 1) { \
	       vec_vec[i - row0] *= i_nbin; \
	       if(Q1 != NULL) { \
		  Q1[i - row0] *= i_nbin; \
		  Q3[i - row0] *= i_nbin; \
	       } \
	    } \
	 } else { \
	    sum = 0; \
	    for(j = col0; j <= col1; j++) { \
	       sum += row[j]; \
	    } \
	    vec_vec[i - row0] = sum*i_dcol; \
	 } \
      } \
   }

   switch (reg->type) {
    case TYPE_U8:
      {
	 U8 **rows = reg->rows_u8, *row;

	 DO_WORK(U8);
      }

      break;
    case TYPE_S8:
      {
	 S8 **rows = reg->rows_s8, *row;

	 DO_WORK(S8);
      }
      break;
    case TYPE_U16:
      {
	 U16 **rows = reg->rows_u16, *row;

	 DO_WORK(U16);
      }

      break;
    case TYPE_S16:
      {
	 S16 **rows = reg->rows_s16, *row;

	 DO_WORK(S16);
      }

      break;
    case TYPE_U32:
      {
	 U32 **rows = reg->rows_u32, *row;

	 DO_WORK(U32);
      }

      break;
    case TYPE_S32:
      {
	 S32 **rows = reg->rows_s32, *row;

	 DO_WORK(S32);
      }

      break;
    case TYPE_FL32:
      {
	 FL32 **rows = reg->rows_fl32, *row;

	 DO_WORK(FL32);
      }

      break;
    default:
      shErrStackPush("phVectorGetFromRegion: unsupported type %d",reg->type);
      shVectorDel(vec);
      return(NULL);
   }
   
   return(vec);
}

/*****************************************************************************/
/*
 * Add a VECTOR to every row of a REGION
 *
 * Only some region types are supported, routine returns 0 if OK, -1 otherwise
 */
int
phVectorAddToRegionRows(REGION *reg,	/* the REGION... */
			const VECTOR *vector) /* and the VECTOR */
{
   int i,j;
   int nrow, ncol;			/* == reg->n{row,col} */
   VECTOR_TYPE *vec;			/* == vector->vec */
   
   shAssert(reg != NULL && vector != NULL && reg->ncol == vector->dimen);
   shAssert(sizeof(vec) == sizeof(vector->vec));
   nrow = reg->nrow; ncol = reg->ncol;
   vec = vector->vec;

   switch(reg->type) {
    case TYPE_U16:
      {
	 U16 *row;

	 for(i = 0; i < nrow; i++) {
	    row = reg->rows_u16[i];
	    for(j = 0; j < ncol; j++) {
	       row[j] += (vec[j] > 0) ? vec[j] + 0.5 : -(-vec[j] + 0.5);
	    }
	 }
      }
      return(0);
    case TYPE_FL32:
      {
	 FL32 *row;
	 for(i = 0; i < nrow; i++) {
	    row = reg->rows_fl32[i];
	    for(j = 0; j < ncol; j++) {
	       row[j] += vec[j];
	    }
	 }
      }
      return(0);
    default:
      shErrStackPush("phVectorAddToRegionRows: "
		     "unsupported REGION type %d",reg->type);
      break;
   }

   return(-1);   
}

/*****************************************************************************/
/*
 * Add a VECTOR to every column of a REGION
 *
 * Only some region types are supported, routine returns 0 if OK, -1 otherwise
 */
int
phVectorAddToRegionCols(REGION *reg,	/* the REGION... */
			const VECTOR *vector) /* and the VECTOR */
{
   int i,j;
   int nrow, ncol;			/* == reg->n{row,col} */
   VECTOR_TYPE *vec;			/* == vector->vec */
   
   shAssert(reg != NULL && vector != NULL && reg->nrow == vector->dimen);
   nrow = reg->nrow; ncol = reg->ncol;
   vec = vector->vec;

   switch(reg->type) {
    case TYPE_U16:
      {
	 U16 *row;
	 int val;			/* value to add */
	 for(i = 0; i < nrow; i++) {
	    row = reg->rows_u16[i];
	    val = (vec[i] > 0) ? vec[i] + 0.5 : -(-vec[i] + 0.5);
	    for(j = 0; j < ncol; j++) {
	       row[j] += val;
	    }
	 }
      }
      return(0);
    case TYPE_FL32:
      {
	 FL32 *row;
	 VECTOR_TYPE val;
	 for(i = 0; i < nrow; i++) {
	    row = reg->rows_fl32[i];
	    val = vec[i];
	    for(j = 0; j < ncol; j++) {
	       row[j] += val;
	    }
	 }
      }
      return(0);
    default:
      shErrStackPush("phVectorAddToRegionRows: "
		     "unsupported REGION type %d",reg->type);
      break;
   }

   return(-1);   
}
