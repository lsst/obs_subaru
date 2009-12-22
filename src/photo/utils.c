/*
 * <AUTO>
 *
 * FILE: utils.c
 *
 * DESCRIPTION:
 * This file contains lots of disparate functions that, in most cases,
 * have little to do with each other.  The idea is that one can find
 * utility functions of general interest collected together here.
 *
 * </AUTO>
 */

/*
 * For now, we'll put contributed code here
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <alloca.h>
#include <unistd.h>
#include <errno.h>

#include "dervish.h"
#include "phConsts.h"
//#include "phDervishUtils.h"		/* utilities which will one day be
//in dervish */
//#include "phChainDiff.h"
#include "phRandom.h"
#include "phExtract.h"
#include "phSkyUtils.h"
//#include "prvt/region_p.h"
#include "phVariablePsf.h"		/* for ACOEFF */
#include "phMeschach.h"
#include "phUtils.h"

/*****************************************************************************/
/*
 * Find the median of a set of PIX data
 */
/*
 * sort a PIX array in place using Shell's method
 */
static void
shshsort(PIX *arr, int n)
{
    unsigned int i, j, inc;
    PIX t;
    
    inc=1;
    do{
        inc *= 3;
        inc++;
    }while(inc <= n);
    do{
        inc /= 3;
        for(i=inc;i<n;i++){
            t = arr[i];
            j=i;
            while(arr[j-inc] > t){
                arr[j] = arr[j-inc];
                j -= inc;
                if(j<inc) break;
            }
            arr[j] = t;
        }
    } while(inc > 1);
}


/*****************************************************************************/
/*
 * Allocate some memory that can be made available in extremis.  This is
 * achieved via a shMemEmptyCB callback
 */
static void *strategic_memory_reserve = NULL; /* reserved memory, to be released when we run out */
static int allocated_reserve = 0;	/* did we allocate a reserve? */


/*
 * Do we have memory saved for later use?
 */
