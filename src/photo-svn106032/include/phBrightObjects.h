#if !defined(PHBRIGHTOBJECTS_H)
#define PHBRIGHTOBJECTS_H

#include "phCalib1.h"
#include "phDgpsf.h"
#include "phObjects.h"
#include "phOffset.h"
/*
 * Encapsulate enough information to subtract a star's wings
 */
typedef struct {
   float p0;				/* central intensity */
   float rowc, colc;			/* row/column centre */
   float beta;				/* slope of wings */
   float rtrans;			/* transition radius */
   float sigmap;			/* power-law `sigma' */
} WINGOBJECT;				/* pragma SCHEMA */

/*
 * Describe a model subtracted from an image as part of sky subtraction
 */
#define MODEL_NCOMP 2			/* max. number of components */

typedef struct {
    int id[2];                		/* the object1's ID (internally, a pointer to it) */

    int ncomp;
    int type[MODEL_NCOMP];		/* type of model */
    float weight[MODEL_NCOMP];		/* weight of each component */
    float rowc[MODEL_NCOMP];		/* centre of */
    float colc[MODEL_NCOMP];		/*           each component */
    float counts[MODEL_NCOMP];		/* number of counts in each component */
    float r_e[MODEL_NCOMP];		/* effective radius of counts in each component */
    float ab[MODEL_NCOMP];		/* axis ratio of each component */
    float phi[MODEL_NCOMP];		/* position angle of each component */
    float spare1[MODEL_NCOMP];		/* spare parameter for each component */
    float spare2[MODEL_NCOMP];		/* another spare parameter for each component */
} OBJECT_MODEL;				/* pragma SCHEMA */

WINGOBJECT *phWingobjectNew(void);
void phWingobjectDel(WINGOBJECT *);

OBJECT_MODEL *phObjectModelNew(void);
void phObjectModelDel(OBJECT_MODEL *mo);

void phObjectModelIdSetFromPtr(OBJECT_MODEL *om, const OBJECT1 *obj1);
OBJECT1 *phObjectModelPtrGetFromId(OBJECT_MODEL *om);

void
phCompositeProfileCopy(COMP_PROFILE *cprof1,   /* target cprof */  
                       COMP_PROFILE *cprof2);   /* source cprof */

int 
phCompositeProfileAdd(COMP_PROFILE *prof, const OBJECT1 *obj1,
			   const OBJMASK *satur_mask, int is_it_wing, 
                           float profmin);
int 
phCompositeProfileFind(COMP_PROFILE *cprof, int medflg);

int
phSaturatedStarCentroid(const REGION *reg, /* region containing star */
			const OBJMASK *objmask, /* pixels in bright star */
			const DGPSF *psf, /* the core PSF */
			float sky,	/* sky level */
			float *xc, float *yc); /* returned position */

WINGOBJECT *
phBrightStarWings(OBJECT1 *obj1,	/* the object in question */
		  const COMP_PROFILE *cprof, /* a composite profile */
		  const FRAMEPARAMS *fparams, /* info on offsets etc. */
		  int medflg,		/* use median profile? */
		  float r,		/* use only profile to this radius */
		  BINREGION *wingreg,	/* add wings into this BINREGION */
		  float beta,		/* power law slope to assume */
		  float sigmap,		/* "sigma" for power law wings */
		  float frac,		/* fraction of power in wings (wrt
					   PSF counts) */
		  float rtrans,		/* transition radius to a parabola */
		  float nsigma);	/* mark pixels where we subtracted more
					   than nsigma*skysigma as NOTCHECKED*/

DGPSF *
phStarParamsFind(const COMP_PROFILE *comp_prof,	/* fitted profile */
		 const DGPSF *psf,	/* input values of parameters */
		 float profmin,		/* minimum profile value to consider */
		 int medflg,		/* use median not mean profile? */
		 int fit_g2,		/* fit a second Gaussian component */
		 int fit_p,		/* fit a power-law component */
		 int fit_sigmap,	/* fit "sigma" for power law */
		 int fit_beta,		/* fit index of power law */
		 float *chisq);		/* reduced chisq of fit */

float
phPSFCountsGetFromProfile(const OBJECT1 *obj1, /* the object */
			  const COMP_PROFILE *cprof, /* the composite */
			  int i0,	/* and the first good radial point */
			  int medflg,	/* use median profile? */
			  float r_max);	/* use only profile to this radius */

float
phPowerLawSetFromCompositeProfile(const COMP_PROFILE *cprof, /* the composite profile */
                                  int iprofMin, /* first value of profMean[] to use  */
                                  float beta, /* the power law is r^{-beta}; if 0 use value in DGPSF */
                                  int use_mean   /* Use mean, not median, profile */
                        );
float
phPowerLawSetFromObject1(const OBJECT1 *obj1, /* the object with a calculated profile */
                         int iprofMin, /* first value of profMean[] to use  */
                         float beta, /* the power law is r^{-beta}; if 0 use value in DGPSF */
                         int use_mean   /* Use mean, not median, profile */
                        );

#endif

