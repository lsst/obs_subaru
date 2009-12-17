/*****************************************************************************
******************************************************************************
**
** File:
**
**	histsubs.c
**
** ABSTRACT:
**	This file contains routines to histogram unsigned integer data.
**	Also, one can find the ntiles of a histogram array.
**
** ENVIRONMENT:
**	ANSI C
**
** AUTHORS:	Creation date: 1992
**		Don Petravick
**		Heidi Newberg
**
******************************************************************************
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "shCHist.h"

/* Return the maximum value of the histogram */
/* If the peak of the histogram is flat, the first bin in the flat top
	will be returned. */

unsigned int shHistFindPeak(HIST *hists) {

	unsigned int numbins, i, peak;
	unsigned int hmax, temp;

	peak = 0;
	numbins = hists->nbins;
	for (i=0, hmax=0;i<numbins;i++) {
		temp = hists->bins[i];
		if (temp > hmax) {
			hmax = temp;
			peak = i;
		}
	}
	return (peak);
}

/* Allocate an arrary of histgram structures, along with storage for the
*  histgram bins.
*  The histogram parameters are initialized.
*  The histgram has two additional bins for underflow and overflow.
*  Space for these should NOT be specified by the user.
*/

HIST *shHistConstructHists(unsigned int nhists, unsigned int sbin, unsigned int nbins, unsigned int binwid) {

  HIST *hists;

  unsigned int i,j;
  unsigned int *lcl_suip = NULL;
  hists = (HIST *)calloc(nhists, sizeof(HIST));
  if (hists == NULL) return (hists);  /* couldn't calloc the array of hists*/
  /* Have calloc'ed the array of hists, now go and calloc bins for each hist*/
  for(i=0; i<nhists; i++) {
    lcl_suip = (unsigned int *)calloc(nbins, sizeof(hists->bins));
    hists[i].bins = lcl_suip;
  }

  if (lcl_suip == NULL) {  /*Couldn't calloc all the space for the bins */
    for (i=0; i<nhists; i++) if (hists[i].bins != NULL) free (hists[i].bins);
    free(hists);
    hists = NULL;
  }

  else { /* Initialize the parameters */
    for (i=0; i<nhists; i++) {
      hists[i].base = sbin;
      hists[i].nbins = nbins;
      hists[i].binwid = binwid;
      hists[i].underflow = 0;
      for (j=0;j<(hists[i].nbins); j++) hists[i].bins[j]=0;
      hists[i].overflow = 0;
      hists[i].naccum = 0;
    }
  }

  return (hists);
}



/* Initialize the histogram bins to zero */

void shHistInitHists (HIST *hists, unsigned int nhists) {

  unsigned int i,j;

  for (i=0;i<nhists; i++) {
    hists->underflow = 0;
    for (j=0;j<(hists->nbins); j++) hists->bins[j]=0;
    hists->overflow = 0;
    hists->naccum = 0;
    hists++;
  }

}



/* Deallocate the Histograms and free the storage */

void shHistDestructHists(HIST *hists , unsigned int nhists){

  unsigned int i;

  /* free bins for each hist*/
  for(i=0; i<nhists; i++) free (hists[i].bins);
  /* free array of histogram structures*/
  free(hists);

}



/* Allocate space for the Ntile arrays */

unsigned int **shHistConstructNtiles(int numPercentiles, unsigned int nhists) {
  unsigned int i;
  unsigned int **ntiles;
  unsigned int *lcl_suip = NULL;
  ntiles = (unsigned int **)calloc(nhists, sizeof(unsigned int *));
  if (ntiles == NULL) return (ntiles); /* couldn't calloc the array of ntiles*/
  /*  Have calloc'ed the array of ntiles, now go and calloc space for the 
      ntiles  */
  for (i=0; i<nhists; i++) {
    lcl_suip = (unsigned int *)calloc(numPercentiles, sizeof(unsigned int));
    ntiles[i] = lcl_suip;
  }

  if (lcl_suip == NULL) {  /*Couldn't calloc all the space for the ntiles */
    for (i=0; i<numPercentiles; i++) if (ntiles[i] != NULL) free (ntiles[i]);
    free(ntiles);
    ntiles = NULL;
  }

  return (ntiles);

}



