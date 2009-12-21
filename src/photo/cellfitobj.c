#include <alloca.h>
#include <string.h>
#include "dervish.h"
#include "phObjc.h"
#include "phDgpsf.h"
#include "phFitobj.h"
#include "phCellFitobj.h"
#include "phMathUtils.h"
#include "phUtils.h"
#include "phObjects.h"
#include "phConsts.h"
#include "phFocus.h"
#include "phObjectCenter.h"
#include "phMeasureObj.h"
#include "phTestInfo.h"

#define NPARAM 8			/* max number of parameters to fit */
#define SQRT3 1.732050808		/* sqrt(3) */
/*
 * The orders of interpolation used in the various parameters. Note that
 * it isn't sufficient to simply change these to change the order used!
 */
#define IORDER_INC 2			/* "inclination", really b/a */
#define IORDER_SIZE 2			/* size */
#if 0
#define IORDER_SEEING ??		/* seeing; defined in phDgpsf.h */
#endif

#define CORRECT_MODEL_MOMENTS 1		/* correct moments of PSF model as
					   well as `true' PSF */
#define NORMALIZE_COST_BY_CHISQ 1	/* normalise the cost in chisq_seeing
					   by the un-costed chisq */
#define QMOMENT_RMAX 6			/* evaluate psf's second moments
					   within circle of radius QMOMENT_RMAX
					   If <= 0, integrate to infinity */

static CELLPROCATHEADER cellProCatHead;
static long *model_off = NULL;
static spro_cheader *prof_cat = NULL;
static int use_median = 1;		/* use median not mean profile */

/*
 * index and weight for a model
 */
struct p_model {
    int i; 	/* index of model */
    float w;	/* weight to be given to this model */
};
/*
 * a suite of models for e.g. exponential disks
 */
typedef struct {
    int mod_nsize;
    int mod_nincl;
    int mod_offset;
    spro_catentry *entries;
} MODEL_ENTRIES;

static MODEL_ENTRIES dev_entries;	/* DeVaucouleurs models */
static MODEL_ENTRIES exp_entries;	/* Exponential models. */
static MODEL_ENTRIES psf_entries;	/* PSFs */

static PSF_COEFFS seeing_ind_arr[NCOLOR]; /*current seeing in various colours*/
static PSF_COEFFS *seeing_ind = NULL;	/* current seeing */

/*
 * The PSF_COEFFS returned by phFitPsfFromReg(), phFitCellPsfCoeffs(), and
 * phFitCellPsfCoeffsFromCells() belongs to static_psf
 */
static DGPSF static_psf;

/*
 * Cos and sin coefficients for Fourier expansion of sum(data*model/sigma^2)
 */
static double A[NSEC/2], B[NSEC/2];

/*
 * Variables used to communicate with the cell_fit_model routine
 */
static int fit_sky_level = 0;		/* should we fit the sky? */
static int fit_phi = 0;			/* should we fit the position angle? */
static float model_amp;			/* amplitude of best-fit model */
static float model_ampErr;		/* error in amplitude */
static float model_amp2[2], model_amp2Err[2]; /* amplitudes (&errors) of
						 comps. of two-model models*/

static float sky_level;			/* the resulting value */
static float totflux;			/* estimated total flux in an object */
static float totfluxErr;		/* error in total flux */
static float model_phi;			/* position angle of best model (rad)*/
static float model_phiErr;		/* error in position angle */

static float rsize_min, rsize_max;	/* extremal values of rsize for
							    available models */
static float aratio_min;		/* minimum value of rsize for
							    available models */
/*
 * Now the caches; there are colour-by-colour caches of the seeing-interpolated
 * models, a cache of models as read from disk, and a cache of all the PSF
 * profiles in the catalogue
 */
static COMP_CSTATS *model_cache_arr[NCOLOR] = {NULL, NULL, NULL, NULL, NULL};
static COMP_CSTATS *model_cache = NULL;
static int cache_size = -1;		/* size of model cache;
					   also no. of models for each seeing*/

#define NHASH (1U << 16)		/* size of read cache table;
					   must be 2^n */
static struct {
   int index;				/* index of model */
   COMP_CSTATS cs;			/* the model itself */
} read_cache[NHASH];

typedef struct {			/* PSF cache */
   float *model[2];			/* values of PSF; c.f. fit_ctx.data
					   (i == 0) ? double : single */
   float sigma2[2];			/* second moment of PSFs */
   float totflux[2];			/* total flux in PSFs */
} PSF_CACHE;

static PSF_CACHE *psf_cache = NULL;	/* cached PSFs from catalogue */
static int psf_cache_size = -1;		/* number of PSFs in psf_cache */
static int psf_cache_ncells = -1;	/* number of cells in PSFs */

static COMP_CSTATS *model_cs = NULL;	/* COMP_CSTATS for the current model */

#define NAMELEN 100
static char catalog_file[NAMELEN] = "";	/* Name of catalogue file. */
static FILE *catalog_fd = NULL;		/* catalog_file's file descriptor */

static void expand_model(float phi, float *model, int nannuli);

static float
fit_cell_seeing_psf(const REGION *reg,	/* region containing psf */
		    float sigma,	/* smooth by N(0,sigma^2) to centroid*/
		    float sky,		/* sky level */
		    DGPSF *psf,		/* PSF, including indices, weights
					   for PSF interp; or NULL */
		    PSF_COEFFS *coeffs, /* the cellfit coeffs; or NULL */
		    const PSF_CONSTRAINT *constraint); /* If non-NULL, only
						      consider nearby models */
static COMP_CSTATS *
find_phi(COMP_CSTATS *stats_model0,	/* model at 0 degrees */
	 int nannuli,			/* dimension of data[], sig[] */
	 float *data,			/* object's profile */
	 float *sig);			/* errors in data */
static int
seeing_cells_get(PSF_CACHE *psf_cache,	/* Put model here */
		 int ind,		/* desired model */
		 int single);		/* make a single component model? */

static REGION *
phChisqRegion(void (*cell_fit_func)(int m, int n, double *p, double *fvec, int *iflag),
	      int m,			/* number of data points */
	      float aratio0,		/* initial value of a/b */
	      float aratio1,		/* final value of a/b */
	      int naratio,		/* number of aratios */
	      float rsize0,		/* initial value of re */
	      float rsize1,		/* final value of re */
	      int nrsize);		/* number of rsizes */

/*****************************************************************************/

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

static COMP_CSTATS *
comp_cstatsNewFromCstats(const COMP_CSTATS *cs_in, /* COMP_CSTATS to copy */
			 int ncell_min)	/* with at least this many cells */
{
   COMP_CSTATS *cs;

   shAssert(cs_in != NULL);

   cs = comp_cstatsNew(cs_in->ncells > ncell_min ? cs_in->ncells : ncell_min);

   if(cs_in->mean == NULL) {		/* override use_median */
      cs->mean = NULL; cs->median = cs->mem;
   } else {
      cs->mean = cs->mem; cs->median = NULL;
   }
   memcpy(cs->mem, cs_in->mem,
	  (USE_MODEL_SIG ? 2 : 1)*sizeof(float)*cs_in->ncells);
   cs->sigma2 = cs_in->sigma2;
   cs->totflux = cs_in->totflux;

   return(cs);
}

static void
comp_cstatsDel(COMP_CSTATS *cs)
{
   if(cs != NULL) {
      shFree(cs->mem);
      shFree(cs);
   }
}

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

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Set the current colour for cell fitting. Asserts that phInitCellFitobj
 * has been called. If a negative colour is specified, _no_ PSF is valid
 *
 * If coeffs is non-NULL, it'll be taken to be the PSF for the colour c
 */
void
phFitCellColorSet(int c,		/* the desired colour */
		  const PSF_COEFFS *coeffs) /* describe the PSF */
{
   int i;
   
   if(c < 0) {				/* no PSF is valid */
      seeing_ind = NULL;
      return;
   }

   shAssert(model_off != NULL);
   shAssert(c >= 0 && c < NCOLOR);

   seeing_ind = &seeing_ind_arr[c];
   model_cache = model_cache_arr[c];

   if(coeffs == NULL) {
      shAssert(model_cache != NULL);
      shAssert(seeing_ind->L >= 0);
      return;
   }
/*
 * We are initialising this colour for a new frame
 *
 * set the seeing
 */
   seeing_ind->L = coeffs->L;
   for(i = 0;i < IORDER_SEEING;i++) {
      seeing_ind->i[i] = coeffs->i[i];
      seeing_ind->w[i] = coeffs->w[i];
   }
   seeing_ind->residuals = coeffs->residuals;
/*
 * clear the model cache
 */
   model_cache = model_cache_arr[c];
   
   shAssert(model_cache != NULL);
   for(i = 0; i < cache_size; i++) {
      model_cache[i].ncells = -1;
   }
}

/*
 * <AUTO EXTRACT>
 *
 * Free data structures for the fitting routines.
 */
void
phFiniCellFitobj(void)
{
    int c;
    int i, j;

    if(model_off == NULL) {		/* not initialised */
       return;
    }

    memset(&static_psf, '\0', sizeof(static_psf));
    
    shFree(model_off);
    model_off = NULL;
    
    shFree(prof_cat->proc_catentry[0]);
    shFree(prof_cat->proc_catentry);
    prof_cat = NULL;
    
    memset(&cellProCatHead, '\0', sizeof(cellProCatHead));	  
    memset(&dev_entries, '\0', sizeof(dev_entries));
    memset(&exp_entries, '\0', sizeof(exp_entries));
    memset(&psf_entries, '\0', sizeof(psf_entries));

    memset(seeing_ind_arr, '\0', sizeof(seeing_ind_arr));
    seeing_ind = NULL;			/* no psf selected */
    
    for(c = 0;c < NCOLOR;c++) {
       if((model_cache = model_cache_arr[c]) != NULL) {
	  shFree(model_cache[0].mem);
	  shFree(model_cache);
	  model_cache_arr[c] = NULL;
       }
    }
    model_cache = NULL;
    cache_size = -1;

    if(read_cache[0].cs.mem != NULL) {
       shFree(read_cache[0].cs.mem);
    }
    memset(read_cache, '\0', sizeof(read_cache));
    
    if(psf_cache_size > 0) {
       for(i = 0; i < psf_cache_size; i++) {
	  for(j = 0; j <= 1; j++) {	/* (j == 1) ? single : double */
	     shFree(psf_cache[i].model[j]);
	  }
       }
       shFree(psf_cache); psf_cache = NULL;
       psf_cache_size = psf_cache_ncells = -1;
    }

    comp_cstatsDel(model_cs);
    model_cs = NULL;
    
    catalog_file[0] = '\0';
    shAssert(catalog_fd != NULL);
    fclose(catalog_fd); catalog_fd = NULL;
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
    int i;
    int ncells;				/* number of cells in model */
    int nitem;				/* number of items read */
    unsigned int tmp;			/* used in finding h */
    unsigned int h = 0;			/* hash value */

    shAssert(index >= 0 && index < cellProCatHead.max_models);
/*
 * hash the index. The hash function was tuned by RHL, and appears to work
 * well for the values of index used in a typical photo run
 */
    for(tmp = index; tmp != 0; tmp >>= 1) {
       h = ((5*h) >> 1) ^ (tmp & 0xf);
    }
    h &= (NHASH - 1);
/*
 * see if the model's in the read cache already
 */
    if(read_cache[h].index == index) {	/* yes; it's in the read cache */
       if(cs == NULL) {
	  return(&read_cache[h].cs);
       } else {
	  cs->ncells = read_cache[h].cs.ncells;
	  cs->totflux = read_cache[h].cs.totflux;
	  memcpy(cs->mem,read_cache[h].cs.mem,
					read_cache[h].cs.ncells*sizeof(float));

	  if(use_median) {
	     cs->median = cs->mem; cs->mean = NULL;
	  } else {
	     cs->mean = cs->mem; cs->median = NULL;
	  }
	  
#if USE_MODEL_SIG
	  if(cs->sig != NULL) {
	     memcpy(cs->sig,read_cache[h].cs.sig,
					read_cache[h].cs.ncells*sizeof(float));
	  }
#endif
	  return(cs);
       }
    } else {
       if(cs == NULL) {
	  cs = &read_cache[h].cs;
       }
    }
/*
 * read model
 */
    fseek(fd, model_off[index], SEEK_SET);
    nitem = 0;
    nitem += fread(&cs->ncells, sizeof(int), 1, fd);
    nitem += fread(&cs->totflux, sizeof(float), 1, fd);
    shAssert(nitem == 2 && cs->ncells <= MAXCELLS);
    
    ncells = cs->ncells;
    
    if(use_median) {
       cs->median = cs->mem; cs->mean = NULL;
       fseek(fd, ncells*sizeof(float), SEEK_CUR); /* skip the mean */
       nitem = fread(cs->median, sizeof(float), ncells, fd);
    } else {
       cs->mean = cs->mem; cs->median = NULL;
       nitem = fread(cs->mean, sizeof(float), ncells, fd);
       fseek(fd, ncells*sizeof(float), SEEK_CUR); /* skip the median */
    }
    shAssert(nitem == ncells);

#if USE_MODEL_SIG
    if(cs->sig != NULL) {
       nitem = fread(cs->sig, sizeof(float), ncells, fd);
       shAssert(nitem == ncells);
    }
#endif
/*
 * Normalise the profile to be 1.0 at the centre.
 * This should be done for us in cellprof.dat
 */
#if 1					/* XXX */
    if(normalize) {
       float norm = 1/cs->mem[0];
       cs->totflux *= norm;
       for(i = 0; i < ncells; i++) {
	  cs->mem[i] *= norm;
       }
#if USE_MODEL_SIG
       if(cs->sig != NULL) {
	  for(i = 0; i < ncells; i++) {
	     cs->sig[i] *= norm;
	  }
       }
#endif
    }
#endif
/*
 * put model in read cache
 */
    if(cs == &read_cache[h].cs) {
       read_cache[h].index = index;
       read_cache[h].cs.ncells = ncells;
       read_cache[h].cs.totflux = cs->totflux;
       memcpy(read_cache[h].cs.mem,cs->mem,ncells*sizeof(float));
#if USE_MODEL_SIG
       if(cs->sig != NULL) {
	  memcpy(read_cache[h].cs.sig,cs->sig,ncells*sizeof(float));
       }
#endif

       if(use_median) {
	  read_cache[h].cs.median = read_cache[h].cs.mem;
	  read_cache[h].cs.mean = NULL;
       } else {
	  read_cache[h].cs.mean = read_cache[h].cs.mem;
	  read_cache[h].cs.median = NULL;
       }
    }

    return(cs);
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

/*****************************************************************************/
/*
 * These functions return a COMP_CSTATS containing a realization of a
 * seeing-convolved Exponential or de Vaucouleurs law.
 */
static CELL_PROF fit_ctx;

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

#define DEBUG 0				/* if true, save things for gdb;
					   if > 1, do some expensive checks */

#if TEST_2D_PROFILES			/* these'll end up in TEST_INFO */
static int test_nprof2D;
static float test_profMean2D[NCELL];
static float test_profErr2D[NCELL];
static float test_profModel2D[NCELL];
#endif

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
    mp.class = -1;			/* unknown */
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
 * Find the best-fit combination of two models
 *
 * N.b. this routine cannot be called by phLMMinimise(); if you want to
 * do that you'll have to repackage it (and maybe buy a faster computer)
 */

static void
cell_fit_2models(COMP_CSTATS *(*cells_make)(const MODEL_PARAMS *),
		 int ndata,
		 int nparam,
		 double *p,
		 double *fvec,
		 int *iflag)
{
#if DEBUG
    float sigsav[NCELL];
#endif
    int class;				/* class of a model */
    float *data;			/* mean or median */
    double det;				/* determinant of normal equations */
    float fiddle_fac;			/* amount to fiddle chisq due to
					   parameters being out of bounds */
    int i, j;
    int idata;
    int iann;				/* index for annuli */
    int nannuli;			/* number of annuli */
    int sect0;				/* sector 0 for a given annulus */
    int isect;				/* sector counter */
    float ivar;				/* == 1/sigma^2; unpacked for speed */
    float *model;			/* mean or median */
    float *models[2];			/* mean or median of stats_models[] */
    MODEL_PARAMS mp;
    float phi;				/* A model's position angle */
    float residual_flux[2];		/* flux in residual table */
    float sigma;			/* model-reduced sigma for a point */
    COMP_CSTATS *stats_models[2] = {NULL, NULL}; /* models at best-fit angle */
    float *sig;				/* standard deviation of data */
    double sum_dm0, sum_dm1;		/* sums of data*model[01] */
    double sum_m0m0, sum_m0m1, sum_m1m1; /* sums of data*model[01]*model[01] */
    int ncells = -1;			/* min number of cells in models */

    shAssert(*iflag >= 0);
    shAssert(nparam <= NPARAM);
    shAssert(ndata == fit_ctx.ncell);
    shAssert(nparam == 0 || p != NULL);
    shAssert(fvec != NULL);
/*
 * Stuff changing parameters into the context.
 */
    mp.class = -1;			/* unknown */
    mp.psf = NULL;

    mp.aratio = 1.0;
    mp.rsize = rsize_max;
#if !defined(NDEBUG)			/* shAssert is active */
    for(i = 0; i < nparam; i++) {
       shAssert(p[i] == p[i]);		/* i.e. not a NaN */
    }
#endif

    shAssert(nparam == 8);
    shAssert(fit_phi == 0);
    shAssert(fit_sky_level == 0);

    data = fit_ctx.data;
    sig = fit_ctx.sig;
    for(i = 0; i < 2; i++) {
       class = p[0 + 4*i];
       mp.aratio = p[1 + 4*i];
       mp.rsize = p[2 + 4*i];
       phi = p[3 + 4*i]*M_PI/180;
/*
 * see if any parameters are out of range; if so set them to their limits,
 * and remember to increase the final fvec vector correspondingly
 */
       switch (class) {
	case PSF_MODEL:
	  set_rsize_limits(&psf_entries);
	  cells_make = psf_cells_make;
	  break;
	case DEV_MODEL:
	  set_rsize_limits(&dev_entries);
	  cells_make = dev_cells_make;
	  break;
	case EXP_MODEL:
	  set_rsize_limits(&exp_entries);
	  cells_make = exp_cells_make;
	  break;
	default:
	  shFatal("cell_fit_2models: unknown galaxy class %d",class);
       }
       
       fiddle_fac = 1.0;
       
       if(mp.aratio < aratio_min) {
	  fiddle_fac *= 1 + pow(aratio_min - mp.aratio,2);
	  mp.aratio = aratio_min;
       }
       if(mp.aratio > 1) {		/* minimiser can choose a better phi */
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
 * Build the model, and copy it into a safe place
 */
       stats_models[i] = comp_cstatsNewFromCstats((*cells_make)(&mp),
						  MAXCELLS);
/*
 * get uncompressed number of cells.
 */
       if(ncells < 0 || stats_models[i]->ncells > ncells) {
	  ncells = stats_models[i]->ncells;
       }
       ncells = ncells < ndata ? ncells : ndata;
       if(ncells == 0) {
	  continue;
       }
       nannuli = (ncells - 1)/(NSEC/2) + 1;
       
       shAssert(ncells > 2);		/* not fitting the params, just I0 */
/*
 * Unpack the compressed cell data
 */
       models[i] = use_median? stats_models[i]->median : stats_models[i]->mean;
       shAssert(models[i] != NULL);
       expand_model(phi, models[i], nannuli);
    }
/*
 * correct for the known discrepancy between the psf and our best
 * representation of it
 */    
    residual_flux[0] = residual_flux[1] = 0;
    if(seeing_ind != NULL) {		/* we have a model for the PSF */
       const float *area = seeing_ind->residuals.area;
       float *residuals = seeing_ind->residuals.data;
       int nresid;
       for(i = 0; i < 2; i++) {
	  const float I0 = models[i][0]; /* central value of model */
	  nresid = (ncells < seeing_ind->residuals.ncell) ?
				      ncells : seeing_ind->residuals.ncell;
	  for(j = 0;j < nresid;j++) {
	     models[i][j] += I0*residuals[j];
	     residual_flux[i] += area[j]*residuals[j];
	  }
	  residual_flux[i] *= I0;
       }
    }
/*
 * Did we fail to get the models?
 */
    if(ncells == 0) {
       comp_cstatsDel(stats_models[0]);
       comp_cstatsDel(stats_models[1]);
       
       return;
    }
/*
 * calculate best-fitting amplitudes
 */
    nannuli = (ncells - 1)/(NSEC/2) + 1;

    idata = 0;
    sigma = sig[idata];
    ivar = 1/(sigma*sigma);
 
    sum_dm0 = data[idata]*models[0][idata]*ivar;
    sum_dm1 = data[idata]*models[1][idata]*ivar;
    sum_m0m0 = models[0][idata]*models[0][idata]*ivar;
    sum_m0m1 = models[0][idata]*models[1][idata]*ivar;
    sum_m1m1 = models[1][idata]*models[1][idata]*ivar;
    
    idata++;

    for(sect0 = iann = 1; iann < nannuli; iann++, sect0 += NSEC/2) {
	for(isect = 0; isect < NSEC/2; isect++) {
	    sigma = sig[idata];

	    ivar = 1/(sigma*sigma);

	    sum_dm0 += data[idata]*models[0][idata]*ivar;
	    sum_dm1 += data[idata]*models[1][idata]*ivar;
	    sum_m0m0 += models[0][idata]*models[0][idata]*ivar;
	    sum_m0m1 += models[0][idata]*models[1][idata]*ivar;
	    sum_m1m1 += models[1][idata]*models[1][idata]*ivar;

	    ++idata;
	}
    }

    det = sum_m0m0*sum_m1m1 - sum_m0m1*sum_m0m1;
    shAssert(det != 0.0);

    model_amp2[0] = (sum_m1m1*sum_dm0 - sum_m0m1*sum_dm1)/det;
    model_amp2[1] = (sum_m0m0*sum_dm1 - sum_m0m1*sum_dm0)/det;

    model_amp2Err[0] = sqrt(sum_m0m0/det);
    model_amp2Err[1] = sqrt(sum_m1m1/det);

    model_ampErr *= sqrt(idata/(idata - nparam - fit_phi)); /* correct for
							       missing dof */
/*
 * Does one of those two models have -ve flux?  If so, revert to a
 * single model fit
 */
    if(model_amp2[0] < 0 || model_amp2[1] < 0) {
       int better = i;			/* better of the two models */
       if(model_amp2[0] < 0) {
	  better = 1;
       } else {
	  better = 0;
       }

       nparam = 2;
       switch ((int)p[4*better]) {
	case DEV_MODEL:
	  set_rsize_limits(&dev_entries);
	  cells_make = dev_cells_make;
	  break;
	case EXP_MODEL:
	  set_rsize_limits(&exp_entries);
	  cells_make = exp_cells_make;
	  break;
	case PSF_MODEL:
	  set_rsize_limits(&psf_entries);
	  cells_make = psf_cells_make;
	  nparam = 0;
	  break;
       }
       p[0] = p[1 + 4*better];
       p[1] = p[2 + 4*better];
       model_phi = p[3 + 4*better]*M_PI/180;
       
       cell_fit_model(cells_make, ndata, nparam, p, fvec, iflag);

       model_amp2[better] = model_amp;
       model_amp2Err[better] = model_ampErr;
       model_amp2[!better] = model_amp2Err[!better] = 0;

       comp_cstatsDel(stats_models[0]);
       comp_cstatsDel(stats_models[1]);
       
       return;
    }
/*
 * We _added_ the residuals to the model, so we increased its flux
 */
    totflux = totfluxErr = 0;
    for(i = 0; i < 2; i++) {
       totflux += model_amp2[i]*(stats_models[i]->totflux + residual_flux[i]);
       totfluxErr += model_amp2Err[i]*(stats_models[i]->totflux + residual_flux[i]);
    }

    for(j = 0;j < ncells; j++) {
       models[0][j] = model_amp2[0]*models[0][j] + model_amp2[1]*models[1][j];
    }

    model = models[0];			/* model == composite model */
/*
 * and the residual vector
 */
    idata = 0;
    sigma = sig[idata];
    fvec[idata] = (data[idata] - model[idata])/sigma;
#if DEBUG
    sigsav[idata] = sigma;
#endif
    idata++;

    nannuli = (ncells - 1)/(NSEC/2) + 1;
    for(sect0 = iann = 1; iann < nannuli; iann++, sect0 += NSEC/2) {
	for(isect = 0; isect < NSEC/2; isect++) {
	    sigma = sig[idata];
	    fvec[idata] = fiddle_fac*(data[idata] - model[sect0+isect])/sigma;
#if DEBUG
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
#if DEBUG
	sigsav[idata] = sig[idata];
#endif
     }

    comp_cstatsDel(stats_models[0]);
    comp_cstatsDel(stats_models[1]);
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

static void
expand_fourier_data(float *data, int ndata,
		    int normalize)		/* normalise profile to centre == 1 */
{
    int i;
    int iann;				/* index for annuli */
    int isect;				/* index for sectors */
    const int nannuli = (ndata - 1)/(NSEC/2) + 1; /* number of annuli */
    int sect0;				/* sector 0 for a given annulus */
    float *edata = alloca(ndata*sizeof(float));	/* output */

    edata[0] = data[0];			/* central pixel is one and all alone */

    for(isect = 0; isect < NSEC/2; isect++) {
	for(sect0 = iann = 1; iann < nannuli; iann++, sect0 += NSEC/2) {
	    const float *ann = &data[sect0];	/* find this annulus's data */
	    float theta = isect*2*M_PI/NSEC;
	    float sum = 0;
	    for(i = 0; i < NSEC/2; i++) {
		sum += ann[i]*cos(2*i*theta);
	    }

	    if (normalize) {
		sum /= edata[0];
	    }

	    edata[sect0 + isect] = sum;
	}
    }

    memcpy(data, edata, ndata*sizeof(float)); /* copy expanded model back */
}

static void
expand_fourier_cstats(COMP_CSTATS *model, /* Fourier expansion of model */
		      int normalize)	/* normalise profiles to centre == 1 */

{
    if (model->mean != NULL) {
	expand_fourier_data(model->mean, model->ncells, normalize);
    }
    if (model->median != NULL) {
	expand_fourier_data(model->median, model->ncells, normalize);
    }
    if (model->sig != NULL) {
	expand_fourier_data(model->sig, model->ncells, normalize);
    }
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

static void
cell_fit_BD(int ndata,
	    int nparam,
	    double *p,
	    double *fvec,
	    int *iflag)
{
    cell_fit_2models(NULL, ndata, nparam, p, fvec, iflag);
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
 * Evaluate CHISQ == chi^2 given a set of sums and weights
 */
#if 0
#define CALC_CHISQ(CHISQ) \
{ \
   const double w11 = w->ve[0]; \
   const double w12 = w->ve[1]; \
   const double w21 = w->ve[2]; \
   const double w22 = w->ve[3]; \
    \
   CHISQ = sum_dd - \
     2*(w11*sum_dm11 + w12*sum_dm12 + w21*sum_dm21 + w22*sum_dm22) + \
     w11*(w11*sum_m11m11 + w12*sum_m11m12 + w21*sum_m11m21 + w22*sum_m11m22) +\
     w12*(w11*sum_m11m12 + w12*sum_m12m12 + w21*sum_m12m21 + w22*sum_m12m22) +\
     w21*(w11*sum_m11m21 + w12*sum_m12m21 + w21*sum_m21m21 + w22*sum_m21m22) +\
     w22*(w11*sum_m11m22 + w12*sum_m12m22 + w21*sum_m21m22 + w22*sum_m22m22); \
}
#else
#define CALC_CHISQ(CHISQ) \
{ \
   double sum = 0.0; \
   double tmp; \
   const double w11 = w->ve[0]; \
   const double w12 = w->ve[1]; \
   const double w21 = w->ve[2]; \
   const double w22 = w->ve[3]; \
   \
   for(i = 0;i < ncells;i++) { \
      tmp = \
	w11*model_11[i] + w12*model_12[i] + w21*model_21[i] + w22*model_22[i];\
      tmp = (data[i] - tmp)/sig[i]; \
      sum += tmp*tmp; \
   } \
   CHISQ = sum; \
}
#endif


/*****************************************************************************/
/*
 * given data and sig arrays (of dimen ndata), find the best PSF model in
 * the grid cell with corners (si, di) and (si + 1, di + 1).
 *
 * The best choice of weights is returned as [sd]w[01] (if non-NULL),
 * and the difference between that model and the data (normalised by the
 * model) are returned in residuals[] (if non-NULL)
 */
static float
chisq_seeing(const float *data,		/* data values */
	     const float *sig,		/* s.d. of data */
	     int ndata,			/* number of values */
	     float tau2,		/* <r^2> for data within QMOMENT_RMAX*/
	     int si,			/* index into single Gaussians */
	     int di,			/* index into double Gaussians */
	     int ignore[IORDER_SEEING],	/* which components to ignore */
	     float *sw0,		/* weight of si'th model wrt si+1'th */
	     float *sw1,		/* weight of si+1'th model wrt si'th */
	     float *dw0,		/* weight of di'th model wrt di+1'th */
	     float *dw1,		/* weight of di+1'th model wrt di'th */
	     float *residuals)		/* residuals of best fit, or NULL */
{
   MAT *A;				/* normal equations for LSQ fit are */
   VEC *b;				/*      A*w = b */
   float chisq;				/* returned value of chisq */
   float cost;				/* scaled cost in penalty function */
#if 1
   static int niter = 10;		/* number of times to iterate cost */
   static int print_s2 = 0;		/* control printf */
#endif
#if NORMALIZE_COST_BY_CHISQ
   float cost0 = 1;			/* (cost in penalty function)/chisq0 */
#else
   float cost0 = 1e6;			/* cost in penalty function */
#endif
   float dat;				/* == data[idata] */
   static float frac = 0;		/* sky variance is
					   frac*(central pixel's variance) */
   int i, j;
   int idata;
   int iann;
   int nannuli;
   float ivar;				/* == 1/sigma^2; unpacked for speed */
   int sect0;				/* sector 0 for a given annulus */
   int isect;				/* sector counter */
   int msect;				/* sector in model */
   float *model_11, *model_12;		/* mean|median of single ... */
   float *model_21, *model_22;		/* ... and double Gaussians */
   float mod11, mod12, mod21, mod22;	/* == model_[12][12][msect] */
   int ncells;				/* number of cells in model */
   const int npar = 4;			/* number of parameters being fit */
   float sigma2[IORDER_SEEING];		/* second moments of model components*/
   float totflux[IORDER_SEEING];	/* total flux of model components*/
   double sum_dd;			/* sum(data^2/sigma^2) */
   double sum_dm11, sum_dm12,		/* sum(data*model_11/sigma^2) etc. */
          sum_dm21, sum_dm22;
   double sum_m11m11, sum_m11m12,	/* sum(model_11^2/sigma^2) etc. */
          sum_m11m21, sum_m11m22,
	  sum_m12m12, sum_m12m21,
	  sum_m12m22, sum_m21m21,
	  sum_m21m22, sum_m22m22;
   double tmp;				/* temporary */
   VEC *w;				/* desired weights */

   shAssert(npar <= IORDER_SEEING);

   if(ndata <= 1) {
      if(sw0 != NULL) {
	 *sw0 = *sw1 = *dw0 = *dw1 = 0.0;
      }
      return(1e30);
   }
/*
 * Get models from cache
 */
   ncells = psf_cache_ncells;
   shAssert(ncells > 0);		/* i.e. cache was populated */

   model_11 = psf_cache[si].model[1];
   model_12 = psf_cache[si + 1].model[1];
   model_21 = psf_cache[di].model[0];
   model_22 = psf_cache[di + 1].model[0];

   sigma2[0] = psf_cache[si].sigma2[1];
   sigma2[1] = psf_cache[si + 1].sigma2[1];
   sigma2[2] = psf_cache[di].sigma2[0];
   sigma2[3] = psf_cache[di + 1].sigma2[0];

   totflux[0] = psf_cache[si].totflux[1];
   totflux[1] = psf_cache[si + 1].totflux[1];
   totflux[2] = psf_cache[di].totflux[0];
   totflux[3] = psf_cache[di + 1].totflux[0];
   
   ncells = ncells < ndata ? ncells : ndata;
   nannuli = (ncells - 1)/(NSEC/2) + 1;
/*
 * calculate best-fitting amplitude and weight by least squares; first
 * accumulate the necessary sums
 */
   sum_dd = 0;
   sum_dm11 = sum_dm12 = sum_dm21 = sum_dm22 = 0;
   sum_m11m11 = sum_m11m12 = sum_m11m21 = sum_m11m22 = 0;
   sum_m12m12 = sum_m12m21 = sum_m12m22 = 0;
   sum_m21m21 = sum_m21m22 = 0;
   sum_m22m22 = 0;
   
   for(idata = sect0 = iann = 0; iann < nannuli;
				   sect0 += (iann == 0 ? 1 : NSEC/2), iann++) {
      for(isect = 0; isect < NSEC/2; isect++) {
	 msect = sect0 + isect;
	 mod11 = ignore[0] ? 0 : model_11[msect];
	 mod12 = ignore[1] ? 0 : model_12[msect];
	 mod21 = ignore[2] ? 0 : model_21[msect];
	 mod22 = ignore[3] ? 0 : model_22[msect];
	 dat = data[idata];
#if 1
	 ivar = 1.0/(sig[idata]*sig[idata] + frac*sig[0]*sig[0]);
#else
	 ivar = 1;
#endif
	 idata++;

	 sum_dd += dat*dat*ivar;

	 sum_dm11 += dat*mod11*ivar;
	 sum_dm12 += dat*mod12*ivar;
	 sum_dm21 += dat*mod21*ivar;
	 sum_dm22 += dat*mod22*ivar;

	 sum_m11m11 += mod11*mod11*ivar;
	 sum_m11m12 += mod11*mod12*ivar;
	 sum_m11m21 += mod11*mod21*ivar;
	 sum_m11m22 += mod11*mod22*ivar;

	 sum_m12m12 += mod12*mod12*ivar;
	 sum_m12m21 += mod12*mod21*ivar;
	 sum_m12m22 += mod12*mod22*ivar;

	 sum_m21m21 += mod21*mod21*ivar;
	 sum_m21m22 += mod21*mod22*ivar;

	 sum_m22m22 += mod22*mod22*ivar;

	 if(iann == 0) {
	    break;			/* only 1 sector in central `annulus'*/
	 }
      }
   }
