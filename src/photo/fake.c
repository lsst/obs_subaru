#include <string.h>
#include <math.h>
#include "phFake.h"
#include "phMathUtils.h"

#include <assert.h>

#include <stdarg.h>
/*void trace(const char* fmt, ...) {
 va_list lst;
 printf("%s:%i ", __FILE__, __LINE__);
 va_start(lst, fmt);
 vprintf(fmt, lst);
 va_end(lst);
 }
 */
void phTrace(const char* fn, int line, const char* fmt, ...) {
	va_list lst;
	printf("%s:%i ", fn, line);
	va_start(lst, fmt);
	vprintf(fmt, lst);
	va_end(lst);
}

#include "phObjc.h"

#define printflag(flags, pre, f) if (flags & (pre ## f)) {	\
		printf("    %s\n", #f);					\
	}
static void printAnObjc(OBJC* o, int childnum) {
	printf("Objc");
	if (childnum >= 0) {
		printf(" (child %i)", childnum);
	}
	printf(":\n  parent %p\n  children %p\n  sibbs %p\n",
		   o->parent, o->children, o->sibbs);
    printf("  ncolor %i\n", o->ncolor);
    printf("  colc,rowc %g,%g\n", o->colc, o->rowc);
	printf("  flags:\n");
	printflag(o->flags, OBJECT1_, CANONICAL_CENTER);
	printflag(o->flags, OBJECT1_, BRIGHT);
	printflag(o->flags, OBJECT1_, EDGE);
	printflag(o->flags, OBJECT1_, BLENDED);
	printflag(o->flags, OBJECT1_, CHILD);
	printflag(o->flags, OBJECT1_, PEAKCENTER);
	printflag(o->flags, OBJECT1_, NODEBLEND);
	printflag(o->flags, OBJECT1_, NOPROFILE);
	printflag(o->flags, OBJECT1_, NOPETRO);
	printflag(o->flags, OBJECT1_, MANYPETRO);
	printflag(o->flags, OBJECT1_, NOPETRO_BIG);
	printflag(o->flags, OBJECT1_, DEBLEND_TOO_MANY_PEAKS);
	printflag(o->flags, OBJECT1_, CR);
	printflag(o->flags, OBJECT1_, MANYR50);
	printflag(o->flags, OBJECT1_, MANYR90);
	printflag(o->flags, OBJECT1_, BAD_RADIAL);
	printflag(o->flags, OBJECT1_, INCOMPLETE_PROFILE);
	printflag(o->flags, OBJECT1_, INTERP);
	printflag(o->flags, OBJECT1_, SATUR);
	printflag(o->flags, OBJECT1_, NOTCHECKED);
	printflag(o->flags, OBJECT1_, SUBTRACTED);
	printflag(o->flags, OBJECT1_, NOSTOKES);
	printflag(o->flags, OBJECT1_, BADSKY);
	printflag(o->flags, OBJECT1_, PETROFAINT);
	printflag(o->flags, OBJECT1_, TOO_LARGE);
	printflag(o->flags, OBJECT1_, DEBLENDED_AS_PSF);
	printflag(o->flags, OBJECT1_, DEBLEND_PRUNED);
	printflag(o->flags, OBJECT1_, ELLIPFAINT);
	printflag(o->flags, OBJECT1_, BINNED1);
	printflag(o->flags, OBJECT1_, BINNED2);
	printflag(o->flags, OBJECT1_, BINNED4);
	printflag(o->flags, OBJECT1_, MOVED);

	printflag(o->flags2, OBJECT2_, DEBLENDED_AS_MOVING);
	printflag(o->flags2, OBJECT2_, NODEBLEND_MOVING);
	printflag(o->flags2, OBJECT2_, TOO_FEW_DETECTIONS);
	printflag(o->flags2, OBJECT2_, BAD_MOVING_FIT);
	printflag(o->flags2, OBJECT2_, STATIONARY);
	printflag(o->flags2, OBJECT2_, PEAKS_TOO_CLOSE);
	printflag(o->flags2, OBJECT2_, BINNED_CENTER);
	printflag(o->flags2, OBJECT2_, LOCAL_EDGE);
	printflag(o->flags2, OBJECT2_, BAD_COUNTS_ERROR);
	printflag(o->flags2, OBJECT2_, BAD_MOVING_FIT_CHILD);
	printflag(o->flags2, OBJECT2_, DEBLEND_UNASSIGNED_FLUX);
	printflag(o->flags2, OBJECT2_, SATUR_CENTER);
	printflag(o->flags2, OBJECT2_, INTERP_CENTER);
	printflag(o->flags2, OBJECT2_, DEBLENDED_AT_EDGE);
	printflag(o->flags2, OBJECT2_, DEBLEND_NOPEAK);
	printflag(o->flags2, OBJECT2_, PSF_FLUX_INTERP);
	printflag(o->flags2, OBJECT2_, TOO_FEW_GOOD_DETECTIONS);
	printflag(o->flags2, OBJECT2_, CENTER_OFF_AIMAGE);
	printflag(o->flags2, OBJECT2_, DEBLEND_DEGENERATE);
	printflag(o->flags2, OBJECT2_, BRIGHTEST_GALAXY_CHILD);
	printflag(o->flags2, OBJECT2_, CANONICAL_BAND);
	printflag(o->flags2, OBJECT2_, AMOMENT_UNWEIGHTED);
	printflag(o->flags2, OBJECT2_, AMOMENT_SHIFT);
	printflag(o->flags2, OBJECT2_, AMOMENT_MAXITER);
	printflag(o->flags2, OBJECT2_, MAYBE_CR);
	printflag(o->flags2, OBJECT2_, MAYBE_EGHOST);
	printflag(o->flags2, OBJECT2_, NOTCHECKED_CENTER);
	printflag(o->flags2, OBJECT2_, HAS_SATUR_DN);
	printflag(o->flags2, OBJECT2_, DEBLEND_PEEPHOLE);
	printflag(o->flags2, OBJECT2_, NOT_DEBLENDED_AS_PSF);
	printflag(o->flags2, OBJECT2_, SUBTRACTED_WINGS);

	printflag(o->flags3, OBJECT3_, HAS_SATUR_DN);
	printflag(o->flags3, OBJECT3_, MEASURED);
	printflag(o->flags3, OBJECT3_, GROWN_MERGED);
	printflag(o->flags3, OBJECT3_, HAS_CENTER);
	printflag(o->flags3, OBJECT3_, MEASURE_BRIGHT);

	printf("  N children: %i\n", o->nchild);
	if (o->nchild && o->children) {
		printAnObjc(o->children, 0);
	} 

	if (o->sibbs) {
		printAnObjc(o->sibbs, childnum+1);
	}
}
#undef printflag
void printObjc(OBJC* o) {
	printAnObjc(o, -1);
}


