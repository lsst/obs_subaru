/* <AUTO>
  FILE: atSmoothReg
<HTML>
  Routines to smooth images
</HTML>
  </AUTO>
*/

/***************************************************************
 *
 *  C routines to smooth galaxies
 *
 **************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "dervish.h"
#include "atRegSci.h"
#include "atSmoothReg.h"

RET_CODE atRegSmoothWithMedian(REGION *inRegion, REGION *outRegion, int nSize)

{
   int nhalf, row, col, row0, col0, nrow, ncol;
   double median; 
   REGION *subInRegion;

   /* make sure REGIONs have correct types -- we access their data directly */
   if ((inRegion->type != TYPE_FL32) || (outRegion->type != TYPE_FL32)) {
      shError("atRegSmoothWithMedian: REGIONs not of the correct type");
      return(SH_GENERIC_ERROR);
   }

   /* make sure the input and output REGIONs have the same size. */
   if ((inRegion->nrow != outRegion->nrow) || (inRegion->ncol != outRegion->ncol)) {
      shError("atRegSmoothWithMedian: input and output REGIONs not the same size");
      return(SH_GENERIC_ERROR);
   }

   if ((nSize > inRegion->nrow) || (nSize > inRegion->ncol)) {
      shError("atRegSmoothWithMedian: size of smoothing box larger than size of REGIONs");
      return(SH_GENERIC_ERROR);
   }

   if (nSize % 2 != 1) {
      shError("atRegConvolveWithGaussian: linear size of smoothing box must be an odd number"); 
      return(SH_GENERIC_ERROR);
   }

   nhalf = nSize/2;

   nrow = inRegion->nrow;
   ncol = inRegion->ncol;

   /* first, let's initialize all the pixels in the output region to 1.00... */
   for (row = 0; row < nrow; row++) {
     for (col = 0; col < ncol; col++) {
       outRegion->rows_fl32[row][col]  = 1.00;
     }
   }

   /* now, we are ready to smooth...  */
   for (row = 0; row < nrow; row++) {
     if (row < nhalf) {
       row0 = 0;
     } else if (row < nrow-nhalf) {
       row0 = row-nhalf;
     } else {
       row0 = nrow-nSize;
     }
     for (col = 0; col < ncol; col++) {
       if (col < nhalf) {
	 col0 = 0;
       } else if (col < ncol-nhalf) {
	 col0 = col-nhalf;
       } else {
	 col0 = ncol-nSize;
       }
       subInRegion = shSubRegNew("", inRegion, nSize, nSize, row0, col0, READ_ONLY);
       atRegMedianFindAsDbl(subInRegion, &median);
       outRegion->rows_fl32[row][col] = median;
       shRegDel(subInRegion);
     }
   }

   /* and we're all done */
   return(SH_SUCCESS);

}
