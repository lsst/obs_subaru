/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* htrans.c   H-transform of NX x NY integer image
 *
 * Programmer: R. White		Date: 13 June 1994
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "ftcl.h"
#include "dervish.h"
#include "region.h"
#include "math.h"
#include "dervish_msg_c.h"
#include "errStack.h"
#include "phStructs.h"
#include "phCompUtils.h"

#define COMP_DEBUG 10


static int xshuffle(int *a, int nx, int ny, int nydim);
static int yshuffle(int *a, int nx, int ny, int nydim);

int
htrans(int a[], int nx, int ny)
{
  int nmax, log2n, nxtop, nytop, i, k;
  int shift, mask, mask2, prnd, prnd2, nrnd2;
  int h0, hx, hy, hc, sum1, sum2, sum3, sum4;
  int *p00, *p10, *pend;
  
  /*
   * log2n is log2 of max(nx,ny) rounded up to next power of 2
   */
  nmax = (nx>ny) ? nx : ny;
  log2n = log((float) nmax)/log(2.0)+0.5;
  if ( nmax > (1<<log2n) ) {
    log2n += 1;
  }
  /*
   * set up rounding and shifting masks
   */
  shift = 0;
  mask  = -2;
  mask2 = mask << 1;
  prnd  = 1;
  prnd2 = prnd << 1;
  nrnd2 = prnd2 - 1;
  /*
   * do log2n reductions
   *
   * We're indexing a as a 2-D array with dimensions (nx,ny).
   */
  nxtop = nx;
  nytop = ny;
  for (k = 0; k<log2n; k++) {
    for (i = 0; i<nxtop-1; i += 2) {
      pend = &a[i*ny+nytop-1];
      for (p00 = &a[i*ny], p10 = p00+ny; p00<pend; p00 += 2, p10 += 2) {
	/*
	 * Divide h0,hx,hy,hc by 2 (1 the first time through).
	 */
	sum1 = *(p10+1) + *p10;
	sum2 = *(p00+1) + *p00;
	sum3 = *(p10+1) - *p10;
	sum4 = *(p00+1) - *p00;
	if (shift==0) {
	  h0 = (sum1 + sum2);
	  hx = (sum1 - sum2);
	  hy = (sum3 + sum4);
	  hc = (sum3 - sum4);
	} else {
	  h0 = (sum1 + sum2) >> 1;
	  hx = (sum1 - sum2) >> 1;
	  hy = (sum3 + sum4) >> 1;
	  hc = (sum3 - sum4) >> 1;
	}
	/*
	 * Throw away the 2 bottom bits of h0, bottom bit of hx,hy.
	 * To get rounding to be same for positive and negative
	 * numbers, nrnd2 = prnd2 - 1.
	 */
	*(p10+1) = hc;
	*(p10  ) = ( (hx>=0) ? (hx+prnd)  :  hx        ) & mask ;
	*(p00+1) = ( (hy>=0) ? (hy+prnd)  :  hy        ) & mask ;
	*(p00  ) = ( (h0>=0) ? (h0+prnd2) : (h0+nrnd2) ) & mask2;
      }
      if (p00 == pend) {
	/*
	 * do last element in row if row length is odd
	 * (p00+1), (p10+1) are off edge
	 */
	h0 = (*p10 + *p00) << (1-shift);
	hx = (*p10 - *p00) << (1-shift);
	*p10 = ( (hx>=0) ? (hx+prnd)  :  hx        ) & mask ;
	*p00 = ( (h0>=0) ? (h0+prnd2) : (h0+nrnd2) ) & mask2;
      }
    }
    if (i<nxtop) {
      /*
       * do last row if column length is odd
       * p10, (p10+1) are off edge
       */
      pend = &a[i*ny+nytop-1];
      for (p00 = &a[i*ny]; p00<pend; p00 += 2) {
	h0 = (*(p00+1) + *p00) << (1-shift);
	hy = (*(p00+1) - *p00) << (1-shift);
	*(p00+1) = ( (hy>=0) ? (hy+prnd)  :  hy        ) & mask ;
	*(p00  ) = ( (h0>=0) ? (h0+prnd2) : (h0+nrnd2) ) & mask2;
      }
      if (p00==pend) {
	/*
	 * do corner element if both row and column lengths are odd
	 * (p00+1), p10, (p10+1) are off edge
	 */
	h0 = *p00 << (2-shift);
	*p00 = ( (h0>=0) ? (h0+prnd2) : (h0+nrnd2) ) & mask2;
      }
    }
    /*
     * now shuffle in each dimension to group coefficients by order
     */
    xshuffle(a,nxtop,nytop,ny);
    yshuffle(a,nxtop,nytop,ny);
    /*
     * image size reduced by 2 (round up if odd)
     */
    nxtop = (nxtop+1)>>1;
    nytop = (nytop+1)>>1;
    /*
     * divisor doubles after first reduction
     */
    shift = 1;
    /*
     * masks, rounding values double after each iteration
     */
    mask  = mask2;
    prnd  = prnd2;
    mask2 = mask2 << 1;
    prnd2 = prnd2 << 1;
    nrnd2 = prnd2 - 1;
  }
  return(SH_SUCCESS);
}

