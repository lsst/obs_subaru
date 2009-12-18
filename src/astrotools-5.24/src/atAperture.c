/* <AUTO>
   FILE: atAperture.c
<HTML>
Tools for finding aperture magnitudes
</HTML>
</AUTO> */
#include "astrotools.h"
#include "dervish.h"

double at_find_median(double numbers[], int number_of_numbers);
static void at_select_amongst(double numbers[], 
		int number_of_numbers, int kth_place);
static void at_swap(double a[], int i, int j)
        { double t = a[i]; a[i] = a[j]; a[j] = t;} 

U16 at_find_medianU16(U16 numbers[], int number_of_numbers);
static void at_select_amongstU16(U16 numbers[], 
		int number_of_numbers, int kth_place);
static void at_swapU16(U16 a[], int i, int j)
        { U16 t = a[i]; a[i] = a[j]; a[j] = t;} 


void find_avgdev( double numbers[], int number_of_numbers, 
		  double *Fmean, double *Favgdev );
void find_clippedmean( double numbers[], int number_of_numbers, 
		       int number_of_iterations, double sigma_clip,
		       double *sigma_avgdev, double *sigma_mean, 
		       int *number_used);


/*<AUTO EXTRACT>
  Find the mean in an aperture

<P>
ALGORITHM:

	The routine goes through each pixel in a box inscribed by the
outer aperture.  It calculates the distance from the center of each
pixel in the box to the center of the aperture.  If the distance is less
than the outer aperture and greater than or equal to the inner radius,
then the value in that pixel is written to the "values" array, and
the number of pixels, "pixels", is incremented.  If there are zero
pixels, zero is returned.  If there is one pixel, its value is returned
as the mean.  Otherwise, the at_find_median, find_avgdev, or 
find_clippedmean routine is called to compute the median, mean, or
clippedmean which will be returned as mean.  If median is chosen,
the "values" array is partially sorted.  The results are passed back as 
pixels, mean, and avgError in the subroutine interface.  Avgerr is not 
computed for medians, so -1 is returned for this case.

The subroutine returns:
	0 if everything is okay
	1 if the radius extends outside the edge of the image
	2 if there were more pixels than max_values (this should never 
          occur)

</AUTO>*/
int  atMeanFindInAperture(
    REGION *regPtr,    /* region */
    double row,        /* row position of object */
    double column,     /* columns position of object */
    double inRadius,   /* inner radius */
    double outRadius,  /* outer radius */
    double *mean,   /* returned mean */
    double *avgError,  /* returned sigma on the mean
                         (this is set to -1 for 
                          medians, and could be quite 
                          wrong for sigma clipping,
                          especially with low 
                          nsigmaclip) */
    double *pixels,    /* pixels used to find 
                          the mean */
    double nsigmaclip, /* -1 - do median, 0 - do mean,
			>0 - gives number of sigma 
			at which to clip */
    double iter        /* number of iterations */
  ) {
  int r, rmin, rmax;		/* step through rows */
  int c, cmin, cmax;		/* step through columns */
  int max_values;		/* max values to be medianed /clipmeaned */
  int used_pixels;		/* number of pixels used in clipped_mean */
  double inRadius2, outRadius2;	/* radiuses squared */
  double delr2,delc2,distance;	/* calculate distance */
  double *values;		/* values in the annulus (malloced within) */

  int returnValue;		/* 0 if included; 1 if touching border;
				   2 if memory overwrite */

/* initialize some values */
  returnValue = 0;
  *mean = 0;
  *avgError = -1;
  *pixels = 0;
  used_pixels = 0;
  inRadius2 = inRadius*inRadius;
  outRadius2 = outRadius*outRadius;

  rmin = (int)floor(row - outRadius); rmax = (int)ceil(row + outRadius);
  cmin = (int)floor(column - outRadius); cmax = (int)ceil(column + outRadius);

/* make sure we are inside the region */
  if (rmin < 0 || 
      rmax > regPtr->nrow-1 ||
      cmin < 0 ||
      cmax > regPtr->ncol-1) returnValue = 1;

  if (rmin < 0) rmin = 0;
  if (rmax > regPtr->nrow-1) rmax = regPtr->nrow-1;
  if (cmin < 0) cmin = 0;
  if (cmax > regPtr->ncol-1) cmax = regPtr->ncol-1;

/* Malloc memory for the values array */
  if (inRadius == 0) {
    max_values = (int) ceil(3.15*(outRadius+0.5)*(outRadius+0.5));
  } else {
    max_values = (int) ceil(
      3.15 * ( (outRadius+.5)*(outRadius+0.5) - (inRadius-.5)*(inRadius-.5) )
    );
  }
  values = (double *)shMalloc(max_values*sizeof(double));

/* loop over rows and columns */
  for (r=rmin; r<=rmax; r++) {
    for (c=cmin; c<=cmax; c++) {
      delr2 = ((double)r-row+0.5)*((double)r-row+0.5);
      delc2 = ((double)c-column+0.5)*((double)c-column+0.5);
      distance = delr2+delc2;
      if ( (distance < outRadius2) && (distance >= inRadius2) ) {
        /* Fill the pixel array */
        if (*pixels >= max_values) {
          printf ("Overwriting memory in atMeanFindInAperture");
          returnValue = 2;
        }
        values[(int)(*pixels)] = shRegPixGetAsDbl(regPtr, r, c);
        (*pixels)++;
      }
    }
  }

  if (*pixels>1) {
    if (nsigmaclip < 0) {
      *mean = at_find_median(values, (int)*pixels);
    } else if (nsigmaclip == 0) {
      find_avgdev(values, (int)*pixels, mean, avgError);
    } else {
      find_clippedmean(values, (int)*pixels, iter, nsigmaclip, 
	avgError, mean, &used_pixels);
    }
  } else {
    if (*pixels==1) *mean = values[0];
  }

  if (nsigmaclip > 0) *pixels = (double)used_pixels;

  /* Free the values array */
  shFree(values);

  return returnValue;
}

