
   /**********************************************************************
    * <AUTO>
    * FILE: library.c                                                    *
    *                                                                    * 
    *  a library of general-purpose routines adapted from Mirella/Mirage,*
    *    one of the major components being routines to measure           *
    *    radial profiles.                                                *
    *                                                                    * 
    * </AUTO>
    **********************************************************************/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "dervish.h"
#include "phConsts.h"
#include "phUtils.h"
#include "phLibrary.h"
#include "phSpanUtil.h"
#include "cpgplot.h"

#define MAX(A,B) ((A) > (B) ? (A) : (B))
#define MIN(A,B) ((A) < (B) ? (A) : (B))

#define LIB_ERRLEVEL   2    /* shDebug level for error messages herein */

   /* we'll make a temporary histogram in this */
static int ph_histu16[MAX_U16 + 1];    

#undef   DEBUG2                   /* print out values of radii after parsing */

#define STRLENGTH    500    /* max length of aperture string in param file */

   /* 
    * some special values used in translating distance from object 
    * centroid to aperture number in profile 
    */
#define TABLE_IGNORE   254       /* this pixel falls in no aperture */
#define TABLE_CLOSE    255       /* this pixel falls into 'center' area */
#define PROFILE_SCALE  100.0     /* multiply squared dist by this factor */
                                 /*   then do array lookup to find aper */

   /************************************************************************
    * <AUTO EXTRACT>
    *
    * ROUTINE: phHistU16FromRegion
    *
    * DESCRIPTION:
    * given a REGION of type U16, create a PHHIST structure, fill it
    * with appropriate values, and return a pointer to the new structure.
    * Ignore all pixels with MASK values that form bitwise AND with
    * the given "mask_flag" argument.
    * 
    * Print an error message and return NULL if the REGION has no MASK.
    *
    * input:   reg          region from which we make histogram
    *          mask_flag    we ignore an pixel whose MASK value
    *                            will AND with this char (for example,
    *                            could be MASK_NOTCHECKED)
    *
    * return pointer to new PHHIST if all goes well, 
    * or     NULL if there's a problem.
    * </AUTO>
    */


