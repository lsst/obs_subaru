/* 95/02/23 Photopipe version for dssprof code */

/* This module provides a function which creates a set of raw model
 * profiles for exponential disks and deVaucouleurs bulges in a 
 * range of sizes and flattenings. The catalog is created as a disk
 * file, "rgalprof.dat", which has a header which begins with an
 * instance of the struct spro_cheader which can also be used
 * for later seeing-convolved versions. This is followed by an
 * array of struct spro_catentry which give the sizes, physical
 * parameters, and offsets into the file of each of the 406 images.
 * All images have the major axis along the x axis.
 * The images are stored as upper righ-hand quarters, with a fringe
 * of MAXSM pixels on the right and left to
 * facilitate smoothing. The left-hand fringe has values reflected
 * about the origin (which is at (MAXSM,0), of course); the
 * right-hand fringe is filled with zeros. The images are thus
 * almost ready for smoothing; to do so, one only needs read them into
 * a buffer with a y size 2*MAXSM larger, starting at line MAXSM. 
 * Lines 0 thru MAXSM-1 need to be copied from lines 2*MAXSM thru
 * MAXSM+1 in the data, so as to provide a LOWER fringe which is
 * a reflection of the data about the major axis, and the uppermost
 * MAXSM lines need to be cleared. Then one can smooth the image with
 * a filter of half-width MAXSM or smaller and extract and write the quarter
 * again; the smoothed images can be put in a catalog exactly like
 * the raw ones (there are members in spro_cheader for the psf
 * parameters for a double gaussian) but with different sizes. The smoothed
 * ones will be MAXSM smaller in X, MAXSM larger in y, and the origin
 * will be at 0,0.
 *
 * The indexing is historical, FORTRANny, and rather arcane, for which 
 * I apologize profusely; There are three heirarchical indices into the 
 * catalog, each of which runs sequentially through it:
 * 
 * 1.  classes      Type of profile; 0:psf 1:exponential 2:deV laws
 * 2.  types        Flattening families. 0: psf(round only), 1->NEXPINCL
 *                      exponentials from axis ratio 1 to 0.11, 
 *                      NEXPINCL+1 -> NEXPINCL + NDEVINCL + 1, deV laws
 *                      with axis ratios from 1 to 0.33. The numerical
 *                      values quoted are for the current parameters
 *                      NEXPINCL=9, with an ellipticity ratio from one 
 *                      flattening family to the next of 3^(-1/4)    
 * 3.  index        Sizes. (NEXPSIZ*NEXPINCL + NDEVSIZ*NDEVINCL) total.
 *                      There is 1 psf, NEXPSIZ exponential sizes for each
 *                      exponential flattening type, and NDEVSIZ deVauc sizes
 *                      for each deVauc flattening type. Each size has an
 *                      effective radius 3^(-1/5) smaller than the last,
 *                      beginning with an effective radius of RMEXP and RMDEV
 *                      (about 160 and 80 pixels, respectively).
 *
 * There needs to be a function which returns the type and class given
 * an index; the starting indices for types and classes are in the
 * arrays spro_sindex[] and spro_ctindex[] constructed by sprocatset().
 */
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "dervish.h"
#include "phObjc.h"
#include "phDgpsf.h"
#include "phFitobj.h"
#include "phUtils.h"

#if 0
/* 
 * The anndex array in extract.c should be changed to accomodate the
 * radial ratios used here--it is not absolutely necessary, but would
 * be better; it should look like 
 */
static int anndex[] = 
  {0, 1, 2, 3, 5, 8, 12, 18, 28, 43, 67, 104, 161, 250, 387, 601, 932};
/* 0  1  2  3  4  5   6   7   8   9  10   11   12   13   14   15   16 */
/* 
 * and RLIM, FRLIM should become 600, 599.5 
 */
#endif

/*
 * These should agree with those in the header for the cell arrays; in the
 *final product, this file should #include that header 
 */
#define RLIM 600
#define FRLIM ((double)RLIM - 0.5)
#define SYNCRAD 11

#if FLOATING_MODELS
#  define ROUND(X) (X)
#else
#  define ROUND(X) ((int)((X) + phRandomUniformdev()))
#endif
/*
 * We divide each of the innermost (NSUB)x(NSUB/aratio) pixels into a
 * SUBPIXxSUBPIX grid. Why Divide by aratio?  Because otherwise very
 * flattened models (those for which the flux changes significantly
 * within the central line of pixels) show a jump in surface brightness
 * at the first non-subsampled pixel (why? because we are no longer
 * diluting the surface brightness by points ~0.5 pixels above the axis)
 *
 * SUBPIX has to be _large_ to handle direct calculation of the central
 * pixel for deV profiles with e.g. r_e = 0.1.  This is less of a problem
 * when calculating profiles for rgalprof.dat as the small galaxies are
 * made by binning larger ones; still, calculating rgalprof.dat is much
 * quicker than calculating cellprof.dat, and only has to be done once.
 */
#define NSUB 8				/* subpixellate central pixels */
#define SUBPIX 101 			/* how heavily to subpixellate; odd */

#define CT3(x) ((((x)-1)/3 + 1)*3)
        /* least integer >= x which is divisible by 3 */

#define SPITCH 5  
        /*
         * spacing in size for a reduction in size by a factor of 3; ie
         * RFAC = 3^(1./SPITCH)
         */
        
        /* 
         * Increment in sizes; 3^(1/5); this peculiar choice is to facilitate
         * the easy and accurate generation of smaller profiles from large 
         * ones by rebinning; the same mechanism facilitates the generation
         * of flattened profiles, but there we use a slightly more drastic
         * increment,
         */
#if SPITCH == 5
#  define RFAC 1.24573094
#else
#  error Unsupported value of SPITCH
#endif

#if EQUAL_ARATIO_STEPS
#  define FPITCH (NDEVINCL - 1)
#else
#  if 0
#     define FPITCH 4
#  else
#     define FPITCH 8
#  endif
#endif

        /* 
         * spacing in flattening for a reduction in ellipticity by a factor
         * of 3; ie FFAC = 3^(1./FPITCH). Note that the spacing in the
         * model sequence is FPITCH*NEXPSIZ or FPITCH*NDEVSIZ, whichever
         * is appropriate, since the models are stored with the fastest-
         * changing index being the size, next-fastest being the flattening.
         */

        /* 
         * Increment in flattening; 3^(1/FPITCH);
         */
#if !EQUAL_ARATIO_STEPS
#  if (NDEVINCL < FPITCH || NEXPINCL < FPITCH)
#    error When EQUAL_ARATIO_STEPS is true, you must ensure that N*INCL < FPITCH
#  endif
#  define FFAC 0			/* not used */
#else
#  if FPITCH == 4
#     define FFAC 1.31607401
#  elif FPITCH == 8
#     define FFAC 1.14720269
#  endif
#endif
         
         
#define DEFAC -7.66925     
        /* factor in devlaw exponential */

#define EXPFAC -1.67835
        /* 
         * factor in exp law exponential; (-)half-light radius in scale lengths 
         */

#if 0
#define NEXPINCL ...			/* see phFitobj.h */
#define NDEVINCL ...
#endif
        /* 
         * # of inclinations of exponentials and devlaws in catalog, resp;
         * they decrease geometrically from 1.0 by 3^(-.25), so the
         * flattest devlaws are 3:1 and the flattest exponentials are
         * 9:1.
         */
         
#define NSPROCLASS 3
        /* the profiles are described by TYPES, CLASSES, and SIZES. CLASS
         * is the highest specification, and represents the kinds of profiles,
         * here 3: psf, exponentials, and deVauc laws for now. TYPES represent
         * the different projections (flattenings) of these, and there are
         */

