#if !defined(PHOBJECTS_H)
#define PHOBJECTS_H
//#include "region.h"
//#include "shChain.h"
#include "phMaskbittype.h"
#include "phExtract.h"
#include "phFramestat.h"
#include "phObjc.h"
#include "phSpanUtil.h"
#include "phPeaks.h"
#include "phUtils.h"
/*
 * typedefs and prototypes to deal with objects
 */
typedef struct {
   int nlevel;				/* number of levels */
   int size;				/* number of allocated levels */
   OBJMASK **sv;			/* object pixels at various levels */

   float rowc, colc;			/* an estimate of the centre */
   float rowcErr, colcErr;		/* and its standard deviation */
   PEAKS *peaks;			/* all peaks in object */
   int flags;				/* facts about the OBJECT */
   int flags2;				/* more facts about the OBJECT */
   int flags3;				/* internal-use-only facts */
} OBJECT;				/* pragma SCHEMA */


OBJECT *phObjectNew(int nlevel);
void phObjectDel(OBJECT *obj);

PEAK *phPeakNew(void);
void phPeakDel(PEAK *peak);

PEAKS *phPeaksNew(int size);
void phPeaksRealloc(PEAKS *peaks, int n);
void phPeaksDel(PEAKS *peaks);

void phObjectPropertiesCopy(OBJECT *obj1, const OBJECT *obj2);
void phObjectUnion(OBJECT *obj1, OBJECT *obj2, const REGION *reg);

void phInitObjectFinder(void);
void phFiniObjectFinder(void);

CHAIN *phObjectsFind(const REGION *reg, int row0, int col0, int row1,int col1, 
		     int nlevel, const unsigned short *levels,
		     const FRAMEPARAMS *fiparams, int npixel_min_level, int npixel_min);
CHAIN *phObjectsFindInObjmask(const REGION *reg, const OBJMASK *om,
			      int nlevel, const unsigned short *levels,
			      int npixel_min_level, int npixel_min, int npeak_max);
OBJMASK *phObjmaskFindInObjmask(const REGION *reg, const OBJMASK *om,
				int row, int col, int level);

int phObjectPeakFind(const REGION *reg, const OBJMASK *om, PEAKS *peaks);
int phObjectPeaksFind(const REGION *reg, const OBJMASK *om,
		      int delta, PEAKS *peaks, int npeak_max);
void
phPeaksCull(const REGION *reg,		/* region containing PEAKS */
	    PEAKS *peaks,		/* the peaks in question */
	    const OBJMASK *om,		/* the object containing the peaks */
	    int threshold,		/* minimum acceptable peak value */
	    float delta,		/* by how much peaks must exceed
					   the saddlepoint value */
	    float gain,			/* gain of chip */
	    float sky_var,		/* total variance/pixel of sky */
	    float neff);		/* effective area due to smoothing */
void
phObjectChainPeaksCull(const REGION *reg, /* REGION that objects live in */
		       const CHAIN *objects, /* the chain of OBJECTs to cull */
		       int threshold,	/* threshold for acceptable peaks */
		       float delta,	/* amount peaks must exceed
					   their saddle points */
		       const FRAMEPARAMS *fparams, /* gain etc. of chip */
		       float neff);	/* n_{effective} of any smoothing */

void phObjectCenterFit(OBJECT *obj, const REGION *in,
		       const FRAMEPARAMS *fparams, CENTROID_FLAGS flags);
void phObject1CenterFit(OBJECT1 *obj, const REGION *in,
			const FRAMEPARAMS *fparams, int binmax,
			CENTROID_FLAGS flags);
void
phObjmaskShape(const OBJMASK *om,	/* object to convert */
	       int *npixel,		/* number of pixels in OBJMASK */
	       float *cc, float *rc,	/* centroid of OBJMASK */
	       float *Mcc, float *Mcr, float *Mrr, /* second moments */
	       float *rat, float *pa);	/* axis ratio and position angle
					   of major axis */
