/*
 * Here are a selection of mathematical utilities used by photo, that
 * it seems foolish to get from cernlib
 */
#include <stdlib.h>
#include <string.h>
#include <math.h> 
#include <alloca.h>
#include "dervish.h"
#include "astrotools.h"			/* for M_PI */
#include "atMatrix.h"


/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Invert an n*n float matrix arr in place using Gaussian Elimination
 * with pivoting; return -1 for singular matrices
 *
 * Also solve the equation arr*b = x, replacing b by x (if b is non-NULL)
 *
 * The matrix arr is destroyed during the inversion
 *
 *		taken from photo's mathUtils.c at photo version v5_1_3
 */
int
atMatrixInvert(float **arr,		/* matrix to invert */
	       float *b,		/* Also solve arr*x = b; may be NULL */
	       int n)			/* dimension of iarr and arr */
{
   float abs_arr_jk;			/* |arr[j][k]| */
   int row, col;			/* pivotal row, column */
   int i, j, k;
   int *icol, *irow;			/* which {column,row}s we pivoted on */
   int *pivoted;			/* have we pivoted on this column? */
   float pivot;				/* value of pivot */
   float ipivot;			/* 1/pivot */
   float tmp;

   icol = alloca(3*n*sizeof(int));
   irow = icol + n;
   pivoted = irow + n;

   for(i = 0;i < n;i++) {
      pivoted[i] = 0;
   }

   for(i = 0;i < n;i++) {
/*
 * look for a pivot
 */
      row = col = -1;
      pivot = 0;
      for(j = 0;j < n;j++) {
	 if(pivoted[j] != 1) {
	    for(k = 0;k < n;k++) {
	       if(!pivoted[k]) {	/* we haven't used this column */
		  abs_arr_jk = (arr[j][k] > 0) ? arr[j][k] : -arr[j][k];
		  if(abs_arr_jk >= pivot) {
		     pivot = abs_arr_jk;
		     row = j; col = k;
		  }
	       } else if(pivoted[k] > 1) { /* we've used this column already */
		  return(-1);		/* so it's singular */
	       }
	    }
	 }
      }
      shAssert(row >= 0);
/*
 * got the pivot; now move it to the diagonal, i.e. move the pivotal column
 * to the row'th column
 *
 * We have to apply the same permutation to the solution vector, b
 */
      pivoted[col]++;			/* we'll pivot on column col */
      icol[i] = col; irow[i] = row;
      if((pivot = arr[row][col]) == 0.0) {
	 return(-1);			/* singular */
      }

      if(row != col) {
	 for(j = 0;j < n;j++) {
	    tmp = arr[row][j];
	    arr[row][j] = arr[col][j];
	    arr[col][j] = tmp;
	 }
	 if(b != NULL) {
	    tmp = b[row];
	    b[row] = b[col];
	    b[col] = tmp;
	 }
      }
/*
 * divide pivotal row by pivot
 */
      ipivot = 1/pivot;
      arr[col][col] = 1.0;
      for(j = 0;j < n;j++) {
	 arr[col][j] *= ipivot;
      }
      if(b != NULL) {
	 b[col] *= ipivot;
      }
/*
 * and reduce other rows
 */
      for(j = 0;j < n;j++) {
	 if(j == col) {			/* not the pivotal one */
	    continue;
	 }

	 tmp = arr[j][col];
	 arr[j][col] = 0.0;
	 for(k = 0;k < n;k++) {
	    arr[j][k] -= tmp*arr[col][k];
	 }
	 if(b != NULL) {
	    b[j] -= tmp*b[col];
	 }
      }
   }
/*
 * OK, we're done except for undoing a permutation of the columns
 */
   for(i = n - 1;i >= 0;i--) {
      if(icol[i] != irow[i]) {
	 for(j = 0;j < n;j++) {
	    tmp = arr[j][icol[i]];
	    arr[j][icol[i]] = arr[j][irow[i]];
	    arr[j][irow[i]] = tmp;
	 }
      }
   }
   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Invert an n*n double matrix arr in place using Gaussian Elimination
 * with pivoting; return -1 for singular matrices
 *
 * Also solve the equation arr*b = x, replacing b by x (if b is non-NULL)
 *
 * The matrix arr is destroyed during the inversion
 */
int
atDMatrixInvert(double **arr,		/* matrix to invert */
		double *b,		/* Also solve arr*x = b; may be NULL */
		int n)			/* dimension of iarr and arr */
{
   double abs_arr_jk;			/* |arr[j][k]| */
   int row, col;			/* pivotal row, column */
   int i, j, k;
   int *icol, *irow;			/* which {column,row}s we pivoted on */
   int *pivoted;			/* have we pivoted on this column? */
   double pivot;				/* value of pivot */
   double ipivot;			/* 1/pivot */
   double tmp;

   icol = alloca(3*n*sizeof(int));
   irow = icol + n;
   pivoted = irow + n;

   for(i = 0;i < n;i++) {
      pivoted[i] = 0;
   }

   for(i = 0;i < n;i++) {
/*
 * look for a pivot
 */
      row = col = -1;
      pivot = 0;
      for(j = 0;j < n;j++) {
	 if(pivoted[j] != 1) {
	    for(k = 0;k < n;k++) {
	       if(!pivoted[k]) {	/* we haven't used this column */
		  abs_arr_jk = (arr[j][k] > 0) ? arr[j][k] : -arr[j][k];
		  if(abs_arr_jk >= pivot) {
		     pivot = abs_arr_jk;
		     row = j; col = k;
		  }
	       } else if(pivoted[k] > 1) { /* we've used this column already */
		  return(-1);		/* so it's singular */
	       }
	    }
	 }
      }
      shAssert(row >= 0);
/*
 * got the pivot; now move it to the diagonal, i.e. move the pivotal column
 * to the row'th column
 *
 * We have to apply the same permutation to the solution vector, b
 */
      pivoted[col]++;			/* we'll pivot on column col */
      icol[i] = col; irow[i] = row;
      if((pivot = arr[row][col]) == 0.0) {
	 return(-1);			/* singular */
      }

      if(row != col) {
	 for(j = 0;j < n;j++) {
	    tmp = arr[row][j];
	    arr[row][j] = arr[col][j];
	    arr[col][j] = tmp;
	 }
	 if(b != NULL) {
	    tmp = b[row];
	    b[row] = b[col];
	    b[col] = tmp;
	 }
      }
/*
 * divide pivotal row by pivot
 */
      ipivot = 1/pivot;
      arr[col][col] = 1.0;
      for(j = 0;j < n;j++) {
	 arr[col][j] *= ipivot;
      }
      if(b != NULL) {
	 b[col] *= ipivot;
      }
/*
 * and reduce other rows
 */
      for(j = 0;j < n;j++) {
	 if(j == col) {			/* not the pivotal one */
	    continue;
	 }

	 tmp = arr[j][col];
	 arr[j][col] = 0.0;
	 for(k = 0;k < n;k++) {
	    arr[j][k] -= tmp*arr[col][k];
	 }
	 if(b != NULL) {
	    b[j] -= tmp*b[col];
	 }
      }
   }
/*
 * OK, we're done except for undoing a permutation of the columns
 */
   for(i = n - 1;i >= 0;i--) {
      if(icol[i] != irow[i]) {
	 for(j = 0;j < n;j++) {
	    tmp = arr[j][icol[i]];
	    arr[j][icol[i]] = arr[j][irow[i]];
	    arr[j][irow[i]] = tmp;
	 }
      }
   }

   return(0);
}