#define NSPROTYPES (1+NEXPINCL+NDEVINCL)
        /* 
         * different TYPES of profile--classes and projected modifications of
         * each--one psf, 9 exps, 5 devlaws for now 
         */

static char *spro_clnam[NSPROCLASS] = {"psf","exp","deV"};
        /* class names */
static char spro_namlist[8*NSPROTYPES];
        /* storage for all the names */
static char *spro_names[NSPROTYPES];
        /* 
         * name array. The entries (created by sprocatset) are of the form
         * exp0.80 for the exponential family with an ellipticity of 0.80,
         * for example.
         */

int NEXPSIZ = 30, NDEVSIZ = 30;
        /* 
         * number of profiles for each type (sizes). Note that here this is
         * only a function of the CLASS, not the TYPE explicitly, though there
         * is no reason this should be so and, in fact, might reasonably not
         * be, since flattened objects at a given flux can be detected to
         * larger radii along their major axes than round ones 
         */

static int spro_cnprof[NSPROCLASS];
        /*
         * numbers of profiles of each class--need to go to 27 for dev
         * so that 8*r_e <= FRLIM; r_e about 82 pixels = 33 arcsec, starting
         * at r_e of about 0.25 pixel (see RMDEV below); for exp,
         * want dia to 4*r_e ( about 6/alpha), so starting at 0.25, need more 
         * models (30). These diameters go to about 1% of B_e, a dynamic 
         * range from the center of about 400 for the exp profiles but 
         * about 1.e5 for the devlaws before smoothing. This latter is a 
         * problem, and we use a small core radius to fix it. 
         * See devlr() below.
         */
        
static int spro_sindex[NSPROTYPES+1];      
    /* starting indices for each type in 1-D MAXSCAT arrays; last index is
     * one past end.
     */

static int spro_ctindex[NSPROCLASS+1]={0,1,(NEXPINCL+1),(NEXPINCL+NDEVINCL+1)};
    /* starting TYPE indices for each CLASS; last entry is one past end */

static int spro_cnaxr[NSPROCLASS]= {1,NEXPINCL,NDEVINCL};    
    /* numbr of inclinations "" "" */

static float spro_outrat[NSPROCLASS] = {0.0,EXPOUT,DEVOUT};
    /* 
     * largest outer radius for each class.
     */

static int spro_class[NSPROTYPES];
    /* class as a fn of profile type--0,1,or 2 */

static float spro_axr[NSPROTYPES];
    /* projected axis ratios */

#define RMEXP (FRLIM/EXPOUT)
#define RMDEV (FRLIM/DEVOUT)
    /* 
     * largest effective radii in catalog (164.375,82.1875); 
     * chosen so that MAXIMUM cutoff radius for each type is FRLIM,
     * the maximum cell radius. 
     */            

static float spro_crmax[NSPROCLASS]={0.0,RMEXP,RMDEV};
    /* largest scale radii */
    
static double sexpr(double),sdevlr(double);
    /* various profile functions */

static double (*spro_fun[NSPROCLASS])(double) = {NULL,sexpr,sdevlr};
    /* defining functions*/

static void makedevtabl(void),makeexptabl(void);
    /* table-making functions */

static void (*spro_tabl[NSPROCLASS])(void) = { NULL,makeexptabl,makedevtabl};
    /* array of table-making function pointers */
        
#define MAXSM 48
    /* maximum half width (width = 2*MAXSM+1) of seeing-smoothing filter; the
     * raw images are stored on disk with a fringe of this width on
     * both sides to facilitate reading in ready to smooth with no
     * further rearrangement. Should be divisible by 3.
     */
    
static long *catbufsidx;
    /* 
     * x-sizes of catalog images
     */

static long *catbufsidy;
    /*
     * y-sizes of catalog images; the images are stored contiguously
     * on disk, and have left and right `fringes' of MAXSM pixels to 
     * facilitate smoothing.
     */

static unsigned int *catbufoff;
    /* 
     * byte offset into quadrant image file or its memory image, 
     * assuming BLKSIZ-byte file blocks; last entry is total size. 
     */

static int spro_bufsmax = 0;
static int spro_qsmax = 0;
    /* 
     * maximum sizes of buffer and maximum buffer side and flag for
     * initialization. 
     * this needs to be fixed, since there are now two kinds of
     * initialization, for raw and smoothed, and it is not clear that
     * one does not need both simultaneously; maybe we need to keep
     * copies of the header struct around.
     */
static int spro_bufsm2 = 0;
static int spro_qsm2 = 0;
    /*
     * maximum sizes of 'reduced' buffers --ie, first binned-by-3-in-
     * both-directions buffers
     */


/*********************** SEXPR(),SDEVLR() ***************************/

/* 
 * sexpr(r) is a roughly exponential profile; it is cut off smoothly
 * beginning at EXPCUT effective radii (3 r_e or 5.03 scale lengths,
 * and goes to zero with zero derivative at 4 r_e, or 6.71 scale lengths.
 * The profile is normalized to IE_EXP at r_e, so has a central SB of 
 * about 11000 if IE_EXP == 2048. At the cutoff radius the amplitude is 14.
 */

static double
sexpr_nc(double r)			/* r in half-light radii */
{
   return(IE_EXP*exp(EXPFAC*(r - 1.0)));
}

static double
sexpr(double r)				/* r in half-light radii */
{
   double p;
   
   p = sexpr_nc(r);
   
   if(r > EXPCUT) {
      if(r > EXPOUT) {
	 p = 0.0;
      }else{
	 double scr = (r - EXPCUT)/(EXPOUT - EXPCUT);
	 scr = 1 - scr*scr;
	 p *= scr*scr;
      }
   }

   return(p);
}

/*
 * sdevlr(r) is a roughly deVaucouleurs profile; there are modifications
 * both at large and at small radii. The profile is cut off beginning at
 * 7 r_e, at a brightness about .008 of that at r_e, and goes to zero
 * with zero derivative at 8 r_e. The normal deV law has a brightness
 * at the center 1000 times that at r_e, so the dynamic range between
 * the center and 7 r_e is 125,000, which exceeds that of shorts. We
 * therefor put in a core to tame the central spike, with a core radius
 * of .02 r_e, 1.6 pixels for the biggest galaxy we can accomodate. The
 * range of real core radii is large, but this value is not unreasonable.
 * This choice results in a core surface brightness 120 times that at r_e,
 * for a dynamic range of about 15000. The surface brightness at r_e
 * is IE_DEV; if this is 512 the central surface brightness is about 62,000.
 * Dynamic range is a real problem with devlaws; the value at the cutoff
 * (7 r_e) is only 4.
 *
 * The core modification changes the total flux by about a third of a percent,
 * but the cutoff changes the total flux significantly; 5.7% of the flux in a
 * pure deV profle is beyond 8 r_e, and 7.1% is beyond 7 r_e
 */

static double
sdevlr_nc(double r)			/* r in half-light radii */
{
   return(IE_DEV*exp(DEFAC*(pow(r*r + 0.0004, 0.125) - 1.0)));
}

static double
sdevlr(double r)			/* r in half-light radii */
{
   double p;
   
   p = sdevlr_nc(r);
   
   if(r > DEVCUT) {
      if(r > DEVOUT) {
	 p = 0.0;
      } else {
	 double scr = (r - DEVCUT)/(DEVOUT - DEVCUT);
	 scr = 1 - scr*scr;
	 p *= scr*scr;
      }
   }
   
   return(p);
}

/*****************************************************************************/
/*
 * Return the intensity at the effective radius for a model.
 */
