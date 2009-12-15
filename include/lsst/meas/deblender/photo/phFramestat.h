#if !defined(PHFRAMESTAT_H)
#define PHFRAMESTAT_H

/*
#include "phFilter.h"
#include "phSpanUtil.h"
 */
#include "dervish.h"
#include "atTrans.h"
#include "phDgpsf.h"
#include "phConsts.h"
#include "phSkyUtils.h"
#include "phVariablePsf.h"



/*
 * values for status:
 */
#define FIELD_UNKNOWN -1		/* we don't know yet
					   pragma typedef { */
#define FIELD_OK 0			/* All went well */
#define FIELD_ABORTED 1			/* field was aborted */
#define FIELD_MISSING 2			/* field was not reduced */
#define FIELD_TOO_LONG 3		/* field took too long */
#define FIELD_TOO_MANY_BRIGHT_OBJECTS 4 /* field has too many brightobjs to try to reduce */
#define FIELD_SKIPPED 5			/* field was deliberately skipped */
#define FIELD_TOO_LARGE 6		/* field took too much memory
					   pragma } FIELD_FLAGS */

/*
 * This struct will accumulate the status for one ncolor field in the
 * frames pipeline.
 */
typedef struct {
  int status;				/* was this a good run? */

  float rowOffset[NCOLOR], colOffset[NCOLOR]; /* corrections to the TRANS */

  int nobjects;				/* number of objects detected */
  int nchild;				/* number of children */
  int nstars;				/* number of stars */
  int ngals;				/* number of galaxies */

  int saturation_level[NCOLOR];		/* saturation level for frames */

  float neff_psf[NCOLOR];		/* psf's effective area */
  float sky_psp[NCOLOR];		/* sky from PSP */
  float sky_frames[NCOLOR];		/* global sky from frames */
  float sky_frames_sub[NCOLOR];		/* global sky from frames after
					   object subtraction */
  float sigpix[NCOLOR];

  float deV_ap_correction[NCOLOR];	/* deV prof. aperture corr. for stars*/
  float deV_ap_correctionErr[NCOLOR];	/* error in deV_ap_correction */
  float exp_ap_correction[NCOLOR];	/* exp prof. aperture corr. for stars*/
  float exp_ap_correctionErr[NCOLOR];	/* error in exp_ap_correction */
  float deV_model_ap_correction[NCOLOR]; /* stars' deV model aperture corr. */
  float deV_model_ap_correctionErr[NCOLOR]; /* err in deV_model_ap_correction*/
  float exp_model_ap_correction[NCOLOR]; /* stars' exp model aperture corr. */
  float exp_model_ap_correctionErr[NCOLOR]; /* err in exp_model_ap_correction*/

  int nCR[NCOLOR];			/* number of cosmic rays */
  int nbrightobj[NCOLOR];		/* number of bright objects */
  int nfaintobj[NCOLOR];		/* number of faint objects */

  float median_fiberColor[NCOLOR];	/* median fiber colors of stars */
  float median_psfColor[NCOLOR];	/* median PSF colors of stars */
  float Q[NCOLOR], U[NCOLOR];		/* estimated Q, U for stars */

  float adopted_cond3_fac2[NCOLOR];	/* adopted value of CR rejection
					   parameter cond3_fac2 */
  int flags;				/* Union of OBJC.flags for all objects */
  int flags2;				/* Union of OBJC.flags2 for all objects */
  unsigned int objc_flags[NCOLOR];      /* OR of all objc .flags */
  unsigned int objc_flags2[NCOLOR];     /* OR of all objc .flags2 */
} FIELDSTAT;				/* pragma SCHEMA */

/*****************************************************************************/
/*
 * structs giving information about field/frame parameters. Note that
 * sb_mag and sb_counts refer to the same isophote, that at which an
 * elliptical isophote is to be measured (e.g. OBJECT1.iso_rowc)
 */
#define NCLASSIFIER_FIDDLE 3		/* number of fiddle factors for
					   Star/Galaxy classifier */

