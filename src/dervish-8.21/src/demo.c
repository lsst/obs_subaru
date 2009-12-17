/******************************************************************************
*******************************************************************************
**
** FILE:
**
**	demo.c
**
** ABSTRACT:
**      This file contains routines used by the demo.
**
** ENVIRONMENT:
**	ANSI C
**	Tcl
**
**
** ENTRY POINT			SCOPE	DESCRIPTION
** ---------------------------------------------------------------------------
** shD_RegSkyFind               public  Finds the sky background by a Gaussian
**                                      fit to the region histogram.
**
** Conditional compilation is controlled by the following macros:
**
**	none
**
** AUTHORS:	Creation date: May 5, 1993
**		Heidi Newberg
**              Bradford Holden
**
******************************************************************************
*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>

#include "tcl.h"
#include "ftcl.h"
#include "region.h"
#include "shCRegUtils.h"
#include "shCDemo.h"
#include "shCErrStack.h"
#include "shCAssert.h"
#include "shCHist.h"



void shD_RegClippedStatsFind (const REGION *reg, const double lbound,
	const double ubound, double *mean, double 
	*sigma, double *high, int *hrow, int *hcol, double *low, int *lrow, 
	int *lcol)
{


        int r, c;
        double np,sum2,total, temp;

        np = 0;
        total = 0.0;
        sum2 = 0;
        *low = shRegPixGetAsDbl (reg, 0, 0);
        *lrow = 0;
        *lcol = 0;
        *high = shRegPixGetAsDbl (reg, 0, 0);
        *hrow = 0;
        *hcol = 0;

        for (r=0; r<reg->nrow; r++) {

                for (c=0; c<reg->ncol; c++) {

                        temp = shRegPixGetAsDbl (reg, r, c);
			if ((temp > lbound) && (temp < ubound)) {
                        	total += temp;
	                        sum2 +=  temp*temp;
				np++;

                        	if (temp < *low) {
                                	*low = temp;
                                	*lrow = r;
                                	*lcol = c;
                        	}
                        	else if (temp > *high) {
                                	*high = temp;
					*hrow = r;
                                	*hcol = c;
                        	}
			}

                }

        }

        *mean = total/np;
        *sigma = sqrt(fabs((sum2/np)-((double)*mean*(double)*mean)));

        return;
}


void shD_RegStatsFind (const REGION *reg, double *mean, double *sigma,
        double *high, int *hrow, int *hcol, double *low, int *lrow, int *lcol) 
{


        int r, c;
        double np,sum2,total, temp;

        np = (float)reg->nrow* (float)reg->ncol;
        total = 0.0;
        sum2 = 0;
        *low = shRegPixGetAsDbl (reg, 0, 0);
        *lrow = 0;
        *lcol = 0;
        *high = shRegPixGetAsDbl (reg, 0, 0);
        *hrow = 0;
        *hcol = 0;

        for (r=0; r<reg->nrow; r++) {

                for (c=0; c<reg->ncol; c++) {

                        temp = shRegPixGetAsDbl (reg, r, c);
                        total += temp;
                        sum2 +=  temp*temp;

                        if (temp < *low) {
                                *low = temp;
                                *lrow = r;
                                *lcol = c;
                        }
                        else if (temp > *high) {
                                *high = temp;
                                *hrow = r;
                                *hcol = c;
                        }

                }

        }

        *mean = total/np;
        *sigma = sqrt(fabs((sum2/np)-((double)*mean*(double)*mean)));

        return;
}

/* Find the sky background in an image by fitting a Gaussian to the
   peak of a histogram of pixel values				*/

double shD_RegSkyFind (const REGION *reg) 
{

        HIST *hists;
	unsigned int *line;
        unsigned int r, c, binspersigma, nsigma, histmode;
	unsigned int n1,n2,n3;
	double mean,sigma,high,low;
	double h1, h2, h3, histsky, sky;
	int hrow, hcol, lrow, lcol;

	nsigma = 20;		/* sigma each side of the mean */
	binspersigma = 4;

	line = (unsigned int *)calloc(reg->ncol,sizeof(unsigned int));

	/* Find the mean and sigma */
	shD_RegStatsFind (reg, &mean, &sigma, &high, &hrow, &hcol, &low, &lrow, &lcol);
	shD_RegClippedStatsFind(reg, mean-sigma, mean+sigma, &mean, &sigma, &high, &hrow, &hcol, &low, &lrow, &lcol);
	if (sigma<=0) sky = shRegPixGetAsDbl(reg,0,0);
	else {
	/* Histogram nsigma sigma each side of the mean with 
		bin width sigma/binspersigma 				*/
        hists = shHistConstructHists (1, 0, 2*nsigma*binspersigma, 1);
        shHistInitHists(hists,1);
        for (r=0; r<reg->nrow; r++) {
		for (c=0; c<reg->ncol; c++) 
			line[c] = (unsigned int) floor(binspersigma*
                                                    ((shRegPixGetAsDbl(reg,r,c)-mean)/sigma + nsigma));
		shHistAccumulateLine (hists, line, (unsigned int)reg->ncol);
	}

	/* Find the peak bin */
        histmode = shHistFindPeak(hists);

/* Fit a Gaussian to the peak plus its adjacent bins */
	n1 = hists->bins[histmode-1];
	n2 = hists->bins[histmode];
	n3 = hists->bins[histmode+1];
	if ((n1 > 0) && (n2 > 0) && (n3 > 0)) {
	h1 = log ( (double) n1);
	h2 = log ( (double) n2);
	h3 = log ( (double) n3);
	histsky = (double)histmode + 0.5 + (h3-h1)/(2*(2*h2-h1-h3));
      } else {histsky = (double)histmode;}

        shHistDestructHists (hists, 1);

	/* Convert histsky to region sky */

	sky = (histsky/binspersigma - nsigma)*sigma + mean;
      }

	return(sky);

}