float
phIeGetFromFlux(float totflux,		/* total flux */
		int class,		/* class of object */
		float re,		/* effective radius */
		float ab)		/* axis ratio */
{
   shAssert(re*re*ab != 0.0);
   
   switch (class) {
    case PSF_MODEL:
      shFatal("phIeGetFromFlux: I can't handle PSFs");
    case EXP_MODEL:
      return(totflux/(TOTFLUX_EXP*re*re*ab));
    case DEV_MODEL:
      return(totflux/(TOTFLUX_DEV*re*re*ab));
    default:
      shFatal("phIeGetFromFlux: unknown object class: %d",class);
   }

   return(0.0);				/* NOTREACHED */
}

/*
 * Return the total flux of a model, given r_e, a/b, and the intensity
 * at the effective radius.  If no Ie is specified use default for that class
 *
 * Note that this is the _total_ flux, not allowing for the cutoffs
 * that we apply to our model profiles
 */
float
phFluxGetFromIe(float Ie,		/* Surface brightness at r_e; or 0 */
		int class,		/* class of object */
		float re,		/* effective radius */
		float ab)		/* axis ratio */
{
   switch (class) {
    case PSF_MODEL:
      shFatal("phFluxGetFromIe: I can't handle PSFs");
    case EXP_MODEL:
      if(Ie == 0) {
	 Ie = IE_EXP;
      }
      return(Ie*TOTFLUX_CUTOFF_EXP*re*re*ab);
    case DEV_MODEL:
      if(Ie == 0) {
	 Ie = IE_DEV;
      }
      return(Ie*TOTFLUX_CUTOFF_DEV*re*re*ab);
    default:
      shFatal("phFluxGetFromIe: unknown object class: %d",class);
   }

   return(0.0);				/* NOTREACHED */
}

float
phFluxGetFromModel(const MODEL_PARAMS *p)
{
   return(phFluxGetFromIe(0, p->class, p->rsize, p->aratio));
}

/******************** ALLOCATION UTILITIES ********************************/
/*
 * cmatrix allocates and sets up pointer array for general 2-d matrix
 */
static char **
cmatrix(int nrows,
	int ncols,
	int elsize)
{
    int i;
    int nbyte = nrows*ncols*elsize + nrows*sizeof(char *);
    char **p = (char **)shMalloc(nbyte);
    memset((char *)p, '\0', nbyte);
    p[0] = (char *)p + nrows*sizeof(char *);
    for(i=1;i<nrows;i++) p[i] = p[i-1] + ncols*elsize;
    return p;
}

/*  
 * matrix_p reassigns pointers in an already allocated (empty) region known
 * to be large enough; this is only needed when the matrix memory needs to
 * be contiguous. IT DOES NOT MOVE THE FIRST ELEMENT OF THE MATRIX, so
 * the POINTER ARRAY needs to be large enough, as well as the matrix
 * storage area itself. The pointer array need not be contiguous with the
 * matrix storage area.
 */
static void **
matrix_p(int nrows,
	 int ncols,
	 int elsize,
	 void **p)
{
    int i;

    for(i=1;i<nrows;i++) {
       ((char **)p)[i] = ((char **)p)[i-1] + ncols*elsize;
    }
    return p;
}
    
/**********  MAKEDEVTABL(), MAKEEXPTABL(), ALLOC/FREEFUNCTAB() *****/
/*
 * Functions which allocate/free and populate tables for the devlaw, explaw
 * and psf (delta function) as functions of integer r^2; for the cell outer 
 * radius of 658 pixels, this requires about 433,000 entries for the largest 
 * images, for which it is evaluated here; others are done by scaling r^2 
 * before entering the table. For construction of the profiles, we need 
 * have only one at a time; for other purposes (subtraction) we may need 
 * both in memory at once, and we will need to reconsider how to do it.
 * We can save beaucoup time by NOT evaluating the function for each point;
 * beyond 10000, both functions are essentially perfectly represented
 * by linear interpolation every 100 points.
 */
 
static float *spro_functab = NULL;

static void
allocfunctab(void)
{
    int r2lim = (((RLIM+10)*(RLIM+10))/128)*128 + 8 ;
        
    if(!spro_functab){
       spro_functab = (float *)shMalloc(r2lim*sizeof(float));
    }
}

static void
makedevtabl(void)
{
    int i;
    int r2lim = (((RLIM+10)*(RLIM+10))/128)*128;    
    double r;
    double re = RMDEV;

    allocfunctab();    

    for(i=0;i<r2lim;i++){
        r = sqrt((double)i)/re;
        spro_functab[i] = sdevlr(r);
    } 
}

static void
makeexptabl(void)
{
    int i;
    int r2lim = (((RLIM+10)*(RLIM+10))/128)*128 ;
    double r;
    double re = RMEXP;
    
    allocfunctab();
        
    for(i=0;i<r2lim;i++){
        r = sqrt((double)i)/re;
        spro_functab[i] = sexpr(r);
    } 
}

static void 
freefunctab(void)
{
    if(spro_functab) shFree(spro_functab);
    spro_functab = NULL;
}

/*********************** SPROCATSET() *******************************/
/* initialization routine for all static catalog arrays. The argument is
 * 1 for setting up for raw arrays, 2 for smoothed arrays; the offset
 * table and size tables are affected, because the raw arrays leave
 * MAXSM cells at each end of each line for smoothing, and the smoothed
 * arrays in principle need MAXSM pixels at the right and above; thus
 * if the raw image is xs X ys, the raw quarter is 
 *
 *      (xs/2+1+2*MAXSM) X (ys/2+1), origin at MAXSM,0
 *
 * and the smoothed quarter 
 * 
 *      (xs/2+1+MAXSM) X (ys/2+1+MAXSM), origin at 0,0
 *
 * These are the sizes of the disk images; we must in general allocate
 * more space than this (in y) for working lines in the memory images.
 * For smoothing, we need MAXSM empty lines above the quadrant and
 * MAXSM lines which are mirror copies below the quadrant for smoothing.
 * We store the images in order of decreasing size in each flattening
 * type; this is dictated by the method of construction.
 */

#define NOPRO   0 
#define PRORAW  1
#define PROSMTH 2

#define BLKSIZ 4096			/* assumed disk block size */

#define RGHEADSIZ (9*BLKSIZ)
/* 
 * Size of the binary header. Must be at least
 *   (20*MAXSCAT + 48)*(sizeof(struct spro_cheader))
 * bytes, and must be a multiple of BLKSIZ.
 *
 * Note that the declaration of spro_cheader doesn't include space for
 * all the MASCAT entries in proc_catentry[]
 */

