#if !defined(PHCORRECTFRAMES_H)
#define PHCORRECTFRAMES_H

#include "region.h"
#include "phFramestat.h"
#include "phCcdpars.h"
#include "phQuartiles.h"
#include "phCalib1.h"

void phInitCorrectFrames(void);

void phFiniCorrectFrames(void);

int phCorrectFrames (REGION *raw_data,
		     const REGION *bias_data,
		     const REGION *flat_data,
		     const CCDPARS *ccdpars,
		     FRAMEPARAMS *fparams,
		     int bias_scale,
		     int leftbias,
		     int rightbias,
		     int stamp_flag,
		     REGION *output_data,
		     int minval,
		     int fix_bias);

int phFlatfieldsInterp(const REGION *flat2d,
		       REGION *flat_vec);

int phSkyBiasInterp(const REGION *sky_vec,
		       const REGION *drift_vec,
		       const REGION *Odrift_vec,
		       const REGION *Edrift_vec,
		       int rowid,
		       int nadj,
		       float sigrej,
		       int mode,
		       const CCDPARS *ccdpars,
		       CALIB1 *calib);

int phRegGetOddEven(const REGION *reg,
		int row1,	
		int row2,	 
                float *oddMed,    
                float *oddMedErr,  
                float *evenMed,   
                float *evenMedErr);

int phFlatfieldsCalc(QUARTILES *quartiles,
                  REGION *bias,
		  int bias_scale,
                  int left_buffer,
                  int right_buffer,
                  CCDPARS *ccdpars,
                  REGION *flat2d,
                  REGION *skylev,
                  REGION *drift,
                  REGION *Odrift,
                  REGION *Edrift);

int
phPixelInterpolate(int rowc, int colc,	/* the pixel in question */
		   const REGION *reg,	/* in this region */
		   const CHAIN *badmask, /* pixels to ignore */
		   int horizontal,	/* interpolate horizontally */
		   int minval);		/* minimum acceptable pixel value */

#endif

