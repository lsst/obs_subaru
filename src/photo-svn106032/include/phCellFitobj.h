#if !defined(PHCELLFITOBJ_H)
#define PHCELLFITOBJ_H
#include "phFitobj.h"

#define CELLPROF_VERSION "Cell Profiles Version 2_2"

typedef struct {
    char idstr[IDSIZE];			/* ID string for catalogue */
    int crc;				/* CRC for raw profiles */
    int nseeing;			/* number of seeings */
    float fseeing;			/* width of first seeing */
    float dseeing;			/* seeing increment */
    float seeing_ratio;			/* ratio of large to small sigma */
    float seeing_b;			/* ratio of amplitudes of components */
    int max_models;			/* total number of models */
    spro_cheader prof_cat;		/* model descriptions */
} CELLPROCATHEADER;

typedef struct {
    int ncells;				/* number of cells */
    float totflux;			/* total flux in object (to infinity)*/
    float sigma2;			/* second moment of object */
    float *mean;			/* mean profile */
    float *median;			/* median profile */
    float *sig;				/* errors */
    float *mem;				/* malloced space for mean/median
					   and sig */
} COMP_CSTATS;

/*****************************************************************************/
/*
 * The constraints placed upon a PSF's description in terms of sums
 * of models in the model catalogues
 */
typedef struct {
   int si;				/* central single model */
   int di;				/* central double model */
   int nmodel;				/* range of allowed models */
} PSF_CONSTRAINT;

/*****************************************************************************/

int phInitCellFitobj(char *file, int ncolor, int median);
void phFitCellColorSet(int color, const PSF_COEFFS *coeffs);
void phFiniCellFitobj(void);

const DGPSF *phFitPsfFromReg(const REGION *reg, float sigma, float bkgd,
			     const PSF_CONSTRAINT *constraint);
const PSF_COEFFS *phFitCellPsfCoeffs(const REGION *reg,float sigma,float bkgd);
const PSF_COEFFS *phFitCellPsfCoeffsFromCellProf(const CELL_PROF *prof);
int phDgpsfFitFromCellprof(DGPSF *psf, const CELL_PROF *cprof);

void phConstrainPsfModels(PSF_CONSTRAINT *constraint,
			  int color, const FIELDPARAMS *fiparams, int nmodel);

void
phCellProfSet(CELL_PROF *cprof,		/* the CELL_PROF to initialise */
	      const CELL_STATS *cstats, /* cell array info */
	      int median,		/* use median profile? */
	      double bkgd,		/* sky level */
	      double gain,		/* gain of chip */
	      double dark_variance,	/* background variance */
	      double sigma,		/* object's width */
	      double posErr,		/* centering error */
	      int use_difference,	/* include difference of cell
					   pairs in variance? */
	      int sky_noise_only);	/* only include sky noise */

int
phFitCellAsPsf(OBJC *objc,		/* Object to fit */
	       int color,		/* color of object */
	       const CELL_STATS *stats_obj, /* cell array */
	       const FIELDPARAMS *fiparams, /* describe field */
	       int nannuli,		/* number of annuli to use */
	       int sky_noise_only,	/* only consider sky noise */
	       float *I0,		/* central intensity; can be NULL */
	       float *bkgd);		/* the sky level; can be NULL */

float
phFitCellProfAsPsf(const CELL_PROF *cprof, /* cell profile for object */
		   float *bkgd,		/* the sky level; can be NULL */
		   int nannuli);	/* number of annuli to use, if > 0 */

int
phFitCellAsDeV(OBJC *objc,		/* Object to fit */
	       int color,		/* color of object */
	       const CELL_STATS *stats_obj, /* cell array */
	       const FIELDPARAMS *fiparams, /* describe field */
	       int nannuli,		/* number of annuli to use, if > 0 */
	       int sky_noise_only);	/* only consider sky noise */