static int
sprocatset(int flg)
{
    int class,i,j;
    int type = 0;
    int idx = 0;
    float axr;
    int nprof;
    float outrat;
    float ref;
    int totprof;
    int qsidx;
    int qsidy;
    int yext = (flg == PROSMTH ? MAXSM : 0);
    int xext = (flg == PROSMTH ? MAXSM : 2*MAXSM );  
    int bufsiz;
    int classo;
    int type0;
    int fpitchint;

    shAssert( (MAXSM/3)*3 == MAXSM);
    
    /* check argument */
    if(flg != PROSMTH && flg != PRORAW){
        shErrStackPush("PROCATSET: Invalid flag=%d\n",flg);
	return(SH_GENERIC_ERROR);
    }
    /*
     * Allocate some arrays
     */
    catbufsidx = shMalloc(MAXSCAT*sizeof(catbufsidx[0]));
    catbufsidy = shMalloc(MAXSCAT*sizeof(catbufsidy[0]));
    catbufoff = shMalloc((MAXSCAT + 1)*sizeof(catbufoff[0]));
    /*
     * Set numbers of models
     */
    i = 0;
    spro_cnprof[i++] = 1;
    spro_cnprof[i++] = NEXPSIZ;
    spro_cnprof[i++] = NDEVSIZ;

    /* set up spro_names */
    for(i=0;i<NSPROTYPES;i++) spro_names[i] = spro_namlist + 8*i;
    
    /* populate it and other type arrays */
    for(class=0;class<NSPROCLASS;class++){
        axr = 1.0;
        nprof = spro_cnprof[class];
        for(j=0;j<spro_cnaxr[class];j++){
            spro_sindex[type] = idx;
            spro_class[type] = class;
            spro_axr[type] = (j == spro_cnaxr[class] - 1) ? 0 : axr;
            sprintf(spro_names[type],"%s%4.2f",spro_clnam[class],axr);
            idx += spro_cnprof[class];
            type++;
	    if(spro_cnaxr[class] > 1) {
#if EQUAL_ARATIO_STEPS
	       axr = 1 - (float)(j + 1)*(1.0/(spro_cnaxr[class] - 1));
#else
	       axr /= FFAC;
#endif
	    }
        }
    }
    spro_sindex[NSPROTYPES] = spro_sindex[NSPROTYPES-1] 
                                    + spro_cnprof[NSPROCLASS-1];

    shAssert(spro_sindex[NSPROTYPES] ==  MAXSCAT);

    /* calculate the maximum buffer size and size and offset arrays. */   
    totprof = 0;
    catbufoff[0] = RGHEADSIZ;		/* skip over header */
    spro_bufsmax = spro_qsmax = 0;    /* module statics */
    spro_bufsm2  = spro_qsm2  = 0;    /* 'reduced' buffer statics */
    
    class = -1;
    type0 = 0;			/* Silence uninitialized variable warning */
    fpitchint = 0;		/* Silence uninitialized variable warning */
    for(type=0;type<NSPROTYPES;type++){
        
        classo = class;
        class=spro_class[type];
        outrat = spro_outrat[class];
        ref = spro_crmax[class];
        nprof = spro_cnprof[class];
        axr = spro_axr[type];

        if(class != classo){
            type0 = type; /* first type (round) of a new class*/
            fpitchint = FPITCH*nprof;  
                /* interval over which flattening
                 * changes by a factor of 3
                 */
        }
        
        for(j=0;j<nprof;j++){   /* loop over sizes */
            qsidx = (((int)(ref*outrat + 0.5)-1)/3 + 1)*3 + xext ; 
            qsidy = (((int)(ref*outrat*axr + 0.5)-1)/3 + 1)*3 + yext ;
            if(j >= SPITCH){
                /* be SURE that binned image will fit into target buffer */
                while(qsidx - xext < (catbufsidx[totprof-SPITCH] - xext)/3){
                    qsidx += 3;
                }
                while(qsidy - yext < (catbufsidy[totprof-SPITCH] - yext)/3){
                    qsidy += 3;
                }
            }
            if(type-type0 >= FPITCH && j < SPITCH){
                /* be SURE that vertically binned 'template' images will fit 
                 * into target buffer
                 */
                while(qsidy - yext < (catbufsidy[totprof-fpitchint] - yext)/3){
                    qsidy += 3;
                }
            }
               /* 
                * ASSUME that max smoothing filter will be half-width
                * MAXSM-1 pixels.
                */
            if(qsidx < CT3(SYNCRAD + xext)) qsidx = CT3(SYNCRAD + xext);
            if(qsidy < CT3(SYNCRAD + yext)) qsidy = CT3(SYNCRAD + yext);
            catbufsidx[totprof] = qsidx;
            catbufsidy[totprof] = qsidy;
            bufsiz = ((qsidx*qsidy*sizeof(MODEL_PIX) - 1)/BLKSIZ + 1)*BLKSIZ;
               /* set maxima */ 
            if(bufsiz > spro_bufsmax) spro_bufsmax = bufsiz;
            if(bufsiz > spro_bufsm2 && j >=SPITCH) spro_bufsm2 = bufsiz;
            if(qsidx > spro_qsmax) spro_qsmax = qsidx;
            if(qsidx > spro_qsm2 && j >= SPITCH) spro_qsm2 = qsidx;
            catbufoff[totprof+1] = catbufoff[totprof] + bufsiz;
            totprof++;
            ref /= RFAC; 
        }
    }

    return(SH_SUCCESS);
}

/*****************************************************************************/
/* 
 * Makes the binary catalog file with the whole set of raw images
 * as the upper left quadrant only, major axis along the x axis.
 * This file is very large with the limits set to the largest allowed
 * by the cell array code; the ratio of 3^0.2 = 1.246 for model sizes 
 * means that there is a ratio of 1.552 in areas, and the total image 
 * size for a given class is thus about 2.8 times the size of its 
 * largest member, (plus change --about 15%-- for the constant fringe)
 * which in the round case is 600x600 for one quadrant. Thus each round class
 * occupies about 2.4 MB. Each successive inclination occupies 1/1.246 times
 * less storage, so an infinite sequence of such would occupy about 4 times
 * as much storage as the round case; there are two sequences, so the total
 * storage is in the vicinity of 20 MB.
 *
 * The models are stored in index order for the arrays catbufsid(y),
 * catbufoff; ie, by class, star first, exp second, dev third;
 * within each class, by type (flattening), roundest first; within
 * each type, by size, largest first. Thus the order is star; biggest round
 * exponential, next largest.....smallest round exp ; biggest almost round
 * exponential.....smallest; ...... biggest flattest exponential...smallest
 * flattest exponential; Biggest round deV law...,smallest round deV law,
 * ...,...,Biggest flattest deV law...smallest flattest deV law. The
 * whole thing is preceded by a 5120 byte header with a copy of the 
 * appropriate spro_cheader structure giving sizes and offsets for all
 * the models (MAXSCAT models in all).
 *
 * The code is written assuming that it might be more efficient to
 * seek to disk block boundaries, and also by thus doing makes it
 * transparently possible to run the code under operating systems (VMS)
 * in which one MUST seek to block boundaries in efficiently handled
 * binary files. BLKSIZ can be set to unity if desired. The file has a
 * 5120-byte header which contains, among other things, a table of
 * offsets for each member, a table of sizes, and the total number of
 * entries. The construction makes use of the fact that each fifth
 * profile is exactly a factor of three smaller, and that each fourth
 * inclination class is exactly a factor of three flatter. The largest
 * and roundest profiles are constructed first and the rest built from
 * these by binning. This ruse also largely alleviates the problems with
 * severely undersampling the profile for small profiles, since the
 * largest profiles ARE well sampled, and the binning preserves flux
 * properly. (Actually, the very inner parts of even the biggest deV
 * profiles are not quite adequately sampled, and we deal with that
 * here reasonably efficiently.)
 *
 * The construction procedure involves making only 20 models of each class 
 * using the profile functions; they are the largest 5 in each of the 
 * roundest 4 types. All other models are constructed from these by binning
 * by 3 in both x and y to construct the smaller models; the increment
 * in size is such that every fifth model in a size sequence is exactly
 * a factor of three smaller. The flatter models are made by binning
 * the 20 fundamental models by 3 in y; the increment in flattening
 * is such that every 4th flattening sequence is exactly a factor of
 * three flatter....and then binning in both x and y as was done for
 * the rounder models to construct smaller ones. The binning factor must
 * be odd so that pixel centers are preserved on binning, and 3 is the
 * smallest nontrivial odd number, so 3 it is.
 */

