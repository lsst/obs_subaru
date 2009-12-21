#include <alloca.h>
#include <string.h>
#include "dervish.h"
#include "phObjc.h"
#include "phDgpsf.h"
#include "phMathUtils.h"
#include "phUtils.h"
#include "phObjects.h"
#include "phConsts.h"
#include "phCellFitobj.h"

#define NAMELEN 100
static char catalog_file[NAMELEN] = "";	/* Name of catalogue file. */
static FILE *catalog_fd = NULL;		/* catalog_file's file descriptor */


#define NPARAM 8			/* max number of parameters to fit */
/*
 * The orders of interpolation used in the various parameters. Note that
 * it isn't sufficient to simply change these to change the order used!
 */
#define IORDER_INC 2			/* "inclination", really b/a */
#define IORDER_SIZE 2			/* size */

static int use_median = 1;		/* use median not mean profile */

static long *model_off = NULL;

static PSF_COEFFS *seeing_ind = NULL;	/* current seeing */

static COMP_CSTATS *model_cs = NULL;	/* COMP_CSTATS for the current model */

/*
 * a suite of models for e.g. exponential disks
 */
typedef struct {
    int mod_nsize;
    int mod_nincl;
    int mod_offset;
    spro_catentry *entries;
} MODEL_ENTRIES;


/*
 * index and weight for a model
 */
struct p_model {
    int i; 	/* index of model */
    float w;	/* weight to be given to this model */
};

static MODEL_ENTRIES psf_entries;	/* PSFs */
static MODEL_ENTRIES dev_entries;	/* DeVaucouleurs models */
static MODEL_ENTRIES exp_entries;	/* Exponential models. */

/*
 * Variables used to communicate with the cell_fit_model routine
 */
static int fit_sky_level = 0;		/* should we fit the sky? */
static float model_amp;			/* amplitude of best-fit model */
static int fit_phi = 0;			/* should we fit the position angle? */
static float model_phi;			/* position angle of best model (rad)*/
static float sky_level;			/* the resulting value */
static float totflux;			/* estimated total flux in an object */
static float totfluxErr;		/* error in total flux */
static float model_ampErr;		/* error in amplitude */
static float model_phiErr;		/* error in position angle */

static float rsize_min, rsize_max;	/* extremal values of rsize for
							    available models */
static float aratio_min;		/* minimum value of rsize for
							    available models */

/*
 * Cos and sin coefficients for Fourier expansion of sum(data*model/sigma^2)
 */
static double A[NSEC/2], B[NSEC/2];


/*
 * Now the caches; there are colour-by-colour caches of the seeing-interpolated
 * models, a cache of models as read from disk, and a cache of all the PSF
 * profiles in the catalogue
 */
static COMP_CSTATS *model_cache = NULL;
static int cache_size = -1;		/* size of model cache;
					   also no. of models for each seeing*/

typedef struct {			/* PSF cache */
   float *model[2];			/* values of PSF; c.f. fit_ctx.data
					   (i == 0) ? double : single */
   float sigma2[2];			/* second moment of PSFs */
   float totflux[2];			/* total flux in PSFs */
} PSF_CACHE;

static PSF_CACHE *psf_cache = NULL;	/* cached PSFs from catalogue */


/*****************************************************************************/
/*
 * These functions return a COMP_CSTATS containing a realization of a
 * seeing-convolved Exponential or de Vaucouleurs law.
 */
static CELL_PROF fit_ctx;

static void
cell_fit_model(
    COMP_CSTATS *(*cells_make)(const MODEL_PARAMS *),
    int ndata,
    int nparam,
    double *p,
    double *fvec,
    int *iflag
    );
static void
mod_param_to_index0(const MODEL_PARAMS *p,
		    const MODEL_ENTRIES *entries,
		    struct p_model *index)
	;
static COMP_CSTATS *
convolved_model_get(const int index0)	/* index of model to return */
	;

static void
set_rsize_limits(const MODEL_ENTRIES *mod_entries)
	;
static float
find_outer(OBJECT1* obj)
	;
static COMP_CSTATS *
find_phi(COMP_CSTATS *stats_model0,	/* model at 0 degrees */
	 int nannuli,			/* dimension of data[], sig[] */
	 float *data,			/* object's profile */
	 float *sig);			/* errors in data */
static void
expand_model(float phi,			/* desired position angle */
	     float *model,		/* model to expand */
	     int nannuli)		/* number of annuli in model */
	;
static void
mod_re_to_isize(
    double re,
    const MODEL_ENTRIES *entries,
    struct p_model *isize
	);
static void
mod_aratio_to_iinc(
    double aratio,
    const MODEL_ENTRIES *entries,
    struct p_model *iinc
	);
static COMP_CSTATS *
model_read(FILE *fd,			/* file to read */
	   const int index,		/* index of model to read */
	   int normalize,		/* normalise profile? */
	   COMP_CSTATS *cs)		/* a COMP_CSTATS with least MAXCELLS,
					   or NULL */
	;

static double
eval_fourier(double phi,
	     const void *user)		/* NOTUSED */
	;

/*****************************************************************************/
/*
 * Note that this routine fills out the global fit_ctx
 */
static void
setup_cell_data(const CELL_STATS *stats_obj, /* cell array info */
		int median,		/* use median profile? */
		double sky,		/* sky level */
		double gain,		/* gain of chip */
		double dark_variance,	/* background variance */
		double sigma,		/* sigma of PSF */
		double posErr,		/* centering error */
		int use_difference,	/* include difference of cell
					   pairs in variance? */
		int sky_noise_only)	/* only include sky noise */
{
   phCellProfSet(&fit_ctx, stats_obj, median, sky, gain, dark_variance,
				sigma, posErr, use_difference, sky_noise_only);
}

/*****************************************************************************/
/*
 * This function will return a COMP_CSTATS containing a realization of a
 * seeing-convolved model
 */
static COMP_CSTATS *
model_cells_make(const MODEL_PARAMS *p, /* parameters of model to create */
		 const MODEL_ENTRIES *mod_entries
	        )
{
    struct p_model index[IORDER_INC*IORDER_SIZE];
    COMP_CSTATS *cs;
    COMP_CSTATS *cstmp;
    float *cs_model;			/* unaliased from cs->{mean,median} */
    float *cstmp_model;			/* cstmp->{mean or median} */
#if USE_MODEL_SIG
    float *cstmp_sig, *cs_sig;		/* cstmp->sig, cs->sig */
#endif
    const int median = use_median;	/* unaliased */
    int i;
    int im;
    float w;				/* unaliased weight index[].w */
/*
 * create the returned COMP_CSTATS
 */
    cs = model_cs;			/* the global scratch COMP_CSTATS */

    if(use_median) {
       cs_model = cs->median = cs->mem;
       cs->mean = NULL;
    } else {
       cs_model = cs->mean = cs->mem;
       cs->median = NULL;
    }
    
#if USE_MODEL_SIG
    cs_sig = cs->sig;
#endif

    memset(cs->mem,'\0',(USE_MODEL_SIG ? 2:1)*sizeof(float)*MAXCELLS);
    shAssert(*cs->mem == 0.0);		/* check that 0.0 is all 0 bits */
    cs->ncells = -1;
    cs->totflux = 0.0;
/*
 * perform the interpolation to make the model.
 */
    mod_param_to_index0(p, mod_entries, index);
    
    for(im = 0; im < IORDER_INC*IORDER_SIZE; im++) {
        if((w = index[im].w) == 0.0) {
	   continue;
	}
	cstmp = convolved_model_get(index[im].i);
	cstmp_model = median ? cstmp->median : cstmp->mean;
#if USE_MODEL_SIG
	cstmp_sig = cstmp->sig;
#endif
	if(cstmp->ncells > cs->ncells) cs->ncells = cstmp->ncells;

	cs->totflux += w*cstmp->totflux;
	for(i = cstmp->ncells - 1;i >= 0; i--) {
	    cs_model[i] += cstmp_model[i]*w;
#if USE_MODEL_SIG
	    cs_sig[i] += cstmp_sig[i]*w;
#endif
	}
    }

    return cs;
}