// for MODEL_PARAMS
#include "phFitobj.h"

static float find_outer(OBJECT1* obj);
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
	;
static CELL_PROF fit_ctx;
static void
cell_fit_psf(
    int ndata,
    int nparam,
    double *p,
    double *fvec,
    int *iflag
    );
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
static void
cell_fit_model(
    COMP_CSTATS *(*cells_make)(const MODEL_PARAMS *),
    int ndata,
    int nparam,
    double *p,
    double *fvec,
    int *iflag
    );
static int use_median = 1;		/* use median not mean profile */
static int fit_sky_level = 0;		/* should we fit the sky? */
static float sky_level;			/* the resulting value */
static float totflux;			/* estimated total flux in an object */
static float totfluxErr;		/* error in total flux */

static float rsize_min, rsize_max;	/* extremal values of rsize for
									 available models */
static float aratio_min;		/* minimum value of rsize for
								 available models */
//static int fit_phi = 0;			/* should we fit the position angle? */
static float model_phi;			/* position angle of best model (rad)*/
static float model_amp;			/* amplitude of best-fit model */
static float model_ampErr;		/* error in amplitude */
static PSF_COEFFS *seeing_ind = NULL;	/* current seeing */


/*
 * a suite of models for e.g. exponential disks
 */
typedef struct {
    int mod_nsize;
    int mod_nincl;
    int mod_offset;
    spro_catentry *entries;
} MODEL_ENTRIES;
#define IORDER_INC 2			/* "inclination", really b/a */
#define IORDER_SIZE 2			/* size */