/*
 * solve for the relative weights of the single and double Gaussian components
 */
   A = phMatNew(npar, npar);
   b = phVecNew(npar);

   b->ve[0] = sum_dm11;
   b->ve[1] = sum_dm12;
   b->ve[2] = sum_dm21;
   b->ve[3] = sum_dm22;

   A->me[0][0] = sum_m11m11;
   A->me[0][1] = A->me[1][0] = sum_m11m12;
   A->me[0][2] = A->me[2][0] = sum_m11m21;
   A->me[0][3] = A->me[3][0] = sum_m11m22;

   A->me[1][1] = sum_m12m12;
   A->me[1][2] = A->me[2][1] = sum_m12m21;
   A->me[1][3] = A->me[3][1] = sum_m12m22;

   A->me[2][2] = sum_m21m21;
   A->me[2][3] = A->me[3][2] = sum_m21m22;

   A->me[3][3] = sum_m22m22;
/*
 * invert matrix and get solution. Use eigenvalue decomposition to invert
 * the problem; this is equivalent to the usual SVD decomposition, except
 * that the matrix's condition number is squared
 *
 * Any zero eigenvectors are set to infinity as part of the back substitution
 */
   {
      MAT *Q = phMatNew(npar, npar);	/* eigen vectors */
      VEC *lambda = phEigen(A, Q, NULL); /* eigen values */
      
      for(i = 0; i < lambda->dim; i++) {
	 if(lambda->ve[i] > -EPSILON && lambda->ve[i] < EPSILON) {
	    lambda->ve[i] = 0;		/* handled by phEigenBackSub() */
	 }
      }
      
      w = phEigenBackSub(Q, lambda, b);
/*
 * We have the unconstrained set of weights.  We now want to solve the
 * same problem, but with a penalty function to force the best-fit PSF
 * to have the same width as the true PSF.
 *
 * The penalty is
 *   C = cost*(tau^2 - sum_i(w_i*flux_i*sigma_i^2)/sum_i(w_i*flux_i))^2
 * where tau*2 is the desired second moment, and sigma_i^2 is the second moment
 * of the i'th component.  We rewrite this approximately as
 *   C = cost/sum_i(w0_i*flux_i)^2*(sum_i(w_i*flux_i*(tau^2 - sigma_i^2)))^2
 * where w0 are the unconstrained weights; in this formulation the problem's
 * linear.
 */
      if(tau2 > 0) {
	 MAT *Ap = phMatNew(A->m, A->n);
	 int k;

#if NORMALIZE_COST_BY_CHISQ
	 CALC_CHISQ(chisq);
	 cost0 *= chisq;
#endif
	 for(k = niter; k > 0; k--) {
	    tmp = 0;
	    for(i = 0; i < npar; i++) {
	       tmp += w->ve[i]*totflux[i];
	    }
	    cost = cost0/(tmp*tmp + EPSILON);
	    
	    phVecDel(w);

	    for(i = 0; i < npar; i++) {
	       for(j = i; j < npar; j++) {
		  Ap->me[i][j] = A->me[i][j] +
		    cost*(totflux[i]*(tau2 - sigma2[i]))*
					       (totflux[j]*(tau2 - sigma2[j]));
		  Ap->me[j][i] = Ap->me[i][j];
	       }
	    }
	    
	    phEigen(Ap, Q, lambda);
	    w = phEigenBackSub(Q, lambda, b);
	    
	    {
	       float s2 = 0;
	       tmp = 0;
	       for(i = 0; i < npar; i++) {
		  tmp += w->ve[i]*totflux[i];
		  s2 += w->ve[i]*sigma2[i]*totflux[i];
	       }
	       shAssert(tmp != 0);
	       s2 /= tmp;
	       if(print_s2 == 2 ||
		  (print_s2 == 1 && residuals != NULL && k == 1)) {
		  printf("tau = %.4f s2 = %.4f %.4f (%.4f %.4f %.4f)\n",
			 tau2, s2, tau2 - s2,
			 sqrt(tau2), sqrt(s2), sqrt(tau2) - sqrt(s2));
	       }
	    }
	 }
	 phMatDel(Ap);
      }

      phMatDel(Q); phVecDel(lambda);
   }
   phMatDel(A); phVecDel(b);