double at_find_median(double numbers[], int number_of_numbers) {
  /* declare variables */
  int half;
  double median, median2, oddeven;
  
  /* find the median */
  /* -1 due to c numbering of arrays */
  half = (number_of_numbers-1)/2;
  at_select_amongst(numbers, number_of_numbers, half);
  median = numbers[half];
  
  /* check about the even problem */
  oddeven = number_of_numbers;
  if (oddeven/2. == number_of_numbers/2)  /* then it is even */
    { at_select_amongst(numbers, number_of_numbers, half+1);
    median2 = numbers[half+1];
    median = (median + median2)/2;
    }
  
  /* return answer */
  
  return median;
}

void at_select_amongst(double numbers[], int number_of_numbers, int kth_place) {
  /* declare variables */
  int lower, upper; 
  int i, j;			/* lower and upper counters */
  double current;                 /* a temporary variable */
  
  /* do the work */
  lower = 0;                      /* lower bound   */
  upper = number_of_numbers-1;      /* upper bound   */
  
  while (upper > lower)           /* do till bounds cross at center */
    {
      current = numbers[upper]; 
      i = lower - 1; j = upper;
      for (;;)                /* do for ever   */
	{
	  while (numbers[++i] < current) {}; /* work up the array */
	  j--;    		/* work down the array, stopping at 0 */
	  while (numbers[j] > current && j>0 ) {j--;}
	  if (i >= j) break;       /* when pointers cross, bail out */
	  at_swap(numbers, i, j);     /* other wise, modify array */
	}
      at_swap(numbers, i, upper);               /* finish swap */
      if (i >= kth_place) upper = i-1;       /* change boundries */
      if (i <= kth_place) lower = i+1;
    }
}