struct p_model {
    int i; 	/* index of model */
    float w;	/* weight to be given to this model */
};
static COMP_CSTATS *model_cs = NULL;	/* COMP_CSTATS for the current model */
static void
expand_model(float phi,			/* desired position angle */
			 float *model,		/* model to expand */
			 int nannuli)		/* number of annuli in model */
	;
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
	;
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
phFitCellAsPsfFake(OBJC *objc,		/* Object to fit */
				   int color,		/* color of object */
				   const CELL_STATS *cstats, /* cell array */
				   const FIELDPARAMS *fiparams, /* describe field */
				   int nannuli,		/* number of annuli to use */
				   int sky_noise_only,	/* only consider sky noise */
				   float *I0,		/* central intensity; can be NULL */
				   float *bkgd)		/* the sky level; can be NULL */
{
	int use_difference = 0; /* include difference of cell pairs
							 in cell variance */
	float chisq;
	int nu;

	printf("faking phFitCellAsPsf.\n");
	// called from deblend.c:
	//phFitCellAsPsfFake(child, c, NULL, fiparams, psf_nann[c],
	//                   sky_noise_only, &I0, &pedestal);
	// sky_noise_only=0
	printf("color: %i\n", color);
	printf("nann: %i\n", nannuli);

	// phFitCellAsKnownModel
	{
#define NPARAM 8			/* max number of parameters to fit */

			//void (*cell_fit_func)(int, int, double *, double *, int *) = NULL;
		const FRAMEPARAMS *fparams;		/* unpacked from fiparams */
		double *fvec;			/* residuals for cells */
		int i;
		OBJECT1 *obj;			/* object in question */
		double p[NPARAM];			/* model parameters */
		int npar = 0;			/* number of parameters in p */
		float posErr;			/* error in position */
		const REGION *reg;			/* unpacked from fparams */

		float thecounts, thecountserr;

		float* sky = bkgd;
		float *counts = &thecounts;
		float *countsErr = &thecountserr;

		shAssert(fiparams != NULL && objc != NULL);
		shAssert(color >= 0 && color < fiparams->ncolor && color < objc->ncolor);
		fparams = &fiparams->frame[color];
		shAssert(fparams != NULL && fparams->data != NULL && fparams->psf != NULL);
		obj = objc->color[color];
		shAssert(obj != NULL);

		reg = fparams->data;

		rsize_max = rsize_min = 0;
		aratio_min = 0;
		//set_rsize_limits(&psf_entries);
		//cell_fit_func = cell_fit_psf;
		// npar = 0.

		if(cstats == NULL) {
			printf("phProfileExtract...\n");
			cstats = (const CELL_STATS *)
				phProfileExtract(-1, -1, reg, obj->rowc, obj->colc, find_outer(obj),
								 SOFT_BIAS + fparams->bkgd, obj->skyErr, 0);
			if(cstats == NULL || cstats->syncreg == NULL) {
				shErrStackPush("phFitCellAsKnownModel: "
							   "object is too close to edge (%.3f, %.3f)",
							   obj->rowc, obj->colc);
				nu = 1;
				if(sky != NULL) {
					*sky = 0;
				}
				return(-1);
			}
			trace("Before phProfileMean: cs->ncell %i\n", cstats->ncell);
			obj->profMean[0] = phProfileMean(cstats, 0, 0, 1, NULL);
			obj->nprof = cstats->nannuli_c;
			trace("After phProfileMean: cs->ncell %i\n", cstats->ncell);
		}
		if(obj->rowcErr < 0) {
			posErr = 1;
		} else {
			posErr = sqrt(pow(obj->rowcErr,2) + pow(obj->colcErr,2));
		}

		{ 
			int k;
			printf("cell stats:\n");
			printf("  ncell %i\n", cstats->ncell);
			printf("  nannuli %i\n", cstats->nannuli);
			printf("  nannuli_c %i\n", cstats->nannuli_c);
			printf("  annular %i\n", cstats->annular);
			printf("  col_c, row_c %g, %g\n", cstats->col_c, cstats->row_c);
			printf("  col_1, row_1 %g, %g\n", cstats->col_1, cstats->row_1);
			printf("  orad %i\n", cstats->orad);
			printf("  radii:");
			for (k=0; k<cstats->nannuli; k++)
				printf(" %g", cstats->radii[k]);
			printf("\n");
			printf("  cells:\n");
			for (k=0; k<cstats->ncell; k++) {
				int m;
				struct pstats* p = cstats->cells + k;
				printf("    %i: ntot %i, area %g, mean %g, sig %g, sum %g\n",
					   k, p->ntot, p->area, p->mean, p->sig, p->sum);
				printf("       flg: 0x%x\n", p->flg);
				printf("       data:");
				for (m=0; m<p->ntot; m++)
					printf(" %i", (int)p->data[m]);
				printf("\n");
			}
		}

		printf("setup_cell_data: use_median %i, sky %g, gain %g, dark_var %g, object's width sigma %g, poserr %g, use_diff %i, sky_noise_only %i\n",
			   use_median, obj->sky, phGain(fparams, obj->rowc, obj->colc),
			   phDarkVariance(fparams, obj->rowc, obj->colc),
			   fparams->psf->width, posErr, use_difference, sky_noise_only);
		// fparams->dark_variance -- BINREGION*
		// -- can be a single value, or >= 2x2.

		// note, this calls:
		//   phCellProfSet()
		//   and puts results in global "fit_ctx"

		setup_cell_data(cstats, use_median, obj->sky,
						phGain(fparams, obj->rowc, obj->colc),
						phDarkVariance(fparams, obj->rowc, obj->colc),
						fparams->psf->width, posErr, use_difference, sky_noise_only);
		trace("After setup_cell_data: cs->ncell %i\n", cstats->ncell);

		// set from SDSSDeblender.cc, = 3.
		trace("nannuli=%i\n", nannuli);

		if(nannuli > 0) {
			int ncell = 1 + (nannuli - 1)*(NSEC/2);
			trace("ncell=%i, fit_ctx.ncell=%i\n", ncell, fit_ctx.ncell);
			if(fit_ctx.ncell > ncell) {
				fit_ctx.ncell = ncell;
			}
			trace("ncell=%i, fit_ctx.ncell=%i\n", ncell, fit_ctx.ncell);
		}
		trace("fit_ctx.ncell=%i\n", fit_ctx.ncell);
/*
 * find the residuals vector, optionally fitting a sky level
 */
		fit_sky_level = (sky == NULL) ? 0 : 1;
		fvec = alloca(fit_ctx.ncell*sizeof(double));
		i = 0;
		//cell_fit_func(fit_ctx.ncell, npar, p, fvec, &i);
		cell_fit_psf(fit_ctx.ncell, npar, p, fvec, &i);
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

		nu = fit_ctx.ncell - 1;		/* we _didn't_ fit npar parameters */
		if(fit_sky_level) nu--;

		trace("chisq %g, nu %i\n", chisq, nu);

	}

   OBJECT1 *const obj1 = objc->color[color];

   obj1->chisq_star = chisq;
   obj1->nu_star = nu;
   obj1->star_lnL = phChisqProb(obj1->chisq_star, obj1->nu_star, 1);
   obj1->star_L = exp(obj1->star_lnL);
   if (I0 != NULL) {
	   // global
	   *I0 = model_amp;
	   trace("setting I0 = model_amp = %g\n", model_amp);
   }
   
   trace("returning %g\n", (obj1->chisq_star / obj1->nu_star));
   return (obj1->chisq_star / obj1->nu_star);
}