typedef struct {
   const int filter;			/* name of filter, e.g. r or i; or 0 */
   float dframe;			/* no. of frames away from r filter */
   float flux20;			/* counts in a 20th mag object */
   float sb_mag;			/* surface brightness mag/sq.arcsec */
   float sb_counts;			/* surface brightness (counts/pix^2) */
   const struct dgpsf *psf;		/* PSF for frame */
   const COMP_PROFILE *cprof;		/* composite profile */
   const REGION *data;			/* data being reduced */
   int data_is_smoothed;		/* is the data smoothed? */
   OBJMASK *badmask;			/* mask of bad pixels for frame */
   float global_sky;			/* the single global estimate of sky */
   const BINREGION *sky;		/* values of sky */
   const BINREGION *skyErr;		/* sky sigmas */
   float gc_node;			/* node of Great Circle for frame */
   float gc_incl;			/* incl of Great Circle for frame */
   const TRANS *toGCC;			/* (row,col) -> Great Circle (mu,nu) */
   float psf_app_correction;		/* multiplicative aperture correction
					   for psfCounts */
   float psf_app_correctionErr;		/* error in psf_app_correction */
   float deV_ap_correction;		/* multiplicative aperture correction
					   for deV_counts */
   float deV_ap_correctionErr;		/* error in deV_ap_correction */
   float exp_ap_correction;		/* multiplicative aperture correction
					   for exp_counts */
   float exp_ap_correctionErr;		/* error in exp_ap_correction */
   float deV_model_ap_correction;	/* multiplicative aperture correction
					   for model_counts if deV is best */
   float deV_model_ap_correctionErr;	/* error in deV_model_ap_correction */
   float exp_model_ap_correction;	/* multiplicative aperture correction
					   for model_counts if exp is best */
   float exp_model_ap_correctionErr;	/* error in exp_model_ap_correction */
   const PSF_BASIS *psfBasis;		/* describe PSF variations */
   BINREGION *gain;                     /* mean gain of amplifiers, e/DN */
   float gain0, gain1;			/* gains of left/right amplifiers */
   BINREGION *dark_variance;            /* mean pixel variance due to
					   dark current; DN^2 */
   float bkgd;				/* unsubtracted background of frame,
					   DN (in excess of SOFT_BIAS) */
   int fullWell[2];			/* adopted saturation levels for left
					   and right half of frame */
   int extra_bad_satur_columns;		/* treat neighbouring columns
					   as saturated too? */
   float electronic_ghost_amplitude;	/* amplitude of "electronics ghosts" */
   float smooth_sigma;			/* sigma of Gaussian used in 
					   find faint objects */
   float ffo_threshold;			/* threshold for find faint objects */
   float peak_min_sep;			/* minimum separation to merge PEAKs */
   int peak_smooth_thresh;		/* minimum peak value to smooth object
					   when centroiding */
   int min_peak_statistical_errors;	/* minimum value of peak pixel to quote
					   statistical positional errors */
   int npeak_per_obj_max;		/* max. number of peaks in an object */
   float astrom_floor;			/* minimum 1-d astrometric error;
					   added in quadrature */
   int nann_ap_frame;			/* number of annuli in the aperture
 					   magnitude used to allow for PSF
 					   variations within a frame */
   int nann_ap_run;			/* number of annuli in the aperture
					   magnitude used to allow for PSF
 					   variations during an entire run */
   float ap_corr_run;			/* correct flux within nann_ap_frame
					   to that within nann_ap_run;
					   includes contribution from imperfect
					   KL-PSF model */
   float ap_corr_runErr;		/* Error in psfCounts due to imperfect
					   ap_corr_run */
   float fiberCounts_seeing;		/* seeing to use when calculating
					   fiberCounts */
   int namp;				/* number of amps in use */
   int rowBinning;			/* how much is data binned by row? */
   int colBinning;			/* how much is data binned by column?*/
   float astrom_dcol_left;		/* add this to colc on LEFT of chip */
   float astrom_dcol_right;		/* add this to colc on RIGHT of chip */
} FRAMEPARAMS;				/* pragma SCHEMA */
/*
 * Methods on FRAMEPARAMS
 */
