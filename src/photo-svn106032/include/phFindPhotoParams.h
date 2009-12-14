#if !defined(PHFINDPHOTOPARAMS_H)
#define PHFINDPHOTOPARAMS_H

#include "region.h"
#include "shChain.h"
#include "phCalib1.h"
#include "phExtinction.h"
#include "phFrameinfo.h"
#include "phStar1starcmerge.h"


#define PATCHSEP 60.		/* Mimimum separation in seconds between  
                                 * STARCs in a calibration patch. (1 min.)
				 */
#define REFCOLOR 0.0            /* CALIB should give flux at 0 atm of 
				 * 20th mag star with this color
				 */
#define STARSEP 10		/* Maximum allowable distance between postage
				 * stamp star and matched MT star */
#define POLY      1             /* order of polynomial used by smaDivdif */
                                /*   to interpolate for extinction or    */
                                /*   photometric zero point.             */


#define FPP_ERRLEVEL 2		/* verbose level for shDebug messages  */
#define FPP_ERRLEVEL2 1		/* verbose level for shDebug messages  */

#define MAX_FILTERS 5		/* Maximum number of filters allowed */
#define BADVALUE -100		/* error value indicating bad measurement  */

RET_CODE 
phFindPhotoParams (int ncolors,	        /* number of colors (& lists of pstamps) */
 		  char *filters[],      /* Array of filternames */
                  char *ref_filters[],  /* array of ref filternames */
		  float *colorterms,    /* array of color terms for calibration */
		  float *colortermErrs, /* array of errors for above */
		  CHAIN *pstamps[],     /* 1 list of STAR1s per color */
		  CHAIN *standards,     /* list of STARCs */
		  CHAIN *extinction,    /* list of EXTINCTIONs */
		  CHAIN *frames,        /* list of FRAMEINFOs (time, z) */
		  CALIB1BYTIME *oldcal, /* last good CALIB1BYTIME */
                  float starsep,        /* max dist (arcsec) for matching stars */
                  float sys_ph_err,     /* systematic photometric error */
	          float maxErr,         /* max. error in any mag. allowed for MT stars */
                  int n_max,            /* max polyn. for flux20(t) fit */
                  float *flux20_m,      /* slope for flux20(t) */
                  float *flux20_c,      /* intercept for flux20(t) */
                  float *flux20_chi2,   /* chi2 for the fit flux20(t) */
		  CHAIN *patchcal,      /* output list of CALIB1BYTIMEs */
		  CHAIN *framecal,      /* output list of CALIB1BYFRAMEs */
		  int *patchcal_flags,  /* flags for patchcal filters */
		  int *framecal_flags); /* flags for framecal filters */
#endif
