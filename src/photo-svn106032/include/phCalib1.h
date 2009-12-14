#if !defined(PHCALIB1_H)
#define PHCALIB1_H
#include "phFilter.h"
#include "atTrans.h"
#include "phDgpsf.h"
#include "phConsts.h"
#include "phExtract.h"

typedef enum {
   PSP_FIELD_UNKNOWN = -1,	/* this should never happen */
   PSP_FIELD_OK = 0,		/* everything OK */ 
   PSP_FIELD_PSF22 = 1,		/* forced to take linear PSF across field */
   PSP_FIELD_PSF11 = 2,		/* forced to take constant PSF across field */
   PSP_FIELD_NOPSF = 3,		/* forced to take default PSF */
   PSP_FIELD_ABORTED = 4,	/* aborted processing */
   PSP_FIELD_MISSING = 5,	/* missing (dummy) field */
   PSP_FIELD_OE_TRANSIENT = 6,	/* field with odd/even bias level transient */
   PSP_FIELD_STATUS_MASK = 31,	/* Mask defining which bits are used
				   for status values; higher bits are available
				   to be set with extra information */
   PSP_FIELD_EXTENDED_KL = 0x20,/* Window for KL stars was extended */
   PSP_FIELD_SPARSE = 0x40,     /* field is sparsely populated with KL stars */
   PSP_FIELD_MAX = 0x40000000	/* Maximum possible bit value in a +ve int */
} PSP_STATUS;

/*
 * This struct will hold the calibration for a frame in a single colour
 *
 * N.b. skyerr approx skysig/sqrt(n), where n is the number of pixels used
 * to determine sky
 */
typedef struct {
  char filter[FILTER_MAXNAME];
  PSP_STATUS status;		        /* status of reduction */
  float skyraw;				/* sky level - raw values */
  float sky;				/* sky level - smoothed values */
  float skyerr;				/* s.d. of sky level */
  float skysig;				/* s.d. of sky value in a pixel */
  float skyslope;
  float lbias;
  float lbiasOEdiff;
  float rbias;
  float rbiasOEdiff;
  float flux20;
  float flux20Err;
  DGPSF *psf;                           /* the PSF fit parameters */
  COMP_PROFILE *prof;                   /* the PSF profile */
  int psf_nstar;			/* number of stars added into PSF */
  float psf_ap_correctionBias;		/* bias in psfCounts due to
					   PSF mismatch */
  float psf_ap_correctionErr;           /* error in psfCounts due to
					   PSF mismatch */
  float psf_wingcorr;                   /* flux ratio (F(nann_ap_run)/F(nann_ap_frame) */
  int nann_ap_frame;                    /* number of annuli in the aperture
                                           magnitude used to allow for PSF
                                           variations within a frame */
  int nann_ap_run;                      /* number of annuli in the aperture
                                           magnitude used to allow for PSF
                                           variations during an entire run */
  float ap_corr_run;                    /* the multiplicative correction needed
                                           to correct the flux within
                                           nann_aperture_corr_frame to the flux
                                           within nann_aperture_corr_run */
  float ap_corr_runErr;                 /* error for ap_corr_run */
  float node;            /* long. of ascending node of great circle */
  float incl;            /* inclination of great circle */
  TRANS *toGCC;          /* transforms (row,col) to (mu,nu) of great circle */
  float gain;            /* mean gain of amplifiers, e/DN */
  float dark_variance;   /* mean pixel variance due to dark current; DN^2 */
} CALIB1;


/* This struct will hold the calibration for one frame in all colors */
typedef struct CALIB1BYFRAME {
  int field;				/* which field? */
  int ncolors;				/* how many colours? */
  PSP_STATUS psp_status;		/* status of reduction */
  CALIB1 **calib;			/* calib for each colour */
} CALIB1BYFRAME;			/* pragma SCHEMA */

/* This struct will hold the in all colors for a given time */
typedef struct CALIB1BYTIME {
  double mjd;
  int nearest_frame;
  int MTpatch_number;  /* shows the order of this patch in MT list */
  int ncolors;
  CALIB1 **calib;
} CALIB1BYTIME; /* pragma SCHEMA */


/* Here are the input/output structures for fitsio */

typedef struct {
    const int ncolor;
    int field;
    double mjd;
    PSP_STATUS psp_status;
    float node;
    float incl;
    PSP_STATUS status[NCOLOR];
    float sky[NCOLOR];
    float skysig[NCOLOR];
    float skyerr[NCOLOR];
    float skyslope[NCOLOR];
    float lbias[NCOLOR];
    float rbias[NCOLOR];
    float flux20[NCOLOR];
    float flux20Err[NCOLOR];
    int psf_nstar[NCOLOR];
    float psf_ap_correctionErr[NCOLOR];
    int nann_ap_frame[NCOLOR]; 
    int nann_ap_run[NCOLOR]; 
    float ap_corr_run[NCOLOR]; 
    float ap_corr_runErr[NCOLOR]; 
    float psf_sigma1[NCOLOR];
    float psf_sigma2[NCOLOR];
    float psf_sigmax1[NCOLOR];
    float psf_sigmax2[NCOLOR];
    float psf_sigmay1[NCOLOR];
    float psf_sigmay2[NCOLOR]; 
    float psf_b[NCOLOR];
    float psf_p0[NCOLOR];
    float psf_beta[NCOLOR];
    float psf_sigmap[NCOLOR];
    float psf_width[NCOLOR];
    float psf_psfCounts[NCOLOR];
    float psf_sigma1_2G[NCOLOR];
    float psf_sigma2_2G[NCOLOR]; 
    float psf_b_2G[NCOLOR];
    float psfCounts[NCOLOR];		 
    int prof_nprof[NCOLOR];			 
    float prof_mean[NCOLOR][NANN];		 
    float prof_med[NCOLOR][NANN];		 
    float prof_sig[NCOLOR][NANN];			 
    float gain[NCOLOR];
    float dark_variance[NCOLOR];
} CALIB_IO;                  /* pragma SCHEMA */

/*****************************************************************************/

CALIB1 * phCalib1New(const char filtername);
void phCalib1Del(CALIB1 *calib1);

CALIB1BYFRAME * phCalib1byframeNew(const char *colors);
void phCalib1byframeDel(CALIB1BYFRAME *calib1byframe);

CALIB1BYTIME * phCalib1bytimeNew(const char *colors);
void phCalib1bytimeDel(CALIB1BYTIME *calib1bytime);

CALIB_IO *phCalibIoNew(int ncolor);
void phCalibIoDel(CALIB_IO *calibio);
CALIB_IO *phCalibIoNewFromCalib1byframe(const CALIB1BYFRAME *cframe);
CALIB1BYFRAME *phCalib1byframeNewFromCalibIo(const CALIB_IO *calibio, 
					     const char *filters,
					     const int *psindex);
CALIB_IO *phCalibIoNewFromCalib1bytime(const CALIB1BYTIME *ctime);
CALIB1BYTIME *phCalib1bytimeNewFromCalibIo(const CALIB_IO *calibio, 
					   const char *filters,
					   const int *psindex);
#endif
