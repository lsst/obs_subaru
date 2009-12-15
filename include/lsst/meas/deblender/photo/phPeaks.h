#if !defined(PHPEAKS_H)
#define PHPEAKS_H

/*
 * typedefs and prototypes to deal with PEAKS and PEAKs
 *
 * These are the values of the PEAK.flag field
 */
#define PEAK_NOFLAGS 0x0		/* no flags are set */
#define PEAK_BAND0 0x01			/* detected in band 0 */
#define PEAK_BAND1 0x02			/* detected in band 1 */
#define PEAK_BAND2 0x04			/* detected in band 2 */
#define PEAK_BAND3 0x08			/* detected in band 3 */
#define PEAK_BAND4 0x10			/* detected in band 4 */
#define PEAK_BANDMASK 0x1f		/* mask for the band detection bits */
#define PEAK_REAL 0x20			/* peak has been declared real */
#define PEAK_EDGE 0x40			/* equiv to OBJECT1_EDGE */
#define PEAK_PEAKCENTER 0x80		/* equiv to OBJECT1_PEAKCENTER */
#define PEAK_SATUR 0x100		/* equiv to OBJECT1_SATUR */
#define PEAK_DANGLING 0x1000		/* phPeakDel should free this peak
					   when seen on the PEAK->next list */
#define PEAK_CANONICAL 0x2000		/* coordinates are in canonical band */
#define PEAK_IN_BRIGHT 0x4000		/* found within a bright object */
#define PEAK_BINNED_EDGE 0x8000		/* centre was at edge of binned reg */
#define PEAK_MOVED 0x10000		/* peak's object may be moving */
#define PEAK_BINNED_CENTER 0x20000	/* image was binned while centroiding*/
#define PEAK_FREE 0x40000		/* PEAK has been deleted */

typedef struct peak {
   const int id;			/* unique ID for this PEAK */
   int catID;			        /* Peak's catID, if from known objects file */
   int flags;				/* which band, checked?, etc. */
   float peak;				/* peak value */
   int rpeak, cpeak;			/* peak pixel */
   float rowc, colc;			/* peak of object */
   float rowcErr, colcErr;		/* errors in {row,col}c */
   const struct peak *next;		/* chain merged peaks together */
} PEAK;					/* pragma SCHEMA */

typedef struct {
   int npeak;				/* number of peaks */
   int size;				/* size of peaks array */
   PEAK **peaks;
} PEAKS;				/* pragma SCHEMA */
/*
 * flags to specify how centroiding code should behave.  If it's already
 * smoothed, assume that the filter was N(0,FRAMEPARAMS.smooth_sigma^2)
 */
typedef enum {
   ALWAYS_SMOOTH = 0x1,			/* force smoothing */
   NO_SMOOTH     = 0x2,			/* disable smoothing */
   USE_MEAN      = 0x4,			/* use mean, not median, smoothing */
   SMOOTHED      = 0x8			/* data is already smoothed */
} CENTROID_FLAGS;			/* pragma ignore */
   

PEAK *phPeakNew(void);
PEAK *phPeakNewFromPeak(const PEAK *opeak);
void phPeakDel(PEAK *peak);

PEAK *phPeakCopy(PEAK *peak1, const PEAK *peak2);

PEAKS *phPeaksNew(int size);
void phPeaksRealloc(PEAKS *peaks, int n);
void phPeaksRenew(PEAKS *peaks, int n);
void phPeaksDel(PEAKS *peaks);
int phPeakBand(const PEAK *peak);

void phPeaksSort(const PEAKS *peaks);

#if defined(PHSPANUTIL_H)
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
#endif

#if defined(PHFRAMESTAT_H)
void
phObjectChainPeaksCull(const REGION *reg, /* REGION that objects live in */
		       const CHAIN *objects, /* the chain of OBJECTs to cull */
		       int threshold,	/* threshold for acceptable peaks */
		       float delta,	/* amount peaks must exceed
					   their saddle points */
		       const FRAMEPARAMS *fparams, /* gain etc. of chip */
		       float neff);	/* n_{effective} of any smoothing */
void
phPeakCenterFit(PEAK *peak, const REGION *in, const OBJMASK *mask,
		const FRAMEPARAMS *fparams, int binmax, CENTROID_FLAGS cflags);
#endif

#endif