/* same as above, but for U16 */
U16 at_find_medianU16(U16 numbers[], int number_of_numbers) {
  /* declare variables */
  int half;
  U16 median;
  
  /* find the median */
  /* -1 due to c numbering of arrays */
  half = (number_of_numbers-1)/2;
  at_select_amongstU16(numbers, number_of_numbers, half);
  median = numbers[half];
  
  /* return answer */
  
  return median;
}

void at_select_amongstU16(U16 numbers[], int number_of_numbers, int kth_place) {
  /* declare variables */
  int lower, upper; 
  int i, j;			/* lower and upper counters */
  U16 current;                 /* a temporary variable */
  
  /* do the work */
  lower = 0;                      /* lower bound   */
  upper = number_of_numbers-1;      /* upper bound   */
  
  while (upper > lower)           /* do till bounds cross at center */
    {
      current = numbers[upper]; 
      i = lower - 1; j = upper;
      for (;;)                /* do for ever   */
	{
	  while (numbers[++i] < current) {}; /* work up the array */
	  j--;    		/* work down the array, stopping at 0 */
	  while (numbers[j] > current && j>0 ) {j--;}
	  if (i >= j) break;       /* when pointers cross, bail out */
	  at_swapU16(numbers, i, j);     /* other wise, modify array */
	}
      at_swapU16(numbers, i, upper);               /* finish swap */
      if (i >= kth_place) upper = i-1;       /* change boundries */
      if (i <= kth_place) lower = i+1;
    }
}


void find_avgdev( double numbers[], int number_of_numbers, 
                         double *Fmean, double *Favgdev ) {
  /* declare variables */
  double mean, avgdev;
  int i;
  
  /* be paranoid about integer to float conversion   */
  double N = number_of_numbers;
  
  /* calculate the mean   */
  mean =0;
  for (i=0; i< number_of_numbers; i++) {mean += numbers[i];}
  mean = mean / N ;
  
  /* calculate the avg dev   */
  avgdev =0 ;
  for (i=0; i< number_of_numbers; i++) {avgdev += fabs(numbers[i]-mean) ;}
  avgdev = avgdev / N ;
  
  /* return the answer   */
  *Fmean = mean;
  *Favgdev = avgdev;
  return;
}

void find_clippedmean( double numbers[], int number_of_numbers, 
		       int number_of_iterations, double sigma_clip,
		       double *sigma_avgdev, double *sigma_mean, 
		       int *number_used ) {
  double avgdev=0;
  double mean=0;
  double clip;
  double *new_numbers;
  int new_counter, new_loader, new_number_of_numbers, i, j;
  
  new_numbers = (double *) shMalloc(number_of_numbers * sizeof(double) );
  new_number_of_numbers = number_of_numbers;
  for (j=0; j < number_of_numbers; j++) {
    new_numbers[j] = numbers[j];
  }
  *sigma_avgdev = 0;
  *sigma_mean = 0;
  *number_used = 0;
  
  /* do the iteration */
  for (i=1; i<=number_of_iterations; i++) {
    
    /* calculate the mean and avgdev, noting sigma clipping point */
    find_avgdev(new_numbers, new_number_of_numbers, &mean, &avgdev);
    clip = sigma_clip * avgdev;
    /* find those points inside the sigma clip and store them */
    new_loader  =0;
    for (new_counter=0; new_counter < number_of_numbers; new_counter++) {
      if (fabs(numbers[new_counter]-mean) <= clip)  {
	new_numbers[new_loader] = numbers[new_counter];
	new_loader++;
      } 
    }
    
    /* go back for another iteration */
    new_number_of_numbers = new_loader;
  }
  
  /* calculate the mean and avgdev, noting sigma clipping point */
  find_avgdev(new_numbers, new_number_of_numbers, &mean, &avgdev);
  
  /*  return the answer   */
  shFree (new_numbers);
  
  *sigma_mean = mean;
  *sigma_avgdev = avgdev;
  *number_used = new_number_of_numbers;
  
}