PHHIST *
phHistU16FromRegion(REGION *reg, CHAIN *mask) 
{
   PIX min, max, *pixp;
   int bin, num_bin, row, col, index, total_number;
   int *intp, *binp;
   PHHIST *new;
   OBJMASK *iomask;		/* merged mask of ignored pixels */
   int mspan;			/* span bookkeeping */
   SPAN *s;
   int nspans;
   int moffx; int moffy;
   int inmasky;

   shAssert(reg != NULL);
   
   /* 
    * this will be a temporary histogram, from which we'll extract
    * a compact histogram later.
    */
   intp = &(ph_histu16[0]);

   /* build the temporary histogram */
   total_number = 0;
   min = MAX_U16;
   max = 0;
   if(mask == NULL) {	/* use all pixels */
       shDebug(LIB_ERRLEVEL,"phHistU16FromRegion: given region has no MASK, use all pixels");

       for(row = 0; row < reg->nrow; row++) {
	  pixp = reg->ROWS[row];
	  for(col = 0; col < reg->ncol; col++, pixp++) {
	     if((index = PIX2INT(*pixp)) < min) {
		if(index < 0) {
		   index = 0;
		}
		min = index;
	     }
	     if (index > max) {
		if(index > MAX_U16) {
		   index = MAX_U16;
		}
		max = index;
	     }
	     
	     ph_histu16[index]++;
	     total_number++;
	  }
       }
   } else {
       iomask = phMergeObjmaskChain(mask);
       mspan = 0;
       moffx = iomask->cmin;
       moffy = iomask->rmin;
       s = iomask->s;
       nspans = iomask->nspan;

       for (row = 0; row < reg->nrow; row++) {
	  while(mspan < nspans && s[mspan].y + moffy < row+reg->row0)
	      mspan++;
	  if(mspan < nspans && s[mspan].y + moffy == row+reg->row0)
	      inmasky = 1;
	  else
	      inmasky = 0;

	   pixp = reg->ROWS[row];
	   for (col = 0; col < reg->ncol; col++, pixp++) {
	       if(inmasky) {
		 while(s[mspan].y + moffy == row+reg->row0
		       && s[mspan].x2 + moffx < col+reg->col0)
		     mspan++;
		 if(mspan < nspans && s[mspan].y + moffy == row+reg->row0
		    && s[mspan].x1 + moffx <= col+reg->col0)
		     continue;
	       }
	       if ((index = PIX2INT(*pixp)) < min) {
		   min = index;
	       }
	       if (index > max) {
		   max = index;
	       }
	       ph_histu16[index]++;
	       total_number++;
	   }
       }
       phObjmaskDel(iomask);
   }
   /* check to see if all pixels failed the mask test - if so, return NULL */
   if (max < min) {
      shDebug(0, "phHistU16FromRegion: no pixels chosen from region?!");
      return(NULL);
   }

   /* create a new PHHIST structure */
   num_bin = (max - min) + 1;
   new = phPhhistNew(num_bin, min);
   new->total_number = total_number;

   intp = &new->value[0];
   binp = &ph_histu16[PIX2INT(min)];
   for (bin = min; bin <= max; bin++) {
      *intp++ = *binp++;
   } 

   /* now reset to zero all elements of the "private" array we touched */
   binp = &ph_histu16[PIX2INT(min)];
   for (bin = min; bin <= max; bin++) {
      *binp++ = 0;
   }

   return(new);
}

   /************************************************************************
    * <AUTO EXTRACT>
    * ROUTINE: phHistU16FromArray
    *
    * DESCRIPTION:
    * given a 1-D array of type U16, create a PHHIST structure, fill it
    * with appropriate values, and return a pointer to the new structure.
    * 
    * input:   nelem        number of elements in array
    *          array        starting address of the array
    *
    * return pointer to new PHHIST if all goes well, 
    * or     NULL if there's a problem.
    * </AUTO>
    */

PHHIST *
phHistU16FromArray(int nelem, PIX *array) 
{
   PIX min, max, *uintp;
   int i, num_bin, index, total_number;
   PHHIST *new;

   if (nelem < 1) {
      return(NULL);
   }

   /* 
    * this will be a temporary histogram, from which we'll extract
    * a compact histogram later.
    */
   shAssert(ph_histu16[0] == 0);

   /* build the temporary histogram */
   total_number = 0;
   min = MAX_U16;
   max = 0;
   uintp = array;
   for (i = 0; i < nelem; i++, uintp++) {
      if ((index = PIX2INT(*uintp)) < min) {
	 min = index;
      }
      if (index > max) {
	 max = index;
      }
      ph_histu16[index]++;
      total_number++;
   }

   /* make a sanity check (probably not necessary, but code could change */
   if (max < min) {
      shErrStackPush("phHistU16FromArray: no elements chosen from array?!");
      return(NULL);
   }

   /* create a new PHHIST structure */
   num_bin = (max - min) + 1;
   new = phPhhistNew(num_bin, min);

   new->total_number = total_number;

   shAssert(sizeof(new->value[0]) == sizeof(ph_histu16[0]));
   memcpy((void *)new->value,(const void *)&ph_histu16[PIX2INT(min)],
	  (max - min + 1)*sizeof(ph_histu16[0]));

   /* now, reset the "private" array's values all to zero */
   memset(&ph_histu16[PIX2INT(min)], 0, num_bin*sizeof(ph_histu16[0]));

   return(new);
}
      

   /***********************************************************************
    * <AUTO EXTRACT>
    *
    * ROUTINE: phNtile(PHHIST *hist, int num_ntiles, float *ntile_vals)
    *
    * DESCRIPTION:
    * given a PHHIST structure, and a number of n'tiles
    * (that is, if given num_ntiles = 2, find the 50'th percentile,
    * if num_ntiles = 3, find the 33'rd and 67'th percentile, etc.),
    * return in the argument 'ntile_vals' floating-point values for
    * each n'tile.  
    *
    * the user must allocate space for the return array "ntile_vals".
    * 
    * use linear interpolation between adjacent bins to calculate the
    * exact value of an n'tile.
    *
    * input:  hist          PHHIST structure (histogram values)
    *         num_ntiles    number of N'tiles to calculate
    *
    * output: ntile_vals    already-allocated array with num_ntiles-1
    *                         elements, into which values are placed
    * 
    * return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
    *
    * </AUTO>
    */ 