int
phStrategicMemoryReserveIsEmpty(void)
{
    return (allocated_reserve && strategic_memory_reserve == NULL) ? 1 : 0;
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Use the system qsort to sort a chain
 *
 *
 * N.b. this routine belongs in shChain.c; it assumes knowledge of
 * CHAIN internals
 *
 * Note also that any cursors that the chain might have may be invalid
 * when this routine returns. This routine doesn't bother to go through
 * all of the possible cursors checking if any exist.
 */
void
shChainQsort(CHAIN *chain,
	     int (*compar)(const void *, const void *))
{
   void **arr;				/* unpack chain into this array */
   CHAIN_ELEM *elem;			/* a link of a chain */
   int i;
   int n;				/* length of chain */

   shAssert(chain != NULL && chain->type != shTypeGetFromName("GENERIC"));

   if((n = chain->nElements) <= 1) {		/* nothing to do */
      return;
   }
/*
 * extract chain into arr[]
 */
   arr = alloca(n*sizeof(void *));

   for(elem = chain->pFirst, i = 0; elem != NULL; elem = elem->pNext, i++) {
      arr[i] = elem->pElement;
   }
/*
 * call the system qsort to do the work
 */
   qsort(arr, n, sizeof(void *), compar);
/*
 * and rebuild the chain
 */
   for(elem = chain->pFirst, i = 0; elem != NULL; elem = elem->pNext, i++) {
      elem->pElement = arr[i];
   }
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * ROUTINE: shRegIntSetVal 
 * 
 * DESCRIPTION: 
 * Set all the values of a integral (8, 16, or 32 bit) region to a given value.
 * This is MUCH faster than regSetWithDbl, and useful in writing tests
 *
 * return: SH_SUCCESS		If region type is supported
 *         SH_GENERIC_ERROR	otherwise
 */

int
shRegIntSetVal(REGION *reg,             /* set all pixels in this region ... */
	       const float val)		/* to this value */
{
   int dsize;				/* sizeof one pixel */
   int i;
   int ncol;				/* == reg->ncol */
   void **rptr;				/* pointer to rows */

   shAssert(reg != NULL);
   ncol = reg->ncol;

   if(reg->type == TYPE_U8) {
      const int pval = val + 0.5;
      for(i = 0;i < ncol;i++) {
	 reg->rows_u8[0][i] = pval;
      }
      rptr = (void **)reg->rows_u8;
      dsize = sizeof(U8);
   } else if(reg->type == TYPE_S8) {
      const int pval = val < 0 ? -(-val + 0.5) : val + 0.5;
      for(i = 0;i < ncol;i++) {
	 reg->rows_s8[0][i] = pval;
      }
      rptr = (void **)reg->rows_s8;
      dsize = sizeof(S8);
   } else if(reg->type == TYPE_U16) {
      const int pval = val + 0.5;
      U16 *row = reg->rows_u16[0];	/* unaliased for speed */
      
      for(i = 0;i < ncol;i++) {
	 row[i] = pval;
      }
      rptr = (void **)reg->rows_u16;
      dsize = sizeof(U16);
   } else if(reg->type == TYPE_S16) {
      const int pval = val < 0 ? -(-val + 0.5) : val + 0.5;
      for(i = 0;i < ncol;i++) {
	 reg->rows_s16[0][i] = pval;
      }
      rptr = (void **)reg->rows_s16;
      dsize = sizeof(S16);
   } else if(reg->type == TYPE_U32) {
      const int pval = val + 0.5;
      for(i = 0;i < ncol;i++) {
	 reg->rows_u32[0][i] = pval;
      }
      rptr = (void **)reg->rows_u32;
      dsize = sizeof(U32);
   } else if(reg->type == TYPE_S32) {
      const int pval = val < 0 ? -(-val + 0.5) : val + 0.5;
      for(i = 0;i < ncol;i++) {
	 reg->rows_s32[0][i] = pval;
      }
      rptr = (void **)reg->rows_s32;
      dsize = sizeof(S32);
   } else if(reg->type == TYPE_FL32) {
      for(i = 0;i < ncol;i++) {
	 reg->rows_fl32[0][i] = val;
      }
      rptr = (void **)reg->rows_fl32;
      dsize = sizeof(FL32);
   } else {
      shError("shRegIntSetVal doesn't handle regions of type %d\n",reg->type);
      return(SH_GENERIC_ERROR);
   }

   for(i = 1;i < reg->nrow;i++) {
      memcpy(rptr[i],(const void *)rptr[0],ncol*dsize);
   }
   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * a qsort comparison function
 */
static int
compar_s32(const S32 *a, const S32 *b)
{
   return(*a - *b);
}




/*
 * sorts an array using a Shell sort, and find the mean and quartiles.
 *
 * If clip is false, use all the data; otherwise find the quartiles for
 * the main body of the histogram, and reevaluate the mean for the main body.
 *
 * The quartile algorithm assumes that the data are distributed uniformly
 * in the histogram cells, and the quartiles are thus linearly interpolated
 * in the cumulative histogram. This is about as good as one can do with
 * dense histograms, and for sparse ones is as good as anything.
 *
 * The returned value is the median, and equals qt[1]
 *
 * This code is taken from photo/src/extract_utils.c
 */
float
phQuartilesGetFromArray(const void *arr, /* the data values */
			int type,	/* type of data */
			int n,		/* number of points */
			int clip,	/* should we clip histogram? */
			float *qt,	/* quartiles (may be NULL) */
			float *mean,	/* mean (may be NULL) */
			float *sig)	/* s.d. (may be NULL) */
{
    int i;
    int sum;
    PIX *data;				/* a modifiable copy of arr */
    register int np;
    const PIX *p;
    int ldex,udex;
    float fdex;
    float fldex;
    int npass;				/* how many passes? 2 => trim */
    int pass;				/* which pass through array? */
    int cdex;    
    int dcell;
    int dlim = 0;
    float qt_s[3];			/* storage for qt is required */

    if(type == TYPE_S32) {		/* handle S32 specially */
       S32 *sdata;			/* modifiable copy of data */
       shAssert(!clip);			/* not (yet?) supported for S32 */
       shAssert(n > 0);
       shAssert(n < 10000);		/* this routine is not optimised */

       sdata = alloca((n + 1)*sizeof(S32)); memcpy(sdata, arr, n*sizeof(S32));
       qsort(sdata, n, sizeof(S32),
	     (int (*)(const void *, const void *))compar_s32);

       if(n%2 == 1) {
	  return(sdata[n/2]);
       } else {
	  return(0.5*(sdata[n/2] + sdata[n/2 + 1]));
       }
    }

    shAssert(type == TYPE_PIX && n > 0);

    if(qt == NULL) {
       qt = qt_s;
    }
    
    npass = clip ? 2 : 1;

    data = alloca((n + 1)*sizeof(PIX)); memcpy(data, arr, n*sizeof(PIX));
    shshsort(data, n);

    for(pass=0;pass < npass;pass++){
       for(i = 0;i < 3;i++) {
	  fdex = 0.25*(float)((i+1)*n);	/*float index*/
	  cdex = fdex;
	  dcell = data[cdex];
	  ldex = cdex;
	  if(ldex > 0) {
	     while(data[--ldex] == dcell && ldex > 0) continue;
	     /* ldex is now the last index for which data<cdex */
	     
	     if(ldex > 0 || data[ldex] != dcell) {
		/* we stopped before the end or we stopped at the
		 * end but would have stopped anyway, so bump it up; 
		 */
		ldex++;
	     }
	  }
	  /* The value of the cumulative histogram at the left edge of the
	   * dcell cell is ldex; ie exactly ldex values lie strictly below
	   * dcell, and data=dcell BEGINS at ldex.
	   */
	  udex = cdex;
	  while(udex < n && data[++udex] == dcell) continue;
	  /* first index for which data>cdex or the end of the array, 
	   * whichever comes first. This can run off the end of
	   * the array, but it does not matter; if data[n] is accidentally
	   * equal to dcell, udex == n on the next go and it falls out 
	   * before udex is incremented again. */
	  
	  /* now the cumulative histogram at the right edge of the dcell
	   * cell is udex-1, and the number of instances for which the data
	   * are equal to dcell exactly is udex-ldex. Thus if we assume
	   * that the data are distributed uniformly within a histogram
	   * cell, the quartile can be computed:
	   */
	  fldex = ldex; 
	  
	  shAssert(udex != ldex);
	  qt[i] = dcell - 1 + 0.5 + (fdex - fldex)/(float)(udex-ldex);
	  
	  /* The above is all OK except for one singular case: if the
	   * quartile is EXACTLY at a histogram cell boundary (a half-integer) 
	   * as computed above AND the previous histogram cell is empty, the
	   * result is not intuitively correct, though the 'real' answer 
	   * is formally indeterminate even with the unform-population-in-
	   * cells ansatz. The cumulative histogram has a segment of zero
	   * derivative in this cell, and intuitively one would place the
	   * quartile in the center of this segment; the algorithm above
	   * places it always at the right end. This code, which can be
	   * omitted, fixes this case.
	   *
	   * We only have to do something if the quartile is exactly at a cell
	   * boundary; in this case ldex cannot be at either end of the array,
	   * so we do not need to worry about the array boundaries .
	   */
	  if(4*ldex == (i+1)*n) {
	     int zext = dcell - data[ldex-1] - 1;
	     
	     if(zext > 0) {
		/* there is at least one empty cell in the histogram
		 * prior to the first data==dcell one
		 */
		qt[i] -= 0.5*zext;
	     }
	  }
       }

       if(npass == 1) {			/* no trimming to be done */
	  if(sig != NULL) {
	     *sig = IQR_TO_SIGMA*(qt[2] - qt[0]);
	  }
       } else {
	  /*
	   * trim the histogram if possible to the first percentile below
	   * and the +2.3 sigma point above
	   */
	  if(pass==0){
	     /* terminate data array--array must be of size (n+1) (JEG code) */
	     data[n] = 0x7fff;
	     /* trim histogram */
	     ldex = .01*n;		/* index in sorted data array at first
					   percentile */
	     dlim = qt[1] + 2.3*IQR_TO_SIGMA*(qt[2] - qt[0]) + 0.5;
	     if(dlim >= data[n-1] || udex >= n) {  /* off top of data or
						      already at end */
		if(ldex == 0) {
		   if(sig != NULL) {
		      *sig = IQR_TO_SIGMA*(qt[2] - qt[0]);
		   }
		   break;		/* histogram is too small; we're done*/
		}
	     } else {
		/* find the index corresponding to 2.3 sigma; this should be
		 * done by a binary search */
		udex--; 
		while(data[++udex] <= dlim){;}
		n = udex - ldex;
		data = data + ldex;
	     }
	  }else{   /* have trimmed hist and recomputed quartiles */
	     if(sig != NULL) {
		*sig = 1.025*IQR_TO_SIGMA*(qt[2]-qt[0]);
	     }
	  }
       }
    }

    if(mean != NULL) {
       sum = 0;
       np = n;
       p = data;
       while(np--){
	  sum += *p++;
       }
       *mean = (float)sum/(float)n;
    }

    return(qt[1]);
}    

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Add a constant to each pixel of an image (not all types are supported)
 *
 * This is MUCH faster than using regAdd
 *
 * return: SH_SUCCESS		If region type is supported
 *         SH_GENERIC_ERROR	otherwise
 */

int
shRegIntConstAdd(REGION *reg,		/* The region ... */
		 const float val,	/* the constant to add */
		 const int dither)		/* dither, not round */
{
   int i;
   int ncol,nrow;			/* unpacked from reg */
   float tmp;

   shAssert(reg != NULL);
   shAssert(!dither || reg->type == TYPE_U16);

   ncol = reg->ncol;
   nrow = reg->nrow;

   if(reg->type == TYPE_U8) {
      U8 **rptr = reg->rows_u8;
      U8 *ptr,*end;
      
      for(i = 0;i < nrow;i++) {
	 ptr = rptr[i];
	 end = ptr + ncol;
	 while(ptr < end) {
	    tmp = *ptr + val + 0.5;
	    *ptr++ = (tmp >= 0) ? (tmp <= MAX_U8 ? tmp : MAX_U8) : 0;
	 }
      }
   } else if(reg->type == TYPE_S8) {
      S8 **rptr = reg->rows_s8;
      S8 *ptr,*end;
      
      for(i = 0;i < nrow;i++) {
	 ptr = rptr[i];
	 end = ptr + ncol;
	 while(ptr < end) {
	    tmp = *ptr + val + 0.5;
	    *ptr++ = (tmp >= MIN_S8) ? (tmp <= MAX_S8 ? tmp : MAX_S8) : MIN_S8;
	 }
      }
   } else if(reg->type == TYPE_U16) {
      U16 **rptr = reg->rows_u16;
      U16 *ptr,*end;
      
      for(i = 0;i < nrow;i++) {
	 ptr = rptr[i];
	 end = ptr + ncol;
	 if(dither) {
	    while(ptr < end) {
	       tmp = *ptr + val + phRandomUniformdev();
	       *ptr++ = (tmp >= 0) ? (tmp <= MAX_U16 ? tmp : MAX_U16) : 0;
	    }
	 } else {
	    while(ptr < end) {
	       tmp = *ptr + val + 0.5;
	       *ptr++ = (tmp >= 0) ? (tmp <= MAX_U16 ? tmp : MAX_U16) : 0;
	    }
	 }
      }
   } else if(reg->type == TYPE_S16) {
      S16 **rptr = reg->rows_s16;
      S16 *ptr,*end;
      
      for(i = 0;i < nrow;i++) {
	 ptr = rptr[i];
	 end = ptr + ncol;
	 while(ptr < end) {
	    tmp = *ptr + val + 0.5;
	    *ptr++ = (tmp >= MIN_S16) ?
				     (tmp <= MAX_S16 ? tmp : MAX_S16) : MIN_S16;
	 }
      }
   } else if(reg->type == TYPE_U32) {
      U32 **rptr = reg->rows_u32;
      U32 *ptr,*end;
      
      for(i = 0;i < nrow;i++) {
	 ptr = rptr[i];
	 end = ptr + ncol;
	 while(ptr < end) {
	    tmp = *ptr + val + 0.5;
	    *ptr++ = (tmp >= 0) ? (tmp <= MAX_U32 ? tmp : MAX_U32) : 0;
	 }
      }
   } else if(reg->type == TYPE_S32) {
      S32 **rptr = reg->rows_s32;
      S32 *ptr,*end;
      
      for(i = 0;i < nrow;i++) {
	 ptr = rptr[i];
	 end = ptr + ncol;
	 while(ptr < end) {
	    tmp = *ptr + val + 0.5;
	    *ptr++ = (tmp >= MIN_S32) ?
				    (tmp <= MAX_S32 ? tmp : MAX_S32) : MIN_S32;
	 }
      }
   } else if(reg->type == TYPE_FL32) {
      FL32 **rptr = reg->rows_fl32;
      FL32 *ptr,*end;
      
      for(i = 0;i < nrow;i++) {
	 ptr = rptr[i];
	 end = ptr + ncol;
	 while(ptr < end) {
	    *ptr++ += val;
	 }
      }
   } else {
      shError("shRegIntConstAdd doesn't handle regions of type %d\n",
	      reg->type);
      return(SH_GENERIC_ERROR);
   }

   return(SH_SUCCESS);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Copy one region to another. It is also able to convert U16 to FL32,
 * removing the SOFT_BIAS
 *
 * return: SH_SUCCESS		If region type is supported
 *         SH_GENERIC_ERROR	otherwise
 */
int
shRegIntCopy(REGION *out,		/* output region */
	     const REGION *in)		/* input region */
{
   int i, j;
   int ncol,nrow;			/* unpacked from out */

   shAssert(out != NULL && in != NULL);
   shAssert(in->nrow == out->nrow && in->ncol == out->ncol);

   ncol = out->ncol;
   nrow = out->nrow;

   if(out->type == TYPE_U8) {
      shAssert(in->type == out->type);
      for(i = 0;i < nrow;i++) {
	 memcpy(out->rows_u8[i],in->rows_u8[i],ncol);
      }
   } else if(out->type == TYPE_S8) {
      shAssert(in->type == out->type);
      for(i = 0;i < nrow;i++) {
	 memcpy(out->rows_s8[i],in->rows_s8[i],ncol);
      }
   } else if(out->type == TYPE_U16) {
      if(in->type == TYPE_U8) {
	 U8 *iptr;
	 U16 *optr;

	 for(i = 0;i < nrow;i++) {
	    iptr = in->rows_u8[i];
	    optr = out->rows_u16[i];
	    for(j = 0;j < ncol; j++) {
	       optr[j] = iptr[j];
	    }
	 }
      } else if(in->type == TYPE_U16) {
	 for(i = 0;i < nrow;i++) {
	    memcpy(out->rows_u16[i],in->rows_u16[i],ncol*sizeof(U16));
	 }
      } else if(in->type == TYPE_S32) {
	 S32 *iptr;
	 U16 *optr;
	 unsigned int oval;

	 for(i = 0;i < nrow;i++) {
	    iptr = in->rows_s32[i];
	    optr = out->rows_u16[i];
	    for(j = 0;j < ncol; j++) {
	       oval = iptr[j] + SOFT_BIAS;
	       optr[j] = (oval > MAX_U16) ? MAX_U16 : oval;
	    }
	 }
      } else if(in->type == TYPE_FL32) {
	 FL32 *iptr;
	 U16 *optr;
	 unsigned int oval;

	 for(i = 0;i < nrow;i++) {
	    iptr = in->rows_fl32[i];
	    optr = out->rows_u16[i];
	    for(j = 0;j < ncol; j++) {
	       oval = iptr[j] + SOFT_BIAS + 0.5;
	       optr[j] = (oval > MAX_U16) ? MAX_U16 : oval;
	    }
	 }
      } else {
	 shError("shRegIntCopy doesn't convert regions of type %d to U16\n",
		 in->type);
	 return(SH_GENERIC_ERROR);
      }
   } else if(out->type == TYPE_S16) {
      shAssert(in->type == out->type);
      for(i = 0;i < nrow;i++) {
	 memcpy(out->rows_s16[i],in->rows_s16[i],ncol*sizeof(S16));
      }
   } else if(out->type == TYPE_U32) {
      shAssert(in->type == out->type);
      for(i = 0;i < nrow;i++) {
	 memcpy(out->rows_u32[i],in->rows_u32[i],ncol*sizeof(U32));
      }
   } else if(out->type == TYPE_S32) {
      shAssert(in->type == out->type);
      for(i = 0;i < nrow;i++) {
	 memcpy(out->rows_s32[i],in->rows_s32[i],ncol*sizeof(S32));
      }
   } else if(out->type == TYPE_FL32) {
      if(in->type == TYPE_U16) {
	 U16 *iptr;
	 FL32 *optr;

	 for(i = 0;i < nrow;i++) {
	    iptr = in->rows_u16[i];
	    optr = out->rows_fl32[i];
	    for(j = 0;j < ncol; j++) {
	       optr[j] = (int)(iptr[j] + 0.499) - SOFT_BIAS; /* If we added 0.5
								then
								U16--FL32--U16
								would add 1 */
	    }
	 }
      } else if(in->type == TYPE_FL32) {
	 for(i = 0;i < nrow;i++) {
	    memcpy(out->rows_fl32[i],in->rows_fl32[i],ncol*sizeof(FL32));
	 }
      } else {
	 shError("shRegIntCopy doesn't convert regions of type %d to FL32\n",
		 in->type);
	 return(SH_GENERIC_ERROR);
      }
   } else {
      shError("shRegIntCopy doesn't handle regions of type %d\n",
	      out->type);
      return(SH_GENERIC_ERROR);
   }

   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * Calculate the linear combination of two images, specifically given
 * regions reg1 and reg2, and three constants a, b, and c, carry out
 *   reg1 = a + b*reg1 + c*reg2
 * If c is 0, reg2 may be NULL
 *
 * return: reg1 (or a new region if how == 2)
 *         NULL	otherwise
 */
REGION *
shRegIntLincom(REGION *reg1,
	       const REGION *reg2,
	       float a,
	       float b,
	       float c,
	       LINCOM_FLAGS flag)	/* How to handle non-identical REGIONS:
					   LINCOM_EXACT:     fail
					   LINCOM_INTERSECT: add the intersection of regions
					   LINCOM_UNION:     grow reg1 to contain union of regions */
{
   int i, j;
   int ncol,nrow;			/* unpacked from reg1 */
   int val;

   shAssert(reg1 != NULL && (c == 0.0 || reg2 != NULL));
   /*
    * Do we need to find just the overlapping parts of the REGIONs?
    */
   if(flag != LINCOM_EXACT && reg2 != NULL) {	/* flag is irrelevant if !reg2 */
      const int row0_1 = reg1->row0;
      const int col0_1 = reg1->col0;
      const int row1_1 = reg1->row0 + reg1->nrow - 1;
      const int col1_1 = reg1->col0 + reg1->ncol - 1;
      const int row0_2 = reg2->row0;
      const int col0_2 = reg2->col0;
      const int row1_2 = reg2->row0 + reg2->nrow - 1;
      const int col1_2 = reg2->col0 + reg2->ncol - 1;

      if(flag == LINCOM_INTERSECT) {	/* intersection */
	  /*
	   * Figure out boundary of intersect in absolute coordinates
	   */
	  const int row0 = (row0_1 > row0_2) ? row0_1 : row0_2;
	  const int col0 = (col0_1 > col0_2) ? col0_1 : col0_2;
	  const int row1 = (row1_1 < row1_2) ? row1_1 : row1_2;
	  const int col1 = (col1_1 < col1_2) ? col1_1 : col1_2;
	  
	  if(row1 >= row0 && col1 >= col0) {
	      const int nrow = row1 - row0 + 1;
	      const int ncol = col1 - col0 + 1;
	      REGION *sreg1 = shSubRegNew("", reg1, nrow, ncol,
					  row0 - row0_1, col0 - col0_1, 0);
	      REGION *sreg2 = shSubRegNew("", reg2, nrow, ncol,
					  row0 - row0_2, col0 - col0_2, 0);
	      
	      REGION *ret = shRegIntLincom(sreg1, sreg2, a, b, c, LINCOM_EXACT);
	      
	      shRegDel(sreg1);
	      shRegDel(sreg2);

	      return (ret == NULL) ? NULL : reg1;
	  } else {
	      ;				/* no intersection */
	  }
      } else if(flag == LINCOM_UNION) {		/* calculate Union */
	  /*
	   * Figure out bounding box of reg1 & reg2 in absolute coordinates
	   */
	  const int row0 = (row0_1 < row0_2) ? row0_1 : row0_2;
	  const int col0 = (col0_1 < col0_2) ? col0_1 : col0_2;
	  const int row1 = (row1_1 > row1_2) ? row1_1 : row1_2;
	  const int col1 = (col1_1 > col1_2) ? col1_1 : col1_2;
	  
	  const int nrow = row1 - row0 + 1;
	  const int ncol = col1 - col0 + 1;
	  if (nrow == reg1->nrow && ncol == reg1->ncol) { /* a perfect fit */
	      return(shRegIntLincom(reg1, reg2, a, b, c, LINCOM_INTERSECT));
	  } else {
	      REGION *out = shRegNew("union", nrow, ncol, reg1->type);
	      shRegIntSetVal(out, a);
	      out->row0 = row0; out->col0 = col0;
	      
	      if(shRegIntLincom(out, reg1, a, b, c, LINCOM_INTERSECT) == NULL ||
		 shRegIntLincom(out, reg2, a, b, c, LINCOM_INTERSECT) == NULL) {
		  shRegDel(out);

		  return(NULL);
	      }

	      out->mask = NULL;		/* XXX should be union of input masks */
	      phSpanmaskDel((SPANMASK *)reg1->mask); reg1->mask = NULL;
	      shRegDel(reg1);

	      return(out);
	  }
      } else {
	  shFatal("shRegIntLincom: Illegal value of flag: %d\n", flag);
      }
       
      return(reg1);
   }

   ncol = reg1->ncol;
   nrow = reg1->nrow;

   if(reg2 != NULL && (reg2->ncol != ncol || reg2->nrow != nrow)) {
      shError("shRegIntLincom: region sizes differ\n");
      return(NULL);
   }

   if(reg1->type == TYPE_U16) {
      U16 **rrow1 = reg1->rows_u16;
      U16 **rrow2 = (reg2 == NULL) ? NULL : reg2->rows_u16;
      U16 *row1, *row2;

      if(reg2 != NULL && reg1->type != reg2->type) {
	 shError("shRegIntLincom: region types differ\n");
	 return(NULL);
      }
      
      for(i = 0;i < nrow;i++) {
	 row1 = rrow1[i];
	 if(reg2 == NULL) {
	    for(j = 0;j < ncol;j++) {
	       val = a + b*row1[j] + 0.5;
	       row1[j] = val < 0 ? 0 : (val > MAX_U16 ? MAX_U16 : val);
	    }
	 } else {
	    row2 = rrow2[i];
	    for(j = 0;j < ncol;j++) {
	       val = a + b*row1[j] + c*row2[j] + 0.5;
	       row1[j] = val < 0 ? 0 : (val > MAX_U16 ? MAX_U16 : val);
	    }
	 }
      }
   } else if(reg1->type == TYPE_S32) {
      S32 **rrow1 = reg1->rows_s32;
      S32 **rrow2 = (reg2 == NULL) ? NULL : reg2->rows_s32;
      S32 *row1, *row2;
      
      if(reg2 != NULL && reg1->type != reg2->type) {
	 shError("shRegIntLincom: region types differ\n");
	 return(NULL);
      }
      
      for(i = 0;i < nrow;i++) {
	 row1 = rrow1[i];
	 if(reg2 == NULL) {
	    for(j = 0;j < ncol;j++) {
	       row1[j] = a + b*row1[j] + 0.5;
	    }
	 } else {
	    row2 = rrow2[i];
	    for(j = 0;j < ncol;j++) {
	       row1[j] = a + b*row1[j] + c*row2[j] + 0.5;
	    }
	 }
      }
   } else if(reg1->type == TYPE_FL32) {
      FL32 **rrow1 = reg1->rows_fl32, *row1;
      
      if(reg2 == NULL) {
	 for(i = 0;i < nrow;i++) {
	    row1 = rrow1[i];
	    for(j = 0;j < ncol;j++) {
	       row1[j] = a + b*row1[j];
	    }
	 }
      } else if(reg2->type == TYPE_U16) {
	 U16 **rrow2 = reg2->rows_u16, *row2;

	 for(i = 0;i < nrow;i++) {
	    row1 = rrow1[i];
	    row2 = rrow2[i];
	    for(j = 0;j < ncol;j++) {
	       row1[j] = a + b*row1[j] + c*(row2[j] - SOFT_BIAS);
	    }
	 }
      } else if(reg2->type == TYPE_FL32) {
	 FL32 **rrow2 = reg2->rows_fl32, *row2;

	 for(i = 0;i < nrow;i++) {
	    row1 = rrow1[i];
	    row2 = rrow2[i];
	    for(j = 0;j < ncol;j++) {
	       row1[j] = a + b*row1[j] + c*row2[j];
	    }
	 }
      } else {
	 shError("shRegIntLincom: region types are incompatible\n");
	 return(NULL);
      }
   } else {
      shError("shRegIntLincom "
	      "doesn't handle regions of type %d\n", reg1->type);
      return(NULL);
   }

   return(reg1);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Clip all the values of a region (currently only PIX) within a mask
 * to lie at or below a specified value
 *
 * The mask must be wholly enclosed within the region
 */
void
phRegIntClipValInObjmask(REGION *reg,	/* clip all pixels in this region ...*/
			 const OBJMASK *om, /* within this mask... */
			 const int drow, const int dcol, /* offset om by (drow, dcol) */
			 const int val)	/* to lie at or below this value */
{
   int i, j;
   int j0, j1;				/* range of j values in a row */
   int nspan;				/* == om->nspan */
   int row0, col0;			/* == reg->{row,col}0 */
   PIX **rows, *row;			/* == reg->ROWS, reg->ROWS[] */
   SPAN *s;				/* a span of om */
   int y, x1, x2;			/* unpacked from s */

   shAssert(reg != NULL && reg->type == TYPE_PIX);
   rows = reg->ROWS; row0 = reg->row0 - drow; col0 = reg->col0 - dcol;
   shAssert(om != NULL);
   nspan = om->nspan;

   for(i = 0; i < nspan; i++) {
      s = &om->s[i];
      y = s->y; x1 = s->x1; x2 = s->x2;
      if(y - row0 < 0 || y - row0 >= reg->nrow) {
	  continue;
      }
      row = rows[y - row0];
      j0 = x1 - col0; j1 = x2 - row0 + 1;
      if(j0 < 0) j0 = 0;
      if(j1 > reg->ncol) j1 = reg->ncol;
      for(j = j0; j < j1; j++) {
	 if(row[j] > val) {
	    row[j] = val;
	 }
      }
   }
}