#if 0
/*
 * Don't allow very negative weights.
 *
 * This is not a good idea; consider an N(0,s) + 0.05 N(0,3s) PSF.
 * A reasonable solution given N(0,t) and N(0,t) + 0.1111 N(0,3t) tables is
 *   [N(0,t) + 0.1111 N(0,3t)] - 0.0611 N(0,3t)
 * I.e. with quite a negative weight.  Of course, 
 *   [N(0,t) + 0.1111 N(0,3t)] + 1.2222 N(0,t)
 * is also possible.
 */
   {
      static float wlim = -1;
      int imin = 0; float wmin = w->ve[imin];
      for(i = 0; i < IORDER_SEEING; i++) {
	 if(ignore[i]) {
	    w->ve[i] = 0;
	 } else {
	    if(w->ve[i] < wmin) {
	       imin = i;  wmin = w->ve[imin];
	    }
	 }
      }

      if(wmin < wlim) {
	 ignore[imin] = 1;
      }
   }
#endif
/*
 * Call ourself recursively if we've decided to ignore some models
 */
   {
      int nignore = 0;
      for(i = 0; i < npar; i++) {
	 nignore += ignore[i] ? 1 : 0;
      }
      shAssert(nignore < npar);
      
      if(nignore > 0) {
	 phVecDel(w);
	 
	 return(chisq_seeing(data, sig, ndata, tau2, si, di, ignore,
			     sw0, sw1, dw0, dw1, residuals));
      }
   }
/*
 * Find chi^2 for that fit
 */
   CALC_CHISQ(chisq);

   if(tau2 > 0) {			/* include cost function */
      double sum;
      sum = tmp = 0;
      for(i = 0; i < npar; i++) {
	 sum += w->ve[i]*totflux[i];
	 tmp += w->ve[i]*totflux[i]*sigma2[i];
      }
      shAssert(sum != 0);
      tmp /= sum;
      
      chisq += cost0*pow(tau2 - tmp, 2);
   }
/*
 * Maybe make the best-fit model and evaluate the residuals
 */
   if(residuals != NULL) {
      float model;			/* model in a cell */
      float imodel0 = 0.0;		/* == 1/model[0]; initialise for gcc */
      const double w11 = w->ve[0];
      const double w12 = w->ve[1];
      const double w21 = w->ve[2];
      const double w22 = w->ve[3];

      for(i = 0; i < ncells;i++) {
	 model = w11*model_11[i] + w12*model_12[i] +
					     w21*model_21[i] + w22*model_22[i];
	 if(i == 0) {
	    imodel0 = 1/model;
	 }
	 residuals[i] = (data[i] - model)*imodel0;
      }
   }
/*
 * return answers, if desired
 */
   if(sw0 != NULL) {
      *sw0 = w->ve[0];
      *sw1 = w->ve[1];
      *dw0 = w->ve[2];
      *dw1 = w->ve[3];
   }
   phVecDel(w);

   return(chisq);
}

/*****************************************************************************/

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


/*****************************************************************************/
/*
 * Make an initial guess at the parameters for a model if we haven't
 * run Measure Objects
 */
