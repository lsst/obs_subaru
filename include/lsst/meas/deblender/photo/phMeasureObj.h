#if !defined(PHMEASUREOBJ_H)
#define PHMEASUREOBJ_H

/*
#include "dervish.h"
#include "phSpanUtil.h"
#include "phRandom.h"
#include "p_phMeasureObj.h"
 */
#include "phCalib1.h"
#include "phFramestat.h"
#include "phObjc.h"


int phObjcDeblend(OBJC *objc, const FIELDPARAMS *fparams);

void
phDeblendSet(REGION *scr0,		/* buffers for smoothing operator */
	     REGION *scr1,		/*  may be NULL if no */
	     REGION *scr2,		/*   peak finding or */
	     REGION *scr3);		/*    deblending is desired */
void
phDeblendUnset(void);

void phRemoveAtlasImage(const OBJC *objc, const FIELDPARAMS *fiparams);
void phInsertAtlasImage(const OBJC *objc, const FIELDPARAMS *fiparams);

int
phVelocityFind(OBJC *objc,		/* OBJC whose velocity is desired */
	       const FIELDPARAMS *fiparams, /* info about the frame */
	       float *row,		/* fitted row positions, or NULL */
	       float *rowErr,		/* errors in row, or NULL */
	       float *col,		/* fitted column positions, or NULL */
	       float *colErr,		/* errors in col, or NULL */
	       float *rchisq);		/* reduced chi^2, or NULL */

int phObjcMakeChildren(OBJC *objc,		/* give this OBJC a family */
		       const FIELDPARAMS *fiparams); /* misc. parameters */

OBJC *phObjcChildNew(OBJC *objc,	/* the parent */
		     const PEAK *peak,	/* which peak? */
		     const FIELDPARAMS *fparams, /* gain, soft_bias, etc. */
		     int is_child);	/* make new OBJC a child, not a sibb */

#if defined(NOPE)
/*
 * The current best estimate of the PSF
 */
//extern const PSF_REG *KLPsf[NCOLOR];

/*
 * Here's the `science code':
 */
int
phInitMeasureObj(
		 FIELDPARAMS *fparams,	/* where to store the parameters */
		 int ncolor,		/* number of colours being processed */
		 float *sb_mag,		/* surf brightness of isophotes for
					   measuring object ellipticity,
					   isophotal quantities, mag/arcsec^2*/
		 RANDOM *rand);
int phFiniMeasureObj(void);

RET_CODE
phMeasureObjColorSet(FIELDPARAMS *fiparams, /* describe field */
		     FIELDSTAT *fieldstat, /* stats for field's reduction */
		     int color,		/* index of colour being set up */
		     const CALIB1 *calib, /* calibration info */
		     const REGION *data, /* corrected frame */
		     const BINREGION *sky, /* typical sky */
		     const BINREGION *skyErr, /* sky noise */
		     const PSF_BASIS *basis); /* variable PSF */
void phMeasureObjUnset(FIELDPARAMS *fparams);



RET_CODE
phMeasureObjc(OBJC *objc,		/* the OBJC to measure */
	      const FIELDPARAMS *fparams, /* and how to measure it */
	      FIELDSTAT *fieldstat,	/* and a summary of what we found */
	      int remove_atlas_images,	/* replace atlas images by noise? */
	      int bright,		/* are we measuring bright objects? */
	      int fit_models);		/* should we fit models to objects? */

void phFieldstatSetFromMO(FIELDSTAT *fieldstat);
#endif // NOPE


/*
 * These procedures are not really meant to be called by the general
 * public, but they are not strictly private either; so I'll put them
 * in the public .h file with some misgivings
 */
#if defined(NOPE)
void phObjcCenterCalc(OBJC *objc, const FIELDPARAMS *fparams, int use_color);

float
phPsfCountsSetupFromDgpsf(int color, /* the colour in question */
			  const DGPSF *psf, /* PSF in this band */
			  int set_coeffs); /* set coeffs_psf[]? */
void
phPsfCountsFromSincRegion(OBJC *objc,	/* object being measured */
			  int color,	/* which is this colour */
			  const CELL_STATS *prof, /* for the sinced region */
			  const FIELDPARAMS *fiparams); /* describe field */

float
phPsfSetAtPoint(int color,		/* which colour are we processing? */
		const PSF_BASIS *basis,	/* describe spatial variation of PSF */
		float rowc, float colc,	/* desired position in frame */
		float sigma,		/* smooth by N(0,sigma^2) to centroid*/
		float *ap_correction_err); /* error in ap. correction or NULL*/
float
phPsfSetFromRegion(int c,		/* which colour are we processing? */
		   const REGION *reg,	/* region containing PSF
					   (centered in region) */
		   float sigma,		/* smooth by N(0,sigma^2) to centroid*/
		   float *ap_correction_err); /* error in ap. corr., or NULL */

float
phPsfCountsFromRegion(const REGION *reg, /* region containing object */
		      int c,		/* band that data was taken in */
		      float rpeak, float cpeak, /* central pixel of object */
		      float ap_correction, /* aperture correction to apply */
		      float bkgd,	/* reg's sky level (not subtracted) */
		      float gain,	/* gain of amplifier */
		      float dark_variance, /* variance of dark background */
		      float bkgd_sub,	/* pre-subtracted background level */
		      float *err);	/* error in counts, or NULL */
float
phApertureCounts(const CELL_STATS *prof, /* the extracted profile */
		 int nann,		/* number of annuli to use */
		 float gain,		/* gain of amplifier */
		 float dark_variance,	/* variance of dark background */
		 float sky,		/* sky level */
		 float *apCountsErr);	/* error in aperture counts, or NULL */
float
phApertureCountsFromProfMean(const OBJC *objc, /* the object to measure */
			     int c,	/* in this band */
			     const FIELDPARAMS *fiparams,
			     int nann,	/* number of annuli to use */
			     float *apCountsErr); /* error in aperture counts,
						    or NULL */
int
phStokesParamEval(const REGION *reg,	/* input data */
		  int c,		/* the desired aperture */
		  int bkgd,		/* the REGION's background level */
		  int rowc, int colc,	/* central pixel of object */
		  int which,		/* 'Q' or 'U' */
		  int normalise,	/* if true, normalise the answer */
		  float *val);		/* the answer */
void phObjcChildDel(OBJC *child);	/* the child to destroy */
void phObjcChildrenDel(OBJC *objc);	/* the object whose children are
					   to be destroyed */

int phObjcDeblendMovingChild(OBJC *objc, const FIELDPARAMS *fparams);

void phDeblendedObjcRecenter(OBJC *objc, const FIELDPARAMS *fparams);


#if defined(PH_MATH_UTILS)
   void p_phInitEllipseFit(void);
   void p_phFiniEllipseFit(void);
   VEC *p_phEllipseFit(const float *r, const float *rErr, int debias);
   void p_phEllipseFitDebias(VEC *coeffs);
#endif

void phEstimateAstromBias(float *row_bias, float *col_bias,
			  const REGION *psf, const FRAMEPARAMS *fparams,
			  int binmax, CENTROID_FLAGS flags);

void phAdaptiveMomentsCalc(OBJC *objc,
			   int color,
			   const FIELDPARAMS *fparams);

int
phCentroidAndDebias(const REGION *reg, const FRAMEPARAMS *fparams,
		    int rpeak, int cpeak,
		    float *rowc, float *colc, float *rowcErr, float *colcErr);
void
phSaturSetInObject1Chain(const REGION *reg, /* region containing objects */
			 CHAIN *objs);	/* chain of objects */

#endif // NOPE

#endif
