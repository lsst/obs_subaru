#if !defined(PHSTAR1_H)
#define PHSTAR1_H

#include "phDgpsf.h"
#include "phCalib1.h"
#include "phCcdpars.h"
#include "phObjc.h"

#define STAR1_ERRLEVEL 2
#define MAXCHISQ 5.0

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/*
 * Flags for star1s
 *
 * Note that the STAR1_CLIP_... bits are not set; rather an auxiliary element
 * STAR1.badstar is used; see comment above clip_stars in findPsf.c.
 */
typedef enum {
   STAR1_NOFLAGS =         0x1,		/* no flags are set */
   STAR1_SATURATED =       0x2,		/* star is saturated */
   STAR1_CR =              0x4,		/* cosmic ray */
   STAR1_BADPSF =          0x8,		/* psf is deviant */
   STAR1_NOPROFILE =      0x10,		/* too small to estimate a profile */
   STAR1_NOAPMAG =        0x20,		/* profile too small for apmag */
   STAR1_BADCENTER =      0x40,		/* couldn't find center of star1 */
   STAR1_NOSTOKES =       0x80,		/* couldn't calculate U/Q */
   STAR1_BADWIDTH =      0x100,		/* too wide */
   STAR1_ASYMMETRIC =    0x200,		/* found to be asymmetric on rotation*/
   STAR1_MULTIPLE =      0x400,		/* multiple peaks (stringent limit)*/
   STAR1_BADSKY =        0x800,		/* bad sky level */
   STAR1_OFFCHIP =      0x1000,		/* part of object is off the chip */
   STAR1_FAKEWING =     0x2000,		/* fake wing star */
   STAR1_BADMULTIPLE =  0x4000,		/* multiple peaks (weak limit)*/
   STAR1_CLIP_UQ =      0x8000,		/* clipped on U/Q */
   STAR1_CLIP_SIGMA1 = 0x10000,		/* clipped on sigma1 */
   STAR1_CLIP_SIGMA2 = 0x20000,		/* clipped on sigma2 */
   STAR1_CLIP_B =      0x40000,		/* clipped on b */
   STAR1_CLIP_CELLS =  0x80000		/* clipped on cell profile */
} STAR1_FLAGS;

/*
 * characterise an object's asymmetry
 */
typedef struct {
   int amp_resid;			/* max amplitude of residuals */
   float rms_resid;			/* rms residual */
   REGION *star;			/* the original, sinc shifted, object*/
   REGION *residual;			/* the residuals */
} SYMDIAG;

typedef struct {
   char filter[FILTER_MAXNAME]; /* this info is for this filter */
   int frame;                   /* corresponding frame */
   int firststar;		/* first star for this frame, corresponding to starflags[0] */
   int size;			/* number of starflags allocated, should be >= nstars*/
   int *starflags;	        /* flags showing if a star is included */
                                /* in calculation of this frame's PSF */
   int Nallstars;               /* # of potentially usable stamps for this frame */
   int Nframestars;             /* # of potentially usable stamps for this frame FROM this frame */
   int NgoodKLbasis;		/* number of stars used to determine PSF KL basis, directly stored */
   int NgoodKLcoeffs;           /* number of stars used to determine KL coeffs, directly stored */
   int NgoodBasis;		/* number of stars used to determine PSF KL basis, computed from flags */
   int NgoodCoeffs;             /* number of stars used to determine KL coeffs, computed from flags */
   int Nphoto;                  /* number of stars used to compute photometric bias and errors */
   int NbadSSb;                 /* stars with bad sigma1,2 and b, flags are 1-7 */
   int NbadCell;                /* number of stars which survived cell clipping */ 
   int FbadCell;                /* flag for stars rejected in cell clipping */
   int NbadQU;                  /* number of stars which survived QU clipping */ 
   int FbadQU;                  /* flag for stars rejected in QU clipping */
   float PSFwidth;              /* effective seeing determined from cprof_bar */
   int Nprof;                   /* number of points in the composite profile */
   float sig1med;               /* median of sigma1s */
   float sig1sig;               /* sigma for sigma1s */
   float sig2med;               /* median of sigma2s */
   float sig2sig;               /* sigma for sigma2s */
   float bmed;                  /* median of b */
   float bsig;                  /* sigma for b */
   float Qmedian;               /* median for Qs */
   float Qsigma;                /* sigma for Qs */
   float Umedian;               /* median for Us */
   float Usigma;                /* sigma for Us */
   float dQUmed;                /* median for distance in QU plane */
   float dQUsig;                /* sigma for distance in QU plane */
   float RMSmed;                /* median for RMS size */
   float RMSsig;                /* sigma for RMS size */
   float sig1PSF;               /* PSF sigma1 for this frame */
   float sig2PSF;               /* PSF sigma2 for this frame */
   float ptarr[NANN];           /* composite profile */
   float pt1;                   /* a characteristic point from comp_profile (debugging) */
   float pt2;                   /* a characteristic point from comp_profile (debugging) */
   float pt3;                   /* a characteristic point from comp_profile (debugging) */
   float chisq_max;             /* requested chi2, needed in plot_pspstats */
   float ap_corr;               /* aperture correction adopted for the frame center */
   float apCorr_max;            /* maximum aperture correction anywhere on this frame */
   float apCorr_min;            /* minimum aperture correction anywhere on this frame */
   float apCrat_med;            /* median value of the ratio (in mags) of aperture correction
                                   determined from reconstructed PSF at a star's position and the
                                   aperture correction determined directly from that star's region */
   float wingcorr;              /* median value of the ratio (F(nann_ap_run)/F(nann_ap_frame) */
   float apCrat_q25;            /* same as above but the first quartile */      
   float apCrat_q75;            /* same as above but the third quartile */    
   float apCrat_rms;            /* SIQR*(q75-q25) */   
   float width_med;             /* median value of the effective PSF width (in arcsec) */
   float width_q25;             /* same as above but the first quartile  */
   float width_q75;             /* same as above but the third quartile  */
   float patch_flux20;          /* flux20 if overlaping with MT patch, -1 otherwise */
} FRAME_INFORMATION;			/* pragma SCHEMA */