// from cellfitobj.c
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


#if 0
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
    
    memset(cs->mem,'\0',sizeof(float)*MAXCELLS);
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
		if(cstmp->ncells > cs->ncells) cs->ncells = cstmp->ncells;

		cs->totflux += w*cstmp->totflux;
		for(i = cstmp->ncells - 1;i >= 0; i--) {
			cs_model[i] += cstmp_model[i]*w;
		}
    }

    return cs;
}
#endif

static int fcompare(const void* v1, const void* v2) {
	const float* f1 = v1;
	const float* f2 = v2;
	if (*f1 < *f2)
		return -1;
	if (*f1 == *f2)
		return 0;
	return 1;
}

static float find_median_nonconst(float* x, int N) {
	if (N == 0)
		return 0.0;
	qsort(x, N, sizeof(float), fcompare);
	if (N % 2 == 1)
		return x[N/2];
	assert(N/2-1 >= 0);
	return (x[N/2 - 1] + x[N/2]) /2.0;
}

static COMP_CSTATS *
psf_cells_make(const MODEL_PARAMS *p)	/* parameters of model to create */
{
	//return model_cells_make(p, &psf_entries);

#if 0
typedef struct {
    int clazz;			/* class of model (psf, exp, deV) */
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

typedef struct {
   int ncell;				/* number of cells in {data,sig} */
   int is_median;			/* is data median profile? */
   float flux;				/* == sum(data*area) */
   float data[MAXCELLS];	        /* mean or median profile */
   float sig[MAXCELLS];			/* errors in data */
   float area[MAXCELLS];		/* number of pixels in data[] */
} CELL_PROF;				/* pragma SCHEMA */
//(global fit_ctx)

#endif

COMP_CSTATS* cs = NULL;
int nann;
int ncell;
int k;
const CELL_STATS* geom;
double maxrad;
int xlo, xhi, ylo, yhi;
// subpixel shift of the center: [0,1).
 float dx, dy;
 float x,y;
 float step;
 float** pix = NULL;
 int* npixels = NULL;
 int npix;
// SUPER-HACK
 float psfsigma = 2.0; // pixels, matching Deblender.cc:"s"
 int i;

 dx = dy = 0.0;
 // subpixelization
 step = 0.25;

trace("psf_cells_make():\n");
cs = calloc(1, sizeof(COMP_CSTATS));
trace("fit_ctx: ncell %i, flux %g\n", fit_ctx.ncell, fit_ctx.flux);

ncell = fit_ctx.ncell;
nann = (ncell-1)/(NSEC/2) + 1;
geom = phProfileGeometry();

trace("  radii:");
for (k=0; k<=nann; k++)
	printf(" %g", geom->radii[k]);
printf("\n");
// outer radius.
maxrad = geom->radii[nann];

trace("median? %i\n", fit_ctx.is_median);
// (yes, median)

	 ylo = xlo = floor(-maxrad);
	 yhi = xhi = ceil(maxrad);

     int nstep = ceil(1./step) * ceil(1./step);

	 npix = (yhi - ylo + 1) * (xhi - xlo + 1) * nstep;

     trace("maxrad = %g\n", maxrad);
     trace("lo: %i, hi: %i\n", xlo, xhi);
     trace("npix: %i\n", npix);

	 // pixel lists...
	 pix = malloc(ncell * sizeof(float*));
	 npixels = calloc(ncell, sizeof(int));
	 for (i=0; i<ncell; i++)
		 pix[i] = malloc(npix * sizeof(float));

	 int NX = ceil(1 + (xhi - xlo)/step);
	 int NY = ceil(1 + (yhi - ylo)/step);
	 int* cellid = malloc(NX * NY * sizeof(int));
	 int* phcellid = malloc(NX * NY * sizeof(int));
	 int ix, iy = -1;

	 for (y=ylo; y<=yhi; y+=step) {
		 iy++;
		 ix = -1;
		 for (x=xlo; x<=xhi; x+=step) {
			 int ri;
			 int si;
			 int ci;
			 double theta;
			 double G;
			 // in which annulus does it belong?
			 double r = sqrt((dx-x)*(dx-x) + (dy-y)*(dy-y));

			 // DEBUG -- cell ids
			 ix++;
			 assert(ix >= 0);
			 assert(ix < NX);
			 assert(iy >= 0);
			 assert(iy < NY);
			 cellid[iy * NX + ix] = -1;
			 phcellid[iy * NX + ix] = -1;

			 if (r > maxrad)
				 continue;
			 for (ri=0;; ri++)
				 if (r < geom->radii[ri+1])
					 break;
			 assert(ri < nann);
			 // in which sector does this pixel belong?
			 theta = atan2(y-dy, x-dx);
			 // FIXME -- see extract.c:290 -- these are wrong by 180 degrees!!
			 // -- but see also extract.c:380 -- also off by 15 degrees? (-0.5)
			 si = floor((-0.5 + theta / (2.0*M_PI)) * NSEC);
			 if (si < 0)
				 si += NSEC;
			 // mirror
			 if (si >= NSEC/2)
				 si -= NSEC/2;
			 // which cell is that?
			 if (ri == 0)
				 ci = 0;
			 else
				 ci = (ri-1)*NSEC/2 + si + 1;

			 // DEBUG -- cell ids
			 //printf("pix %g,%g -> %i,%i\n", x-dx, y-dy, (int)round(x-dx), (int)round(y-dy));
			 // NOTE, phGetCellid's args are row,column.
			 int phci = phGetCellid((int)round(y-dy), (int)round(x-dx));
			 //trace("pixel (x,y) = (%g,%g): ring %i, sector %i, cell %i, photo's cell %i\n", x, y, ri, si, ci, phci);
			 cellid[iy * NX + ix] = ci;
			 phcellid[iy * NX + ix] = phci;

             //trace("ci = %i  (max %i)\n", ci, ncell);
			 assert(ci >= 0);
			 assert(ci < ncell);
             //trace("npixels[ci] = %i  (max %i)\n", npixels[ci], npix);
             assert(npixels[ci] >= 0);
             assert(npixels[ci] < npix);

			 G = exp(-0.5 * (r*r)/(psfsigma*psfsigma));
			 pix[ci][npixels[ci]] = G;
			 npixels[ci]++;

			 cs->totflux += G;
			 // second moment
			 cs->sigma2 += G * r*r;
		 }
	 }

	 // DEBUG -- cell ids
     /*
	 fprintf(stderr, "cellids=[");
	 for (iy=0; iy<NY; iy++) {
		 fprintf(stderr, "[");
		 for (ix=0; ix<NX; ix++)
			 fprintf(stderr, "%i,", cellid[iy*NX + ix]);
		 fprintf(stderr, "],");
	 }
	 fprintf(stderr, "]\n");
	 fprintf(stderr, "phcellids=[");
	 for (iy=0; iy<NY; iy++) {
		 fprintf(stderr, "[");
		 for (ix=0; ix<NX; ix++)
			 fprintf(stderr, "%i,", phcellid[iy*NX + ix]);
		 fprintf(stderr, "],");
	 }
	 fprintf(stderr, "]\n");
      */
	 free(cellid);
	 free(phcellid);


	 cs->mem = cs->median = malloc(2 * ncell * sizeof(float));
	 cs->sig = cs->mem + ncell;
	 cs->ncells = ncell;

	 for (i=0; i<ncell; i++) {
		 cs->median[i] = find_median_nonconst(pix[i], npixels[i]);
		 trace("psf model: cell %i has %i pixels, median %g\n", i, npixels[i], cs->median[i]);
		 // HACK!
		 //cs->sig[i] = sqrt(cs->median[i]);
		 cs->sig[i] = 0.01 * 1.0/(2.*M_PI*psfsigma*psfsigma);
		 free(pix[i]);
	 }

	 free(pix);
	 free(npixels);

	 return cs;
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
	trace("model produced %i cells; set nannuli to %i\n", ncells, nannuli);

    shAssert(ncells > nparam);
/*
 * Unpack the compressed cell data
 */
    data = fit_ctx.data;
    model = use_median ? stats_model->median : stats_model->mean;
    shAssert(model != NULL);
    sig = fit_ctx.sig;

	/*
	 for (i=0; i<ncells; i++) {
	 trace("cell %i: data %g, model %g, sigma %g\n",
	 i, data[i], model[i], sig[i]);
	 }
	 fprintf(stderr, "celldata = [");
	 for (i=0; i<ncells; i++)
	 fprintf(stderr, "%g,", data[i]);
	 fprintf(stderr, "]\ncellmodel = [");
	 for (i=0; i<ncells; i++)
	 fprintf(stderr, "%g,", model[i]);
	 fprintf(stderr, "]\ncellsigma = [");
	 for (i=0; i<ncells; i++)
	 fprintf(stderr, "%g,", sig[i]);
	 fprintf(stderr, "]\n");
	 */

/*
 * find the best position angle, if so desired
 */
	trace("skipping expand_model\n");
	//expand_model(model_phi, model, nannuli);
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
	trace("residual_flux %g\n", residual_flux);
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
	trace("sky_level = %g, model_amp = %g, err %g\n",
		  sky_level, model_amp, model_ampErr);
    model_ampErr *= sqrt(idata/(idata - nparam)); /* correct for
												   missing dof */
/*
 * We _added_ the residuals to the model, so we increased its flux
 */
    totflux = model_amp*(stats_model->totflux + residual_flux);
    totfluxErr = model_ampErr*(stats_model->totflux + residual_flux);

    for(i = 0;i < ncells;i++) {
		model[i] = model_amp*model[i] + sky_level;
    }

	for (i=0; i<ncells; i++) {
		trace("cell %i: data %g, fit model %g, sigma %g  -->  chi %g\n",
			  i, data[i], model[i], sig[i], (model[i] - data[i])/sig[i]);
	}

	/*
	 fprintf(stderr, "cellmodel2 = [");
	 for (i=0; i<ncells; i++)
	 fprintf(stderr, "%g,", model[i]);
	 fprintf(stderr, "]\n");
	 */

/*
 * and the residual vector
 */
	trace("fiddle = %g\n", fiddle_fac);

    idata = 0;
    sigma = sig[idata];
    fvec[idata] = (data[idata] - model[idata])/sigma;
	//trace("idata %i, data %g, model[%i] %g, sigma %g, chi %g\n", idata, data[idata], idata, model[idata], sig[idata], (data[idata] - model[idata])/sigma);
		  

#if DEBUG || TEST_2D_PROFILES
    sigsav[idata] = sigma;
#endif
    idata++;

    nannuli = (ncells - 1)/(NSEC/2) + 1;
    for(sect0 = iann = 1; iann < nannuli; iann++, sect0 += NSEC/2) {
		for(isect = 0; isect < NSEC/2; isect++) {
			sigma = sig[idata];
			//trace("iann %i, isect %i, idata %i, data %g, model[%i] = %g, sigma %g, chi %g\n", iann, isect, idata, data[idata], sect0+isect, model[sect0+isect], sig[idata], (data[idata] - model[sect0+isect])/sigma);
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
		//trace("idata %i, data %g, sigma %g, chi %g\n", idata, data[idata], sig[idata], data[idata]/sig[idata]);
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
		//trace("sky_noise_only; sky=%g, gain=%g, dark_var=%g, cell->area=%g  -->  var=%g\n", sky, gain, dark_variance, cell->area, var);
	} else {
		if((var_photon = (val1 + sky)/gain + dark_variance) > 0) {
			var_photon /= cell->area;
		} else {
			var_photon = 1e10;
		}
		var = var_photon;
		var += sinc_fac*pow(data[0]*corr[icorr][0],2);
		var += pow(var_min_frac*data[0], 2);
		//trace("not sky_noise_only; val1=%g, sky=%g, gain=%g, dark_variance=%g, cell->area=%g\n", val1, sky, gain, dark_variance, cell->area);
		//trace("photon variance %g, sinc_fac %g, var_min_frac %g\n", var_photon, sinc_fac*pow(data[0]*corr[icorr][0],2), pow(var_min_frac*data[0], 2));
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
				// trace("sky_noise_only; area1=%g, area2=%g, sky=%g, gain=%g, dark_var=%g  -->  var=%g\n", area1, area2, sky, gain, dark_variance, var);
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

				// in my tests it's photon-noise dominated.
				/*
				 trace("not sky_noise_only; val1=%g, val2=%g, area1=%g, area2=%g, sky=%g, gain=%g, dark_variance=%g\n",
				 val1, val2, area1, area2, sky, gain, dark_variance);
				 trace("photon variance %g, sinc_fac %g, centering %g, var_min_frac %g\n",
				 var_photon,
				 sinc_fac*pow(data[0]*corr[icorr][i],2),
				 centering_fac*pow(data[k]*rmean*posErr/(sigma*sigma), 2),
				 pow(var_min_frac*data[k], 2));
				 */
			}
			shAssert(var == var);
	 
			sig[k] = sqrt((var < 1e-5) ? 1e-5 : var);
		}
	}
}








