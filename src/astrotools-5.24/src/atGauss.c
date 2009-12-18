/* <AUTO>
  FILE: atGauss
<HTML>
  Routines to convolve with a Gaussian
  Two functions were barrowed from jpgObjectFind.c or the photometric pipeline.
</HTML>
  </AUTO>
*/
/* HTML

   New name:                  Old Name:
   atGaussVectorMake          phMakeGaussianVector
   atRegConvolveWithGaussian  phRegConvolveWithGaussian  

The prototypes are in ../include/atGauss.h
I changed this from Michael R. version; all three regionss are of type FL32.
Chris S.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "dervish.h"

/* <AUTO EXTRACT>

   ROUTINE:  atGaussVectorMake

   DESCRIPTION:

<HTML>
  This creates a 1-D array through gaussian with given sigma. The number of    
  elements in the array is returned in 'nvec', and a pointer to the     
  array is returned in 'gvec'.  The array will have an odd number of    
  elements, with the peak value in the center element, and will be      
  symmetric around the center, like so:                                 
<listing>   
             0.05   0.15   0.40   1.0   0.40  0.15  0.05                 
</listing>

<p> 
 Note that the elements of the 1-D vector are equal to the square      
  roots of the diagonal elements of the equivalent 2-D matrix.              
</HTML>

  RETURN VALUES:

<HTML>
<listing>
   SH_SUCCESS         if we can allocate and form the new guassin array
   SH_GENERIC_ERROR   if we can't allocate
</listing> 
</HTML> */

/*****************************************************************************
 *                                                                           *
 * Function :                                                                *
 *     int     atGaussVectorMake(const double sigma,                         *
 *                                  int *nvec,                               *
 *                                  float **gvec);                           *
 *                                                                           *
 * Description :                                                             *
 *     Create 1-D array through gaussian with given sigma.  The number of    *
 *     elements in the array is returned in 'nvec', and a pointer to the     *
 *     array is returned in 'gvec'.  The array will have an odd number of    *
 *     elements, with the peak value in the center element, and will be      *
 *     symmetric around the center, like so:                                 *
 *               0.05   0.15   0.40   1.0   0.40  0.15  0.05                 *
 *                                                                           *
 *     Note that the elements of the 1-D vector are equal to the square      *
 *     roots of the diagonal elements of the equivalent 2-D matrix.          * 
 * Return value :                                                            *
 *     SH_SUCCESS     if we can allocate and form the new gaussian array     *
 *     SH_GENERIC_ERROR   if we can't allocate                               *
 *                                                                           *
 * Functions called from here :                                              *
 *     None.                                                                 *
 *                                                                           *
 *****************************************************************************/

int atGaussVectorMake(const double sigma, int *nvec, float **gvec)
{
    const double threshold = 0.01;
    int i, j, rmax, size;
    double arg, x0, sum;

    rmax = floor(sqrt(-2.0 * log(threshold)) * sigma * 3.0);
    
    size = 2 * rmax + 1;

    if ((*gvec = (float *) shMalloc(size*sizeof(float))) == NULL) {
        shError("atGaussVectorMake: can't allocate for array");
        return(SH_GENERIC_ERROR);
    }
    *nvec = size;
    x0 = size/2;
    for (i = 0; i < size; i++) {
        arg = (i - x0)*(i - x0)/(2.0*sigma*sigma);
        *(*gvec + i) = exp(-arg);
    }
    /* now normalize the components of the vector */
    sum = 0;
    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            sum += (*(*gvec + i))*(*(*gvec + j));
        }
    }
    sum = sqrt(sum);
    for (i = 0; i < size; i++) {
        *(*gvec + i) /= sum;
        shDebug(2, "  gvec[%d] = %7.3f\n", i, *(*gvec + i));
    }

    return(SH_SUCCESS);
}

/* <AUTO EXTRACT>

  ROUTINE: atRegConvolveWithGaussian 

  DESCRIPTION:

<HTML>

<p>
Given an input region (type FL32), an output region (type FL32), a temporary region (type FL32), and a 1-D gaussian, this convoloves  the input region with the gaussian in both directions and places the output in the output region.
<p>
The idea is that we want to convolve a region  with a two-dimensional gaussian, which is symmetric and has equal widths in both directions.  The given vector must be a 1-D slice through the peak of the gaussian. xs We assume:
<ol>
<li>the gaussian vector has an odd number of elements
<li>the gaussian has the same width in both directions
<li>the gaussian is symmetric about the peak value
</ol>
<p>
Thus, the 1-D slice should have an odd number of elements, with the peak value in the center element, and equal values on either side, like so:
<listing>
                0.05  0.15  0.45  1.0  0.45  0.15  0.05   
</listing>

</HTML>

  RETURN VALUES:
<HTML>
<listing>
   SH_SUCCESS               if all goes well
   SH_GENERIC_ERROR         if not
</listing>
</HTML>
</AUTO>*/

/*****************************************************************************
 * Given an input REGION (type FL32), an output REGION (type FL32), 
 * a temporary REGION (also type FL32),
 * and a 1-D gaussian (vector plus number of elements),
 * convolve the input REGION with the gaussian 
 * in both directions (by rows and by cols).  Place the output in the
 * output REGION.
 *
 * The idea is that we want to convolve a REGION with a two-dimensional
 * gaussian, which is symmetric and has equal widths in both dimensions.
 * The given vector must be a 1-D slice through the peak of the gaussian.
 * We assume:
 *
 *        1. the gaussian vector has an odd number of elements
 *        3. the gaussian has the same width in both directions
 *        4. the gaussian is symmetric about the peak value
 *
 * Thus, the 1-D slice should have an odd number of elements, with
 * the peak value in the center element, and equal values on either 
 * side, like so:
 *
 *               0.05  0.15  0.45  1.0  0.45  0.15  0.05
 *
 * Return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
 */

