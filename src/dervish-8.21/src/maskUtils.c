/*****************************************************************************
******************************************************************************
**
** FILE:
**	maskUtils.c
**
** ABSTRACT:
**      This file contains routines that support manipulation of masks.
**
**      THIS MODULE IS NOT YET UP TO SPECIFICATION SINCE:
**         It has NOT been tested AT ALL.
**         Add a compare function, mostly to allow regression testing of this
**            and other modules
**
**	
**				NYI == Not yet implemented
** ENTRY POINT			SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
**  shMaskCopy		        public	Copy a mask 
**  shMaskClear			public	Clear the contents of a mask
**  shMaskAndEqualsValue        public  "mask &= value"
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	NONE
**
** AUTHORS:
**	Creation date:  Jan 24, 1996
**	Don Petravick
**      Eileen Berman
******************************************************************************
******************************************************************************/


#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "region.h"
#include <shCAssert.h>

/*============================================================================
**============================================================================
**
** LOCAL MACROS, DEFINITIONS, ETC.
**
**============================================================================
*/

/*Check if masks have the same number of rows and cols*/
static int samesizeMask2 (const MASK *mask1, const MASK *mask2)
{   
   if (mask1->nrow == mask2->nrow &&
       mask1->ncol == mask2->ncol) return 1;
   return 0;   
}


/*
 * Copy the pixels from maskIn to maskOut pixel-by-pixel.
 *
 * - The caller gaurantees both masks are of the same size.
 * - The use of memmove is indiciated as things may overlap.
 */ 

void shMaskCopy(const MASK *maskIn, MASK *maskOut)
{
   int r;
   size_t len    = sizeof (maskIn->rows[0][0]) * maskIn->ncol;

   shAssert(maskIn  != (MASK *)0);
   shAssert(maskOut != (MASK *)0);
   shAssert(samesizeMask2(maskIn, maskOut));

   for (r=0; r<maskIn->nrow; r++) memmove (maskOut->rows[r], maskIn->rows[r], len);

   return;
}


/*****************************************************************************/
/*
 * Clear a mask, setting all pixels to 0
 *
 * RETURN VALUE:
 *	none
 */
void
shMaskClear(MASK *mask)
{
   int i;
   
   if(mask == NULL || mask->rows == NULL ||
      mask->nrow <= 0 || mask->ncol <= 0) return;

   memset((void *)mask->rows[0],'\0',sizeof(mask->rows[0][0])*mask->ncol);
   for(i = 1;i < mask->nrow;i++) {
      memcpy((void *)mask->rows[i],(void *)mask->rows[0],
	     sizeof(mask->rows[0][0])*mask->ncol);
   }
}

int
shMaskAndEqualsValue(MASK *mask, const int val)
{
    int r, c;
    int npix = 0;			/* number of non-zero pixels */

    shAssert(mask != NULL && !(mask->nrow > 0 && mask->rows == NULL));

    for (r = 0; r < mask->nrow; r++) {
	const int ncol = mask->ncol;
	unsigned char *ptr = mask->rows[r];
	for (c = 0; c < ncol; c++) {
	    *ptr &= val;
	    if (*ptr++) npix++;
	}
    }

    return npix;
}