int
phNtile(PHHIST *hist, int num_ntiles, float *ntile_vals)
{
   int *histp, sum, tile, lower_bound, upper_bound;
   float fbound;   

   if ((hist == NULL) || (hist->num_bin <= 0) || (hist->value == NULL)) {
      shErrStackPush("phNtile: given histogram structure is NULL or invalid");
      return(SH_GENERIC_ERROR);
   }
   if ((num_ntiles <= 0) || (ntile_vals == NULL)) {
      shErrStackPush("phNtile: num_ntiles is invalid of ntile_vals is NULL");
      return(SH_GENERIC_ERROR);
   }

   /* 
    * we loop through the (N-1) n'tiles we must calculate, doing the
    * lowest-percentile ones first.
    */
   histp = &(hist->value[0]);
   sum = 0;
   for (tile = 0; tile < num_ntiles - 1; tile++) {
      fbound = ((float) ((tile + 1)*hist->total_number))/num_ntiles;
      lower_bound = fbound;
      upper_bound = lower_bound + 1;
      if (upper_bound > hist->total_number) {
	 upper_bound = hist->total_number;
      }

      /* first, find the bin that contains the N'tile point */
      while (sum < upper_bound) {
	 sum += *histp++;
      }

      /* 
       * at this point, the desired N'tile must lie in the _previous_
       * bin (because we incremented it after dereferencing).
       */
      histp--;

      /*
       * NOW the N'tile lies in the current bin.  We use linear interpolation
       * between the previous and current bins 
       * to calculate a floating-point value for the n'tile.  Assume that
       * each bin runs from (binval - 0.5) to (binval + 0.5).  If the current
       * bin causes the sum to equal fbound exactly, we'll return a value
       * of (binval + 0.5) as a result.
       */
      ntile_vals[tile] = hist->min_bin + (histp - hist->value) + 0.5
		     - ((float)sum - fbound)/((float)(*histp));

      /* 
       * done with the tile'th n'tile.  Before we go on to the next
       * one, we arrange to process this current bin again, as the first
       * bin in the upcoming loop (if the current bin contains many
       * objects, it might contain the (tile+1)'th n'tile, too).
       * since we're going to process this bin again, subtract its
       * contribution to the sum here.
       */
      sum -= *histp;
   }

   return(SH_SUCCESS);
}
   

   /***********************************************************************
    * <AUTO EXTRACT>
    * ROUTINE: phHistU16Stats
    *
    * DESCRIPTION:
    * given a PHHIST structure, which must already be filled with data,
    * do the following:
    * 
    *   1. if the "clip_flag" argument is 1,
    *         create a "main body" histogram, by throwing out
    *         points which would be more than 2.7 sigma from the mean
    *         if the distribution were gaussian
    *         (otherwise, use the entire histogram)
    *
    *   2. using data which satisfies this criterion, calculate
    *             A. the quartiles
    *             B. the mean
    *             C. the mode (via 3*median - 2*mean)
    *             D. equiv of std dev from mean, via 
    *                   0.759*(3rd quartile - 1st quartile)
    * 
    * use linear interpolation between adjacent bins to calculate the
    * exact value of an n'tile.
    *
    * input:  hist          PHHIST structure (histogram values)
    *         nsigma        == 0, use all data;  > 0, use 
    *                            only the data within +/- nsigma
    *
    * output: quartiles[3]  already-allocated array for the quartiles 
    *                               of (clipped) histogram
    *         mean          mean of (clipped) hist
    *         mode          mode of (clipped) hist
    *         sigma         "std dev" from mean of (clipped) hist
    * 
    * return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
    *
    * </AUTO>
    */ 