#define MAXELEM    300       /* gaussian vector must have fewer elements */

int atRegConvolveWithGaussian(REGION *input, REGION *temp, REGION *output, 
			      int nelem, float *gvec)

{
   int i, j, nhalf, row, col;
   register float t;
   float *ppxin;
   float pxtemp, pxin, *ppxout, *ppxout1, *ppxout2;

   shDebug(2, "phRegConvWGauss: reg (%4d x %4d)", input->nrow, input->ncol);

   /* make sure REGIONs have correct types -- we access their data directly */
   if ((input->type != TYPE_FL32) || (output->type != TYPE_FL32) ||
            (temp->type != TYPE_FL32)) {
      shError("atRegConvolveWithGaussian: REGIONs not of the correct types");
      return(SH_GENERIC_ERROR);
   }
   /* make sure the input and output REGIONs have the same size, and that
      the temporary REGION is same size or larger. */
   if ((input->nrow != output->nrow) || (input->ncol != output->ncol)) {
      shError("atRegConvolveWithGaussian: input and output REGIONs not the same size");
      return(SH_GENERIC_ERROR);
   }
   if ((input->nrow > temp->nrow) || (input->ncol > temp->ncol)) {
      shError("atRegConvolveWithGaussian: input REGION larger than temp");
      return(SH_GENERIC_ERROR);
   }

   /* now examine the 1-D slice through the center of the gaussian, 'gvec' */
   if (nelem >= MAXELEM) {
      shError("atRegConvolveWithGaussian: gaussian vector has more than %d elements",
	       MAXELEM);
      return(SH_GENERIC_ERROR);
   }
   if (nelem % 2 != 1) {
      shError("atRegConvolveWithGaussian: gaussian vector must have odd number of elems"); 
      return(SH_GENERIC_ERROR);
   }
   nhalf = nelem/2;
   for (i = 0; i < nelem/2; i++) {
      if (gvec[i] != gvec[nelem - (i + 1)]) {
	 shError("atRegConvolveWithGaussian: gaussian vector must be symmetric about center");
	 return(SH_GENERIC_ERROR);
      }
   }

   /* okay, we're ready to convolve; first, do in the row-direction,
      and put the result into the "temp" region.  In this case, we can
      make the execution quicker by using pointers, since we're stepping
      in the same direction as the pointers go. */
   for (row = 0; row < nhalf; row++) {
      ppxout1 = &(temp->rows_fl32[row][0]);
      ppxout2 = &(temp->rows_fl32[(input->nrow - 1) - row][0]);
      for (col = 0; col < input->ncol; col++) {
	 *ppxout1++ = 0;
	 *ppxout2++ = 0;
      }
   }
   for (row = nhalf; row < input->nrow - nhalf; row++) {
      ppxin  = &(input->rows_fl32[row][nhalf]);
      ppxout = &(temp->rows_fl32[row][0]);
      for (col = 0; col <= nelem; col++) {
	 *ppxout++ = 0;
      }
      ppxout = &(temp->rows_fl32[row][nhalf]);
      for (col = nhalf; col < output->ncol - nhalf; col++) {
	 pxin = (float) *ppxin++;       
	 t = pxin*gvec[0];
	 *(ppxout - nhalf) += t;
	 *(ppxout + nhalf)  = t;
	 for (i = 1, j = nhalf - 1; i < nhalf; i++, j--) {
	    t = pxin*gvec[i];
	    *(ppxout - j) += t;
	    *(ppxout + j) += t;
	 }
	 *ppxout++ += pxin*gvec[nhalf];
      }
   }

   /* now, we do the col-direction.  Take data from the "temp" region and
      place the output of calculations into the "output" region.  Here,
      we can't use pointers, because we're stepping through rows, the
      first index in the array. */
   for (row = 0; row < input->nrow; row++) {
      ppxout1 = &(output->rows_fl32[row][0]);
      ppxout2 = &(output->rows_fl32[row][input->ncol - nhalf]);
      for (col = 0; col < nhalf; col++) {
	 *ppxout1++ = 0.0;
	 *ppxout2++ = 0.0;
      }
   }
   for (col = nhalf; col < input->ncol - nhalf; col++) {
      for (row = 0; row <= nelem; row++) {
	    output->rows_fl32[row][col] = 0;
      }
      for (row = nhalf; row < input->nrow - nhalf; row++) {
	 pxtemp = temp->rows_fl32[row][col];
	 t = pxtemp*gvec[0];
	 output->rows_fl32[row - nhalf][col] += t;
	 output->rows_fl32[row + nhalf][col]  = t;
	 for (i = 1, j = nhalf - 1; i < nhalf; i++, j--) {
	    t = pxtemp*gvec[i];
	    output->rows_fl32[row - j][col] += t;
	    output->rows_fl32[row + j][col] += t;
	 }
	 output->rows_fl32[row][col] += pxtemp*gvec[nhalf];
      }
   }

   /* and we're all done */
   return(SH_SUCCESS);
}