float
phGain(const FRAMEPARAMS *fparams,	/* the source of information for this frame */
       const int row,			/* row and */
       const int col);			/*          column position within the chip; coordinates exclude overscan */

float
phDarkVariance(const FRAMEPARAMS *fparams, /* the source of information for this frame */
       const int row,			/* row and */
       const int col);			/*          column position within the chip; coordinates exclude overscan */

typedef struct {
   int run;				/* current run */
   int camCol;				/* current camCol */
   int fieldnum;			/* current field number */
/*
 * These are constants for a run of photo
 */
   int ncolor;				/* number of colours */
   const char filters[5];		/* names of filters */
   int ncol;				/* col dimension of a (trimmed) frame*/

   int scan_overlap;			/* frame overlap in scan direction */
   int run_overlap;			/* frame overlap in run direction */
   float pixscale;			/* pixel scale; arcsec/pixel */
   int binfac;				/* ammount to bin saved binned image */
   int smooth_profs;			/* smooth profiles? */
   int median_profs;			/* use median not mean profiles? */
   int smooth_petro_radii;		/* smooth when finding r_P? */
   int petro_gcv_errors;		/* find R_{Petro} errors by GCV? */
   float petro_f1;			/* ratio of surf. bright. in annulus to
					   that in circle out to Petro radius*/
   float petro_f2;			/* minimum acceptable surface
					   brightness at Petrosian radius */
   float petro_f4;			/* number of Petrosian radii that
					   enclose the Petrosian flux */
   float petro_f5;			/* `Petrosian' radius to use if
					   isophotal radius doesn't exist */
   float fiber_rad;			/* radius of spectro. fibre */
   int ref_band_index;			/* filter index of reference filter */
   int canonical_band_index;		/* filter index of canonical filter
					   for this object; usually this will
					   equal the ref_band_index */
   int nchild_max;			/* maximum number of children to
					   run deblender */
   int classify_bands[NCOLOR];		/* bands to use for OBJC classifier */
   float deblend_min_peak_spacing;	/* minimum spacing of peaks to treat
					   both as children */
   int deblend_min_detect;		/* minimum number of detections to
					   create a child */
   float deblend_psf_Lmin;		/* minimum L to be considered a star */
   int deblend_psf_nann;		/* number of annuli for PSF fit,
					   in width = 1 pixel seeing */
   int deblend_psf_rad_max;		/* radius to subtract PSF to
					   in width = 1 pixel seeing */
   float deblend_allowed_unassigned;	/* maximum fraction of parent's petro
					   flux which can be unaccounted for */
   int deblend_npix_max;		/* maximum number of pixels in a parent to deblend
					   (0 => unlimited) */
   int astrom_tweak_n_min;		/* minimum number of good astrom
					   matches to tweak drow/dcol */
   float sg_classifier[NCLASSIFIER_FIDDLE]; /* star/galaxy fiddle factors */
   int use_galaxy_ap_correction;	/* apply a galaxy aperture corr? */
   float deblend_inner_max;		/* max allowable inner product between
					   deblender templates */
   int psf_constraint_nmodel;		/* constrain PSFs to a range of
					   psf_constraint_nmodel models */
   int deblend_bright_objects;		/* deblend BRIGHT objects */
/*
 * The following can be different for each frame
 */
   FRAMEPARAMS *frame;
} FIELDPARAMS;

/*****************************************************************************/
/*
 * con/destructors for (FIELD|FRAME)(PARAMS|STAT)
 */
FIELDPARAMS *phFieldparamsNew(const char *filters);
void phFieldparamsDel(FIELDPARAMS *fparams);
FIELDSTAT * phFieldstatNew(void);
void phFieldstatDel(FIELDSTAT *mag);
FRAMEPARAMS *phFrameparamsNew(char filter);
void phFrameparamsDel(FRAMEPARAMS *fparams);
void phFrameparamsEmpty(FRAMEPARAMS *fparams);

void phFieldstatClear(FIELDSTAT *fstat);

#endif