int
phProfCatalogMake(const char *cat_name,	/* name of catalogue file to make */
		  int show_progress)	/* print progress report to stderr? */
{
   int nmade = 0;			/* how many models have we made? */
   int nl;				/* starting catalog index for type */
   int type;				/* profile type, type at class bdy */
   int typel,typeh;			/* limiting type indices in main loop*/
   int class;				/* profile class */
   int ic;				/* radius index at type boundary */
   int i,j;
   int nerror = 0;			/* number of errors detected */
   int nprof;				/* how many profiles in this loop? */
   register int is,js;			/* subpixel indices */
   float al;				/* scale radius */
   float axisrat;			/* axis ratio for this set, <= 1.0 */
   float axisrsq;
   float x,y;				/* subpixel coordinates */
   float yfac;				/* 1/axisrat^2 */
   float ysqf;				/* scratch  */
   float r;				/*   "  "   radius */
   int qsidx,qsidy;			/* sides of quadrant buffer */
   float maxrad;			/* maximum radius for current type */
   float frlim = FRLIM;
   float frlim2 = frlim*frlim;
   int bufsiz;				/* buffer size in bytes */
   int nlines;				/* max # of lines in full-size buffer*/
   int nl2;				/* max # lines in first binned-by-3
					   buffer */
   double (*profun)(double);		/* profile function */
   MODEL_PIX **rbuft[SPITCH];		/* `primary template' buffers */
   MODEL_PIX **rbuf2[SPITCH];		/* `reduced template' buffers */
   MODEL_PIX **rbufd[SPITCH];
   MODEL_PIX **rbufs;
   MODEL_PIX *rb; 			/* working line pointers */     
   MODEL_PIX *rb1;
   MODEL_PIX *rb2;
   MODEL_PIX *rb3;
   int n;
   int qsidxt[5], qsidyt[5];		/* sizes of primary and */
   int qsidx2[5], qsidy2[5];		/*        secondary template buffers */
   int qsidxs;				/* sides of source buffer */
   int qsidys;				/*                      when binning */
   FILE *fdes;
   int ti;
   char chead[RGHEADSIZ];
   spro_cheader *pcp = (spro_cheader *)chead;
   spro_catentry *pcpe;
   int *header = (int *)chead;
   int flatfac;
   int sizfac;
   float fsfac;				/* (reduction factor from max size)^2*/
   int ylim, xlim;			/* limits such that spro_functab is
					   never addressed out of bounds */
   int iy2, ix2;
   int j0;
   float sum;
   int iout;
   MODEL_PIX val;			/* value of a pixel */
#if !FLOATING_MODELS
   MODEL_PIX val1;
   int nshift,nshift1,nshift2;
#endif
   int epixnorm = 11000;		/* peak exp model pixel value */
   int dpixnorm = 62000;		/* peak deV model pixel value */
   int pixnorm;				/* peak pixel for current class */

   shAssert(SUBPIX%2 == 1);		/* I told you SUBPIX had to be odd */
/*
 * init the static arrays
 */
    if(sprocatset(PRORAW) != SH_SUCCESS) {
       return(SH_GENERIC_ERROR);
    }
/* 
 * allocate the buffers. Note that there are dummy lines to fill out
 * full allocation of disk blocks. We will allocate more lines than 
 * necessary to facilitate binning and avoid painful thought.
 */
   nlines = (spro_bufsmax + 3*BLKSIZ)/(spro_qsmax*sizeof(MODEL_PIX)) + 1;
   nl2    = (spro_bufsm2  + 3*BLKSIZ)/(spro_qsm2*sizeof(MODEL_PIX)) + 1;

   for(i=0;i<SPITCH;i++){
      rbuft[i] = (MODEL_PIX **)cmatrix(spro_qsmax,nlines+4,sizeof(MODEL_PIX));
      rbuf2[i] = (MODEL_PIX **)cmatrix(spro_qsm2 ,nl2   +4,sizeof(MODEL_PIX));
      rbufd[i] = (MODEL_PIX **)shMalloc(spro_qsm2*sizeof(MODEL_PIX *));
      rbufd[i][0] = rbuf2[i][0];
   }
/*
 * rbufd is a pointer array used in binning in place, in which
 * 2 working matrices use the same physical space. The origins of
 * the matrices are the same as those for the 'real' array rbuf2[].
 *
 * NB!!!! we need to redo the pointer array for each model to make the
 * data contiguous for writing to disk !!!!!! 
 *
 *
 * Open the output file and populate and write the header
 */
   if((fdes = fopen(cat_name,"wb")) == NULL) {
      shErrStackPushPerror("phProfCatalogMake: "
			   "Cannot create catalogue file %s",cat_name);
      return(SH_GENERIC_ERROR);
   }
    
   shAssert(sizeof(spro_cheader) + MAXSCAT*sizeof(spro_catentry) < RGHEADSIZ);
   memset((char *)header,0,RGHEADSIZ);

   pcp->proc_catentry = shMalloc(MAXSCAT*sizeof(spro_catentry *));
   pcp->proc_catentry[0] =
		      (spro_catentry *)((char *)header + sizeof(spro_cheader));
   for(i = 0; i < MAXSCAT; i++) {
      pcp->proc_catentry[i] = pcp->proc_catentry[0] + i;
   }
    
   strcpy(pcp->proc_catver, RAWPROF_VERSION " | ");
   strcat(pcp->proc_catver, phPhotoName());
   shAssert(strlen(pcp->proc_catver) < IDSIZE);
   
   pcp->proc_bufmax   = spro_bufsmax;
   pcp->proc_linemax  = spro_qsmax;
   pcp->proc_maxscat  = MAXSCAT;
   pcp->proc_pssig1   = 0.;    
   pcp->proc_pssig2   = 0.;    
   pcp->proc_psamp1   = 0.;        
   pcp->proc_smmax    = MAXSM;
   pcp->proc_nexpincl = NEXPINCL;
   pcp->proc_ndevincl = NDEVINCL;
   pcp->proc_nexpsiz  = NEXPSIZ;
   pcp->proc_ndevsiz  = NDEVSIZ;
   i = 0;
   pcp->cutoff[i++] = 0;
   pcp->cutoff[i++] = EXPOUT;
   pcp->cutoff[i++] = DEVOUT;
   shAssert(i == NCLASSTYPE);

   class = type = -1;
   axisrat = 1.;
   al = -1;
   for(ic = 0;ic < MAXSCAT; ic++) {
      pcpe = pcp->proc_catentry[ic];
/*
 * check for type and class boundaries
 */
      if(ic == spro_sindex[type+1] - 1) {
	 al = 0;			/* last model has re == 0 */
      } else if(ic == spro_sindex[type+1]) {
	 type++;
	 al = spro_crmax[ic == 0 ? 0 : class];
      }
      if(type == spro_ctindex[class+1]){
	 class++;
	 al = spro_crmax[class];
      }
      axisrat = spro_axr[type];
/*
 * set header quantities
 */
      pcpe->scat_offset = catbufoff[ic];
      pcpe->scat_xsize  = catbufsidx[ic];
      pcpe->scat_ysize  = catbufsidy[ic];
      pcpe->scat_reff   = al;
      pcpe->scat_axr    = axisrat;
      pcpe->scat_class  = class;
      pcpe->scat_type   = type;
      pcpe->scat_index  = ic;
      al /= RFAC;
   }
/*
 * Copy the spro_cheader values into the header block.
 *
 * No, this is not a good design, but it's what JEG did long ago, when
 * these values were in the static part of the header
 */
   memcpy((char *)header + sizeof(spro_cheader), pcp->proc_catentry[0],
	  MAXSCAT*sizeof(spro_catentry));

   if(fwrite(header,RGHEADSIZ,1,fdes) != 1) {
      shErrStackPush("phProfCatalogMake: error writing header");
      nerror++;
   }
/*
 * make the catalog
 * 
 * first make the star; this is really just a placeholder, because
 * we do not derive the smoothed star profile by smoothing but by
 * constructing a psf in the smoothed buffer, for dynamic range reasons.
 */
   qsidx = catbufsidx[0];
   qsidy = catbufsidy[0];
   bufsiz = ((qsidx*qsidy*sizeof(MODEL_PIX) - 1)/BLKSIZ + 1)*BLKSIZ;
   nmade++;
/* 
 * set up pointers, clear the whole thing and set the central 
 * pixel to maximum 
 */
   matrix_p(qsidy,qsidx,sizeof(MODEL_PIX),(void **)rbuft[0]);
   memset((char *)(rbuft[0][0]),0,bufsiz);
   (rbuft[0])[0][MAXSM] = 65535;
   
   if(fwrite(rbuft[0][0],bufsiz,1,fdes) != 1) {
      shErrStackPush("phProfCatalogMake: error writing star");
      nerror++;
   }
/*
 * now do the galaxies
 */
    if(show_progress) {
       fprintf(stderr,"n   (n    %%)  xs  ys type class   a/b  size\n");
    }
    
    pixnorm = -1;
    for(class = 1; class < NSPROCLASS; class++) {
        switch (class) {
	 case PSF_MODEL:
	   shFatal("phProfCatalogMake: PSF in galaxy part of code");
	   break;			/* make compiler happy */
	 case EXP_MODEL:
	   pixnorm = epixnorm; break;
	 case DEV_MODEL:
	   pixnorm = dpixnorm; break;
	 default:
	   shFatal("phProfCatalogMake: unknown class %d",class);
	}
	
        profun = spro_fun[class];
        (*spro_tabl[class])();
        maxrad = spro_crmax[class];

        typeh = spro_ctindex[class+1];	/* last type +1 for this class */

        nprof = spro_cnprof[class];	/* note that the # profiles of a given
					   type is fn of class only. */
/* 
 * the types for primary construction (using the profile function)
 * are the first FPITCH flattening types; the rest are derived
 * by vertical binning. Once we have made our SPITCH fundamental sizes
 * in these fundamental flattenings, we may derive all smaller models
 * by horizontal-and-vertical binning. Hence our strategy is to make the
 * outermost loop over fundamental flattenings; all the models derivable
 * from these flattenings are made in sequence to avoid reconstructing
 * the fundamental types more than once.
 *
 * NB!!! the following code ONLY works if RFAC is the 1/SPITCH root of 3, since
 * it makes use of the fact that every SPITCHth size can be derived from the
 * last by binning by 3. Likewise, FFAC must be the 1/FPITCH root of 3, since
 * we use the fact that every FPITCHth flattening can be derived from the last
 * by binning vertically by 3.
 *
 * This caveat isn't strictly true; if you make the number of models larger
 * than SPITCH then only the primary models will ever be used.  This has not
 * been tested.  The same is true for FPITCH, but in this case there's a
 * simpler alternative, namely to set EQUAL_ARATIO_STEPS to be true, in
 * which case the rebinning-vertically code isn't used and you get models
 * in equal steps of a/b.  This is probably what you want.
 */
        for(typel = spro_ctindex[class];typel < spro_ctindex[class] + FPITCH;
								     typel++) {
	   for(type = typel; type < typeh; type += FPITCH) {
	      nl = spro_sindex[type];
	      al = spro_crmax[class];
	      axisrat = spro_axr[type];
	      axisrsq = axisrat*axisrat;
	      
	      for(ic = nl; ic < nl + SPITCH; ic++) {
		 shAssert(al == pcp->proc_catentry[nmade]->scat_reff);
		 ti = ic - nl;		/* template index 0-4 */
		 qsidx = catbufsidx[ic];
		 qsidy = catbufsidy[ic];
		 if(type == typel) {
		    shAssert(axisrat > 1e-10);
/* 
 * one of the SPITCH `primary' sizes of a round model;
 * build a template using the profile function
 */
                    qsidxt[ti] = qsidx;
                    qsidyt[ti] = qsidy;
		    
                    matrix_p(qsidy,qsidx,sizeof(MODEL_PIX), (void **)rbuft[ti]);
		    
                    bufsiz = ((qsidx*qsidy*sizeof(MODEL_PIX)- 1)/BLKSIZ + 1)*
									BLKSIZ;
		    shAssert(bufsiz == catbufoff[ic+1] - catbufoff[ic])
                    memset((char *)(rbuft[ti][0]),0,bufsiz); 
/* 
 * now fill the buffer--for devlaws we must treat the area near the origin
 * specially; we in fact do so for all profile classes
 */
                    fsfac = al*al/(maxrad*maxrad);
                    sizfac = (256./fsfac) + 0.5;
                    flatfac = (256./axisrsq) + 0.5;
                    yfac = 1./axisrsq;
 
                    ylim = axisrat*al*frlim/maxrad;
                    shAssert(ylim <= qsidy);
                
                    for(i=0;i<ylim;i++){
                        iy2 = (i*i*flatfac)/256;
                        xlim = sqrt(frlim2*fsfac-iy2);
                        shAssert(xlim <= qsidx - MAXSM);
			
                        rb = rbuft[ti][i] + MAXSM;
#if 1
                        if(i < NSUB){    /* subpixellate 1/NSUB near origin */
                            j0 = NSUB;
                            for(j = 0; j < j0; j++){
                                sum = 0.;
                                for(is = 0; is < SUBPIX; is++){
				   y = (SUBPIX*(i - 0.5) + is + 0.5)/SUBPIX;
				   ysqf = y*y*yfac;
				   for(js = 0; js < SUBPIX; js++){
				      x = (SUBPIX*(j - 0.5) + js + 0.5)/SUBPIX;
				      r = sqrt(x*x + ysqf);
				      sum += (*profun)(r/al);
                                    }
                                }
                                rb[j] = ROUND(sum/(SUBPIX*SUBPIX));
                            }
                        } else {
			   j0 = 0;
			}
#else
                        if(i < NSUB){    /* subpixellate 1/NSUB near origin */
                            j0 = NSUB;
                            for(j=0;j<j0;j++){
                                sum = 0.;
                                for(is=0;is<8;is++){
                                    y = 0.0625*(16*i + 2*is - 7);
                                    ysqf = y*y*yfac;
                                    for(js=0;js<8;js++){
                                        x = 0.0625*(16*j + 2*js - 7);
                                        r = sqrt(x*x + ysqf);
                                        sum += (*profun)(r/al);
                                    }
                                }
                                rb[j] = ROUND(sum/64);
                            }
                        } else {
			   j0 = 0;
			}
#endif
                        ix2 = j0*j0;
                        for(j=j0;j<xlim;j++){
                            rb[j] = ROUND(spro_functab[((ix2+iy2)*sizfac)/256]);
                            ix2 += 2*j + 1;
                        }
                    }
                                 
                    /* fill in the negative-x fringe by reflection */
                    for(i=0;i<qsidy;i++){
                        n = MAXSM;
                        rb = rbuft[ti][i]+MAXSM+1;
                        rb1 = rbuft[ti][i]+MAXSM-1;
                        while(n--){
                            *rb1-- = *rb++;
                        }
                    }
		 } else {
/*
 * not a `primary' size, so we can get our model by binning in y.
 *
 * The case axisrat == 0 is special, and is treated in floating point
 * as it takes no time anyway (it has bad cache performance too)
 */
		    if(axisrat < 1e-10) {
		       bufsiz = ((qsidx*sizeof(MODEL_PIX)-1)/BLKSIZ + 1)*BLKSIZ;
		       shAssert(qsidx <= qsidxt[ti]);
		       val = rbuft[ti][0][MAXSM];

		       for(j = 0;j < qsidxt[ti];j++) {
			  val += 2*rbuft[ti][j][MAXSM];
		       }
		       fsfac = rbuft[ti][0][MAXSM]/val;

		       iout = 0;
		       for(j = 0;j < qsidxt[ti];j++) {
			  val = 0;
			  for(i = 0;i < qsidyt[ti];i++){
			     val += rbuft[ti][j][i];
			  }
			  val = rbuft[ti][0][i] + 2*val;
			  rbuft[ti][iout][i] = fsfac*val + 0.5;
		       }
		       iout++;
		    } else {
		       bufsiz = ((qsidx*qsidy*sizeof(MODEL_PIX)- 1)/BLKSIZ + 1)*
									BLKSIZ;
		       shAssert(qsidx == qsidxt[ti]);
		       
#if FLOATING_MODELS
		       iout = 0;                    
		       for(i = 0; i < qsidyt[ti]; i += 3) {
			  const float third = 1/3.0;
			  rb = rbuft[ti][i];
			  rb1 = rbuft[ti][i+1];
			  rb2 = (i > 0) ? rbuft[ti][i-1]: rbuft[ti][1];
			  rb3 = rbuft[ti][iout];
			  n = qsidxt[ti];
			  while(n--){
			     *rb3++ = third*(*rb++ + *rb1++ + *rb2++);
			  }
			  iout++;
		       }
#else
		       /* set scaling; compute central pixel sum */
		       val = rbuft[ti][0][MAXSM] + 2*rbuft[ti][1][MAXSM];
		       nshift = -1;
		       while((val >> (++nshift)) > pixnorm){;}
		       val1 = val >> nshift;
		       nshift1 = nshift;
		       while(val1 + (val>>(++nshift1)) > pixnorm){;}
		       val1 += (val >> nshift1);
		       nshift2 = nshift1;
		       while(val1 + (val>>(++nshift2)) > pixnorm){;}
               
		       iout = 0;                    
		       for(i=0;i<qsidyt[ti];i+= 3){
			  rb = rbuft[ti][i];
			  rb1 = rbuft[ti][i+1];
			  rb2 = ( i ? rbuft[ti][i-1]: rbuft[ti][1] );
			  rb3 = rbuft[ti][iout];
			  n = qsidxt[ti];
			  while(n--){
			     val = *rb++ + *rb1++ + *rb2++;
			     *rb3++ = (val >> nshift) + (val >> nshift1)
			       + (val >> nshift2);
			  }
			  iout++;
		       }
#endif
		       shAssert(iout == qsidyt[ti]/3);
		    }
		    for(i=iout;i<qsidyt[ti];i++){ 
		       memset(rbuft[ti][i],0, qsidxt[ti]*sizeof(MODEL_PIX));
		    }
		    qsidyt[ti] = qsidy;
		 }
/*
 * time to write to disk
 */
		 if(show_progress) {
		    nmade++;
		    fprintf(stderr,"%-3d (%5.2f%%) "
			    "%3d %3d  %2d   %d     %4.2f %6.2f               \r",
			    nmade,100.0*nmade/MAXSCAT,
			    qsidx,qsidy,type,class,axisrat,al);
		 }
		 
		 if(fseek(fdes,catbufoff[ic],0) == -1) {
		    shErrStackPushPerror("phProfCatalogMake: "
					 "error seeking for rbuft[%d][0]",ti);
		    nerror++;
		 }
		 if(fwrite(rbuft[ti][0],bufsiz,1,fdes) != 1) {
		    shErrStackPushPerror("phProfCatalogMake: "
					 "error writing rbuft[%d][0]",ti);
		    nerror++;
		 }
		 al /= RFAC;
	      }
/*
 * OK. Now have the SPITCH templates for this type. Build the rest by binning
 */
	      shAssert(ic == nl+SPITCH);
	      for(; ic < nl + nprof; ic++) {
		 if(ic == nl + nprof - 1) {
		    al = 0;		/* special case */
		 }
		 shAssert(al == pcp->proc_catentry[nmade]->scat_reff);

		 ti = (ic - nl) % SPITCH;
		 qsidx = catbufsidx[ic];
		 qsidy = catbufsidy[ic];
		 bufsiz = ((qsidx*qsidy*sizeof(MODEL_PIX)-1)/BLKSIZ + 1)*BLKSIZ;
		 
		 /* 
		  * set up pointers for dest, which is ALWAYS the physical
		  * space of rbuf2[ti].
		  */
		 matrix_p(qsidy+3,qsidx,sizeof(MODEL_PIX),(void **)rbufd[ti]);
		 
		 /* select source; the first five are made directly from the
		  * templates; thereafter, recursively from themselves
		  */
		 if(ic < nl + 2*SPITCH){  /* source is template buffer */
		    rbufs = rbuft[ti];
		    qsidys = qsidyt[ti];
		    qsidxs = qsidxt[ti];
		 } else {
		    rbufs = rbuf2[ti];  /* source is last-binned set */
		    qsidys = qsidy2[ti];
		    qsidxs = qsidx2[ti];
		 }
		 

		 if(al == 0) {		/* just a single pixel */
		    float almin = pcp->proc_catentry[nmade - 1]->scat_reff;

		    for(i = 0; i < qsidy; i++) {
		       memset(rbufd[ti][i],0,qsidx*sizeof(MODEL_PIX));
		    }
		    iout = i;
		    
		    rbufd[ti][0][MAXSM] =
		      phFluxGetFromIe(0, (class == 1 ? EXP_MODEL : DEV_MODEL),
				      almin, 0.5);
		 } else {
		    /* collapse in both x and y */
#if FLOATING_MODELS
		    iout = 0;
		    for(i = 0; i < qsidys; i += 3) {
		       const float ninth = 1/9.0;
		       rb  = rbufs[i+1] + MAXSM;
		       rb1 = rbufs[i] + MAXSM;
		       rb2 = (i ? rbufs[i-1] : rbufs[1]);
		       rb2 += MAXSM;
		       rb3 = rbufd[ti][iout] + MAXSM;
		       /* 'main body' plus a third of
			* the right smoothing fringe
			*/
		       for(n = qsidxs - MAXSM; n > 0; n -= 3) {
			  *rb3++ = ninth*(*rb  + *(rb+1)  + *(rb-1) +
					  *rb1 + *(rb1+1) + *(rb1-1) +
					  *rb2 + *(rb2+1) + *(rb2-1));
			  rb  += 3;
			  rb1 += 3;
			  rb2 += 3;
		       }
		       rb3 = rbufd[ti][iout] + qsidx - MAXSM;
		       for(n = MAXSM; n >= 0; n--) {
			  *rb3++ = 0;
		       }
		       iout++;
		    }
#else
		    /* set scaling; compute central pixel sum */
		    val = rbufs[0][MAXSM] + 2*rbufs[0][MAXSM+1]
		      + 2*rbufs[1][MAXSM] + 4*rbufs[1][MAXSM+1];
		    nshift = -1;
		    while((val >> (++nshift)) > pixnorm){;}
		    val1 = val >> nshift;
		    nshift1 = nshift;
		    while(val1 + (val>>(++nshift1)) > pixnorm){;}
		    val1 += (val >> nshift1);
		    nshift2 = nshift1;
		    /* collapse in both x and y */
		    while(val1 + (val>>(++nshift2)) > pixnorm){;}
		    iout = 0;
		    for(i=0;i<qsidys;i += 3) {
		       rb  = rbufs[i+1] + MAXSM;
		       rb1 = rbufs[i] + MAXSM;
		       rb2 = (i ? rbufs[i-1] : rbufs[1]);
		       rb2 += MAXSM;
		       rb3 = rbufd[ti][iout] + MAXSM;
		       n = qsidxs - MAXSM;  
		       /* 'main body' plus a third of
			* the right smoothing fringe
			*/
		       while(n>0){
			  val = ( *rb  + *(rb+1)  + *(rb-1)
				 + *rb1 + *(rb1+1) + *(rb1-1)
				 + *rb2 + *(rb2+1) + *(rb2-1) 
				 );
			  *rb3++ = (val>>nshift) + (val>>nshift1) 
			    + (val>>nshift2);
			  rb  += 3;
			  rb1 += 3;
			  rb2 += 3;
			  n -= 3;
		       }
		       n = MAXSM;
		       rb3 = rbufd[ti][iout] + qsidx - MAXSM;
		       while(n--) *rb3++ = 0;
		       iout++;
		    }
#endif
		 }
		 /* 
		  * clear enough of the rest of the buffer that the
		  * write will be padded by zeros.
		  */
		 for(i=iout;i<qsidy; i++){ 
		    memset(rbufd[ti][i],0,qsidx*sizeof(MODEL_PIX));
		 }
		 memset(rbufd[ti][qsidy],0,BLKSIZ);    
		 
		 /* fill in the negative-x fringe by reflection */
		 for(i=0;i<qsidy;i++){
		    n = MAXSM;
		    rb = rbufd[ti][i]+MAXSM+1;
		    rb1 = rbufd[ti][i]+MAXSM-1;
		    while(n--){
		       *rb1-- = *rb++;
		    }
		 }
		 
		 /* write it */
		 if(show_progress) {
		    nmade++;
		    fprintf(stderr,"%-3d (%5.2f%%) "
			   "%3d %3d  %2d   %d     %4.2f %6.2f              \r",
			    nmade,100.0*nmade/MAXSCAT,
			    qsidx,qsidy,type,class,axisrat,al);
		 }
                
		if(fseek(fdes,catbufoff[ic],0) == -1) {
		   shErrStackPushPerror("phProfCatalogMake: "
					"error seeking for rbufd[%d][0]",ti);
		   nerror++;
		}
		if(fwrite(rbufd[ti][0],bufsiz,1,fdes) != 1) {
		   shErrStackPushPerror("phProfCatalogMake: "
					"error writing rbufd[%d][0]",ti);
		   nerror++;
		}
		
                qsidx2[ti] = qsidx;   
                qsidy2[ti] = qsidy;
                al /= RFAC;
                /* set up pointers for use of the dest buffer as SOURCE */
                matrix_p(qsidy+3,qsidx,sizeof(MODEL_PIX), (void **)rbuf2[ti]);
	      }
	   }
	}
     }

    if(show_progress) {
       fprintf(stderr,"\n");
    }
    
    fclose(fdes);
    for(ti = 0;ti < SPITCH;ti++){ 
        shFree(rbuft[ti]);
        shFree(rbuf2[ti]);
        shFree(rbufd[ti]);
    }
    freefunctab();

    shFree(catbufsidx); shFree(catbufsidy);
    shFree(catbufoff);
    shFree(pcp->proc_catentry);

    if(nerror > 0) {
       shErrStackPush("phProfCatalogMake: %d errors writing %s",
		      nerror,cat_name);
       return(SH_GENERIC_ERROR);
    } else {
       return(SH_SUCCESS);
    }
}