static COMP_CSTATS *
psf_cells_make(const MODEL_PARAMS *p)	/* parameters of model to create */
{
   return model_cells_make(p, &psf_entries);
}

static COMP_CSTATS *
dev_cells_make(
	       const MODEL_PARAMS *p /* parameters of model to create */
	       )
{
   return model_cells_make(p, &dev_entries);
}

static COMP_CSTATS *
exp_cells_make(
	       const MODEL_PARAMS *p /* parameters of model to create */
	       )
{
   return model_cells_make(p, &exp_entries);
}


static void
cell_fit_psf(
    int ndata,
    int nparam,
    double *p,
    double *fvec,
    int *iflag
    )
{
    cell_fit_model(psf_cells_make, ndata, nparam, p, fvec, iflag);
}

static void
cell_fit_dev(
    int ndata,
    int nparam,
    double *p,
    double *fvec,
    int *iflag
    )
{
    cell_fit_model(dev_cells_make, ndata, nparam, p, fvec, iflag);
}

static void
cell_fit_exp(
    int ndata,
    int nparam,
    double *p,
    double *fvec,
    int *iflag
    )
{
    cell_fit_model(exp_cells_make, ndata, nparam, p, fvec, iflag);
}


/*****************************************************************************/
/*
 * here are the functions to fit cell arrays with PSFs, De Vaucouleurs models,
 * or exponential disks
 *
 * <AUTO EXTRACT>
 *
 * Fit an object with a PSF; if <stats_obj> is NULL, the routine will fill
 * it for you
 *
 * If sky is non-NULL, a local background level is fit along with the
 * psf, and its central value returned (i.e. the value of cell 0)
 *
 * If nannuli is >= 1, only that many annuli will be used in the fit
 *
 * Returns chi^2/nu
 */
