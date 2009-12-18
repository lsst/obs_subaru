#ifndef ATAPERTURE_H
#define ATAPERTURE_H

#ifdef __cplusplus
extern "C" {
#endif
#include "dervish.h"

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
			    );
  
double at_find_median(double numbers[], int number_of_numbers);

U16 at_find_medianU16(U16 numbers[], int number_of_numbers);


#ifdef __cplusplus
}
#endif

#endif