/* Deallocate the Ntiles and free the storage */

void     shHistDestructNtiles(unsigned int **ntiles , unsigned int nhists) {
  
  unsigned int i;

  /* free lists of percentiles for each column */
  for (i=0; i<nhists; i++) free (ntiles[i]);
  /* free ntile pointer */
  free(ntiles);

}



/* Accumulate an unsigned int input line into an array of histograms */

void shHistArrayAccumulateLine(HIST *hists, unsigned int *line, unsigned int nele) {
  
  register unsigned int i;
  register unsigned int index;
  register unsigned int element;
  register HIST *hist = hists;


  for (i=0; i<nele; i++) {
    element = *line++;
    if (hist->base > element) hist->underflow++;
    else {
      index = (element - hist->base)/hist->binwid;
      if (index >= hist->nbins) hist->overflow++;
      else hist->bins[index]++;
    }
    hist->naccum++;                   
    hist++; 
  }
}



/* Accumulate an unsigned int input line into a single histogram */

void shHistAccumulateLine(HIST *hists, unsigned int *line, unsigned int nele) {
  
  register unsigned int i;
  register unsigned int index;
  register unsigned int element;
  register HIST *hist = hists;
  
  for (i=0; i<nele; i++) {
    element = *line++;
    if (hist->base > element) hist->underflow++;
    else {
      index = (element - hist->base)/hist->binwid;
      if (index >= hist->nbins) hist->overflow++;
      else hist->bins[index]++;
    }
    hist->naccum++;                   
  }
}


/* This routine takes an array of histograms and generates the ntiles for
   each histogram into ntiles.  ntilesWantedPercentiles is a pointer to
   an array telling which percentiles are requested.  The space for the
   ntiles must already be malloced and the size must agree with the number
   of histograms and the number of ntiles requested.  The unsigned int
   ntiles[i][j] is the ith histogram and the jth percentile              */

void shHistDoNtiles(HIST *hists, int nhists, unsigned int **ntiles, int *ntilesWantedPercentiles) {

  int *percentile;   /*Current percentile we want to find*/
  unsigned int bound;  /*The we accumulate to locate the current %-ile*/
  unsigned int  sum;   /*The number of elements we have accumulated*/
  unsigned int *binptr;       /*pointer to walk through the histogram bins*/
  HIST             *hist;         /*Current histogram*/
  unsigned int  *ntile;     /*Current field in current ntile structure*/
  float temp;
  int i;      

  for (i=0; i<nhists; i++) {
    percentile = ntilesWantedPercentiles;
    hist = hists++;              /*set current input  histogram*/
    ntile= *ntiles++;   /*set to first field in current output ntile*/
    binptr = (hist->bins)-1;
    sum = hist->underflow; 

    /* Locate each requested percentile by walking through the histogram until
        BOUND values are passed. Take the ntile as the integer truncation of
        the average of the BOUNDth-1, BOUNDth, and BOUNDth+1 histogram
	accumulation. */

    while (*percentile) {
      bound = *percentile * hist->naccum/ (unsigned)100;
      if (bound <= hist->underflow) {
        *ntile++ = hist->base;
      }
      else if ( (bound + hist->overflow) >= hist->naccum) {
        *ntile++ = hist->base + hist->binwid * hist->nbins;
      }
      else {
        while (sum < bound) sum += (int)*(++binptr);   

        temp = (float)(hist->base) +
		(float)(binptr-hist->bins) * (float)hist->binwid +
		(float)(sum-bound) * (float)hist->binwid / *binptr;
        *ntile++ = (unsigned int)temp;

        /* stow the percentile into the output.
        * roll back one cell, to make the loop handle cases where more than one
	* ntile is in the same bin.  Be careful of the case where the ntile is
	* contained in the first bin */
        sum -= *binptr;
        binptr = binptr-1; if (binptr < hist->bins -1) binptr = hist->bins -1;
      }
      percentile++;
    } /*end while list of percentiles is not exhausted*/
  }/*end for each hist*/
}

