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




