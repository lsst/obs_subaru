#if !defined(PHFITOBJ_H)
#define PHFITOBJ_H

#define USE_MODEL_SIG 0			/* use the model sig? */

/*
 * Generate the old pre-v5_3_57 (DR1?) model magnitudes with 20%
 * errors at the bright end
 */
#define BROKEN_MODEL_MAGS 0
/*
 * Are models made (and written to rgalprof.dat) in floating point?
 */
#define FLOATING_MODELS 1

#if FLOATING_MODELS
   typedef FL32 MODEL_PIX;		/* type of model REGION pixels */
#  define TYPE_MODEL TYPE_FL32		/* type of regions for models */
#  define rows_model rows_fl32		/* name of proper REGION.rows type */
#else
   typedef U16 MODEL_PIX;		/* type of model REGION pixels */
#  define TYPE_MODEL TYPE_U16		/* type of regions for models */
#  define rows_model rows_u16		/* name of proper REGION.rows type */
#endif

/*
 * Definitions that need to be communicated between profile
 * generation and profile fitting.
 */
#include "phFramestat.h"

/*
 * Parameters for profiles of deVaucouleurs models.  The profiles start
 * to deviate from e^{-r^{1/4}} at DEVCUT, going to zero at DEVCUT
 */
#define CUTOFF_PROFILES 1		/* Cutoff the profiles? */

#define IE_DEV 512.0			/* Surface brightness at r_e */

#if 0
#  define TOTFLUX_DEV 22.66523373	/* total flux of a deV model, in
					   the absence of any cutoff */
#else  /* allow for core radius */
#  define TOTFLUX_DEV 22.44790903	/* total flux of a deV model, in
					   the absence of any cutoff */
#endif

#define DEVCUT (7*DEVOUT/8.0)

#if CUTOFF_PROFILES
#  define DEVOUT 8.0			/* cutoff's between DEVCUT and DEVOUT*/
#  if 0
#     define TOTFLUX_CUTOFF_DEV 21.2776	/* total flux of a deV model, in
					   the presence of the cutoff */
#  else  /* allow for core radius */
#     define TOTFLUX_CUTOFF_DEV 21.0603	/* total flux of a deV model, in
					   the presence of the cutoff */
#  endif
#else
#  define DEVOUT 16.0			/* cutoff's between DEVCUT and DEVOUT*/
#  if 0
#     define TOTFLUX_CUTOFF_DEV 22.2956	/* total flux of a deV model, in
					   the presence of the cutoff */
#  else  /* allow for core radius */
#     define TOTFLUX_CUTOFF_DEV 22.0783	/* total flux of a deV model, in
					   the presence of the cutoff */
#  endif
#endif

/*
 * And for exponential models too
 */
#define IE_EXP 2048.0			/* Surface brightness at r_e */
#define TOTFLUX_EXP 11.94849475		/* total flux of a exponential model in
					   the absence of any cutoff */
#define EXPCUT (3*EXPOUT/4.0)

#if CUTOFF_PROFILES
#  define EXPOUT 4.0			/* cutoff's between EXPCUT and EXPOUT*/
#  define TOTFLUX_CUTOFF_EXP 11.7558	/* total flux of a exponential model in
					   the presence of the cutoff */
#else
#  define EXPOUT 5.0			/* cutoff's between EXPCUT and EXPOUT*/
#  define TOTFLUX_CUTOFF_EXP 11.8961	/* total flux of a exponential model in
					   the presence of the cutoff */
#endif
/*
 * Number of inclinations in the catalogue. The "+1" is for the infinitely
 * flat model that gets made specially
 */
#if 1
#  define EQUAL_ARATIO_STEPS 1
#  define NDEVINCL (20 + 1)
#  define NEXPINCL NDEVINCL
#else
#  define EQUAL_ARATIO_STEPS 0
#  define NDEVINCL (8 + 1)
#  define NEXPINCL (8 + 1)
#endif

/* Number of profiles for each size. */
extern int NEXPSIZ, NDEVSIZ;

/* Total number of catalogue entries. */
#define MAXSCAT (1 + NEXPSIZ*NEXPINCL + NDEVSIZ*NDEVINCL)

/*
 * Description of entry in profile catalogue. First values of class
 */
#define PSF_MODEL 0			/* pragma typedef { */
#define EXP_MODEL 1
#define DEV_MODEL 2			/* pragma } MODEL_TYPES */