/*
 * shuffle in x direction to get even elements at beginning of
 * array, odd elements at end.  procedure is to copy odd elements
 * to temporary array, compress even elements to beginning of row,
 * and this copy odd elements to end of array.
 * int a[];	 array to shuffle				  
 * int nx;	 number of elements in column		
 * int ny;	 number of elements in row		
 * int nydim;	 actual length of row in array	
 */
static int
xshuffle(int a[], int nx, int ny, int nydim)
{
  int j, *p1, *p2, *pt, *pend, *tmp;

  /*
   * get temporary storage for shuffling elements
   */
  tmp  = (int *) malloc(((ny+1)/2)*sizeof(int));
  if(tmp == (int *) NULL) {
    fprintf(stderr, "htrans: insufficient memory\n");
    exit(-1);
  }
  for (j = 0; j<nx; j++) {
    /*
     * copy odd elements to tmp
     */
    pend = &a[nydim*j+ny-1];
    for (pt = tmp, p1 = &a[nydim*j+1]; p1 <= pend; p1 += 2, pt += 1)
      *pt = *p1;
    /*
     * compress even elements into first half of A
     */
    for (p1 = &a[nydim*j+1], p2 = &a[nydim*j+2]; p2 <= pend; p1 += 1, p2 += 2)
      *p1 = *p2;
    /*
     * put odd elements into 2nd half
     */
    (void) memcpy((void *)p1, (void *)tmp, (ny/2)*sizeof(int));
  }
  free(tmp);
  return (SH_SUCCESS);
}

/*
 * shuffle in y direction to get even elements at beginning of
 * array, odd elements at end.  This is done using a somewhat
 * complicated method for efficiency.  The straightforward
 * approach is slow because the scattered memory accesses don't
 * take advantage of the cache (I think.)  This version does
 * operations a row at a time so that consecutive memory locations
 * are accessed.
 * int a[]    array to shuffle
 * int nx     number of elements in column
 * int ny     number of elements in row		
 * int nydim  actual length of row in array	
 */
static int
yshuffle(int a[] , int nx, int ny, int nydim)
{
  int j, k, tt, oddoffset, *tmp;
  int *p, *pt;
  unsigned char *flag;
  
  /*
   * get temporary storage for shuffling elements
   */
  tmp  = (int *) malloc(ny*sizeof(int));
  flag = (unsigned char *) malloc(nx*sizeof(unsigned char));
  if(tmp == (int *) NULL || flag == (unsigned char *) NULL) {
    shError("htrans: insufficient memory\n");
    return(SH_GENERIC_ERROR);
  }
  /*
   * initialize flag array telling whether row is done
   */
  for (j=0; j<nx; j++) flag[j] = 1;
  oddoffset = (nx+1)/2;
  /*
   * shuffle each row to appropriate location
   * row 0 is already in right location
   */
  for (j=1; j<nx; j++) {
    if (flag[j]) {
      flag[j] = 0;
      /*
       * where does this row belong?
       * second factor = 0 for even rows, oddoffset for odd rows
       */
      k = (j>>1) + ((j & 1) ? oddoffset : 0);
      if (j != k) {
	/*
	 * copy the row
	 */
	(void) memcpy((void *)tmp,(void *) &a[nydim*j],ny*sizeof(int));
	/*
	 * keep shuffling until we reach a row that is done
	 */
	while (flag[k]) {
	  flag[k] = 0;
	  /*
	   * do the exchange
	   */
	  for (p = &a[nydim*k], pt=tmp; p < &a[nydim*k+ny]; p++, pt++) {
	    tt = *p;
	    *p = *pt;
	    *pt = tt;
	  }
	  k = (k>>1) + ((k & 1) ? oddoffset : 0);
	}
	/*
	 * copy the last row into place
	 * this should always end up with j=k
	 */
	(void) memcpy((void *)&a[nydim*k],(void *)tmp,ny*sizeof(int));
	if (j != k) {
	  shError("error: yshuffle failed!\nj=%d k=%d\n", j, k);
	  return(SH_GENERIC_ERROR);
	}
      }
    }
  }
  free(tmp);
  free(flag);
  return (SH_SUCCESS);
}