/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Make an OBJC's children, returning the number created (may be 0)
 *
 * Once an OBJC has been through phObjcMakeChildren() it has a non-NULL
 * children field which points at one of its children; in addition its
 * OBJECT1_BLENDED bit is set, and nchild is one or more.
 *
 * Each child has a non-NULL parent field which points to its parent, and may
 * have a sibbs field too. It has its OBJECT1_CHILD bit set.


phObjcMakeChildren
 */
int
phObjcMakeChildrenFake(OBJC *objc,		/* give this OBJC a family */
					   const FIELDPARAMS *fiparams) /* misc. parameters */
{
	const PEAK *cpeak;			/* == objc->peaks->peaks[] */
	int i, j;
	int nchild;				/* number of children created */
	PEAK *peak;				/* a PEAK in the OBJC */
	float min_peak_spacing;

	shAssert(objc != NULL && objc->peaks != NULL && fiparams != NULL);
	min_peak_spacing = fiparams->deblend_min_peak_spacing;
	nchild = objc->peaks->npeak;
	/*
	 * Done with moving objects. See if any of the surviving peaks are
	 * too close
	 */
	trace("objc->peaks->npeak %i, nchild %i\n", objc->peaks->npeak, nchild);
	for(i = 0; i < objc->peaks->npeak; i++) {
		PEAK *const peak_i = objc->peaks->peaks[i];
		PEAK *peak_j;
		float rowc_i, colc_i;		/* == peak_i->{col,row}c */
		float rowcErr_i, colcErr_i;	/* == peak_i->{col,row}cErr */
		float rowc_j, colc_j;		/* == peak_j->{col,row}c */
		float rowcErr_j, colcErr_j;	/* == peak_j->{col,row}cErr */

		if(peak_i == NULL) {
			continue;
		}
		shAssert(peak_i->flags & PEAK_CANONICAL);
      
		rowc_i = peak_i->rowc;
		colc_i = peak_i->colc;
		rowcErr_i = peak_i->rowcErr;
		colcErr_i = peak_i->colcErr;
		for(j = i + 1; j < objc->peaks->npeak; j++) {
			if(objc->peaks->peaks[j] == NULL) {
				continue;
			}

			peak_j = objc->peaks->peaks[j];
			rowc_j = peak_j->rowc;
			colc_j = peak_j->colc;
			rowcErr_j = peak_j->rowcErr;
			colcErr_j = peak_j->colcErr;
			if(pow(fabs(rowc_i - rowc_j) - rowcErr_i - rowcErr_j, 2) +
			   pow(fabs(colc_i - colc_j) - colcErr_i - colcErr_j, 2) <
			   min_peak_spacing*min_peak_spacing) {
				objc->flags2 |= OBJECT2_PEAKS_TOO_CLOSE;
				/*
				 * If the two peaks are in the same band, delete peak_j otherwise add
				 * it to peak_i's ->next list.  If there's already a peak on the ->next
				 * list in the same band, average their positions
				 */
				phMergePeaks(peak_i, peak_j);

				objc->peaks->peaks[j] = NULL;
				nchild--;

				i--;			/* reconsider the i'th peak */
				break;
			}
		}
	}
	/*
	 * We demand that the children are detected in at least deblend_min_detect
	 * bands; reject peaks that fail this test
	 */
	trace("objc->peaks->npeak %i, nchild %i\n", objc->peaks->npeak, nchild);
	for(i = 0; i < objc->peaks->npeak; i++) {
		int n;				/* number of detections */

		if(objc->peaks->peaks[i] == NULL) {
			continue;
		}

		for(n = 0, peak = objc->peaks->peaks[i]; peak != NULL;
			peak = (PEAK *)peak->next) {
			n++;
		}
		if(n < fiparams->deblend_min_detect) {
			objc->flags2 |= OBJECT2_TOO_FEW_DETECTIONS;
	 
			phPeakDel(objc->peaks->peaks[i]);
			objc->peaks->peaks[i] = NULL;
			nchild--;
		}
	}
	/*
	 * condense the peaks list
	 */
	trace("objc->peaks->npeak %i, nchild %i\n", objc->peaks->npeak, nchild);
	if(nchild != objc->peaks->npeak) {
		for(i = j = 0; i < objc->peaks->npeak; i++) {
			if(objc->peaks->peaks[i] != NULL) {
				objc->peaks->peaks[j++] = objc->peaks->peaks[i];
			}
		}
		shAssert(j == nchild);
		for(i = nchild; i < objc->peaks->npeak; i++) {
			objc->peaks->peaks[i] = NULL;	/* it's been moved down */
		}
		phPeaksRealloc(objc->peaks, nchild);
	}
	/*
	 * and create the desired children
	 */
	trace("objc->peaks->npeak %i, nchild %i\n", objc->peaks->npeak, nchild);
	if(objc->peaks->npeak > fiparams->nchild_max) { /* are there too many? */
		objc->flags |= OBJECT1_DEBLEND_TOO_MANY_PEAKS;
		phPeaksRealloc(objc->peaks, fiparams->nchild_max);
	}

	trace("objc->peaks->npeak %i, nchild %i\n", objc->peaks->npeak, nchild);
	for(i = 0;i < objc->peaks->npeak;i++) { /* create children */
		cpeak = objc->peaks->peaks[i];
		(void)phObjcChildNew(objc, cpeak, fiparams, 1);
	}

	return(objc->peaks->npeak + ((objc->sibbs != NULL) ? 1 : 0));
}