/* This struct will hold the magnitude in one color for an object */
typedef struct {
  const int id;
  int starid;
  int frame;
  double ra;
  double raErr;
  double dec;
  double decErr;
  int aperture;		     /* Index of profile radius of edge of aperture */
  float apCounts;            /* aperture counts determined directly for this star */
  float apCountsErr;         /* and associated error */
  float apCounts1;           /* aux */
  float apCounts2;           /* aux */
  float apC1area;            /* aux */      
  float apC2area;            /* aux */
  float wingcorr;            /* aux=apCounts2/apCounts1 */
  float psfCounts;           /* psfCounts from star's syncreg (phPsfCountsFromRegion) */
  float psfCountsErr;        /* psfCountsErr from star's syncreg (phPsfCountsFromRegion) */
  float count_rat;           /* = -2.5*log10(apCounts/psfCounts) */
  float count_ratErr;        /* error in count_rat */
  float apCorr_PSF;          /* ap. corr. from phPsfSetAtPoint at the position of this star */
  float psfCounts_PSF;       /* psfCounts determined from PSF reconstructed at star's position */
  float apCounts_PSF;        /* apCounts determined from PSF reconstructed at star's position */
  float Eff_width;           /* effective width for this star */
  float PSFwidth;            /* effective width determined from PSF reconstructed at star's position */
  int PSFflag;               /* flag showing PSF status of this star on its own frame */
                             /* if 1 this star was used for QA on KL stuff */
  int badstar;               /* flag showing status in the list for the current frame */
  int used4coeffs;           /* flag showing whether this star was used for determining
                                KL coefficients on the current frame */
  int used4photo;            /* flag showing whether this star was used for determining
                                KL coefficients on the current frame */

  float Q, U;	             /* Stokes parameters */
  float M_rms;				/* RMS size from adaptive moments */
  DGPSF *dgpsf;
  OBJECT1 *object;
  CELL_PROF *cprof;	     /* cell array profile */
  SYMDIAG *sym;		     /* describe object's (a)symmetry */
  STAR1_FLAGS flags;
  REGION *syncreg;	     /* REGION containing sync shifted region */
} STAR1;


/* This structure is a stripped down version of STAR1, designed to carry calibration info */
typedef struct {
  const int id;
  int starid;
  int frame;
  double ra;
  double dec;
  int aperture;		     /* Index of profile radius of edge of aperture */
  float apCounts;
  float apCountsErr;
  STAR1_FLAGS flags;
} STAR1PC;


STAR1 *phStar1New(void);
void phStar1Del(STAR1 *star1);
void phStar1Relax(STAR1 *star1);

STAR1PC *phStar1pcNew(void);
void phStar1pcDel(STAR1PC *star1);

FRAME_INFORMATION *phFrameInformationNew(int size);
void phFrameInformationDel(FRAME_INFORMATION *frame_info);

STAR1 *
phStar1MakeFromReg(REGION *reg, int skyboxsize, 
		   float sky, float skyerr, float skysig, 
		   const FRAMEPARAMS *fparams, REGION *scr1, REGION *scr2,
		   int aperture, int psf_def_rad, float cr_min_s, int cr_min_e,
		   float cr_cond3, float cr_cond32, int threshold,
		   float sigma_multi, float sigma_badsky, float second_peak);

SYMDIAG *phSymdiagNew(void);
void phSymdiagDel(SYMDIAG *symdiag);

STAR1 *phStar1MakeAtPos(REGION *reg, float row, float col, 
			int regsize);

SYMDIAG *phStar1Symmetric(STAR1 *star1, const CELL_STATS *cstats,
			  const FRAMEPARAMS *fparams, int threshold);

RET_CODE
phMakeTestStarlist(int nframes, int nstars, int amp, float change,
		   CHAIN *starlist, CHAIN *truepsfs, CHAIN *framepsfs,
		   int stampsize);

RET_CODE
phPsfReject(CHAIN *star1clist, char *filters[], float nsigma, int niter, 
	    int min_starnum, int critical_amplitude);

RET_CODE
phMakeStarlist(CHAIN *star1clist, CHAIN *starlist, CHAIN *ps_rejects,  
	       int ifilter, int first_frame, int last_frame,
               STAR1_FLAGS star1c_require_mask,STAR1_FLAGS star1c_ignore_mask, 
               STAR1_FLAGS star1_require_mask, STAR1_FLAGS star1_ignore_mask);
RET_CODE
phMakeStarPClist(CHAIN *star1clist, CHAIN *starlist, CHAIN *rejects,  
	       int ifilter, int first_frame, int last_frame, STAR1_FLAGS flags2ignore);

#endif