int
phHistU16Stats(PHHIST *hist, double nsigma, float *quartiles, 
                          float *mean, float *mode, float *sigma)
{
   int *histp, sum, weighted_sum, bin_val, tile, lower_bound, upper_bound;
   int i, bin, bottom_bin, top_bin, ignore_sum, num_ntiles, total_number;
   float bottom_frac_bin, top_frac_bin;
   float fbound, rough_median, sig23, rough_quartiles[3];

   if ((hist == NULL) || (hist->num_bin <= 0) || (hist->value == NULL)) {
      shErrStackPush("phHistU16Stats: given histogram structure is NULL or invalid");
      return(SH_GENERIC_ERROR);
   }
   if (nsigma < 0 ) {
      shErrStackPush("phHistU16Stats: nsigma < 0");
      return(SH_GENERIC_ERROR);
   }
   if ((quartiles == NULL) || (mean == NULL) || (mode == NULL) || 
	    (sigma == NULL)) {
      shErrStackPush("phHistU16Stats: given NULL output pointer(s)");
      return(SH_GENERIC_ERROR);
   }

   /* 
    * first, find rough values for quartiles, using all data
    */
   phNtile(hist, 4, rough_quartiles);
   for (i = 0; i < 3; i++) {
      quartiles[i] = rough_quartiles[i];
   }

   if (nsigma == 0) {
      bottom_bin = 0;
      top_bin = hist->num_bin;
   }
   else {
      /*
       * now, we trim the histogram to the range that _would_ correspond to
       * +/- nsigma if the distribution were gaussian.  Here's what we
       * do: we use the rough quartile values to calculate where the +/- 
       * nsigma bins would be.  We'll then make a second pass, using only
       * data between those bins.
       * 
       * recall that sigma for a gaussian is 0.741*(3rd quart - 1st quart),
       * so nsigma*sigma = nsigma*0.741 = 1.70*(3rd quart - 1st quart).
       * 
       * We add 0.5 before determining the top and bottom bins since
       * the bin containing x really goes from x-.5 to x+.5. For
       * example, if rough_median-sig23 = 156.8, the bottom bin to
       * include in the clipped histogram is the one centered on 157,
       * and we should exclude the 0.3 of that bin (appropriately
       * interpolated)
       */
      rough_median = rough_quartiles[1];
      sig23 = nsigma*0.741*(rough_quartiles[2] - rough_quartiles[0]);
      top_frac_bin = 0.5 + rough_median + sig23 - hist->min_bin;
      top_bin = (int) top_frac_bin;
      top_frac_bin -= top_bin;	/* fractional part of top bin to include */
      if (top_bin >= hist->num_bin) {
	  top_frac_bin = 0.;
	  top_bin = hist->num_bin;
      }

      bottom_frac_bin = 0.5 + rough_median - sig23 - hist->min_bin;
      bottom_bin = (int) bottom_frac_bin;
      bottom_frac_bin -= bottom_bin; /* part of bottom bin to exclude */
      if (bottom_bin <= 0) {
	  bottom_frac_bin = 0.;
	  bottom_bin = 0;
      }

      /* Debugging stuff */
#if 0 
      tmp=0.;
      for (i=0; i<hist->num_bin; i++) {
	  tmp += hist->value[i];
	  printf("%3d  %3d  %3d  %4d  ",i,i+hist->min_bin,hist->value[i],
		 (int)tmp);
	  if (i == bottom_bin) { 
	      printf("b=%f\n",bottom_frac_bin);
	  } else if (i == top_bin) {
	      printf("t=%f\n",top_frac_bin);	      
	  } else if (i == ((int) (quartiles[1]+0.5) - hist->min_bin))  {
	      printf("median = %f\n",quartiles[1]);
	  } else {
	      printf("\n");
	  }
      }
      printf("sig23=%f\n",sig23);
      printf("%f\t%f\n",quartiles[1]-sig23,quartiles[1]+sig23);
#endif
      /*
       * if the bin limits are the histogram limits, then all data falls
       * within the 2.3-sigma cutoff.  We can skip a second pass.
       */
      if ((bottom_bin == 0) && (top_bin == hist->num_bin)) {
         /* do nothing */ ;
      }
      else {
         /*
          * now, make a second pass, using only data in bins 
          *           [ bottom_bin <= bin < top_bin ]
          * we have to perform the calculations here, rather than call phNtile
          * because we don't want to use all bins or all of the counts in
	  * the bins on the boundary.
          */

         /* 
          * first step is to figure out how many of the total number of items
          * fall into the bins-to-be-ignored.
          */
         histp = &(hist->value[0]);
         ignore_sum = 0;
         for (bin = 0; bin < bottom_bin; bin++) {
	    ignore_sum += *histp++;
         }

	 ignore_sum += bottom_frac_bin * *histp + 0.5; 
         histp = &(hist->value[top_bin]);
	 ignore_sum += (1-top_frac_bin)* *histp++ + 0.5; 
         for (bin = top_bin+1; bin < hist->num_bin; bin++) { 
	    ignore_sum += *histp++;
         }
         /* now we can calculate quartiles. algorithm same as phNtile */
         num_ntiles = 4;
         histp = &(hist->value[bottom_bin]);
	 /*Count only part of bottom bin*/
	 sum = (1-bottom_frac_bin)* *histp++ + 0.5; /* for rounding */
         total_number = hist->total_number - ignore_sum;
         for (tile = 0; tile < num_ntiles - 1; tile++) {
            fbound = ((float) ((tile + 1)*total_number))/num_ntiles;
            lower_bound = fbound;
            upper_bound = lower_bound + 1;
            if (upper_bound > total_number) {
	       upper_bound = total_number;
            }
            /* first, find the bin that contains the N'tile point */
            while (sum < upper_bound) {
	       sum += *histp++;
            }

            /* 
             * at this point, the desired N'tile must lie in the _previous_
             * bin (because we incremented it after dereferencing).
             */
            histp--;

            /*
             * NOW the N'tile lies in the current bin.  We use linear 
             * interpolation between the previous and current bins 
             * to calculate a floating-point value for the n'tile.  Assume that
             * each bin runs from (binval - 0.5) to (binval + 0.5).  If the 
	     * current bin causes the sum to equal fbound exactly, we'll return 
   	     * a value of (binval + 0.5) as a result.
             */
            quartiles[tile] = hist->min_bin + (histp - hist->value) + 0.5
		     - ((float)sum - fbound)/((float)(*histp));

            /* 
             * done with the tile'th n'tile.  Before we go on to the next
             * one, we arrange to process this current bin again, as the first
             * bin in the upcoming loop (if the current bin contains many
             * objects, it might contain the (tile+1)'th n'tile, too).
             * since we're going to process this bin again, subtract its
             * contribution to the sum here.
             */
            sum -= *histp;
	 }
      }
   }

   /*
    * at this point, we use the (possibly trimmed) histogram (as indicated by
    * bottom_bin and top_bin) to calculate the mean, and after that,
    * to calculate approximations to mode and std-dev from mean.
    */
   histp = &(hist->value[bottom_bin]);
   sum = 0;
   weighted_sum = 0;
   bin_val = hist->min_bin + bottom_bin;
   for (bin = bottom_bin; bin < top_bin; bin++) {
      sum          += *histp;
      weighted_sum += (*histp++) * bin_val++;  
   }
   if (sum == 0) {
      shErrStackPush("phHistU16Stats: sum of items in clipped histogram = 0");
      return(SH_GENERIC_ERROR);
   }
   *mean = ((float)weighted_sum/(float)sum);
   *mode = 3.0*quartiles[1] - 2.0*(*mean);      /* only an approximation */
   /*
    * for a gaussian, sigma = 0.741(3rd quartile - 1st quartile), but, if
    * clip_flag == 1, we have
    * thrown out 2% of the data, causing clipped quartiles to be 
    * slightly closer together than true ones; so fraction is a bit larger.
    */
   if (nsigma == 0) {
      *sigma = 0.741*(quartiles[2] - quartiles[0]);
   }
   else {
       /* This needs to be adjusted for nsigma other than 2.3 */
      *sigma = 0.759*(quartiles[2] - quartiles[0]);   /* ditto */
   }

   return(SH_SUCCESS);
}
   
   
   /***********************************************************************
    * <AUTO EXTRACT>
    * ROUTINE: phRegStats
    *
    * DESCRIPTION:
    * given a REGION, and an optional objmask,
    * do the following:
    * 
    *   1. if mask != NULL, ignore all masked pixels 
    *
    *   2. if "clip_flag" = 1, create a clipped histogram; 
    *         otherewise, use all data in the histogram
    *
    *   3. calculate
    *             A. the quartiles
    *             B. the mean
    *             C. the mode (via 3*median - 2*mean)
    *             D. equiv of std dev from mean, via 
    *                   0.741*(3rd quartile - 1st quartile)    if false
    *                   0.759*(3rd quartile - 1st quartile)    if true
    * 
    * use linear interpolation between adjacent bins to calculate the
    * exact value of an n'tile.
    *
    * input:  region        REGION from which to calculate stuff
    *                               (must have a MASK)
    *         objmask       pixels to ignore (can be NULL)
    *
    *         clip_flag     false, don't clip; true, do
    * 
    * output: quartiles[3]  already-allocated array for the quartiles 
    *                               of clipped histogram
    *         mean          mean of clipped hist
    *         mode          mode of clipped hist
    *         sigma         "std dev" from mean of clipped hist
    * 
    * return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if not.
    *
    * </AUTO>
    */ 