int
phFitCellAsExp(OBJC *objc,		/* Object to fit */
	       int color,		/* color of object */
	       const CELL_STATS *stats_obj, /* cell array */
	       const FIELDPARAMS *fiparams, /* describe field */
	       int nannuli,		/* number of annuli to use, if > 0 */
	       int sky_noise_only);	/* only consider sky noise */
float
phFitCellAsKnownModel(OBJC *objc,		/* Object to fit */
		      int color,               /* color of object */
		      const CELL_STATS *cstats, /* object's cell array */
		      const FIELDPARAMS *fiparams, /* describe field */
		      int nannuli,	/* number of annuli to use, if > 0 */
		      int class,	/* desired class of model */
		      float ab, float phi, float r, /* model parameters */
		      int use_difference, /* include difference of cell
					     pairs when estimating variance? */
		      int sky_noise_only, /* only include sky noise in var. */
		      float *counts, float *countsErr, /* counts & error */
		      float *sky,	/* estimated sky level, or NULL */
		      int *nu);		/* number of d.o.f., or NULL */
float
phFitCellAsTwoKnownModels(OBJC *objc,		/* Object to fit */
			  int color,               /* color of object */
			  const CELL_STATS *cstats, /* object's cell array */
			  const FIELDPARAMS *fiparams, /* describe field */
			  int nannuli,	/* number of annuli to use */
			  float ab_deV,	/* exp model axis ratio */
			  float phi_deV, /* exp model p.a. (degrees) */
			  float re_deV,	/* exp model r_effective */
			  float ab_exp,	/* deV model axis ratio */
			  float phi_exp, /* deV model p.a. (degrees) */
			  float re_exp,	/* deV model r_effective */
			  int use_difference, /* include difference of cell
						 pairs when estimating var.? */
			  int sky_noise_only, /* only sky noise in var. */
			  float *counts, float *countsErr, /* counts & error */
			  int *nu,	/* number of degrees of freedom */
			  float *frac_deV); /* fraction of counts in the deV */
const CELL_PROF *
phCellProfGetFromKnownModel(int model, float counts,
			    float ab, float phi, float r);
REGION *
phFitCellChisq(OBJECT1 *obj,		/* Object to fit */
	       int color,		/* color of object */
	       const FIELDPARAMS *fiparams, /* describe field */
	       int use_deV,		/* use a deV not an exponential */
	       float aratio0,		/* initial value of a/b */
	       float aratio1,		/* final value of a/b */
	       int naratio,		/* number of aratios */
	       float rsize0,		/* initial value of re */
	       float rsize1,		/* final value of re */
	       int nrsize,		/* number of rsizes */
	       int sky_noise_only,	/* cell variance == sky noise? */
	       int per_dof);		/* divide chi^2 by degrees of freedom */

REGION *
phFittedModelMake(const PSF_BASIS *basis, /* PSF for this field */
		  const OBJC *objc,	/* reconstruct this object */
		  int c,		/* in this band */
		  int class,		/* type of model desired */
		  float sigma,		/* smoothing sigma for centroiding */
		  int correct_residuals, /* correct for residuals from KL? */
		  int exact,		/* make exact models? */
		  int cellprof);	/* return an image of the cellprof model */

const char *phFormatCatentry(const spro_catentry *entry);

int phCompareProfileFiles(const char *file1, /* One file */
			  const char *file2, /* the other file */
			  int model0,	/* First model to compare */
			  int model1,	/* Last model to compare (if <= 0: counted from end) */
			  const float eps,	/* largest allowable fractional difference */
			  const float feps,	/* Maximum allowed fractional difference */
			  int normalize); /* Normalise profiles to centre == 1 */
int phCompareRawProfileFiles(const char *file1, /* One file */
			     const char *file2, /* the other file */
			     int n0,	/* First model to compare (0 indexed) */
			     int n1,	/* Last model to compare (if -ve, counted from end) */
			     const float eps, /* largest allowable fractional difference */		    
			     const float feps, /* Maximum allowed fractional difference */
			     int normalize,/* Normalise profiles to centre == 1 */
			     const char *dump_filename); /* format for output filenames */
#endif