#define NCLASSTYPE 3			/* number of classes of models */

typedef struct spro_catentry_T {
    int    scat_offset;    /* file offset of image */
    U16    scat_xsize;     
    U16    scat_ysize;     /* x and y sizes */
    float  scat_reff;      /* effective radius */
    float  scat_axr;       /* ellipticity */
    U8     scat_class;     /* class */
    U8     scat_type;      /* type  (ellipticity bin) */
    U16    scat_index;     /* catalog index */
} spro_catentry;			/* pragma IGNORE */

/*
 * Description of entire catalogue file.
 */
#define IDSIZE 60			/* Size for ID string */
#define RAWPROF_VERSION "Raw Profiles Version 1_2"

typedef struct spro_cheader_T {
    char  proc_catver[IDSIZE];		/* ID string for catalogue */
    int   proc_bufmax;			/* maximum size of disk image */
    int   proc_linemax;			/* maximum length of line */
    int   proc_maxscat;			/* number of entries */
    int   proc_nexpincl;		/* number of exp inclinations */
    int   proc_ndevincl;		/* number of deV inclinations */
    int   proc_nexpsiz;			/* number of exp sizes */
    int   proc_ndevsiz;			/* number of deV sizes*/
    float proc_pssig1;			/* little gaussian sigma */
    float proc_pssig2;			/* big gaussian sigma */
    float proc_psamp1;			/* ampl of little gaussian */
    int   proc_smmax;			/* smoothing fringe */
    float cutoff[NCLASSTYPE];		/* cutoffs for models */
    spro_catentry **proc_catentry;	/* catalog descrip & pointers */
} spro_cheader;				/* pragma IGNORE */

int phProfCatalogMake(const char *cat_name, int show_progress);

/*
 * Description of model parameters.
 */

typedef struct {
    int class;			/* class of model (psf, exp, deV) */
    float aratio;		/* axis ratio */
    float rsize;		/* r_eff (exponential) or */
				/* r_eff(deVoucouleurs), in pixels */
    float orient;		/* orientation */
    int octant;			/* which octant is position angle in? */
    DGPSF *psf;			/* PSF */
    float *data;		/* fitting data */
    float *var;			/* fitting sigma */
    int exact;			/* is an exact model desired? */
} MODEL_PARAMS;			/* pragma SCHEMA */


int phInitFitobj(char *file);
void phFiniFitobj(void);
int p_phReadCatentries(spro_cheader *header, FILE *fd);

int phProfileTableHeaderCheck(const char *file, int is_celltable, int hash);
int phProfileTableHeaderPrint(const char *file, int is_celltable);
int phProfileTableHashGet(const char *file, int *hash);

REGION *phAnyModelMake(const MODEL_PARAMS *p,float *flux,int *max,float *scale);
REGION *phAnyModelMakeScaled(const MODEL_PARAMS *p, float totflux, int *max);
REGION *p_phMakeExactModel(const MODEL_PARAMS *p);
REGION *phMakePowerLawModel(const float I0, const float beta, const float rmin,
                            int nrow, int ncol, double rowc, double colc);                            

float phTotalFluxGet(const CELL_STATS *stats_model,
		     const MODEL_PARAMS *p);
float phFluxGetFromModel(const MODEL_PARAMS *p);
float phIeGetFromFlux(float totflux, int class, float re, float ab);
float phFluxGetFromIe(float Ie, int class, float re, float ab);


int phCellMakeProfCat(
    char *outfile,
    int nseeing,		/* number of seeings */
    double fseeing,		/* width of first seeing */
    double dseeing,		/* seeing increment */
    float psf_sigma_ratio,		/* ratio of sigma2 to sigma1 */
    float psf_amp_ratio,		/* ratio "b" of two Gaussians */
    int exact,				/* make exact models */
    int show_progress);			/* print progress report to stderr? */

REGION *
shRegIntRotate(REGION *in,		/* region to rotate */
	       double angle,		/* angle to rotate by; degrees */
	       int bgxel,		/* value for pixels not in in */
	       int use_sinc,		/* use sinc interpolation */
	       int square);		/* make output region square */

REGION *
phResidualsFind(const REGION *reg1,	/* one region */
		const REGION *reg2,	/* the one to be subracted */
		float drow, float dcol); /* reg2's origin is at (drow, dcol) in
					   reg1 */

void phCompareRawHeaders(const spro_cheader *hdr1, const spro_cheader *hdr2);

#endif