static void
estimate_params(OBJECT1 *obj,		/* Object to estimate */
		const CELL_STATS *stats_obj) /* results of phProfileExtract()*/
{
    float l_total;		/* total flux */
    float l_int;
    float r_inner;
    float r_outer;
    float r_e;
    int i;
    int n_ann;
    float lr[NANN];		/* flux in annuli */
    float aratio, phi;			/* axis ratio and p.a. */

    n_ann = stats_obj->nannuli;
    l_total = 0.0;
    r_inner = stats_obj->geom[0].outer;
    lr[0] = M_PI*r_inner*r_inner*phProfileMean(stats_obj, 0, 0, 0, NULL);
    for(i = 1; i < n_ann; i++) { /* get radial profile */
	r_outer = stats_obj->geom[(i-1)*NSEC + 1].outer;
	lr[i] = phProfileMean(stats_obj, i, 0, 0, NULL)*M_PI*
					   (r_outer*r_outer - r_inner*r_inner);
	r_inner = r_outer;
	l_total += lr[i];
    }

    n_ann = i;
    l_int = 0.0;
    for(i = 0; i < n_ann; i++) { /* find 1/2 light radius */
	l_int += lr[i];
	if(l_int > l_total/2.0)
	    break;
    }
/*
 * Linear interpolation for 1/2 light radius
 */
    r_e = stats_obj->geom[(i-1)*NSEC + 1].inner
	+ (l_total/2.0 - (l_int - lr[i]))
	   *(stats_obj->geom[(i-1)*NSEC + 1].outer
	     - stats_obj->geom[(i-1)*NSEC + 1].inner)/lr[i];
    phObjmaskShape(obj->mask,
		   NULL, NULL, NULL, NULL, NULL, NULL, &aratio, &phi);
    if(aratio < aratio_min) {
       aratio = aratio_min;
    }
    phi *= M_PI/180;

    obj->petroR50 = r_e;
    obj->Q = (1 - aratio)/(1 + aratio)*cos(2*phi);
    obj->U = (1 - aratio)/(1 + aratio)*sin(2*phi);
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
 * Fit a PSF stored in fit_ctx with a linear combination of a "canonical"
 * double Gaussian and a single Gaussian which are stored in the cell
 * catalogue; each of the double and single Gaussians are
 * of course themselves linearily interpolated
 *
 * The residual table is also filled out, and the likelihood value set
 */
static float
fit_cell_seeing_psf_cellProf(PSF_COEFFS *seeing, /* indices, weights for PSF */
			     float tau2, /* second moment of PSF; if > 0, the
					    fit is forced to have this width */
			     const PSF_CONSTRAINT *constraint)
{
   float chisq_min, chisq;
   double residual_flux;		/* total flux in residuals */
   int i,j,ncell;
   int i0, i1, j0, j1;			/* range of i, j */
   int ignore[IORDER_SEEING];		/* which components to ignore */
   int si_min, di_min;			/* best model lies in [si, si + 1] */
   float sw0, sw1;			/* weights of single models */
   float dw0, dw1;			/* weights of double models */
   float sw0_min, sw1_min, dw0_min, dw1_min; /* best values of sw0 etc. */
/*
 * are there enough points?
 */
   if(fit_ctx.ncell <= 1) {
      for(i = 0;i < IORDER_SEEING;i++) {
	 seeing->i[i] = -1;
	 seeing->w[i] = 0;
      }
      
      seeing->L = -1;
      seeing->residuals.ncell = 0;
      seeing->residuals.is_median = 0; 
      
      return(-1);
   }
/*
 * Look for the best fit (by exhaustive search; it's usually fast enough)
 * unless [sd]i0 are set, in which case restrict ourselves to the
 * neighbourhood of [sd]i0.  Remember that cells are defined by their corner,
 * so the limit is cellProCatHead.nseeing - 2 not cellProCatHead.nseeing - 1
 */
   if(constraint != NULL && constraint->nmodel > 0) {
      shAssert(constraint->nmodel%2 == 1);
      i0 = constraint->si - constraint->nmodel/2;
      i1 = constraint->si + constraint->nmodel/2;
      j0 = constraint->di - constraint->nmodel/2;
      j1 = constraint->di + constraint->nmodel/2;

      if(i0 < 0) i0 = 0;
      if(j0 < 0) j0 = 0;
      if(i1 >= cellProCatHead.nseeing - 1) i1 = cellProCatHead.nseeing - 2;
      if(j1 >= cellProCatHead.nseeing - 1) j1 = cellProCatHead.nseeing - 2;
   } else {
      i0 = j0 = 0;
      i1 = j1 = cellProCatHead.nseeing - 2;
   }
/*
 * Don't fit components with ignore[] != 0
 */
   for(i = 0; i < IORDER_SEEING; i++) {
      ignore[i] = 0;
   }
/*
 * Minimise chisq by searching the grid
 */
   chisq_min = 0;
   si_min = di_min = -1;
   sw0_min = sw1_min = dw0_min = dw1_min = 0;
   for(i = i0; i <= i1; i++) {
      for(j = j0; j <= j1; j++) {
	 chisq = chisq_seeing(fit_ctx.data, fit_ctx.sig, fit_ctx.ncell, tau2,
			      i, j, ignore, &sw0, &sw1, &dw0, &dw1, NULL);
	 
	 if(si_min < 0 || chisq < chisq_min) {
	    chisq_min = chisq;
	    si_min = i; di_min = j;
	    sw0_min = sw0; sw1_min = sw1;
	    dw0_min = dw0; dw1_min = dw1;
	 }
      }
   }
   
   seeing->i[0] = 2*di_min;		/* even models are double Gaussian */
   seeing->w[0] = dw0_min;
   seeing->i[1] = 2*(di_min + 1);
   seeing->w[1] = dw1_min;
   
   seeing->i[2] = 2*si_min + 1;		/* odd models are single Gaussian */
   seeing->w[2] = sw0_min;
   seeing->i[3] = 2*(si_min + 1) + 1;
   seeing->w[3] = sw1_min;
/*
 * Normalise weights
 */
   {
      float sum = 0;
      for(i = 0;i < IORDER_SEEING;i++) {
	 sum += seeing->w[i];
      }
      shAssert(sum != 0.0);
      
      for(i = 0;i < IORDER_SEEING;i++) {
	 seeing->w[i] /= sum;
      }
   }
/*
 * Find the difference between the true PSF and the best fit that our
 * linear combination of Gaussians can achieve.
 */
   ncell = fit_ctx.ncell;
   shAssert(ncell <= sizeof(fit_ctx.data)/sizeof(fit_ctx.data[0]));

   (void)chisq_seeing(fit_ctx.data, fit_ctx.sig, ncell, tau2,
		      si_min, di_min, ignore,
		      NULL, NULL, NULL, NULL, seeing->residuals.data);

   seeing->residuals.is_median = 0; 
   for(i = 0; i < ncell; i++) {
      seeing->residuals.sig[i] = 0.0;
   }
/*
 * ignore residuals of less than 5e-5
 */
   for(i = ncell - 1;i >= 0; i--) {
      if(fabs(seeing->residuals.data[i]) > 5e-5) {
	 break;
      }
   }
   seeing->residuals.ncell = i + 1;
/*
 * Calculate flux in residual table, setting seeing->residuals.area[] as we go
 */
   residual_flux = 0.0;
   for(i = 0; i < seeing->residuals.ncell; i++) {
      residual_flux += fit_ctx.area[i]*seeing->residuals.data[i];
      seeing->residuals.area[i] = fit_ctx.area[i];
   }
   seeing->residuals.flux = residual_flux;
/*
 * and return the likelihood
 */
#if 0
   seeing->L = phChisqProb(chisq_min, fit_ctx.ncell - 1, L_SOFT);
#else
   seeing->L = chisq_min/(fit_ctx.ncell - 1);
#endif
   
   return(seeing->L);
}

/*****************************************************************************/
/*
 * A simple routine to find an object's second moment. It isn't available
 * globally as its terribly open to abuse; I use it here on well-determined
 * PSFs
 */
static float
find_second_moment(const REGION *reg,	/* region containing object */
		   float rowc, float colc, /* object's centre */
		   float sky)		/* sky value */
{
   int i, j;
   int nrow, ncol;			/* == reg->n{row,col} */
   double r2, c2;			/* == ({row,col} - {row,col}c)**2 */
   PIX *row;				/* == reg->ROWS[] */
   double sum, sumQ;			/* sum(I), sum((r*2 + c^2)*I) */
   double val;				/* value of a pixel */
   
   shAssert(reg != NULL && reg->type == TYPE_PIX);

   nrow = reg->nrow; ncol = reg->ncol;

   sum = sumQ = 0;

   rowc -= 0.5; colc -= 0.5;		/* centre of pixel's (0.5, 0.5) */

   for(i = 0; i < nrow; i++) {
      row = reg->ROWS[i];
      r2 = (i - rowc)*(i - rowc);

#if QMOMENT_RMAX > 0
      if(r2 > QMOMENT_RMAX*QMOMENT_RMAX) {
	 continue;
      }
#endif
	   
      for(j = 0; j < ncol; j++) {
	 c2 = (j - colc)*(j - colc);
	 
#if QMOMENT_RMAX > 0
	 if(r2 + c2 > QMOMENT_RMAX*QMOMENT_RMAX) {
	    if(j > colc) {
	       break;			/* no more points in this row */
	    } else {
	       continue;
	    }
	 }
#endif
	 
	 val = row[j] - sky;
	 sum += val;
	 sumQ += val*(r2 + c2);
      }
   }

   if(sum <= 0) {
      return(-1);
   } else {
      return(0.5*sumQ/sum);		/* r^2 or c^2 moment, not sum */
   }
}

/*****************************************************************************/
/*
 * Various interfaces which eventually call fit_cell_seeing_psf_cellProf
 */
static float
fit_cell_seeing_psf_cells(const CELL_STATS *stats_obj,
			  float tau2,	/* second moment of PSF, evaluated
					   within QMOMENT_RMAX */
			  float gain,	/* amplifier gain */
			  float dark_variance, /* variance at zero flux */
			  float sky,	/* sky level */
			  PSF_COEFFS *seeing, /* describe the PSF */
			  const PSF_CONSTRAINT *constraint) /* If non-NULL,
						 only consider nearby models */
{
   float L;				/* returned likelihood */
   int use_difference = 0;		/* include difference of cell
					   pairs in variance? */
   int sky_noise_only = 0;		/* only include sky noise */
   
   shAssert(stats_obj != NULL);

   setup_cell_data(stats_obj, use_median, sky, gain, dark_variance, 0, 0,
		   use_difference, sky_noise_only);		   

   L = fit_cell_seeing_psf_cellProf(seeing, tau2, constraint);

   return(L);
}

static float
fit_cell_seeing_psf(const REGION *reg,	/* region containing psf */
		    float sigma,	/* smooth by N(0,sigma^2) to centroid*/
		    float sky,		/* sky level */
		    DGPSF *psf,		/* the PSF to be fit; or NULL */
		    PSF_COEFFS *coeffs, /* the cellfit coeffs; or NULL */
		    const PSF_CONSTRAINT *constraint) /* If non-NULL, only
						      consider nearby models */
{
   float chisq;				/* returned chi^2 */
   float bkgd = SOFT_BIAS;		/* non-subtracted background level */
   int rpeak = reg->nrow/2, cpeak = reg->ncol/2; /* guess as to peak pixel */
   float dark_variance = 0;		/* variance at zero flux */
   float gain = 1;			/* gain of amplifier */
   int nannuli;				/* number of annuli to extract */
   float rowc, colc;			/* centre of object */
   const CELL_STATS *stats_obj;
   float tau2;				/* second moment of PSF */
   
   shAssert(model_off != NULL);		/* we're initialised */

   if(coeffs == NULL) {
      shAssert(psf != NULL);
      coeffs = &psf->coeffs;
   }
   shAssert(coeffs != NULL);

   if(phObjectCenterFind(reg, cpeak, rpeak, sky,
			 sigma, bkgd, dark_variance, gain,
			 &colc, &rowc, NULL, NULL, NULL, NULL, NULL) < -4) {
      shError("fit_cell_seeing_psf: error finding centre of PSF");
      rowc = rpeak; colc = cpeak;
   }
/*
 * Estimate object's second moment.
 */
   tau2 = find_second_moment(reg, rowc, colc, SOFT_BIAS + sky);
/*
 * extract profile and do the work
 */
   nannuli = (psf_cache_ncells - 1)/(NSEC/2) + 1; /* we don't need any more */
   stats_obj = phProfileExtract(-1, -1, reg, rowc, colc, -nannuli,
				SOFT_BIAS + sky, 0.0, 0);
   if(stats_obj == NULL || stats_obj->syncreg == NULL) {
      shError("fit_cell_seeing_psf: "
	      "star is too close to the edge to determine PSF");
      chisq = 1e10;
   } else {
      chisq = fit_cell_seeing_psf_cells(stats_obj, tau2,
					gain, dark_variance, sky, coeffs,
					constraint);
      if(psf != NULL) {
	 phDgpsfFitFromCellprof(psf, &fit_ctx);

	 psf->a = 1.0;
	 psf->width =			/* n.b. is this sqrt(2) too small?? */
	   phGaussianWidthEstimate(reg, rowc, colc, SOFT_BIAS + sky, NULL);
      }
   }

   return(chisq);
}

/*****************************************************************************/
/*
 * Find the best represention of the KL PSF at the centre of the frame
 * in terms of the PSFs used to build the model tables. The indices of
 * the single- and double-components are saved in psf_[sd]i0.
 */
void
phConstrainPsfModels(PSF_CONSTRAINT *constraints,
		     int color,
		     const FIELDPARAMS *fiparams,
		     int nmodel)	/* constrain to this range of models */
{
   const FRAMEPARAMS *fparams;
   PSF_REG *KLPsf;
   const DGPSF *psf;			/* estimate of the PSF;
					   N.b. We don't own this object! */
 
   shAssert(color >= 0 && color < NCOLOR);

   if(fiparams == NULL ||
      fiparams->frame[color].psfBasis == NULL) { /* invalidate constraints */
      constraints[color].nmodel = -1;
      constraints[color].si = constraints[color].di = -1;

      return;
   }

   fparams = &fiparams->frame[color];
   shAssert(fparams->data != NULL);

   KLPsf = phPsfKLReconstruct(fparams->psfBasis,
			      fparams->data->nrow/2, fparams->data->ncol/2,
			      TYPE_PIX);
   
   psf = phFitPsfFromReg(KLPsf->reg, fparams->psf->sigma1_2G, 0, NULL);

   constraints[color].nmodel = nmodel;
   constraints[color].si = (psf->coeffs.i[2] - 1)/2;
   constraints[color].di = psf->coeffs.i[0]/2;

   phPsfRegDel(KLPsf);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return the coefficients for the best cell fit to a psf.
 */
const DGPSF *
phFitPsfFromReg(const REGION *reg,	/* region containing psf */
		float sigma,		/* smooth by N(0,sigma^2) to centroid*/
		float sky,		/* background level */
		const PSF_CONSTRAINT *constraint) /* if non-null only consider
						     models near constraint */
{
   (void)fit_cell_seeing_psf(reg, sigma, sky, &static_psf, NULL, constraint);

   return(&static_psf);
}

const PSF_COEFFS *
phFitCellPsfCoeffs(const REGION *reg,	/* region containing psf */
		   float sigma,		/* smooth by N(0,sigma^2) to centroid*/
		   float sky)		/* background level */
{
   PSF_CONSTRAINT *cons = NULL;		/* don't constrain PSF fit */
   
   (void)fit_cell_seeing_psf(reg, sigma, sky, NULL, &static_psf.coeffs, cons);

   return(&static_psf.coeffs);
}

const PSF_COEFFS *
phFitCellPsfCoeffsFromCells(const CELL_STATS *prof, /* extracted profile */
			    const FRAMEPARAMS *fparams, /* describe frame */
			    float sky)
{
   PSF_CONSTRAINT *cons = NULL;		/* don't constrain PSF fit */
/*
 * N.b. this estimate of tau2 is not very good, and is biased high; the
 * problem is gradients within the cells. Hence the fiddle factor fac
 */
   float tau2 = phSigma2GetFromProfile(prof, NULL, NULL);
   float fac = 0.7;
   
   (void)fit_cell_seeing_psf_cells(prof, fac*tau2,
				   phGain(fparams, prof->row_c, prof->col_c),
				   phDarkVariance(fparams, prof->row_c, prof->col_c),				   
				   sky, &static_psf.coeffs, cons);

   return(&static_psf.coeffs);
}

const PSF_COEFFS *
phFitCellPsfCoeffsFromCellProf(const CELL_PROF *prof) /* extracted profile */
{
   PSF_CONSTRAINT *cons = NULL;		/* don't constrain PSF fit */
   float tau2 = 0;			/* second moment of object; XXX */
   
   fit_ctx = *prof;
   (void)fit_cell_seeing_psf_cellProf(&static_psf.coeffs, tau2, cons);

   return(&static_psf.coeffs);
}

/*****************************************************************************/
/*
 * Return a set of IORDER_SEEING DGPSFs corresponding to a PSF_COEFFS; note
 * that you are responsible for freeing them
 */
static void
dgpsfs_from_psf_coeffs(const PSF_COEFFS *seeing,
		      DGPSF *psfs[IORDER_SEEING])
{
   const int di = seeing->i[0]/2;	/* even models are double Gaussian */
   const int si = (seeing->i[2] - 1)/2;	/* odd models are single Gaussian */
   int i;
/*
 * Find the correct sigma/b values
 */
   shAssert(IORDER_SEEING == 4);	/* => split into single/double */
   for(i = 0; i < IORDER_SEEING; i++) {
      psfs[i] = phDgpsfNew();

      if(i >= 2) {			/* single Gaussian */
	 psfs[i]->sigmax1 = cellProCatHead.fseeing +
					     (si + i%2)*cellProCatHead.dseeing;
      } else {				/* double Gaussian */
	 psfs[i]->sigmax1 = cellProCatHead.fseeing +
					     (di + i%2)*cellProCatHead.dseeing;
	 psfs[i]->sigmax2 = psfs[i]->sigmax1*cellProCatHead.seeing_ratio;
	 psfs[i]->b = cellProCatHead.seeing_b;
      }
      psfs[i]->sigmay1 = psfs[i]->sigmax1;
      psfs[i]->sigmay2 = psfs[i]->sigmax2;
      psfs[i]->a = seeing->w[i];
   }
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

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Fit an object with a known model specified as values of r_e, a/b, and phi
 * for both the deV and exp best-fit models.
 *
 * The fraction of the counts in the deV, the total counts in the model, and
 * an estimate of the error therein, are returned if the corresponding pointers
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
			  float *frac_deV) /* fraction of counts in the deV */
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
   
   fit_phi = 0;

   cell_fit_func = cell_fit_BD;
   p[npar++] = DEV_MODEL;
   p[npar++] = ab_deV;
   p[npar++] = re_deV;
   p[npar++] = phi_deV;

   p[npar++] = EXP_MODEL;
   p[npar++] = ab_exp;
   p[npar++] = re_exp;
   p[npar++] = phi_exp;
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
   fit_sky_level = 0;
   fvec = alloca(fit_ctx.ncell*sizeof(double));
   i = 0; cell_fit_func(fit_ctx.ncell, npar, p, fvec, &i);
/*
 * set return variables, if so desired
 */
   if(counts != NULL) {
      *counts = totflux;
   }
   if(countsErr != NULL) {
      *countsErr = totfluxErr;
   }
   if(frac_deV != NULL) {
      *frac_deV = model_amp2[0]/(model_amp2[0] + model_amp2[1]);
   }
/*
 * Evaluate chi^2
 */
   chisq = 0.0;
   for(i = 0; i < fit_ctx.ncell; i++) {
      chisq += fvec[i]*fvec[i];
   }

   if(nu != NULL) {
      *nu = fit_ctx.ncell - 2;		/* we _didn't_ fit npar parameters */
      if(fit_sky_level) (*nu)--;
   }

   return(chisq);
}

/*****************************************************************************/
/*
 * Given a COMP_CSTATS, return a pointer to the static CELL_PROF corresponding
 * to the unpacked version of that model
 */
static const CELL_PROF *
cstats_to_cellprof(const float phi,
		   const COMP_CSTATS *stats_model)
{
   static CELL_PROF cprof;		/* model to return */
   int i;
   float *model;			/* values of the desired profile */
   int nannuli;				/* number of annuli */
   int ncells;				/* number of cells in model */

   ncells = stats_model->ncells;
   nannuli = (ncells - 1)/(NSEC/2) + 1;
   model = shMalloc(ncells*sizeof(float));
   {
       const float *_model =
	   use_median ? stats_model->median : stats_model->mean;
       shAssert(_model != NULL);
       memcpy(model, _model, ncells*sizeof(float));
   }
   
   expand_model(phi*M_PI/180.0, model, nannuli);
/*
 * correct for the known discrepancy between the psf and our best
 * representation of it
 */
   if(seeing_ind != NULL) {		/* i.e. we have a PSF model */
      const float I0 = model[0];	/* central value of model */
      float *residuals = seeing_ind->residuals.data;
      int nresid = (ncells < seeing_ind->residuals.ncell) ?
	ncells : seeing_ind->residuals.ncell;
      for(i = 0;i < nresid;i++) {
	 model[i] += I0*residuals[i];
      }
   }
/*
 * pack that model into cprof.
 */
   cprof.ncell = ncells;
   cprof.is_median = use_median;
   memcpy(cprof.data, model, ncells*sizeof(float));
   memset(cprof.sig, '\0', ncells*sizeof(float));
   memset(cprof.area, '\0', ncells*sizeof(float));

   return(&cprof);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return the cell profile for a given model
 *
 * N.b. this routine is untested. RHL.
 */
#if 1
const CELL_PROF *
phCellProfGetFromKnownModel(int class,	/* type of model */
			    float counts,
			    float ab,
			    float phi,
			    float re)
{
   COMP_CSTATS *(*cells_make)(const MODEL_PARAMS *p) = NULL; /* make a desired model*/
   MODEL_PARAMS mp;			/* model parameters */
   COMP_CSTATS *stats_model;		/* desired model */

   mp.class = class;
   mp.psf = NULL;
   switch (class) {
    case PSF_MODEL:
      mp.aratio = 1.0;
      mp.rsize = 0;
      cells_make = psf_cells_make;
      break;
    case DEV_MODEL:
      mp.aratio = ab;
      mp.rsize = re;
      cells_make = dev_cells_make;
      break;
    case EXP_MODEL:
      mp.aratio = ab;
      mp.rsize = re;
      cells_make = exp_cells_make;
      break;
    default:
      shFatal("phCellProfGetFromKnownModel: unknown galaxy class %d",class);
   }
/*
 * Build the model and unpack the compressed cell data
 */
   totflux = counts;
   stats_model = (*cells_make)(&mp);

   return cstats_to_cellprof(phi, stats_model);
}
#endif

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Fit an object with a PSF; If there is a residual table, it will be
 * applied to the model
 *
 * If sky is non-NULL, a local background level is fit along with the
 * psf, and it's central value returned.
 *
 * If nannuli is >= 1, only that many annuli will be used in the fit
 *
 * Return a measure of the goodness of fit
 */
float
phFitCellProfAsPsf(const CELL_PROF *cprof, /* cell profile for object */
		   float *sky,		/* the sky level; can be NULL */
		   int nannuli)		/* number of annuli to use */
{
   float chisq;
   double *fvec;			/* residuals for cells */
   int i;
   float nu;				/* no. of degrees of freedom for fit */
   
   shAssert(model_off != NULL);		/* we're initialised */
   shAssert(seeing_ind != NULL);	/* with a PSF selected */

   set_rsize_limits(&psf_entries);

   fit_ctx = *cprof;

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
   fit_phi = 0; model_phi = 0;
   fvec = alloca(fit_ctx.ncell*sizeof(double));
   i = 0; cell_fit_psf(fit_ctx.ncell, 0, NULL, fvec, &i);
   if(sky != NULL) {
      *sky = sky_level;
   }
/*
 * Evaluate chi^2
 */
   chisq = 0.0;
   for(i = 0; i < fit_ctx.ncell; i++) {
      chisq += fvec[i]*fvec[i];
   }

   nu = fit_ctx.ncell - 1;
   if(fit_sky_level) nu--;
#if 0
   return(phChisqProb(chisq, nu, L_SOFT));
#else
   return(chisq/nu);
#endif
}

static int
fit_cell_as_galaxy(OBJECT1 *obj,            /* Object to fit */
		   const CELL_STATS *stats_obj, /* object's cell array */
		   int color,               /* color of object */
		   const FIELDPARAMS *fiparams, /* describe field */
		   int nannuli,		/* number of annuli to use, if > 0 */
		   int sky_noise_only,	/* cell variance == sky noise? */
		   int class,		/* class of galaxy */
		   float *chisq,	/* chisq for this fit */
		   int *nu,		/* number of degrees of freedom */
		   float *re,		/* best fit model's size */
		   float *ab,		/* best fit model's axis ratio */
		   float *phi,		/* best fit model's position angle */
		   float *flux,		/* best fit model's total flux
					   (or NULL) */
		   float *reErr,	/* error in size */
		   float *abErr,	/* error in axis ratio */
		   float *phiErr,	/* error in position angle */
		   float *fluxErr)	/* error in total flux (or NULL) */
{
    void (*cell_fit_func)(int, int, double *, double *, int *) = NULL;
    MAT *covar;				/* covariance matrix of fit */
    const FRAMEPARAMS *fparams;         /* unpacked from fiparams */
    float factor;			/* scale for LM steps */
    int info;                           /* return code from LM minimiser */
    double norm;                        /* p[]'s norm */
    double p[NPARAM];                   /* model parameters */
    int nfev;
    const int npar = 2;
    float posErr;			/* error in position */
    double tol = 1e-6;
    double sumsq_tol;
    int use_difference = 0;		/* include difference of cell pairs
					   in cell variance */

    shAssert(model_off != NULL);	/* we're initialised */
    shAssert(obj != NULL);
#if 0
    shAssert(obj->profMean[0] > -999);
#else
    if(obj->profMean[0] < -999) {
       shErrStackPush("fit_cell_as_galaxy: "
		      "obj->profMean[0] == %g < -999 (id == %d)",
						    obj->profMean[0], obj->id);
       obj->flags |= OBJECT1_BADSKY;
       return(SH_GENERIC_ERROR);
    }
#endif
    shAssert(fiparams != NULL && color >= 0 && color < fiparams->ncolor);
    shAssert(npar <= NPARAM);
    fparams = &fiparams->frame[color];
    shAssert(fparams != NULL && fparams->data != NULL && fparams->psf != NULL);
    
    if(stats_obj == NULL) {		/* measure objects wasn't run */
       stats_obj = (const CELL_STATS *)
	 phProfileExtract(-1, -1, fparams->data, obj->rowc, obj->colc,
			  find_outer(obj), SOFT_BIAS + fparams->bkgd,
			  obj->skyErr, 0);
       shAssert(stats_obj != NULL && stats_obj->syncreg != NULL);
       estimate_params(obj, stats_obj);
    }

    if(obj->rowcErr < 0) {
       posErr = 1;
    } else {
       posErr = sqrt(pow(obj->rowcErr,2) + pow(obj->colcErr,2));
    }

    setup_cell_data(stats_obj, use_median, obj->sky,
		    phGain(fparams, obj->rowc, obj->colc),
		    phDarkVariance(fparams, obj->rowc, obj->colc),
		    fparams->psf->width, posErr, use_difference, sky_noise_only);

    fit_phi = 1;			/* solve for position angle */
    fit_sky_level = 0;
    switch (class) {
     case DEV_MODEL:
       set_rsize_limits(&dev_entries);
       cell_fit_func = cell_fit_dev;
       break;
     case EXP_MODEL:
       set_rsize_limits(&exp_entries);
       cell_fit_func = cell_fit_exp;
       break;
     default:
       shFatal("fit_cell_as_galaxy: unknown galaxy class %d",class);
    }
/*
 * Only use some annuli?
 */
    if(nannuli > 0) {
       int ncell = 1 + (nannuli - 1)*(NSEC/2);
       if(fit_ctx.ncell > ncell) {
	  fit_ctx.ncell = ncell;
       }
    }
/*
 * Dump chi^2 surface
 */
   {
      static volatile char *chisq_filename = NULL;
      static volatile int foo = 0;
      if (foo) {
	  chisq_filename = "chisq.fits";
	  foo = 0;
      }
      if(chisq_filename != NULL) {
	  int aratio0=0, aratio1=1, naratio=100;
	  int rsize0=0, rsize1=10, nrsize=1000;
	  REGION *chisq =
	      phChisqRegion(cell_fit_func, fit_ctx.ncell,
			    aratio0, aratio1, naratio,
			    rsize0, rsize1, nrsize);
	  
	  shRegWriteAsFits(chisq, (char *)chisq_filename, STANDARD, 2, DEF_NONE, NULL,0);
	  
	  shRegDel(chisq);
	  chisq_filename = NULL;
      }
   }
/*
 * Set up initial guess.
 */
    {
       float P = sqrt(pow(obj->Q,2) + pow(obj->U,2));
       p[0] = (1 - P)/(1 + P);
    }
    p[1] = (obj->petroR50 < 0.1) ? 0.1 : obj->petroR50;

    sumsq_tol = 1;
#if BROKEN_MODEL_MAGS
    factor = 0;				/* choose default */
#else
    factor = 0.1;			/* don't take wild steps */
#endif

    covar = phMatNew(npar, npar);
    info = phLMMinimise(cell_fit_func, fit_ctx.ncell, npar, p, &norm,
			&nfev, tol, sumsq_tol, factor, covar);

    if(info == 0 || info & ~07) {
       norm = 1e10;
    }
/*
 * Reduce the measured parameters to the desired range
 */
    if(p[0] < aratio_min) {		/* aratio */
       p[0] = aratio_min;
    } else if(p[0] > 1.0) {
       p[0] = 1.0/p[0];
       model_phi += M_PI/2;
    }

    if(p[1] < rsize_min) {		/* rsize */
       p[1] = rsize_min;
    }
    
    while(model_phi < 0.0) {
       model_phi += M_PI;
    }
    while(model_phi >= M_PI) {
       model_phi -= M_PI;
    }
/*
 * set return values.
 *
 * If the reduced chi^2 we measure is less than unity, scale up the error
 * estimates by sqrt(nu/chi^2). Why do this? Because we on occasion over
 * estimate errors in the radial profiles, and I don't want this to be
 * reflected in unrealistically low errors in derived parameters.
 */
    {
       float fac;			/* factor discussed in comment above */
       int nu_s;			/* storage for *nu */
       if(nu == NULL) {
	  nu = &nu_s;
       }
       *nu = fit_ctx.ncell - npar - 1;

       if(fit_phi) (*nu)--;
       if(fit_sky_level) (*nu)--;

       *chisq = norm*norm;
       
       fac = (norm*norm < *nu) ? sqrt(*nu)/norm : 1;
       
       *ab = p[0]; *abErr = fac*sqrt(covar->me[0][0]);
       *re = p[1]; *reErr = fac*sqrt(covar->me[1][1]);
       *phi = model_phi*180/M_PI;

       shAssert(*ab >= 0);
       shAssert(*re >= 0);

       *phiErr = model_phiErr;
       if(model_phiErr >= 0) {		/* i.e. a real value */
	  *phiErr *= fac*180/M_PI;
       }
       if(flux != NULL) {
	  *flux = totflux;
       }
       if(fluxErr != NULL) {
	  *fluxErr = fac*totfluxErr;
       }
    }

    phMatDel(covar);

    if(info < 0) {                      /* user exit */
       shFatal("fit_cell_as_galaxy: user exit");
    }
    if(info == 0) {                     /* improper input params */ 
       if(fit_ctx.ncell < npar) {
          shErrStackPush("fit_cell_as_galaxy: too few data points");
       } else {
          shErrStackPush("fit_cell_as_galaxy: "
					    "improper inputs to LM Minimiser");
       }
       return(SH_GENERIC_ERROR);
    } else if(info & ~07) {
       shErrStackPush("fit_cell_as_galaxy: LM Minimiser returns %d",info);
       return(SH_GENERIC_ERROR);
    }
    
    return(SH_SUCCESS);
}

/*
 * <AUTO EXTRACT>
 *
 * Fit an object with an deVaucouleurs model; if <stats_obj> is NULL,
 * the routine will fill it for you.
 *
 * The fit is performed with the noise in each cell given solely by the
 * sky noise (so as to avoid getting different results for the same object
 * as the S/N varies), and then repeated with the correct variance to
 * estimate the noise/likelihood.
 *
 * Returns chi^2/nu
 */
int
phFitCellAsDeV(OBJC *objc,		/* Object to fit */
               int color,               /* color of object */
	       const CELL_STATS *cstats, /* object's cell array */
               const FIELDPARAMS *fiparams, /* describe field */
	       int nannuli,		/* number of annuli to use, if > 0 */
	       int sky_noise_only)	/* only consider sky noise */
{
   OBJECT1 *obj1;
   int use_difference = 0;		/* use difference term in variance */

   shAssert(objc != NULL && color >= 0 && color < objc->ncolor);
   obj1 = objc->color[color];
   shAssert(obj1 != NULL);

#if TEST_2D_PROFILES
   if(objc->test == NULL) objc->test = phTestInfoNew(objc->ncolor);
#endif
   
   {
      static volatile char *chisq_filename = NULL;
      if(chisq_filename != NULL) {
	 int use_deV=1;
	 int aratio0=0, aratio1=1, naratio=100;
	 int rsize0=0, rsize1=3, nrsize=300;
	 int per_dof=1;
	 REGION *chisq =
	   phFitCellChisq(obj1, color, fiparams, use_deV,
			  aratio0, aratio1, naratio,
			  rsize0, rsize1, nrsize, sky_noise_only, per_dof);
	 shRegWriteAsFits(chisq, (char *)chisq_filename, STANDARD, 2, DEF_NONE, NULL,0);

	 shRegDel(chisq);
	 chisq_filename = NULL;
      }
   }

   if(fit_cell_as_galaxy(obj1, cstats, color, fiparams, nannuli,sky_noise_only,
			 DEV_MODEL, &obj1->chisq_deV, &obj1->nu_deV,
			 &obj1->r_deV, &obj1->ab_deV, &obj1->phi_deV,
			 &obj1->counts_deV,
			 &obj1->r_deVErr, &obj1->ab_deVErr, &obj1->phi_deVErr,
			 &obj1->counts_deVErr) == SH_SUCCESS) {
#if 0 && TEST_2D_PROFILES
      objc->test->nprof2D[color] = test_nprof2D;
      memcpy(objc->test->profMean2D[color], test_profMean2D,
						   test_nprof2D*sizeof(float));
      memcpy(objc->test->profErr2D[color], test_profErr2D,
						   test_nprof2D*sizeof(float));
      memcpy(objc->test->profdeV2D[color], test_profModel2D,
						   test_nprof2D*sizeof(float));
#endif
      sky_noise_only = 0;
      obj1->chisq_deV =
	phFitCellAsKnownModel(objc, color, cstats, fiparams, nannuli, DEV_MODEL,
			      obj1->ab_deV, obj1->phi_deV, obj1->r_deV,
			      use_difference, sky_noise_only,
			      NULL, NULL, NULL, NULL);

#if 1 && TEST_2D_PROFILES
      objc->test->nprof2D[color] = test_nprof2D;
      memcpy(objc->test->profMean2D[color], test_profMean2D,
						   test_nprof2D*sizeof(float));
      memcpy(objc->test->profErr2D[color], test_profErr2D,
						   test_nprof2D*sizeof(float));
      memcpy(objc->test->profdeV2D[color], test_profModel2D,
						   test_nprof2D*sizeof(float));
#endif

      obj1->deV_lnL = phChisqProb(obj1->chisq_deV, obj1->nu_deV, 1);
      obj1->deV_L = exp(obj1->deV_lnL);
      
      return(obj1->chisq_deV/obj1->nu_deV);
   } else {
      obj1->deV_lnL = -MAXLOG;
      obj1->deV_L = 0;
      return(2e9);
   }
}

/*
 * <AUTO EXTRACT>
 *
 * Fit an object with an exponential disk model; if <stats_obj> is NULL,
 * the routine will fill it for you
 *
 * The fit is performed with the noise in each cell given solely by the
 * sky noise (so as to avoid getting different results for the same object
 * as the S/N varies), and then repeated with the correct variance to
 * estimate the noise/likelihood.
 *
 * Returns chi^2/nu
 */
int
phFitCellAsExp(OBJC *objc,		/* Object to fit */
               int color,               /* color of object */
	       const CELL_STATS *cstats, /* object's cell array */
               const FIELDPARAMS *fiparams, /* describe field */
	       int nannuli,		/* number of annuli to use, if > 0 */
	       int sky_noise_only)	/* only consider sky noise */
{
   OBJECT1 *obj1;
   int use_difference = 0;		/* use difference term in variance */

   shAssert(objc != NULL && color >= 0 && color < objc->ncolor);
   obj1 = objc->color[color];
   shAssert(obj1 != NULL);

   if(fit_cell_as_galaxy(obj1, cstats, color, fiparams, nannuli, sky_noise_only,
			 EXP_MODEL, &obj1->chisq_exp, &obj1->nu_exp,
			 &obj1->r_exp, &obj1->ab_exp, &obj1->phi_exp,
			 &obj1->counts_exp,
			 &obj1->r_expErr, &obj1->ab_expErr, &obj1->phi_expErr,
			 &obj1->counts_expErr) == SH_SUCCESS) {

      sky_noise_only = 0;
      obj1->chisq_exp =
	phFitCellAsKnownModel(objc, color, cstats, fiparams, nannuli, EXP_MODEL,
			      obj1->ab_exp, obj1->phi_exp, obj1->r_exp,
			      use_difference, sky_noise_only,
			      NULL, NULL, NULL, NULL);
      
      obj1->exp_lnL = phChisqProb(obj1->chisq_exp, obj1->nu_exp, 1);
      obj1->exp_L = exp(obj1->exp_lnL);
      
      return(obj1->chisq_exp/obj1->nu_exp);
   } else {
      obj1->exp_lnL = -MAXLOG;
      obj1->exp_L = 0;
      return(2e9);
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * A debugging function to return a REGION that contains a 2-dimensional
 * cut through the chi^2 surface associated with fitting a model
 */
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
	       int per_dof)		/* divide chi^2 by degrees of freedom*/
{
   float chisq;				/* chisq for a model */
   REGION *creg;			/* region of chisq values */
   float daratio, drsize;		/* delta between aratio/rsize values */
   const FRAMEPARAMS *fparams;		/* unpacked from fiparams */
   double *fvec;			/* residuals */
   int i, j, k;
   int iflag = 1;			/* needed by cell_fit_{deV,exp} */
   int ndata;				/* number of data points */
   double p[NPARAM];			/* model parameters */
   float posErr;			/* error in position */
   const REGION *reg;			/* the region wherein the obj dwells */
   const int npar = 2;
   CELL_STATS *stats_obj;
   int use_difference = 0;		/* include difference of cell pairs
					   in cell variance */
   
   shAssert(model_cache != NULL);
   shAssert(obj != NULL && obj->mask != NULL);
   shAssert(fiparams != NULL && color >= 0 && color < fiparams->ncolor);
   shAssert(npar <= NPARAM);
   fparams = &fiparams->frame[color];
   shAssert(fparams != NULL && fparams->data != NULL && fparams->psf != NULL);
   reg = fparams->data;
   
   stats_obj = phProfileGetLast();
   if(stats_obj->id != obj->id) {
      stats_obj =
	   phProfileExtract(-1, -1, reg, obj->rowc, obj->colc, find_outer(obj),
			    SOFT_BIAS + fparams->bkgd, obj->skyErr, 0);
   }

   if(obj->rowcErr < 0) {
      posErr = 1;
   } else {
      posErr = sqrt(pow(obj->rowcErr,2) + pow(obj->colcErr,2));
   }

   setup_cell_data(stats_obj, use_median, obj->sky,
		   phGain(fparams, obj->rowc, obj->colc),
		   phDarkVariance(fparams, obj->rowc, obj->colc),
		   fparams->psf->width, posErr, use_difference, sky_noise_only);
/*
 * make output model and residual vector
 */
   ndata = fit_ctx.ncell;
   creg = shRegNew("chisq",naratio,nrsize,TYPE_FL32);
   fvec = shMalloc(ndata*sizeof(double));
/*
 * Parameters to fit
 */   
   daratio = (naratio == 1) ? 1 : (aratio1 - aratio0)/(naratio - 1);
   drsize = (nrsize == 1) ? 1 : (rsize1 - rsize0)/(nrsize - 1);

   set_rsize_limits(use_deV ? &dev_entries : &exp_entries);
   
   for(i = 0;i < naratio;i++) {
      p[0] = aratio0 + i*daratio;
      for(j = 0;j < nrsize;j++) {
	 if(i == naratio - 1 && j == 0) {			/* PSF */
	    cell_fit_psf(ndata, 0, NULL, fvec, &iflag);
	 } else { 
	    p[1] = rsize0 + j*drsize;
	    
	    fit_phi = 1;
	    if(use_deV) {
	       cell_fit_dev(ndata, npar, p, fvec, &iflag);
	    } else {
	       cell_fit_exp(ndata, npar, p, fvec, &iflag);
	    }
	 }

	 chisq = 0;
	 for(k = 0;k < ndata;k++) {
	    chisq += fvec[k]*fvec[k];
	 }

	 if(per_dof) {
	    chisq /= (ndata - npar - fit_phi);
	 }

	 creg->rows_fl32[i][j] = chisq;
      }
   }

   shFree(fvec);

   return(creg);
}

/*
 * Another debugging function; this one uses the cell_fit_func
 * that we pass to LMMinimise
 */
static REGION *
phChisqRegion(void (*cell_fit_func)(int m, int n, double *p, double *fvec, int *iflag),
	      int m,			/* number of data points */
	      float aratio0,		/* initial value of a/b */
	      float aratio1,		/* final value of a/b */
	      int naratio,		/* number of aratios */
	      float rsize0,		/* initial value of re */
	      float rsize1,		/* final value of re */
	      int nrsize)		/* number of rsizes */
{
    double chisq;			/* calculated chi^2 for a model */
    REGION *creg = shRegNew("chisq",naratio,nrsize,TYPE_FL32); /* region of chisq values */
    double *fvec = shMalloc(m*sizeof(double)); /* residual vector */
    int i, j, k;
    const int n = 2;			/* number of parameters */
    int iflag = 0;
    double p[NPARAM];			/* model parameters */    
    const float daratio = (naratio == 1) ? 1 : (aratio1 - aratio0)/(naratio - 1);
    const float drsize = (nrsize == 1) ? 1 : (rsize1 - rsize0)/(nrsize - 1);
   
    for(i = 0;i < naratio;i++) {
	p[0] = aratio0 + i*daratio;
	for(j = 0;j < nrsize;j++) {
	    p[1] = rsize0 + j*drsize;
	    
	    cell_fit_func(m, n, p, fvec, &iflag);
	    
	    chisq = 0;
	    for(k = 0;k < m; k++) {
		chisq += fvec[k]*fvec[k];
	    }
	    
	    creg->rows_fl32[i][j] = chisq;
	}
    }
    
    shFree(fvec);

    return(creg);
}

/******************************************************************************/

static REGION *cstats_to_region(const float phi,
				const COMP_CSTATS *cstmp,
				int *max)			/* max. pixel value before rotation,
								   including any SOFT_BIAS, or NULL */
{
    const CELL_PROF *cellprof = cstats_to_cellprof(phi, cstmp);
    int i;
    int r, c;
    const int nrow = 1 + 2*SYNCRAD + 2;
    const int ncol = nrow;
    REGION *oreg = shRegNew("cstats", nrow, ncol, TYPE_FL32);

    shRegClear(oreg);

    for (r = 0; r < nrow; r++) {
	for (c = 0; c < ncol; c++) {
	    int cell = phGetCellid(r - nrow/2, c - ncol/2);
	    if (cell < 0) {
		continue;
	    } else if (cell == 0) {
		i = 0;
	    } else {
		int isec = (cell - 1)%NSEC;// sector index
		int iann = (cell - 1)/NSEC;// annulus index
		i = 1 + iann*NSEC/2 + isec%(NSEC/2); // index into CELL_PROF

		if (i >= cellprof->ncell) {
		    continue;
		}
	    }

	    oreg->rows_fl32[r][c] = cellprof->data[i];
	}
    }

    if (max != NULL) {
	*max = oreg->rows_fl32[nrow/2][ncol/2] + 0.5;
    }

    return oreg;
}

/*****************************************************************************/
/*
 * Return a region containing the model of a galaxy, possibly allowing for the
 * discrepancy between the PSF and the model PSF
 *
 * The algorithm used parallels that used to reconstruct the cellarray
 * models used in fitting
 */
REGION *
phFittedModelMake(const PSF_BASIS *basis, /* PSF for this field */
		  const OBJC *objc,	/* reconstruct this object */
		  int c,		/* in this band */
		  int class,		/* type of model desired */
		  float sigma,		/* smoothing sigma for centroiding */
		  int correct_residuals, /* correct for residuals from KL? */
		  int exact,		/* make exact models? */
		  int cellprof)		/* return an image of the cellprof model */
{
   const float bkgd = SOFT_BIAS;	/* non-subtracted background level */
   int cell_index;			/* index to extracted profile */
   const COMP_CSTATS *cstmp;		/* a model read from disk */
   float counts;			/* counts in desired model */
   const float dark_variance = 0;	/* variance at zero flux */
   const float gain = 1;		/* gain of amplifier */
   REGION *gal = NULL;			/* the desired galaxy model */
   int gal_cpeak, gal_rpeak;		/* galaxy model's peak pixel */
   float gal_rowc, gal_colc;		/* centre of galaxy model */
   int i, j;
   PSF_REG *KLPsf;			/* K-L PSF at this point */
   struct p_model index[IORDER_INC*IORDER_SIZE];
   const MODEL_ENTRIES *mod_entries;	/* sets of models */
   int model_max = 0;			/* max pixel value in a model */
   int model_pm;			/* [+-]1; sign for term of model */
   MODEL_PARAMS mp;			/* describe desired model */
   const int nmod = cellProCatHead.prof_cat.proc_maxscat; /* number of models
							   at given seeing */
   const OBJECT1 *obj1;			/* == objc->color[c] */
   REGION *psf_reg;			/* region to construct DG psf */
   float psf_peak[IORDER_SEEING];	/* peak values for PSF components */
   float phi;				/* desired position angle */
   float psf_ratio;			/* ratio DG-PSF/KL-PSF */
   int psf_cpeak, psf_rpeak;		/* KLPsf's peak pixel */
   int psf_nrow, psf_ncol;		/* == KLPsf->reg->n{row,col} */
   float psf_rowc, psf_colc;		/* centre of KLPsf */
   const PSF_COEFFS *seeing;		/* describe desired PSF */
   DGPSF *psfs[IORDER_SEEING];		/* PSFs corresponding to PSF_COEFFS */
   REGION *sgal;			/* == shSubRegNew("", gal, ... */
   REGION *streg;			/* == shSubRegNew("", treg, ... */
   REGION *treg = NULL;			/* temp REGION */
   float treg_peak;			/* peak value for model component */
   float w;				/* weight of seeing component */

   shAssert(model_cache != NULL && catalog_fd != NULL);
   shAssert(objc != NULL && \
	    c >= 0 && c < objc->ncolor && objc->color[c] != NULL);
   obj1 = objc->color[c];
/*
 * Set the PSF at this point in the frame
 */
   (void)phPsfSetAtPoint(c, basis, obj1->rowc, obj1->colc, sigma, NULL);
   KLPsf = phPsfKLReconstruct(basis, obj1->rowc, obj1->colc, TYPE_PIX);
   psf_nrow = KLPsf->reg->nrow; psf_ncol = KLPsf->reg->ncol;

   psf_reg = shRegNew("", psf_nrow, psf_ncol, TYPE_PIX);
   shRegIntSetVal(psf_reg, SOFT_BIAS);

   if(seeing_ind == NULL) {
      return(NULL);
   }
   seeing = &seeing_ind_arr[c];
/*
 * Set the parameters of the desired model
 */
   mp.class = class;
   mp.exact = exact;
   switch (mp.class) {
    case DEV_MODEL:
      mod_entries = &dev_entries;

      mp.psf = NULL;			/* set later */
      phi = obj1->phi_deV; mp.orient = 0; 
      mp.rsize = obj1->r_deV;
      mp.aratio = obj1->ab_deV;

      counts = obj1->counts_deV;
      break;
    case EXP_MODEL:
      mod_entries = &exp_entries;

      mp.psf = NULL;			/* set later */
      phi = obj1->phi_exp; mp.orient = 0; 
      mp.rsize = obj1->r_exp;
      mp.aratio = obj1->ab_exp;

      counts = obj1->counts_exp;
      break;
    case PSF_MODEL:
      mod_entries = &psf_entries;

      mp.psf = NULL;			/* set later */
      phi = 180; mp.orient = 0; 
      mp.rsize = mp.aratio = 0;

      counts = obj1->psfCounts;
      break;
    default:
      shFatal("I don't know how to make models of class %d", class);
      mod_entries = NULL; counts = phi = 0.0; /* NOTREACHED */
      break;				/* NOTREACHED */
   }

   set_rsize_limits(mod_entries);
   if(mp.rsize > rsize_max) {
      mp.rsize = rsize_max;
   } else if(mp.rsize < rsize_min) {
      mp.rsize = rsize_min;
   }
/*
 * Perform the interpolation to make the model, and also the PSF used
 * to create that model.
 *
 * First determine the required models/weights
 */
   mod_param_to_index0(&mp, mod_entries, index);
   for(i = 0; i < IORDER_INC*IORDER_SIZE; i++) {
      index[i].i -= mod_entries->mod_offset;
   }

   dgpsfs_from_psf_coeffs(seeing, psfs);
/*
 * Accumulate a term of the PSF into psf_reg;
 * we only need do this once as all PSFs are the same
 */
   for(j = 0; j < IORDER_SEEING; j++) {
      psf_peak[j] = seeing->w[j];
      phDgpsfAdd(psf_reg, psfs[j], 0,
		 psf_nrow/2 + 0.5, psf_ncol/2 + 0.5, 5000*psf_peak[j]);
   }
/*
 * Sum over the galaxy models; the inner loop reconstitutes the PSF model
 */
   for(i = 0; i < IORDER_INC*IORDER_SIZE; i++) {
      if(exact) {
	 if(i > 0) {			/* only need one component */
	    break;
	 }

	 mp.rsize = mod_entries->entries[index[i].i].scat_reff;
	 mp.aratio = mod_entries->entries[index[i].i].scat_axr;
	 cell_index = mod_entries->entries[index[i].i].scat_index;
      } else {
	 w = index[i].w;
	 if(fabs(w) < 1e-6) {
	    continue;
	 }
	 
	 mp.rsize = mod_entries->entries[index[i].i].scat_reff;
	 mp.aratio = mod_entries->entries[index[i].i].scat_axr;
	 cell_index = mod_entries->entries[index[i].i].scat_index;
      }
/*
 * Sum over seeing components to make a term of the galaxy model;
 * first read cell model to get peak value
 */
      for(j = 0; j < IORDER_SEEING; j++) {
	 float flux;			/* debugging only */
	 mp.psf = psfs[j];
	 if (!cellprof) {
	     treg = phAnyModelMake(&mp, &flux, &model_max, NULL);

	     if(treg->type != TYPE_FL32) {
		 REGION *tmp = treg;
		 treg = shRegNew("floating treg", treg->nrow, treg->ncol, TYPE_FL32);
		 shRegIntCopy(treg, tmp);
		 treg->mask = tmp->mask; tmp->mask = NULL;
		 shRegDel(tmp);
		 
		 model_max -= SOFT_BIAS;
	     }
	 }

	 cstmp = model_read(catalog_fd, cell_index + seeing->i[j]*nmod,
					      (cell_index == 0 ? 1 : 0), NULL);

	 treg_peak = psf_peak[j];
	 if (cellprof) {
	     treg = cstats_to_region(phi, cstmp, &model_max);
	 } else {
	     treg_peak *= cstmp->totflux/flux;
	 }

	 if(treg_peak > 0) {
	    model_pm = 1;
	 } else {
	    treg_peak *= -1;
	    model_pm = -1;
	 }
	 shRegIntConstMult(treg, treg_peak*index[i].w);

	 phSpanmaskDel((SPANMASK *)treg->mask); treg->mask = NULL;

	 if(model_max == 0) {
	    shRegDel(treg);
	    continue;
	 }
/*
 * Do we have a model that we're accumulating?  If not make one.
 * We didn't do this earlier as we had no idea what size would be wanted
 */
	 if(gal == NULL) {
	    int nrow = treg->nrow, ncol = treg->ncol;
	    if(nrow < psf_nrow + 5) {
	       nrow = psf_nrow + 5;
	    }
	    if(ncol < psf_ncol + 5) {
	       ncol = psf_ncol + 5;
	    }
      
	    if(nrow%2 == 0) nrow++;
	    if(ncol%2 == 0) ncol++;
	    
	    gal = shRegNew("", nrow, ncol, treg->type);
	    shRegClear(gal);
	 }
/*
 * Prepare to add new term treg to galaxy model; we may have to fiddle the
 * sizes of regions first
 */
	 sgal = gal;
	 streg = treg;
	 
	 if(gal->nrow != treg->nrow || gal->ncol != treg->ncol) {
	    if(gal->nrow < treg->nrow) { /* expand gal to size of treg */
	       REGION *ngal = shRegNew("", treg->nrow, treg->ncol, treg->type);
	       shAssert(gal->ncol <= treg->ncol);
	       shRegIntSetVal(ngal, gal->rows_fl32[0][0]);
	       
	       sgal = shSubRegNew("", ngal, gal->nrow, gal->ncol,
				  0.5*(ngal->nrow - gal->nrow),
				  0.5*(ngal->ncol - gal->ncol), NO_FLAGS);
	       shRegIntCopy(sgal, gal);
	       shRegDel(gal);
	       
	       shRegDel(sgal);
	       sgal = gal = ngal;
	    } else {			/* expand treg to size of gal */
	       REGION *ntreg = shRegNew("", gal->nrow, gal->ncol, gal->type);
	       shAssert(treg->ncol <= gal->ncol);
	       shRegIntSetVal(ntreg, treg->rows_fl32[0][0]);
	       
	       streg = shSubRegNew("", ntreg, treg->nrow, treg->ncol,
				   0.5*(ntreg->nrow - treg->nrow),
				   0.5*(ntreg->ncol - treg->ncol),NO_FLAGS);
	       shRegIntCopy(streg, treg);
	       shRegDel(treg);
	       
	       shRegDel(streg);
	       streg = treg = ntreg;
	    }
	 }
/*
 * Actually add that component into the galaxy model
 */
	 shRegIntLincom(sgal, streg, -model_pm*treg->rows_fl32[0][0], 1, model_pm, LINCOM_EXACT);
	 
	 if(streg != treg) {
	    shRegDel(streg);
	 }
	 shRegDel(treg);
	 if(sgal != gal) {
	    shRegDel(sgal);
	 }
      }
   }
   shAssert(gal != NULL);

   for(i = 0; i < IORDER_SEEING; i++) {
      phDgpsfDel(psfs[i]); psfs[i] = NULL;
   }
/*
 * Fix total counts to agree with input object, then ensure that the
 * sky level's exactly 0
 */
   {
      float model_counts = 0;
      phRegStatsFromQuartiles(gal, 1, -1,
			      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			      NULL, NULL, &model_counts);
      shRegIntConstMult(gal, counts/model_counts);
      shAssert(gal->rows_fl32[0][0] == 0);
   }

   if (cellprof) {
       shRegDel(psf_reg);
       phPsfRegDel(KLPsf);

       return(gal);
   }
/*
 * Convert back to PIX if neccessary
 */
   if(gal->type != TYPE_PIX) {
      REGION *tmp = gal;
      int ret;
      int rpeak, cpeak;			/* position of maximum */
      ret = phRegIntMaxPixelFind(gal, SOFT_BIAS,
				 gal->nrow/2 - 10, gal->ncol/2 - 10, 
				 gal->nrow/2 + 10, gal->ncol/2 + 10, 
				 &rpeak, &cpeak, NULL, NULL);
      shAssert(ret == 0);
      
      shRegIntConstMult(gal, 60000/gal->rows_fl32[rpeak][cpeak]);

      gal = shRegNew("PIX gal", treg->nrow, treg->ncol, TYPE_PIX);
      shRegIntCopy(gal, tmp);
      gal->mask = tmp->mask; tmp->mask = NULL;
      shRegDel(tmp);
   }
/*
 * Rotate model to desired p.a.
 * XXX Is this correct?  We already built it at approximately the right phi
 */
   phi = 180 - phi;
   if(fabs(phi) > 1e-4) {
      int use_sinc = 1;
      REGION *rgal = shRegIntRotate(gal, phi, gal->ROWS[0][0], use_sinc, 0);
      shRegDel(gal); gal = rgal;
   }
/*
 * find the model galaxy's centre
 */
   gal_rowc = 0.5*gal->nrow; gal_rpeak = (int)gal_rowc;
   gal_colc = 0.5*gal->ncol; gal_cpeak = (int)gal_colc;   
/*
 * correct for the known discrepancy between the psf and our best
 * representation of it
 *
 * The definition adopted in chisq_seeing() is that
 *     residuals = (real_psf - model_psf)/model_psf[0]
 * where the real_psf is properly scaled to minimise the residuals. The
 * error is taken to be Poisson in the model counts, sigma^2 == model,
 * so the desired estimate, sum(data*model/sigma^2)/sum(model^2/sigma^2),
 * reduces to sum(data)/sum(model)
 *
 * The way that these residuals are used is
 *	model += model[0]*residuals
 */
   if(correct_residuals) {
      REGION *residuals = NULL;		/* residuals from PSF fit */

      psf_cpeak = KLPsf->reg->ncol/2; psf_rpeak = KLPsf->reg->nrow/2;
      if(phObjectCenterFind(KLPsf->reg, psf_cpeak, psf_rpeak, 0.0,
			    sigma, bkgd, dark_variance, gain,
			    &psf_colc, &psf_rowc,
			    NULL, NULL, NULL, NULL, NULL) < -4) {
	 psf_rowc = psf_rpeak; psf_colc = psf_cpeak;
	 shError("phFittedModelMake: error finding centre of PSF");
      }
#if 1
      residuals = phRegIntShift(NULL, KLPsf->reg, NULL, 5,
				(psf_rpeak + 0.5) - psf_rowc,
				(psf_cpeak + 0.5) - psf_colc);
#else
      residuals = phRegIntShift(NULL, KLPsf->reg, NULL, 5, 0.0, 0.0);
#endif
      
      {					/* find scale factor for KLPsf */
	 double sumDG = 0.0, sumKL = 0.0;
	 for(i = 0; i < psf_nrow; i++) {
	    for(j = 0; j < psf_ncol; j++) {
	       sumDG += psf_reg->ROWS[i][j] - SOFT_BIAS;
	       sumKL += KLPsf->reg->ROWS[i][j] - SOFT_BIAS;
	    }
	 }
	 psf_ratio = sumDG/sumKL;
      }
      
      w = gal->ROWS[gal_rpeak][gal_cpeak] - SOFT_BIAS;
      w /= psf_reg->ROWS[psf_nrow/2][psf_ncol/2] - SOFT_BIAS;
      
      shRegIntLincom(residuals, psf_reg, (1 - w*(psf_ratio - 1))*SOFT_BIAS,
		     w*psf_ratio, -w, 0);
      shRegIntConstAdd(residuals, SOFT_BIAS - residuals->ROWS[0][0], 0);
#if 0
/*
 * offset residuals to same sub-pixel centre as galaxy model
 */
      (void)phRegIntShift(residuals, residuals, NULL, 5,
			  gal_rowc - (gal_rpeak + 0.5),
			  gal_colc - (gal_cpeak + 0.5));
#endif
/*
 * Add PSF residuals to the galaxy model.
 */
      treg = shSubRegNew("", gal, psf_nrow, psf_ncol,
			 gal_rpeak - psf_nrow/2, gal_cpeak - psf_ncol/2,NO_FLAGS);
      if(treg == NULL) {
	 shError("Failed to correct residuals for object %d at (%.3f, %.3f)",
		 obj1->id, obj1->rowc, obj1->colc);
      } else {
	 shRegIntLincom(treg, residuals, -SOFT_BIAS, 1, 1, LINCOM_EXACT);
	 shRegDel(treg);
      }
      shRegDel(residuals);
   }
/*
 * offset galaxy model to have the same centre as the real object
 */
   {
      float drow = obj1->rowc - gal_rowc;
      float dcol = obj1->colc - gal_colc;
      drow -= (int)drow; dcol -= (int)dcol;
      (void)phRegIntShift(gal, gal, NULL, 5, drow, dcol);
      gal_rowc += drow; gal_colc += dcol;
   }
   gal->row0 = obj1->rowc > gal_rowc ?
		obj1->rowc - gal_rowc + 0.5 : -(-(obj1->rowc - gal_rowc) + 0.5);
   gal->col0 = obj1->colc > gal_colc ?
		obj1->colc - gal_colc + 0.5 : -(-(obj1->colc - gal_colc) + 0.5);
/*
 * Fix total counts to agree with input object, then ensure that the
 * sky level's exactly 0.  We should be close, but we need to be right.
 */
#if 1
   {
      float model_counts = 0;
      phRegStatsFromQuartiles(gal, 1, -1,
			      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			      NULL, NULL, &model_counts);
      model_counts -= (float)gal->ROWS[0][0]*gal->nrow*gal->ncol;
      shRegIntConstMult(gal, counts/model_counts);
      shRegIntConstAdd(gal, SOFT_BIAS - gal->ROWS[0][0], 0);
   }
#endif
/*
 * cleanup
 */
   shRegDel(psf_reg);
   phPsfRegDel(KLPsf);

   return(gal);
}

/******************************************************************************/
 /*
  * Compare two cell profile files
  *
  * N.B. This routine is only for use while debugging problems!
  */
typedef struct {
    CELLPROCATHEADER header;
    int nmodel;
    int nseeing_type;
    COMP_CSTATS ***models;
} ALL_MODELS;


static ALL_MODELS *read_all_models(const char *file, /* file to read */
				   int normalize) /* normalise profiles to centre == 1 */
{
    int i, iseeing, ipsf_type;
    const int nfilter = 1;		/* number of filters */
    const int median = 0;		/* use median not mean? */
    const int normalise = 0;		/* normalise models */
    ALL_MODELS *all = NULL;		/* data to return */
    int nseeing;			/* number of seeings per model */
    int nseeing_type;			/* number of types of PSF */

    if(phInitCellFitobj((char *)file, nfilter, median) != SH_SUCCESS) {
	return NULL;
    }
 /*
  * Allocate space for all those models
  */
    all = shMalloc(sizeof(ALL_MODELS));
    all->header = cellProCatHead;
    {
	char *ptr = strrchr(all->header.idstr, '$'); // catalogue code failed to NUL pad
	if (ptr != NULL && ptr < all->header.idstr + IDSIZE) {
	    *(ptr + 1) = '\0';
	}
    }

    all->nmodel = 
	psf_entries.mod_nincl*psf_entries.mod_nsize +
	exp_entries.mod_nincl*exp_entries.mod_nsize +
	dev_entries.mod_nincl*dev_entries.mod_nsize;
    shAssert(all->nmodel == cellProCatHead.prof_cat.proc_maxscat);
    nseeing = all->header.nseeing;
    nseeing_type = all->nseeing_type = 2;

    all->models = shMalloc(all->nmodel*sizeof(COMP_CSTATS ***));
    all->models[0] = shMalloc(all->nmodel*nseeing*nseeing_type*sizeof(COMP_CSTATS **));
    for (i = 0; i < all->nmodel; i++) {
	all->models[i] = all->models[0] + i*nseeing*nseeing_type;
    }
    memset(all->models[0], '\0', all->nmodel*nseeing*nseeing_type*sizeof(COMP_CSTATS *));
/*
 * Time to read them 
 */
    for(iseeing = 0; iseeing < nseeing; iseeing++) {
	for(ipsf_type = 0; ipsf_type < nseeing_type; ipsf_type++) {
	    for (i = 0; i < all->nmodel; i++) {
		const int j = iseeing + ipsf_type*nseeing; /* index for seeing model */
		all->models[i][j] = comp_cstatsNew(MAXCELLS);
		model_read(catalog_fd, i + j*all->nmodel, normalise, all->models[i][j]);
		
		expand_fourier_cstats(all->models[i][j], normalize);
	    }
	}
    }

    phFiniCellFitobj();

    return all;
}

static void free_all_models(ALL_MODELS *all)
{
    int i,j;
    
    if(all == NULL) return;

    for (i = 0; i < all->nmodel; i++) {
	for (j = 0; j < all->header.nseeing; j++) {
	    comp_cstatsDel(all->models[i][j]);
	}
    }
    
    shFree(all->models[0]);
    shFree(all->models);
    shFree(all);
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

static char *
describe_model(const ALL_MODELS *model, int imodel)
{
    static char descrip[100];

    sprintf(descrip, "%4d %-20s", imodel,
	    phFormatCatentry(model->header.prof_cat.proc_catentry[imodel]));

    return descrip;
}

static char *
describe_seeing(const ALL_MODELS *model, int iseeing,
		int iseeing_type)
{
    static char descrip[100];

    if (iseeing_type == 0) {
	sprintf(descrip, "%4.2fC",
		model->header.fseeing + iseeing*model->header.dseeing);
    } else {
	sprintf(descrip, "%4.2f ",
		model->header.fseeing + iseeing*model->header.dseeing);
    }
    return descrip;
}

static void
compare_cell_models(const float eps,	/* largest allowable fractional difference */
		    const float feps,	/* Maximum allowed fractional difference */		    
		    const ALL_MODELS *model1, /* all the models in file1 */
		    const ALL_MODELS *model2, /* all the models in file2 */
		    int imodel,		/* which model */
		    int iseeing,	/* which seeing */
		    int iseeing_type)   /* which seeng type */
{
    int i;
    int nseeing = model1->header.nseeing;
    const COMP_CSTATS *mod1 = model1->models[imodel][iseeing + iseeing_type*nseeing]; /* first model */
    const COMP_CSTATS *mod2 = model2->models[imodel][iseeing + iseeing_type*nseeing]; /* second model */
    const int ncells = (mod1->ncells <= mod2->ncells) ? mod1->ncells : mod2->ncells;

    if(mod1->ncells != mod2->ncells) {
	fprintf(stderr,"Model %s seeing %s number of cells differ: %d %d\n",
		describe_model(model1, imodel), describe_seeing(model1, iseeing, iseeing_type),
		mod1->ncells, mod2->ncells);
    }		
    if(fabs(mod1->totflux - mod2->totflux) > eps &&
       fabs(mod1->totflux - mod2->totflux) > feps*fabs(mod1->totflux + mod2->totflux)/2) {
	fprintf(stderr,"Model %s seeing %s total flux in object (to infinity) differ: %g %g\n",
		describe_model(model1, imodel), describe_seeing(model1, iseeing, iseeing_type),
		mod1->totflux, mod2->totflux);
    }		
#if 0					/* not stored in the file */
    if(mod1->sigma2 != mod2->sigma2) {
	fprintf(stderr,"Model %s seeing %s second moment of object differ: %g %g\n",
		describe_model(model1, imodel), describe_seeing(model1, iseeing, iseeing_type),
		mod1->sigma2, mod2->sigma2);
    }
#endif

    if((mod1->mean == NULL) + (mod2->mean == NULL) == 1) {
	fprintf(stderr,"Model %s seeing %s only file%d's mean is NULL\n",
		describe_model(model1, imodel), describe_seeing(model1, iseeing, iseeing_type),
		(mod1->mean == NULL) ? 1 : 2);
    }		
    if((mod1->median == NULL) + (mod2->median == NULL) == 1) {
	fprintf(stderr,"Model %s seeing %s only file%d's median is NULL\n",
		describe_model(model1, imodel), describe_seeing(model1, iseeing, iseeing_type),
		(mod1->median == NULL) ? 1 : 2);
    }		
    if((mod1->sig == NULL) + (mod2->sig == NULL) == 1) {
	fprintf(stderr,"Model %s seeing %s only file%d's sig is NULL\n",
		describe_model(model1, imodel), describe_seeing(model1, iseeing, iseeing_type),
		(mod1->sig == NULL) ? 1 : 2);
    }		

    for (i = 0; i < ncells; i++) {
	if(mod1->mean != NULL && mod2->mean != NULL &&
	   fabs(mod1->mean[i] - mod2->mean[i]) > eps &&
	   fabs(mod1->mean[i] - mod2->mean[i]) > feps*fabs(mod1->mean[i] + mod2->mean[i])/2) {
	    fprintf(stderr,"Model %s seeing %s mean[%d:%d] profiles differ: %g %g\n",
		    describe_model(model1, imodel), describe_seeing(model1, iseeing, iseeing_type),
		    (i == 0 ? 1 : 1 + (i - 1)/(NSEC/2)), (i == 0 ? 0 : 1 + (i - 1)%(NSEC/2)),
		    mod1->mean[i], mod2->mean[i]);
	}		

	if(mod1->median != NULL && mod2->median != NULL &&
	   fabs(mod1->median[i] - mod2->median[i]) > eps &&
	   fabs(mod1->median[i] - mod2->median[i]) > feps*fabs(mod1->median[i] + mod2->median[i])/2) {
	    fprintf(stderr,"Model %s seeing %s median[%d:%d] profiles differ: %g %g\n",
		    describe_model(model1, imodel), describe_seeing(model1, iseeing, iseeing_type),
		    (i == 0 ? 1 : 1 + (i - 1)/(NSEC/2)), (i == 0 ? 0 : 1 + (i - 1)%(NSEC/2)),
		    mod1->median[i], mod2->median[i]);
	}		
	if(mod1->sig != NULL && mod2->sig != NULL &&
	   fabs(mod1->sig[i] - mod2->sig[i]) > eps &&
	   fabs(mod1->sig[i] - mod2->sig[i]) > feps*fabs(mod1->sig[i] + mod2->sig[i])/2) {
	    fprintf(stderr,"Model %s seeing %s errors[%d:%d] differ: %g %g\n",
		    describe_model(model1, imodel), describe_seeing(model1, iseeing, iseeing_type),
		    (i == 0 ? 1 : 1 + (i - 1)/(NSEC/2)), (i == 0 ? 0 : 1 + (i - 1)%(NSEC/2)),
		    mod1->sig[i], mod2->sig[i]);
	}
    }
}

int
phCompareProfileFiles(const char *file1, /* One file */
		      const char *file2, /* the other file */
		      int n0,		/* First model to compare (0 indexed) */
		      int n1,		/* Last model to compare (if -ve, counted from end) */
		      const float eps,	/* largest allowable fractional difference */		    
		      const float feps, /* Maximum allowed fractional difference */
		      int normalize)	/* Normalise profiles to centre == 1 */
{
    int i;
    int iseeing;			/* index into size of PSF core */
    int iseeing_type;    		/* is PSF composite or single? */
    ALL_MODELS *model1, *model2;	/* all the models in the file */
    int nmodel;				/* number of models in shorter file */
    int nseeing;			/* number of seeings in shorter file */
    int nseeing_type;			/* number of forms of PSF */

    model1 = read_all_models(file1, normalize);
    model2 = read_all_models(file2, normalize);

    if(model1 == NULL || model2 == NULL) {
	return -1;
    }

    nseeing_type = model1->nseeing_type;

    if(strcmp(model1->header.idstr, model2->header.idstr) != 0) {
	fprintf(stderr,"ID strings differ:\n\"%s\"\n\"%s\"\n",
		model1->header.idstr, model2->header.idstr);
    }
    if(model1->nmodel != model2->nmodel) {
	fprintf(stderr,"Number of models differ: %d %d\n", model1->nmodel, model2->nmodel);
    }
    nmodel = (model1->nmodel <= model2->nmodel) ? model1->nmodel : model2->nmodel;

    if(model1->header.max_models != model2->header.max_models) {
	fprintf(stderr,"max_models differ: %d %d\n",
		model1->header.max_models, model2->header.max_models);
    }
    if(model1->header.crc != model2->header.crc) {
	fprintf(stderr,"Raw profile CRCs differ: 0x%x 0x%x\n",
		model1->header.crc, model2->header.crc);
    }

    if(model1->header.nseeing != model2->header.nseeing) {
	fprintf(stderr,"Number of seeing models differ: %d %d\n",
		model1->header.nseeing, model2->header.nseeing);
    }
    nseeing = (model1->header.nseeing <= model2->header.nseeing) ?
	model1->header.nseeing : model2->header.nseeing;

    if(model1->header.fseeing != model2->header.fseeing) {
	fprintf(stderr,"widths of first seeing differ: %g %g\n",
		model1->header.fseeing, model2->header.fseeing);
    }
    if(model1->header.dseeing != model2->header.dseeing) {
	fprintf(stderr,"seeing deltas differ: %g %g\n",
		model1->header.dseeing, model2->header.dseeing);
    }
    if(model1->header.seeing_ratio != model2->header.seeing_ratio) {
	fprintf(stderr,"seeing ratios differ: %g %g\n",
		model1->header.seeing_ratio, model2->header.seeing_ratio);
    }
    if(model1->header.seeing_b != model2->header.seeing_b) {
	fprintf(stderr,"seeing bs differ: %g %g\n",
		model1->header.seeing_b, model2->header.seeing_b);
    }
/*
 * Check raw headers
 */
    phCompareRawHeaders(&model1->header.prof_cat, &model2->header.prof_cat);
/*
 * Compare cell profiles
 */
    if (n1 < 0) {
	n1 += nmodel;
    }

    if (n0 >= nmodel) {
	n0 = nmodel - 1;
    }
    if (n1 >= nmodel) {
	n1 = nmodel - 1;
    }

    for (i = n0; i <= n1; i++) {
	for (iseeing = 0; iseeing < nseeing; iseeing++) {
	    for (iseeing_type = 0; iseeing_type < nseeing_type; iseeing_type++) {
		compare_cell_models(eps, feps, model1, model2, i, iseeing, iseeing_type);
	    }
	}
    }
 /*
  * Clean up
  */
    free_all_models(model1);
    free_all_models(model2);

    return 0;
}