int
phRegStats(REGION *reg, double nsigma, CHAIN *mask, 
                    float *quartiles, float *mean, float *mode, float *sigma)
{
   PHHIST *hist;

   if (reg == NULL) {
      shErrStackPush("phRegStats: region is NULL");
      return(SH_GENERIC_ERROR);
   }
   if ((quartiles == NULL) || (mean == NULL) || (mode == NULL) ||
	    (sigma == NULL)) {
      shErrStackPush("phRegStats: output argument(s) is NULL");
      return(SH_GENERIC_ERROR);
   }

   switch (reg->type) {
    case TYPE_U16:
      if ((hist = phHistU16FromRegion(reg, mask)) == NULL) {
	 shErrStackPush("phRegStats: phHistU16FromRegion returns NULL");
	 return(SH_GENERIC_ERROR);
      }

      if (phHistU16Stats(hist, nsigma, quartiles, mean, mode, sigma) != 
	  SH_SUCCESS) {
	 shErrStackPush("phRegStats: phHistU16Stats returns with error");
	 phPhhistDel(hist);
	 return(SH_GENERIC_ERROR);
      }

      phPhhistDel(hist);
      
      break;
    default:
/*
 * The following is actually identical to the U16 code, but it needn't be
 */
      if ((hist = phHistU16FromRegion(reg, mask)) == NULL) {
	 shErrStackPush("phRegStats: phHistU16FromRegion returns NULL");
	 return(SH_GENERIC_ERROR);
      }

      if (phHistU16Stats(hist, nsigma, quartiles, mean, mode, sigma) != 
	  SH_SUCCESS) {
	 shErrStackPush("phRegStats: phHistU16Stats returns with error");
	 phPhhistDel(hist);
	 return(SH_GENERIC_ERROR);
      }

      phPhhistDel(hist);
      
      break;
   }
      
   return(SH_SUCCESS);
}