int
phFitCellAsPsf(OBJC *objc,		/* Object to fit */
	       int color,		/* color of object */
	       const CELL_STATS *cstats, /* cell array */
	       const FIELDPARAMS *fiparams, /* describe field */
	       int nannuli,		/* number of annuli to use */
	       int sky_noise_only,	/* cell variance == sky noise? */
	       float *I0,		/* central intensity; can be NULL */
	       float *sky)		/* the sky level; can be NULL */
{
   OBJECT1 *const obj1 = objc->color[color];
   int use_difference = 0;		/* include difference of cell pairs
					   in cell variance */

   if((obj1->chisq_star =
       phFitCellAsKnownModel(objc, color, cstats, fiparams, nannuli, PSF_MODEL,
			     0.0, 0.0, 0.0, use_difference, sky_noise_only,
			     NULL, NULL, sky, &obj1->nu_star)) == -1) {
      if(I0 != NULL) {
	 *I0 = 0;
      }
      return(2e9);
   }
   
   obj1->star_lnL = phChisqProb(obj1->chisq_star, obj1->nu_star, 1);
   obj1->star_L = exp(obj1->star_lnL);

   if(I0 != NULL) {
      *I0 = model_amp;
   }

   return(obj1->chisq_star/obj1->nu_star);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Fit an object with a known model specified as a type, and values of
 * r_e, a/b, and phi. The total counts in the model, and an estimate
 * of the error therein, are returned if the corresponding pointers
 * are non-NULL. As a side effect, model_amp and model_ampErr are set.
 *
 * if <stats_obj> is NULL, the routine will fill it for you
 *
 * If sky is non-NULL, a local background level is fit along with the model
 *
 * If nannuli is >= 1, only that many annuli will be used in the fit
 *
 * Return the likelihood (or -1 in case of problems)
 */
float
phFitCellAsKnownModel(OBJC *objc,		/* Object to fit */
		      int color,               /* color of object */
		      const CELL_STATS *cstats, /* object's cell array */
		      const FIELDPARAMS *fiparams, /* describe field */
		      int nannuli,	/* number of annuli to use */
		      int class,	/* desired class of model */
		      float ab,		/* model axis ratio */
		      float phi,	/* model p.a. (degrees) */
		      float re,		/* model r_effective */
		      int use_difference, /* include difference of cell
					     pairs when estimating variance? */
		      int sky_noise_only, /* only include sky noise in var. */
		      float *counts, float *countsErr, /* counts & error */
		      float *sky,	/* estimated sky level, or NULL */
		      int *nu)		/* number of degrees of freedom */
{
   void (*cell_fit_func)(int, int, double *, double *, int *) = NULL;
   float chisq;
   const FRAMEPARAMS *fparams;		/* unpacked from fiparams */
   double *fvec;			/* residuals for cells */
   int i;
   OBJECT1 *obj;			/* object in question */
   double p[NPARAM];			/* model parameters */
   int npar = 0;			/* number of parameters in p */
   float posErr;			/* error in position */
   const REGION *reg;			/* unpacked from fparams */
   
   shAssert(model_off != NULL);		/* we're initialised */
   shAssert(seeing_ind != NULL);	/* with a PSF selected */
   shAssert(fiparams != NULL && objc != NULL);
   shAssert(color >= 0 && color < fiparams->ncolor && color < objc->ncolor);
   fparams = &fiparams->frame[color];
   shAssert(fparams != NULL && fparams->data != NULL && fparams->psf != NULL);
   obj = objc->color[color];
   shAssert(obj != NULL);

   reg = fparams->data;
   
   fit_phi = 0; model_phi = phi*M_PI/180;
   switch (class) {
    case PSF_MODEL:
      set_rsize_limits(&psf_entries);
      cell_fit_func = cell_fit_psf;
      break;
    case DEV_MODEL:
      set_rsize_limits(&dev_entries);
      cell_fit_func = cell_fit_dev;
      p[npar++] = ab;
      p[npar++] = re;
      break;
    case EXP_MODEL:
      set_rsize_limits(&exp_entries);
      cell_fit_func = cell_fit_exp;
      p[npar++] = ab;
      p[npar++] = re;
      break;
    default:
      shFatal("phFitCellAsKnownModel: unknown galaxy class %d",class);
   }
   shAssert(npar <= NPARAM);

   if(cstats == NULL) {
      cstats = (const CELL_STATS *)
	phProfileExtract(-1, -1, reg, obj->rowc, obj->colc, find_outer(obj),
			 SOFT_BIAS + fparams->bkgd, obj->skyErr, 0);
      if(cstats == NULL || cstats->syncreg == NULL) {
	 shErrStackPush("phFitCellAsKnownModel: "
			"object is too close to edge (%.3f, %.3f)",
			obj->rowc, obj->colc);
	 if(nu != NULL) {		/* avoid division by 0 */
	    *nu = 1;
	 }
	 if(sky != NULL) {
	    *sky = 0;
	 }
	 return(-1);
      }
      
      obj->profMean[0] = phProfileMean(cstats, 0, 0, 1, NULL);
      obj->nprof = cstats->nannuli_c;
   }

   if(obj->rowcErr < 0) {
      posErr = 1;
   } else {
      posErr = sqrt(pow(obj->rowcErr,2) + pow(obj->colcErr,2));
   }

   setup_cell_data(cstats, use_median, obj->sky,
		   phGain(fparams, obj->rowc, obj->colc),
		   phDarkVariance(fparams, obj->rowc, obj->colc),
		   fparams->psf->width, posErr, use_difference, sky_noise_only);

   if(nannuli > 0) {
      int ncell = 1 + (nannuli - 1)*(NSEC/2);
      if(fit_ctx.ncell > ncell) {
	 fit_ctx.ncell = ncell;
      }
   }
/*
 * find the residuals vector, optionally fitting a sky level
 */
   fit_sky_level = (sky == NULL) ? 0 : 1;
   fvec = alloca(fit_ctx.ncell*sizeof(double));
   i = 0; cell_fit_func(fit_ctx.ncell, npar, p, fvec, &i);
/*
 * set return variables, if so desired
 */
   if(sky != NULL) {
      *sky = sky_level;
   }
   if(counts != NULL) {
      *counts = totflux;
   }
   if(countsErr != NULL) {
      *countsErr = totfluxErr;
   }
/*
 * Evaluate chi^2
 */
   chisq = 0.0;
   for(i = 0; i < fit_ctx.ncell; i++) {
      chisq += fvec[i]*fvec[i];
   }

   if(nu != NULL) {
      *nu = fit_ctx.ncell - 1;		/* we _didn't_ fit npar parameters */
      if(fit_sky_level) (*nu)--;
   }

   return(chisq);
}

static void
cell_fit_model(
    COMP_CSTATS *(*cells_make)(const MODEL_PARAMS *),
    int ndata,
    int nparam,
    double *p,
    double *fvec,
    int *iflag
    )
{
#if DEBUG || TEST_2D_PROFILES
    float sigsav[NCELL];
#endif
    float *data;			/* mean or median */
    float fiddle_fac;			/* amount to fiddle chisq due to
					   parameters being out of bounds */
    const int fit_sky = fit_sky_level;	/* a local copy for optimiser */
    int i;
    int idata;
    int iann;				/* index for annuli */
    int nannuli;			/* number of annuli */
    int sect0;				/* sector 0 for a given annulus */
    int isect;				/* sector counter */
    float ivar;				/* == 1/sigma^2; unpacked for speed */
    float *model;			/* mean or median */
    float mod;				/* model[sector]; unpacked for speed */
    float mod_ivar;			/* == mod/sigma^2;
					   unpacked for speed */
    MODEL_PARAMS mp;
    float residual_flux;		/* flux in residual table */
    float sigma;			/* model-reduced sigma for a point */
    COMP_CSTATS *stats_model;		/* model at best-fit angle */
    float *sig;				/* standard deviation of data */
    double sum, sum_d, sum_m;		/* sums of 1, data, and model */
    double sum_dm, sum_mm;		/* sums of data*model and model^2 */
    int ncells;

    shAssert(*iflag >= 0);
    shAssert(nparam <= NPARAM);
    shAssert(ndata == fit_ctx.ncell);
    shAssert(nparam == 0 || p != NULL);
    shAssert(fvec != NULL);
/*
 * Stuff changing parameters into the context.
 */
    mp.clazz = -1;			/* unknown */
    mp.psf = NULL;

    mp.aratio = 1.0;
    mp.rsize = rsize_max;
    if(nparam >= 1) {
       shAssert(p[0] == p[0]);		/* i.e. not a NaN */
       mp.aratio = p[0];
       if(nparam >= 2) {
	  shAssert(p[1] == p[1]);
	  mp.rsize = p[1];
       }
    }
/*
 * see if any parameters are out of range; if so set them to their limits,
 * and remember to increase the final fvec vector correspondingly
 */
    fiddle_fac = 1.0;

    if(mp.aratio < aratio_min) {
       fiddle_fac *= 1 + pow(aratio_min - mp.aratio,2);
       mp.aratio = aratio_min;
    }
    if(mp.aratio > 1) {			/* minimiser can choose a better phi */
       fiddle_fac *= pow(mp.aratio,2);
       mp.aratio = 1;
    }    
    if(mp.rsize > rsize_max) {
       fiddle_fac *= pow(mp.rsize/rsize_max,2);
       mp.rsize = rsize_max;
    } else if(mp.rsize < rsize_min) {
       fiddle_fac *= pow((mp.rsize - 1)/(rsize_min - 1),2);
       mp.rsize = rsize_min;
    }
/*
 * Build the model
 */
    stats_model = (*cells_make)(&mp);
/*
 * get uncompressed number of cells.
 */
    ncells = stats_model->ncells;
    ncells = ncells < ndata ? ncells : ndata;
    if(ncells == 0) {
       return;
    }
    nannuli = (ncells - 1)/(NSEC/2) + 1;

    shAssert(ncells > nparam);
/*
 * Unpack the compressed cell data
 */
    data = fit_ctx.data;
    model = use_median ? stats_model->median : stats_model->mean;
    shAssert(model != NULL);
    sig = fit_ctx.sig;
/*
 * find the best position angle, if so desired
 */
    if(fit_phi) {
       stats_model = find_phi(stats_model, nannuli, data, sig);
       shAssert(stats_model != NULL);	/* phi determination converged */

       model = use_median ? stats_model->median : stats_model->mean;
    } else {
       expand_model(model_phi, model, nannuli);
    }
/*
 * correct for the known discrepancy between the psf and our best
 * representation of it
 */
    residual_flux = 0;
    if(seeing_ind != NULL) {		/* we have a model for the PSF */
       const float I0 = model[0];	/* central value of model */
       const float *area = seeing_ind->residuals.area;
       float *residuals = seeing_ind->residuals.data;
       int nresid = (ncells < seeing_ind->residuals.ncell) ?
				      ncells : seeing_ind->residuals.ncell;
       for(i = 0;i < nresid;i++) {
	  model[i] += I0*residuals[i];
	  residual_flux += area[i]*residuals[i];
       }
       residual_flux *= I0;
    }
/*
 * calculate best-fitting amplitude; if fit_sky is true, solve
 * for the sky level too
 */
    idata = 0;
    sigma = sig[idata];
    ivar = 1/(sigma*sigma);
    if(fit_sky) {
       sum_d = data[idata]*ivar;
       sum_m = model[idata]*ivar;
       sum = ivar;
    } else {
      sum = sum_m = sum_d = 0;		/* make compilers happy */
    }
    sum_dm = data[idata]*model[idata]*ivar;
    sum_mm = model[idata]*model[idata]*ivar;
    
    idata++;

    for(sect0 = iann = 1; iann < nannuli; iann++, sect0 += NSEC/2) {
	for(isect = 0; isect < NSEC/2; isect++) {
	    mod = model[sect0 + isect];
	    sigma = sig[idata];

	    ivar = 1/(sigma*sigma);
	    mod_ivar = mod*ivar;
	    if(fit_sky) {
	       sum_d += data[idata]*ivar;
	       sum_m += mod_ivar;
	       sum += ivar;
	    }
	    sum_dm += data[idata]*mod_ivar;
	    sum_mm += mod*mod_ivar;

	    ++idata;
	}
    }
    shAssert(idata == ncells);

    if(fit_sky) {
       double det = sum*sum_mm - sum_m*sum_m;

       shAssert(det != 0.0);

       sky_level = (sum_mm*sum_d - sum_m*sum_dm)/det;
       model_amp = (sum*sum_dm - sum_m*sum_d)/det;
       model_ampErr = sqrt(sum/det);
    } else {
       shAssert(sum_mm != 0.0);

       sky_level = 0.0;
       model_amp = sum_dm/sum_mm;
       model_ampErr = sqrt(1/sum_mm);
    }
    model_ampErr *= sqrt(idata/(idata - nparam - fit_phi)); /* correct for
							       missing dof */
/*
 * We _added_ the residuals to the model, so we increased its flux
 */
    totflux = model_amp*(stats_model->totflux + residual_flux);
    totfluxErr = model_ampErr*(stats_model->totflux + residual_flux);

    for(i = 0;i < ncells;i++) {
       model[i] = model_amp*model[i] + sky_level;
    }
/*
 * and the residual vector
 */
    idata = 0;
    sigma = sig[idata];
    fvec[idata] = (data[idata] - model[idata])/sigma;
#if DEBUG || TEST_2D_PROFILES
    sigsav[idata] = sigma;
#endif
    idata++;

    nannuli = (ncells - 1)/(NSEC/2) + 1;
    for(sect0 = iann = 1; iann < nannuli; iann++, sect0 += NSEC/2) {
	for(isect = 0; isect < NSEC/2; isect++) {
	    sigma = sig[idata];
	    fvec[idata] = fiddle_fac*(data[idata] - model[sect0+isect])/sigma;
#if DEBUG || TEST_2D_PROFILES
	    sigsav[idata] = sigma;
#endif

	    ++idata;
	}
    }
    /*
     * The rest of the model must have been zeros.
     */
    for(; idata < ndata; idata++) {
	fvec[idata] = fiddle_fac*data[idata]/sig[idata];
#if DEBUG || TEST_2D_PROFILES
	sigsav[idata] = sig[idata];
#endif
     }
    /*
     * Debugging code; enable in gdb
     */
#if 0
    {
 	static volatile int dump_iteration = 0;
 	if (dump_iteration) {
 	    double sum = 0;
 	    for(i = 0; i < ndata; i++) {
 		sum += fvec[i]*fvec[i];
  	    }
 	    printf("p0 = %g p1 = %g chisq = %g\n", p[0], p[1], sum);
 	}
    }    
#endif
/*
 * Save fit in a global for TEST_INFO?
 */
#if TEST_2D_PROFILES
    test_nprof2D = ndata;
    for(i = 0; i < ndata; i++) {
       test_profMean2D[i] = data[i];
       test_profErr2D[i] = sigsav[i];
       test_profModel2D[i] = data[i] - fvec[i]*sigsav[i]/fiddle_fac;
    }
#endif
}

/*
 * get the indices for a model, without allowing for PSF interpolation
 */
static void
mod_param_to_index0(const MODEL_PARAMS *p,
		    const MODEL_ENTRIES *entries,
		    struct p_model *index)
{
   struct p_model iinc[IORDER_INC];	/*    "    "   "   "  "   "  inc */
   struct p_model isize[IORDER_SIZE];	/*    "    "   "   "  "   "  size */
   int ii;				/* index for inc */
   int is;				/*   "    "  size */
   int ind;				/* index into index[] */
   
   mod_re_to_isize(p->rsize, entries, isize);
   mod_aratio_to_iinc(p->aratio, entries, iinc);
   
   ind = 0;
   for(ii = 0; ii < IORDER_INC; ii++) {
      for(is = 0; is < IORDER_SIZE; is++, ind++) {
	 index[ind].i =
	   entries->mod_offset + iinc[ii].i*entries->mod_nsize + isize[is].i;
	 index[ind].w = iinc[ii].w*isize[is].w;
      }
   }
}

/*****************************************************************************/
/*
 * This routine returns a seeing-convolved model from the cache, or
 * builds it by reading from disk and doing the linear superposition
 * as needs be
 */
static COMP_CSTATS *
convolved_model_get(const int index0)	/* index of model to return */
{
    COMP_CSTATS *cs;			/* the returned model */
    float *cs_model;			/* unaliased from cs->{mean,median} */
    COMP_CSTATS *cstmp;			/* a temp model read from disk */
    float *cstmp_model;			/*unaliased from cstmp->{mean,median}*/
#if USE_MODEL_SIG
    float *cstmp_sig;			/* unaliased from cstmp->sig */
    float *cs_sig;			/*  "  "  "   "   cs->sig */
#endif
    int i, j;
    int index;				/* index of smoothed model */
    int ncells;
    float w;				/* weight for a model */

    shAssert(model_cache != NULL);
    shAssert(index0 >= 0 && index0 < cache_size);

    cs = &model_cache[index0];
    if(cs->ncells < 0) {		/* A miss */
	if(catalog_file[0] == '\0') {
	   shErrStackPush("model_cached_read: no catalog file is declared");
	   return(NULL);
	}

	shAssert(catalog_fd != NULL);
/*
 * setup the COMP_CSTATS that we'll be accumulating in the cache slot
 */
	cs_model = cs->mem;
	if(use_median) {
	   cs->median = cs_model;
	   cs->mean = NULL;
	} else {
	   cs->mean = cs_model;
	   cs->median = NULL;
	}
#if USE_MODEL_SIG
	cs_sig = cs->sig;
#endif

	memset(cs->mem,'\0',(USE_MODEL_SIG ? 2 : 1)*MAXCELLS*sizeof(float));
	shAssert(*cs->mem == 0.0);	/* check that 0.0 is all 0 bits */
	cs->totflux = 0.0;
	
	for(i = 0; i < IORDER_SEEING; i++) {
	   index = index0 + seeing_ind->i[i]*cache_size;

	   cstmp = model_read(catalog_fd, index, (index0 == 0 ? 1 : 0), NULL);
	   ncells = cstmp->ncells;
	   cstmp_model = use_median ? cstmp->median : cstmp->mean;
#if USE_MODEL_SIG
	   cstmp_sig = cstmp->sig;
#endif

	   if(ncells > cs->ncells) {
	      cs->ncells = ncells;
	   }
/*
 * accumulate the models we've read into the cache
 */
	   w = seeing_ind->w[i];
#if !BROKEN_MODEL_MAGS
/*
 * The models with given parameters are all normalised to the same total flux,
 * whereas the PSFs used to determine the seeing_ind->w weights are all
 * normalised to have unit amplitude at the origin and therefore have weights
 * proportional to the PSF's total flux (i.e. sigma^2 for a single Gaussian).
 *
 * Take this into account.
 */
	   if(index0 != 0) {		/* i.e. not a PSF */
	      int ind = seeing_ind->i[i]/2;
	      int single = seeing_ind->i[i]%2 ? 1 : 0;
	      w *= psf_cache[ind].totflux[single];
	   }
#endif
	   
	   cs->totflux += w*cstmp->totflux;
	   for(j = 0;j < ncells;j++) {
	      cs_model[j] += w*cstmp_model[j];
#if USE_MODEL_SIG
	      cs_sig[j] += w*cstmp_sig[j];
#endif
	   }
	}
     }

    return(cs);
}

/*****************************************************************************/
/*
 * Find the limits for rsize/aratio for this model
 */
static void
set_rsize_limits(const MODEL_ENTRIES *mod_entries)
{
   int nmodel;				/* number of models in table */
   
   shAssert(mod_entries != NULL);
   
   nmodel = mod_entries->mod_nsize*mod_entries->mod_nincl;

   rsize_max = mod_entries->entries[0].scat_reff;
   rsize_min = mod_entries->entries[nmodel - 1].scat_reff;
   if(rsize_min == rsize_max) {
      rsize_min = 0;			/* PSF XXX Fix in tables */
   }
   shAssert((rsize_min == rsize_max) ||	/* PSF */
	    (rsize_min != 1 && rsize_max != 0));

#if 0
   aratio_min = mod_entries->entries[nmodel - 1].scat_axr;
#else
   aratio_min = 1/(float)(NDEVINCL - 1); /* the a/b == 0 model is weird */
#endif
}

static float
find_outer(OBJECT1* obj)
{
    float d[4];
    int i;
    float max;

    if(obj->mask == NULL) {
       max = 0;
    } else {
       d[0] = sqrt(pow(obj->mask->cmax - obj->colc + 1,2) +
		   pow(obj->mask->rmax - obj->rowc + 1,2));
       d[1] = sqrt(pow(obj->mask->cmin - obj->colc + 1,2) +
		   pow(obj->mask->rmax - obj->rowc + 1,2));
       d[2] = sqrt(pow(obj->mask->cmax - obj->colc + 1,2) +
		   pow(obj->mask->rmin - obj->rowc + 1,2));
       d[3] = sqrt(pow(obj->mask->cmin - obj->colc + 1,2) +
		   pow(obj->mask->rmin - obj->rowc + 1,2));
       max = d[0];

       for(i = 1; i < 4; i++) {
	  if(d[i] > max)
	    max = d[i];
       }
    }

    if(max < SYNCRAD) {			/* we may as well to the sinc region */
       max = SYNCRAD;
    }

    return(max);
}
static COMP_CSTATS *
find_phi(COMP_CSTATS *stats_model0, /* model at 0 degrees */
	 int nannuli,			/* dimension of data[], sig[] */
	 float *data,			/* object's profile */
	 float *sig)			/* errors in data */
{
   float C[NANN][NSEC/2];		/* coefficients of Fourier expansion */
   static float c_2dtheta[NSEC/2*NSEC/2]; /* == cos(2*i*dtheta) */
   static float s_2dtheta[NSEC/2*NSEC/2]; /* == sin(2*i*dtheta) */
   float data_ivarbar;			/* == data*ivar_bar */
   const float dtheta = 2*M_PI/NSEC;	/* opening angle of a sector */
   static int first = 1;		/* is this the first call? */
   int iann;				/* index for annuli */
   int idata;				/* index for data[] and sig[] */
   int isect;				/* index for sectors */
   float ivarbar;			/* == 1/(sig^2 averaged over annulus)*/
   float *model;			/* models at phi degrees */
   int niter_max = 100;			/* maximum number of Brent iterations*/
   double phi;				/* desired value of position angle */
   int r;				/* index for Fourier order */
   int sect0;				/* sector 0 for a given annulus */
   float sum;
   float tmp;
   double tol = 1e-3;			/* acceptable tolerance for minimiser*/
   
   shAssert(NSEC/2 == 6);		/* hand coded stuff follows */
/*
 * Set the {c,s}_2dtheta[] arrays on first call
 */
   if(first) {
      for(r = 0;r < NSEC/2*NSEC/2;r++) {
	 c_2dtheta[r] = cos(2*r*dtheta);
	 s_2dtheta[r] = sin(2*r*dtheta);
      }
      first = 0;
   }
/*
 * Initialise the Fourier expansion of the errors
 */
   for(r = 0; r < NSEC/2; r++) {
      A[r] = B[r] = 0;
   }
   
   model = use_median ? stats_model0->median : stats_model0->mean;
   memcpy(C[1], &model[1], (nannuli - 1)*NSEC/2*sizeof(float));

   for(sect0 = iann = 1; iann < nannuli; iann++, sect0 += NSEC/2) {
/*
 * Evaluate chi^2 == sum((model - data)/sigma^2)'s Fourier coefficients
 *
 * For the model/sigma^2 to be independent of phi, we have to average
 * the variance in the annulus. This is a Good Thing; it means that we
 * only need keep the sum(model*data/sigma^2) term, and that we therefore
 * don't need to worry about the normalisation of the model at this point
 */
      sum = 0;
      for(isect = 0; isect < NSEC/2; isect++) {
	 idata = sect0 + isect;
	 sum += sig[idata];
      }
      sum /= NSEC/2;
      ivarbar = 1/(sum*sum);
      
      for(isect = 0; isect < NSEC/2; isect++) {
	 idata = sect0 + isect;
	 data_ivarbar = data[idata]*ivarbar;
	 for(r = 0; r < NSEC/2; r++) {
	    tmp = data_ivarbar*C[iann][r];
	    
	    A[r] -= tmp*c_2dtheta[r*isect]; /* == A[r] -= tmp*cos(2*r*theta) */
	    B[r] -= tmp*s_2dtheta[r*isect]; /* == B[r] -= tmp*sin(2*r*theta) */
	 }
      }
   }
/*
 * OK, we have the error Fourier decomposed. Proceed to minimise it
 */
#if DEBUG > 1
   {
      float th[100], E[100];
      int i;
      for(i = 0; i < 100; i++) {
	 th[i] = i*M_PI/100;
	 E[i] = eval_fourier(th[i], NULL);
      }
      shAssert(i == 100);
   }
#endif
   phi = 0;
   if(phBrentMinimum(&phi, 0, M_PI, eval_fourier, NULL, niter_max, tol) != 0) {
      shError("find_phi: minimiser failed to converge");
      return(NULL);
   }
   model_phi = phi;
   model_phiErr = ERROR_IS_BAD;		/* XXX */
/*
 * now that we know phi we can calculate the model at that position angle;
 * put it in cstats0->model
 *
 * The inner loop's equivalent to
    sum += Cann[r]*cos(2*r*(theta - phi));
 *
 */
   for(isect = 0; isect < NSEC/2; isect++, idata++) {
      const float c2phi = cos(2*phi);
      const float s2phi =
		   (2*phi < M_PI) ? sqrt(1-c2phi*c2phi) : -sqrt(1-c2phi*c2phi);
      float c2rphi, s2rphi;		/* {cos,sin}(2*r*phi) */

      for(sect0 = iann = 1; iann < nannuli; iann++, sect0 += NSEC/2) {
	 const float *Cann = C[iann];	/* unalias C */
	 sum = Cann[0];
	 
	 c2rphi = 1;
	 s2rphi = 0;
	 for(r = 1; r < NSEC/2; r++) {
	    tmp = c2rphi*c2phi - s2rphi*s2phi;
	    s2rphi = s2rphi*c2phi + c2rphi*s2phi;
	    c2rphi = tmp;
	    
	    sum += Cann[r]*(c_2dtheta[r*isect]*c2rphi +
						    s_2dtheta[r*isect]*s2rphi);
	 }
	 model[sect0 + isect] = sum;
      }
   }

   return(stats_model0);
}

/*****************************************************************************/
/*
 * Given a model stored as Fourier coefficients, replace it by the values
 * in the cells.
 *
 * Note that this function is _not_ optimised; it's supposed to be used
 * only in non-time-critical places (e.g. finding the best PSF model)
 */
static void
expand_model(float phi,			/* desired position angle */
	     float *model,		/* model to expand */
	     int nannuli)		/* number of annuli in model */
{
   float C[NSEC/2];
   int isect, iann;
   int r;
   int sect0;
   float sum;

   for(sect0 = iann = 1; iann < nannuli; iann++, sect0 += NSEC/2) {
      memcpy(C, &model[1 + (iann - 1)*NSEC/2], NSEC/2*sizeof(float));
      for(isect = 0; isect < NSEC/2; isect++) {
	 sum = C[0];
	 for(r = 1; r < NSEC/2; r++) {
	    sum += C[r]*cos(2*r*(2*M_PI*isect/(float)NSEC - phi));
	 }
	 model[sect0 + isect] = sum;
      }
   }
}

/******************************************************************************/
/*
 * Given an effective radius, return a size index.
 * Assumes that the catalogue is ordered in decreasing size.
 */
static void
mod_re_to_isize(
    double re,
    const MODEL_ENTRIES *entries,
    struct p_model *isize
)
{
    int i;

    shAssert(IORDER_SIZE == 2);
    
    for(i = 0; i < entries->mod_nsize; i++) {
	if(re >= entries->entries[i].scat_reff)
	    break;
    }
    if(i == entries->mod_nsize || entries->mod_nsize == 1) { /* use smallest */
	isize[0].i = 0;
	isize[0].w = 1;
	isize[1].i = 1;
	isize[1].w = 0;
    } else if(i == entries->mod_nsize) { /* use the biggest */
	isize[0].i = i-2;
	isize[0].w = 0;
	isize[1].i = i-1;
	isize[1].w = 1;
    } else {				/* find interpolation coefficients */
       const int i0 = i - 1;
       const int i1 = i;
       const float re0 = entries->entries[i-1].scat_reff;
       const float re1 = entries->entries[i].scat_reff;

       isize[0].i = i0;
       isize[1].i = i1;
/*
 * How should we choose the weights?  If the models all have the same flux
 * then their second moments add and we should use the quadratic formula;
 * but actually the models have the same I_e so we should use a quartic
 * formula.  But it doesn't seem to actually matter much, and we use
 * the linear form for old times' sake.
 */
#if 1
       isize[0].w = (re - re1)/(re0 - re1); /* linear interpolation */
#else
       isize[0].w = (re*re - re1*re1)/(re0*re0 - re1*re1); /* quadratic */
#endif
       isize[1].w = 1.0 - isize[0].w;
       shAssert(isize[1].w >= 0.0)
    }
    
    return;
}

/*
 * Given an axis ratio, return an inclination index.
 * Assumes that the catalogue is ordered in decreasing axis ratio.
 */
static void
mod_aratio_to_iinc(
    double aratio,
    const MODEL_ENTRIES *entries,
    struct p_model *iinc
)
{
    int i;
    int stride;
    
    shAssert(IORDER_INC == 2);

    stride = entries->mod_nsize;
    for(i = 0; i < entries->mod_nincl; i++) {
	if(aratio >= entries->entries[i*stride].scat_axr)
	    break;
    }
    if(i == 0 || entries->mod_nincl == 1) { /* use the largest,
					       i.e. smallest index */
	iinc[0].i = 0;
	iinc[0].w = 1;
	iinc[1].i = 1;
	iinc[1].w = 0;
    } else if(i == entries->mod_nincl) { /* use the smallest,
					    i.e. largest index */
	iinc[0].i = i-2;
	iinc[0].w = 0;
	iinc[1].i = i-1;
	iinc[1].w = 1;
    } else {				/* linear interpolation */
       const int i0 = i - 1;
       const int i1 = i;
       const float axr0 = entries->entries[i0*stride].scat_axr;
       const float axr1 = entries->entries[i1*stride].scat_axr;
       
       iinc[0].i = i0;
       iinc[1].i = i1;

       iinc[0].w = (aratio - axr1)/(axr0 - axr1);
       iinc[1].w = 1 - iinc[0].w;
    }
    
    return;
}


/*****************************************************************************/
/*
 * Read a model from disk, and return a pointer 
 *
 * If the model pointer cs is NULL, you'll be returned a volatile COMP_CSTATS,
 * whose contents may change when model_read() is next called. This is the
 * most efficient way to call this routine
 *
 * If cs isn't NULL, it must contain at least cs->n values (<= MAXCELLS);
 * in general you don't know this number until the function returns. The
 * cs->sig field may be NULL, even if USE_MODEL_SIG is true
 *
 * This function in fact maintains a cache of models, based on hashing the
 * desired index.
 */
static COMP_CSTATS *
model_read(FILE *fd,			/* file to read */
	   const int index,		/* index of model to read */
	   int normalize,		/* normalise profile? */
	   COMP_CSTATS *cs)		/* a COMP_CSTATS with least MAXCELLS,
					   or NULL */
{
	printf("model_read() faked.\n");
	return NULL;
}

/*****************************************************************************/
/*
 * Find the best-fit position angle for a model, and return it
 *
 * First a function to evaluate chi^2's Fourier series. This is equivalent to
 *
   sum = 0;
   for(r = 0; r < NSEC/2;r++) {
      sum += A[r]*cos(2*r*phi) + B[r]*sin(2*r*phi);
   }
 *
 */
static double
eval_fourier(double phi,
	     const void *user)		/* NOTUSED */
{
   const double c2phi = cos(2*phi);
   const double s2phi =
		   (2*phi < M_PI) ? sqrt(1-c2phi*c2phi) : -sqrt(1-c2phi*c2phi);
   double c2rphi, s2rphi;		/* {cos,sin}(2*r*phi) */
   double sum;				/* desired value */
   double tmp;

#if DEBUG > 1				/* an expensive assertion... */
   shAssert(fabs(s2phi - sin(2*phi)) < 1e-5);
#endif

   c2rphi = 1;
   s2rphi = 0;
   sum = A[0];				/* i == 0 */

   c2rphi = c2phi;
   s2rphi = s2phi;
   sum += A[1]*c2rphi + B[1]*s2rphi;	/* i == 1 */
   
   tmp = c2rphi*c2phi - s2rphi*s2phi;
   s2rphi = s2rphi*c2phi + c2rphi*s2phi;
   c2rphi = tmp;
   sum += A[2]*c2rphi + B[2]*s2rphi;	/* i == 2 */

   tmp = c2rphi*c2phi - s2rphi*s2phi;
   s2rphi = s2rphi*c2phi + c2rphi*s2phi;
   c2rphi = tmp;
   sum += A[3]*c2rphi + B[3]*s2rphi;	/* i == 3 */

   tmp = c2rphi*c2phi - s2rphi*s2phi;
   s2rphi = s2rphi*c2phi + c2rphi*s2phi;
   c2rphi = tmp;
   sum += A[4]*c2rphi + B[4]*s2rphi;	/* i == 4 */

   tmp = c2rphi*c2phi - s2rphi*s2phi;
   s2rphi = s2rphi*c2phi + c2rphi*s2phi;
   c2rphi = tmp;
   sum += A[5]*c2rphi + B[5]*s2rphi;	/* i == 5 */
      
   return(sum);
}

/*****************************************************************************/
/*
 * Set the values of a CELL_PROF, given a CELL_STATS. No correction is
 * made for any residual tables that might exist
 *
 * If the CELL_PROF's arrays are too small, they'll be reallocated
 */
void
phCellProfSet(CELL_PROF *cprof,		/* the CELL_PROF to initialise */
	      const CELL_STATS *cstats, /* cell array info */
	      int median,		/* use median profile? */
	      double sky,		/* sky level */
	      double gain,		/* gain of chip */
	      double dark_variance,	/* background variance */
	      double sigma,		/* object's width */
	      double posErr,		/* centering error */
	      int use_difference,	/* include difference of cell
					   pairs in variance? */
	      int sky_noise_only)	/* only include sky noise */
{
    float area1, area2;			/* areas of two cells */
    struct pstats *cell;		/* statistics etc. for a cell */
#define NCORR 7				/* number of corr[] coeffs */
    static float corr[][NCORR] = {
       {				/* 0.8 pixel sigma1 */
	  0.004300, 0.010893, 0.004389, 0.001474, 0.000401, 0.000049, 0.000000
       },
       {				/* 1.0 pixel sigma */
	  0.005069, 0.003796, 0.002311, 0.000892, 0.000294, 0.000081, 0.000007
       },
       {				/* <= 1% correction */
	  0.010000, 0.010000, 0.010000, 0.010000, 0.005000, 0.002000, 0.000000
       },
    };					/* sinc-corrections */
    static int icorr = 1;		/* which corr[] to use */
    int i, j, k;
    float *area, *data, *sig;		/* unaliased from cprof */
    const int nann = cstats->nannuli_c;
    float rmean;			/* mean radius of a cell */
    float val1, val2;			/* values of mean or median */
    float var_diff;			/* variance from val1 - val2 */
    float var_photon;			/* variance from poisson noise */
    float var;				/* variance in a value */
#if 1					/* XXX */
    static float var_min_frac = 1e-2;	/* variance cannot fall below
					   var_min_frac*|data| */
    static int diff_errors = 0;		/* use diff errors? */
    static float sinc_fac = 1.0;	/* fiddle factor for sinc errors */
    static float centering_fac = 0.1;	/* fiddle factor for centering errs */
#endif
    use_difference = diff_errors;	/* XXX overrides argument list */
    
    shAssert(cprof != NULL && cstats != NULL);
    shAssert(sigma != 0.0 || posErr == 0.0);

    if(sigma == 0.0) {
       sigma = 1;			/* avoid division by zero */
    }
/*
 * See if we need more storage, and if so get it
 */
    phCellProfRealloc(cprof, 1 + NSEC/2*(nann - 1));

    area = cprof->area;
    data = cprof->data;
    sig = cprof->sig;
    cprof->is_median = median;
/*
 * Estimates error from the counts in the data and sky, plus a number
 * of other terms that (can) contribute to the variance;
 * their effect is controlled by some arguments and scaling variables
 * (see comments for argument and auto variables).  Note that if the
 * terms other than the sky are included, the fits and thus fluxes are
 * liable to vary systematically with the brightness of the object
 * relative to the sky.
 * 
 * Estimate error from the difference between val1 and val2; the variance
 * of val1 or val2 is 0.5*(val1 - val2)^2, so the variance of the mean
 * is 0.25*(val1 - val2)^2. This term is suppressed if use_difference is false
 *
 * If this is too small, use the Poisson noise from the photon statistics.
 * The total number of electrons is
 *	(area1*(val1 + sky) + area2*(val2 + sky))*gain
 * so the variance of (val1 + val2)/2 (in DN) is
 *	(area1*(val1 + sky) + area2*(val2 + sky))/((area1 + area2)^2*gain)
 * (neglecting the dark current)
 *
 * There is yet another contribution to the variance, namely that due to
 * sinc-shifting. This is taken to be a multiple of the central intensity,
 * with the multiple given by the array corr[] (which was estimated from
 * the results of experiments with a double Gaussian psf).
 *
 * There is (yet another)^2 contribution from centering errors, proportional
 * to the central intensity multiplied by the PSF's gradient.
 *
 * The quoted value of cell->sig includes the contribution to the variance
 * due to the intrinsic gradients, and cannot be used.
 *
 * do the central cell first; it alone has no pair to be coupled with
 */
   cell = &cstats->cells[0];
   if((val1 = median ? cell->qt[1] : cell->mean) < -999) {
      data[0] = 0;
   } else {
      data[0] = val1;
   }
   area[0] = cell->area;

   if(sky_noise_only) {
      var_photon = sky/gain + dark_variance;
      var_photon /= cell->area;

      var = var_photon;
   } else {
      if((var_photon = (val1 + sky)/gain + dark_variance) > 0) {
	 var_photon /= cell->area;
      } else {
	 var_photon = 1e10;
      }
      var = var_photon;
      var += sinc_fac*pow(data[0]*corr[icorr][0],2);
      var += pow(var_min_frac*data[0], 2);
   }

   sig[0] = sqrt((var < 1e-5) ? 1e-5 : var);
/*
 * And now the rest of the annuli
 */
   for(i = 1;i < nann;i++) {
      for(j = 0;j < NSEC/2;j++) {
	 k = 1 + NSEC*(i - 1) + j;
	 cell = &cstats->cells[k];
	 val1 = median ? cell->qt[1] : cell->mean;
	 area1 = cell->area;
	 
	 cell = &cstats->cells[k + NSEC/2];
	 val2 = median ? cell->qt[1] : cell->mean;
	 area2 = cell->area;

	 k = 1 + NSEC/2*(i - 1) + j;
	 data[k] = 0.5*(val1 + val2);
	 area[k] = area1 + area2;
	 
	 if(sky_noise_only) {
	    var_photon = (area1 + area2)*(sky/gain + dark_variance);
	    var_photon /= (area1 + area2)*(area1 + area2);

	    var = var_photon;
	 } else {
	    var_photon = (area1*(val1 + sky) + area2*(val2 + sky))/gain +
						 dark_variance*(area1 + area2);
	    var_photon /= (area1 + area2)*(area1 + area2);
	    
	    if(var_photon < 1e-3) {	/* i.e. -ve */
	       var_photon = 1e10;
	    }
	    
	    if(use_difference) {
	       var_diff = 0.25*(val1 - val2)*(val1 - val2);
	       var = (var_diff > var_photon) ? var_diff : var_photon;
	    } else {
	       var = var_photon;
	    }
/*
 * add in sinc-shift errors
 */
	    if(i < NCORR) {
	       var += sinc_fac*pow(data[0]*corr[icorr][i],2);
	    }
/*
 * and add in the centering errors
 */
	    rmean = cstats->mgeom[k].rmean;
	    var += centering_fac*pow(data[k]*rmean*posErr/(sigma*sigma), 2);
/*
 * finally, add in the floor to the variance
 */
	    var += pow(var_min_frac*data[k], 2);
	 }
	 shAssert(var == var);
	 
	 sig[k] = sqrt((var < 1e-5) ? 1e-5 : var);
      }
   }
}

//////////////////////////// Read catalog....
// this also pulls in shCRegUtils.h
#include "shCFitsIo.h"
#include "phFitObj.h"
////////Constants from makeprof.c:
int NEXPSIZ = 30, NDEVSIZ = 30;
////////////////
static spro_cheader *prof_cat = NULL;
static CELLPROCATHEADER cellProCatHead;
static COMP_CSTATS *model_cache_arr[NCOLOR] = {NULL, NULL, NULL, NULL, NULL};
static int psf_cache_size = -1;		/* number of PSFs in psf_cache */
static int psf_cache_ncells = -1;	/* number of cells in PSFs */
#define NHASH (1U << 16)		/* size of read cache table;
					   must be 2^n */
static struct {
   int index;				/* index of model */
   COMP_CSTATS cs;			/* the model itself */
} read_cache[NHASH];

static COMP_CSTATS *
comp_cstatsNew(int ncells);
static int
seeing_cells_get(PSF_CACHE *psf_cache,	/* Put model here */
		 int ind,		/* desired model */
		 int single);		/* make a single component model? */


/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Initialize data structures for the fitting routines.  For now this
 * means initializing the catalogue data structures.
 *
 * If file is NULL the previous file will be used; if psf is NULL the
 * only effect of this initialisation is to set the file
 */
int
phInitCellFitobj(char *file,		/* catalog file */
		 int nfilter,		/* number of filters */
		 int median)		/* use median not mean? */
{
    int i,j;
    int nitem;
    float (*ptr)[MAXCELLS];		/* used to set up cache pointers */

    use_median = median;		/* copy to global */
    
    if(file == NULL || *file == '\0') {
       file = catalog_file;		/* use previous catalog file */
    }

    if(strcmp(file, catalog_file) != 0) {
       phFiniCellFitobj();

       if(phProfileTableHeaderCheck(file, 1, 0) != SH_SUCCESS) {
	  return(SH_GENERIC_ERROR);
       }

       catalog_fd = fopen(file, "rb");
       shAssert(catalog_fd != NULL);	/* we just checked the file's header */
/*
 * Read in header.
 */
       nitem = fread(&cellProCatHead, sizeof(cellProCatHead), 1, catalog_fd);
       shAssert(nitem == 1);

       nitem = p_phReadCatentries(&cellProCatHead.prof_cat, catalog_fd);
       shAssert(nitem == MAXSCAT);
/*
 * Set up the scratch COMP_CSTATS that we use for returning models,
 * and ensure that the model cache is empty
 */
       model_cs = comp_cstatsNew(MAXCELLS);
       model_cs->ncells = -1;
/*
 * Read in offsets.
 */
       model_off = shMalloc(sizeof(*model_off)*cellProCatHead.max_models);
       nitem = fread(model_off, sizeof(*model_off),
		     cellProCatHead.max_models, catalog_fd);
       shAssert(nitem == cellProCatHead.max_models);

       strncpy(catalog_file,file,NAMELEN);
/*
 * Setup caches.
 *
 * First the model cache, of models that have already been interpolated to
 * the current seeing, so we need a total of
 *  proc_maxscat == 1 + proc_nexpincl*proc_nexpsiz + proc_ndevincl*proc_ndevsiz
 * for each orientation (in each band); a total per band of cache_size
 */
       shAssert(nfilter >= 1);

       if(cache_size < 0) {		/* first call */
	  for(i = 0;i < NCOLOR;i++) {
	     model_cache_arr[i] = NULL;
	  }
       }       
       
       cache_size = cellProCatHead.prof_cat.proc_maxscat;
       for(i = 0;i < nfilter;i++) {
	  if(model_cache_arr[i] == NULL) {
	     model_cache = model_cache_arr[i] =
				      shMalloc(cache_size*sizeof(COMP_CSTATS));
	     ptr = shMalloc((USE_MODEL_SIG ? 2:1)*cache_size*
						       MAXCELLS*sizeof(float));
	     
	     for(j = 0; j < cache_size; j++) {
		model_cache[j].ncells = -1;
		model_cache[j].totflux = 0.0;
		model_cache[j].mem = *ptr++;
		model_cache[j].median = model_cache[j].mean = NULL;
#if USE_MODEL_SIG
		model_cache[j].sig = *ptr++;
#else
		model_cache[j].sig = NULL;
#endif
	     }
	  }
       }
/*
 * now the read cache, of models as read from disk
 */
       ptr = shMalloc((USE_MODEL_SIG ? 2:1)*NHASH*MAXCELLS*sizeof(float));
       for(i = 0;i < NHASH;i++) {
	  read_cache[i].index = -1;
	  read_cache[i].cs.totflux = 0.0;
	  read_cache[i].cs.mem = *ptr++;
	  read_cache[i].cs.median = read_cache[i].cs.mean = NULL;
#if USE_MODEL_SIG
	  read_cache[i].cs.sig = *ptr++;
#else
	  read_cache[i].cs.sig = NULL;
#endif
       }	  
    }
/*
 * Now the cache of all PSFs in the catalogue
 */
    psf_cache_size = cellProCatHead.nseeing;
    psf_cache = shMalloc(psf_cache_size*sizeof(PSF_CACHE));
    for(i = 0; i < psf_cache_size; i++) {
       int ncells, nannuli;
       for(j = 0; j <= 1; j++) {	/* (j == 1) ? single : double */
	  ncells = seeing_cells_get(&psf_cache[i], i, j);
	  nannuli = (ncells - 1)/(NSEC/2) + 1;
	  expand_model(0, psf_cache[i].model[j], nannuli);
	  
	  if(psf_cache_ncells < 0) {
	     psf_cache_ncells = ncells;
	  } else {
	     shAssert(psf_cache_ncells == ncells);
	  }
      }
   }
/*
 * Set up extra pointers.
 */
    prof_cat = &cellProCatHead.prof_cat;

    psf_entries.mod_nincl = 1;
    psf_entries.mod_nsize = 1;
    psf_entries.mod_offset = 0;
    psf_entries.entries = prof_cat->proc_catentry[psf_entries.mod_offset];

    exp_entries.mod_nincl = prof_cat->proc_nexpincl;
    exp_entries.mod_nsize = prof_cat->proc_nexpsiz;
    exp_entries.mod_offset = psf_entries.mod_offset +
				 psf_entries.mod_nincl*psf_entries.mod_nsize;
    exp_entries.entries = prof_cat->proc_catentry[exp_entries.mod_offset];

    dev_entries.mod_nincl = prof_cat->proc_ndevincl;
    dev_entries.mod_nsize = prof_cat->proc_ndevsiz;
    dev_entries.mod_offset = exp_entries.mod_offset +
				 exp_entries.mod_nincl*exp_entries.mod_nsize;
    dev_entries.entries = prof_cat->proc_catentry[dev_entries.mod_offset];

    return(SH_SUCCESS);
}

static COMP_CSTATS *
comp_cstatsNew(int ncells)
{
   COMP_CSTATS *cs = shMalloc(sizeof(COMP_CSTATS));
   cs->mem = shMalloc((USE_MODEL_SIG ? 2 : 1)*sizeof(float)*ncells);
   cs->sig = USE_MODEL_SIG ? cs->mem + MAXCELLS : NULL;
   cs->ncells = ncells;
   cs->totflux = 0.0;
   
   if(use_median) {
      cs->mean = NULL; cs->median = cs->mem;
   } else {
      cs->mean = cs->mem; cs->median = NULL;
   }
   memset(cs->mem, '\0', (USE_MODEL_SIG ? 2 : 1)*sizeof(float)*ncells);

   return(cs);
}

/*****************************************************************************/
/*
 * return a given PSF model from the cell table
 */
static int
seeing_cells_get(PSF_CACHE *psf_cache,	/* Put model here */
		 int ind,		/* desired model */
		 int single)		/* make a single component model? */
{
   const float b = single ? 0 : cellProCatHead.seeing_b;
   COMP_CSTATS cs;			/* read PSF into this struct */
   float flux_1, flux_2;		/* flux in the two components of PSF */
   float *model;			/* the desired model */
   float sigma2_1, sigma2_2;		/* second moments of Gaussian comps. */
   int tind;				/* index into table */
   float totflux;			/* == flux_1 + flux_2 */
   
   shAssert(ind >= 0 && ind < cellProCatHead.nseeing);
/*
 * Find the correct index in the table
 */
   if(single) {
      tind = (2*ind + 1)*cellProCatHead.prof_cat.proc_maxscat;
   } else {
      tind = (2*ind)*cellProCatHead.prof_cat.proc_maxscat;
   }
   tind += psf_entries.mod_offset;
/*
 * we need a COMP_CSTATS to read into
 */
   model = cs.mem = shMalloc(MAXCELLS*sizeof(float));
   cs.sig = NULL;
/*
 * and read the model
 */
   shAssert(catalog_fd != NULL);

   (void)model_read(catalog_fd, tind, 1, &cs);
/*
 * Find that model PSF's quadratic moment, sigma2
 */
   sigma2_1 = pow(cellProCatHead.fseeing + ind*cellProCatHead.dseeing, 2);
   sigma2_2 = pow(cellProCatHead.seeing_ratio, 2)*sigma2_1;

   flux_1 = sigma2_1;			/* flux in first component */
   flux_2 = b*sigma2_2;			/* flux in second component */

#if 0
   totflux = flux_1 + flux_2;		/* before CORRECT_MODEL_MOMENTS */
#else
   totflux = cs.totflux;
#endif
/*
 * sigma2 is only determined to be from pixels within a radius of QMOMENT_RMAX;
 * correct the PSF's sigmas for this effect.
 *
 * If QMOMENT_RMAX <= 0, it's taken to be infinite
 */
#if CORRECT_MODEL_MOMENTS
   if(QMOMENT_RMAX > 0) {
      float corr, corr_Q;		/* correction for flux and 2nd moment*/
      float e;				/* == exp(-rmax^2/2sigma^2) */
      
      e = exp(-QMOMENT_RMAX*QMOMENT_RMAX/(2*sigma2_1));
      corr = (1 - e);
      corr_Q = (1 - (1 + QMOMENT_RMAX*QMOMENT_RMAX/(2*sigma2_1))*e)/corr;

      flux_1 *= corr;
      sigma2_1 *= corr_Q;

      if(!single) {
	 e = exp(-QMOMENT_RMAX*QMOMENT_RMAX/(2*sigma2_2));
	 corr = (1 - e);
	 corr_Q = (1 - (1 + QMOMENT_RMAX*QMOMENT_RMAX/(2*sigma2_2))*e)/corr;
	 
	 flux_2 *= corr;
	 sigma2_2 *= corr_Q;
      }
   }
#endif
/*
 * pack up answers
 */
   psf_cache->model[single ? 1 : 0] = model;
   psf_cache->totflux[single ? 1 : 0] = totflux;
   psf_cache->sigma2[single ? 1 : 0] =
			 (flux_1*sigma2_1 + flux_2*sigma2_2)/(flux_1 + flux_2);

   return(cs.ncells);
}

const char *
phFormatCatentry(const spro_catentry *entry)
{
    static char descrip[100];
    
    switch (entry->scat_class) {
      case DEV_MODEL:
	sprintf(descrip, "deV (%6.2f, %4.2f)", entry->scat_reff, entry->scat_axr);
	break;
      case EXP_MODEL:
	sprintf(descrip, "exp (%.2f, %.2f)", entry->scat_reff, entry->scat_axr);
	break;
      case PSF_MODEL:
	sprintf(descrip, "psf");
	break;
      default:
	abort();
	break;				/* NOTREACHED */
    }

    return descrip;
}