void
phSpanmaskSetFromObjectChain(
			     const CHAIN *chain, /* array of objects */
			     SPANMASK *mask, /* mask of REGION that objs were
						found in */
			     S_MASKTYPE val, /* select type of mask to use */
			     int level /* desired level */
			     );
void
phSpanmaskSetFromObject1Chain(
			     const CHAIN *chain, /* array of object1s */
			     SPANMASK *mask, /* mask to set */
			     S_MASKTYPE val /* select which mask to use (e.g.
						S_MASK_OBJECT) */
			     );
void
phObjectChainUnbin(CHAIN *objects,	/* the chain of OBJECTs */
	       int rbin,		/* binning in row direction */
	       int cbin,		/* binning in column direction */
	       int row0,		/* origin of binned image */
	       int col0,		/*                   in parent */
	       int nrow, int ncol);	/* size of unbinned region, or -1 */
		   
OBJECT *
phObjectNewFromProfile(
		       const CELL_STATS *cstats, /* extracted profile */
		       int ncol, int nrow, /* size of REGION in which
					    object is embedded */
		       float thresh,	/* set bits if profile exceeds thresh*/
		       int set_annulus,	/* no. of sectors above threshold
					   to mask the entire annulus*/
		       int clip		/* if >= 0, calculate clipped mean for
					   annulus, and compare this to thresh
					   to mask entire annulus */
		       );
OBJECT *phObjectGrowFromProfile(const OBJECT *obj, /* the original OBJECT */
				int level, /* the desired level */
				const REGION *reg, /* found in this REGION */
				const FRAMEPARAMS *fparams, /* gain etc. */
				float thresh, /* threshold to grow to */
				int set_sectors, /* no. of sectors above
						    threshold to mask the
						    entire annulus*/
				int clip, /* how to clip */
				float rat, /* critical axis ratio */
				int npix_min, /* min no. of pixels for linear
						 features */
				int rad		/* max radius for profile */
				);
OBJECT *
phObjectGrow(const OBJECT *obj,		/* OBJECT to grow */
	     int lev,			/* desired level */
	     const REGION *reg,		/* REGION obj that lives in */
	     int n			/* by how many pixels to grow */
	     );

void
phObjectChainGrow(CHAIN *chain,		/* the chain of OBJECTs to grow */
		  int lev,		/* desired level */
		  const REGION *reg,	/* REGION that objects live in */
		  int n			/* by how many pixels to grow */
		  );
		     
void
phObjectChainTrim(CHAIN *chain,		/* the chain of OBJECTs to grow */
		  int lev,		/* desired level */
		  int npixmin		/* minimum number of pixels */
                 );
OBJECT1 *phObjectToObject1Convert(OBJECT *obj, /* object to convert */
				  int level,	/* desired level */
				  const REGION *reg, /* region where obj was
							found*/
				  float sky /* sky level */
				  );
CHAIN *phObjectToObject1ChainConvert(CHAIN *chain, /* chain of OBJECTs */
				     int lev,	   /* desired level */
				     const REGION *reg,  /* region where objs
							    were found */
				     const BINREGION *sky /* sky level image*/
				     );
void phObjectChainFlags2Set(CHAIN *objects, int level, int flags2);
void phObjectChainFlags3Set(CHAIN *objects, int level, int flags3);
void phObject1ChainFlagsSet(CHAIN *obj1chain, int obj1_flags);

OBJECT *phLinearFeatureFind(const REGION *reg, const OBJECT *obj,
			    float majaxis, int follow_ridge,
			    int hwidth, int bin, int len, float thresh);
REGION *
phRegionTrimToThreshold(const REGION *in, /* the input region */
			float threshold,  /* the desired threshold */
			int npixel_min,	  /* min number of pixels in an object
					     (<= 0 => no limit is applied) */
			int ngrow,	  /* how many pixels to grow detections */
			S_MASKTYPE masktype); /* which bit plane to set in reg->mask (or -1) */

#endif
