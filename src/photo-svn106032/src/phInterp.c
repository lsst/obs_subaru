
   /**********************************************************************
    * <AUTO>
    * FILE: phInterp.c
    *                                                                    * 
    *  a library of interpolation-type routines, similar to Mirage/Mirella 
    *                                                                    * 
    * </AUTO>
    **********************************************************************/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "dervish.h"
#include "phUtils.h"
#include "phInterp.h"

#define INTERP_ERRLEVEL   2    /* shDebug level for error messages herein */

#undef  DEBUG                     /* print out values of radii after parsing */


   /************************************************************************
    * <AUTO EXTRACT>
    * ROUTINE: phFindIndexFloat
    *
    * DESCRIPTION:
    * Given an array of 'npt' floats, which are assumed to be in monotonically
    * increasing or decreasing order, return the index i of the first element
    * such that array[i] >= 'x' for increasing arrays, or array[i] <= 'x'
    * for decreasing arrays.  
    *
    * we use a binary search for efficiency with possibly large arrays.
    * 
    * input:   npt            number of elements in the array  
    *          array          array of floats, in monotonic order
    *          x              value we're looking for
    * 
    * output:  index          index of array such that value 'x' is between
    *                           array[index] and array[index - 1] (if possible)
    *                           _or_ edge of array closest to 'x'
    *
    * return   SH_SUCCESS         if such an index exists
    *          SH_GENERIC_ERROR   if 'x' is outside array values
    *
    * </AUTO>
    */

int
phFindIndexFloat(int npt, float *array, float x, int *index)
{
   int low, high, mid;

   if (array == NULL) {
      shError("phFindIndexFloat: passed a NULL array");
      return(SH_GENERIC_ERROR);
   }

   low = 0;
   high = npt - 1;
   
   /* figure out if the array is monotonically increasing or decreasing */
   if (array[low] < array[high]) {
      /* array values rise monotonically */
      if ((x < array[low]) || (x > array[high])) {
	 shDebug(INTERP_ERRLEVEL, "phFindIndexFloat: value %f outside array range",
		  x);
	 if (x < array[low]) {
	    *index = low;
	 }
	 else {
	    *index = high;
	 }
	 return(SH_GENERIC_ERROR);
      }
      while (high - low > 1) {
	 mid = (low + high)/2;
	 if (x < array[mid]) {
	    high = mid;
	 }
	 else {
	    low = mid;
	 }
      }
      *index = high;
      return(SH_SUCCESS);
   }
   else {
      /* array values drop monotonically */
      if ((x > array[low]) || (x < array[high])) {
	 shDebug(INTERP_ERRLEVEL, "phFindIndexFloat: value %f outside array range",
		  x);
	 if (x > array[low]) {
	    *index = low;
	 }
	 else {
	    *index = high;
	 }
	 return(SH_GENERIC_ERROR);
      }
      while (high - low > 1) {
	 mid = (low + high)/2;
	 if (x > array[mid]) {
	    high = mid;
	 }
	 else {
	    low = mid;
	 }
      }
      *index = high;
      return(SH_SUCCESS);
   }

}

   
   /********************************************************************
    * <AUTO EXTRACT>
    * ROUTINE: phLinearInterpFloat
    *
    * DESCRIPTION:
    * given two floating point arrays with 'npt' values in each,
    * in which values of some function 'y' are tabulated at each
    * value of an independent variable 'x', perform linear interpolation
    * to find the value of 'y' at the point 'xval'.  Place the 
    * interpolated value into the last argument 'yval'.  
    *
    * input:  npt              number of points in each array
    *         x                values of points in independent array
    *         y                values of points in dependent array
    *         xval             value at which we must interpolate in 'y'
    *
    * output: yval             pointer to calculated value of interpolation
    *
    * return: SH_SUCCESS         if all goes well
    *         SH_GENERIC_ERROR   if a pointer is NULL, or would divide
    *                                by zero.
    * </AUTO>
    */

int
phLinearInterpFloat(int npt, float *x, float *y, float xval, float *yval)
{
   int index;
   float delta, dx;

   /* first, make some sanity checks */
   if ((x == NULL) || (y == NULL) || (yval == NULL)) {
      shError("phLinearInterpFloat: handed NULL pointers");
      return(SH_GENERIC_ERROR);
   }

   /* check for the degenerate cases of 0 or 1 points */
   if (npt == 0) {
      shError("phLinearInterpFloat: handed zero datapoints");
      return(SH_GENERIC_ERROR);
   }
   if (npt == 1) {
      shDebug(INTERP_ERRLEVEL, "phLinearInterpFloat: handed one datum");
      *yval = y[0];
      return(SH_SUCCESS);
   }
   
   /*
    * next, find the appropriate index of the 'x' array which is 
    * greater than the value 'xval'.  call it 'index'.  We will
    * interpolate between x[index - 1] and x[index], if possible.
    */
   if (phFindIndexFloat(npt, x, xval, &index) != SH_SUCCESS) {
      shError("phLinearInterpFloat: can't find index for value %f", xval);
      return(SH_GENERIC_ERROR);
   }

   /* make sanity check */
   if (index == 0) {
      shDebug(INTERP_ERRLEVEL, "phLinearInterpFloat: index == 0; set index = 1");
      index = 1;
   }

   /* okay, now do simple interpolation */
   if ((dx = x[index] - x[index - 1]) == 0) {
      shError("phLinearInterpFloat: equal values in x array, divide by zero");
      return(SH_GENERIC_ERROR);
   }
   delta = (x[index] - xval)/dx;
   *yval = delta*y[index - 1] + (1.0 - delta)*y[index];
   return(SH_SUCCESS);
}

      
