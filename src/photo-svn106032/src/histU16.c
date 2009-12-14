   /*
    * <AUTO>
    * FILE: histU16.c
    *
    * Code to support PHHIST structures
    *
    * </AUTO>
    */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dervish.h"
#include "phHist.h"

   /* 
    * <AUTO EXTRACT>
    *
    * create a new PHHIST structure, with the given number of bins
    * and initial bin.  Allocate space for all the histogram bins,
    * and fill the bins with zeros.
    *
    * return:  pointer to new structure
    */

PHHIST *
phPhhistNew(
	    int num_bin,		/* number of bins in new structure */
	    int min_bin			/* value of first bin */
	    )
{
   PHHIST *new = (PHHIST *)shMalloc(sizeof(PHHIST));

   new->num_bin = num_bin;
   new->min_bin = min_bin;
   new->total_number = 0;
   new->value = (int *)shMalloc(new->num_bin*sizeof(int));
   memset((void *)new->value,'\0',new->num_bin*sizeof(new->value[0]));
   
   shAssert(new->value[0] == 0);	/* check that 0 is all 0 bits */

   return(new);
}

   /*
    * <AUTO EXTRACT>
    *
    * delete a PHHIST structure, freeing the histogram memory first.
    * 
    * input:   hist             structure to delete
    */
void
phPhhistDel(PHHIST *hist)
{
   if(hist == NULL) return;

   if (hist->value != NULL) {
      shFree(hist->value);
   }
   shFree(hist);
}

/*****************************************************************************/
/*
 * Now the same for PHHIST_Ss
 *
 * <AUTO EXTRACT>
 *
 * ROUTINE: phPhhist_sNew
 *
 * DESCRIPTION:
 * create a new PHHIST structure, with the given number of bins
 * and initial bin. Don't zero the storage.
 *
 * return:  pointer to new structure
 *
 * </AUTO>
 */
PHHIST_S *
phPhhist_sNew(
	      int num_bin,		/* number of bins in new structure */
	      int min_bin			/* value of first bin */
	      )
{
   PHHIST_S *new = (PHHIST_S *)shMalloc(sizeof(PHHIST_S));

   new->num_bin = num_bin;
   new->min_bin = min_bin;
   new->value = (unsigned short *)shMalloc(new->num_bin*sizeof(short));

   return(new);
}

/*
 * <AUTO EXTRACT>
 *
 * delete a PHHIST_S structure, freeing the histogram memory first.
 */
void
phPhhist_sDel(
	      PHHIST_S *hist		/* structure to delete */
	      )
{
   if(hist->value != NULL) {
      shFree(hist->value);
   }
   shFree(hist);
}

/*****************************************************************************/
/*
 * And now for cumulative histograms
 *
 * <AUTO EXTRACT>
 *
 * create a new PHHIST_CUMUL structure, with the given number of bins
 * and initial bin. Don't zero the storage.
 *
 * Note that element [-1] of the values array is created; it should always
 * be 0, but it simplifies some code
 *
 * return:  pointer to new structure
 *
 * </AUTO>
 */
PHHIST_CUMUL *
phPhhist_cumulNew(
		  int num_bin,		/* number of bins in new structure */
		  int min_bin		/* value of first bin */
		  )
{
   PHHIST_CUMUL *chist = shMalloc(sizeof(PHHIST_CUMUL));
   chist->cvalue = shMalloc((num_bin + 1)*sizeof(int));
   chist->cvalue++;			/* create chist[-1] */
   chist->num_bin = num_bin;
   chist->min_bin = min_bin;

   return(chist);
}


/*
 * <AUTO EXTRACT>
 *
 * delete a PHHIST_CUMUL structure, freeing the histogram memory first.
 */
void
phPhhist_cumulDel(
		  PHHIST_CUMUL *chist		/* structure to delete */
		  )
{
   if(chist->cvalue != NULL) {
      shFree(&chist->cvalue[-1]);
   }
   shFree(chist);
}