/*****************************************************************************/
/*
 * Calculate a quadrant of a model from scratch; this is only used for
 * simulations and tests of the model-fitting code (in which case we
 * really don't want the models' discreteness to confuse us)
 */
REGION *
p_phMakeExactModel(const MODEL_PARAMS *p)
{
   double (*prof)(double);		/* proc to make a model */
   const float peakval = 62000;		/* peak value in model */
   REGION *qreg;			/* desired region with room to smooth*/
   REGION *reg;				/* upper-right quarter of image */
   FL32 *row;				/* == region->rows_fl32[] */
   const float aratio = p->aratio;
   const int class = p->class;
   const float rsize = p->rsize;
   int i, j;
   int nrow, ncol;			/* size of qreg */
   float r;				/* (elliptical) radius */
   float x2;				/* (x/a)^2 */

   {
      float f = (p->class == EXP_MODEL ? 1.25 : 1.75);
      ncol = f*rsize*spro_outrat[class] +        1 + 2*MAXSM + 2*SYNCRAD + 0.5;
      nrow = f*rsize*spro_outrat[class]*aratio + 1 + 2*MAXSM + 2*SYNCRAD + 0.5;
   }
   qreg = shRegNew("p_phMakeExactModel", nrow, ncol, TYPE_FL32);
   shRegClear(qreg);

   if(class == PSF_MODEL || rsize == 0) { /* easy! */
      qreg->rows_fl32[MAXSM][MAXSM] = peakval;

      return(qreg);
   }

   shAssert(class == EXP_MODEL || class == DEV_MODEL);
#if 1
   prof = (class == EXP_MODEL) ? sexpr : sdevlr;
#else
   prof = (class == EXP_MODEL) ? sexpr_nc : sdevlr_nc;
#endif

   reg =
     shSubRegNew("", qreg, nrow - MAXSM, ncol - MAXSM, MAXSM, MAXSM, NO_FLAGS);
/*
 * OK, we're in a position to calculate the galaxy model in reg.
 */
   shAssert(SUBPIX%2 == 1);		/* I told you SUBPIX had to be odd */
   
   for(i = 0; i < nrow - MAXSM; i++) {
      row = reg->rows_fl32[i];

      if(i >= NSUB) {			/* no subpixellation */
	 j = 0;				/* start at j == 0 */
      } else {				/* subpixellate 1/SUBPIX near origin */
	 double sum;			/* sum for this pixel */
	 int is, js;

	 for(j = 0; j < NSUB/(aratio + 1e-2); j++) {
	    sum = 0;
	    for(is = -SUBPIX/2; is <= SUBPIX/2; is++) {
	       if(aratio < 1e-10) {
		  if(i == 0) {
		     x2 = 0;		/* on axis */
		  } else {
		     break;		/* model == 0 */
		  }
	       } else {		  
		  x2 = pow((SUBPIX*i + is)/aratio, 2);
	       }
	       for(js = -SUBPIX/2; js <= SUBPIX/2; js++){
		  r = sqrt(x2 + pow(SUBPIX*j + js, 2))/SUBPIX;
		  sum += prof(r/rsize);
	       }
	    }
	    row[j] = sum/(SUBPIX*SUBPIX);
	 }
      }

      x2 = pow(i/aratio, 2);
      for(; j < ncol - MAXSM; j++) {
	 r = sqrt(x2 + j*j);
	 row[j] = prof(r/rsize);
      }
   }
/*
 * Scale to make the peak pixel peakval
 */
#if 0
   {
      const float max = qreg->rows_fl32[MAXSM][MAXSM];
      if(max == 0.0) {			/* No flux was added to region */
	 qreg->rows_fl32[MAXSM][MAXSM] = peakval;
      } else {
	 shAssert(max > 0);
	 shRegIntConstMult(reg, peakval/max);
      }
   }
#endif
/*
 * Fill out the rest of qreg; upper left first
 */
   for(i = MAXSM; i < nrow; i++) {
      row = qreg->rows_fl32[i];
      for(j = 1; j < MAXSM; j++) {
	 row[MAXSM - j] = row[MAXSM + j];
      }
   }

   for(i = 1; i < MAXSM; i++) {		/* now the part below the centre */
      memcpy(qreg->rows_fl32[MAXSM - i], qreg->rows_fl32[MAXSM + i],
	     ncol*sizeof(FL32));
   }
/*
 * Clean up and return desired region
 */
   shRegDel(reg);

   return(qreg);
}
