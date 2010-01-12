#if !defined(EXTRACT_H)
#define EXTRACT_H

#include "phSpanUtil.h"

#define NANN  15			/* number of annuli */
#define NSEC  12			/* number of sectors */
#define NCELL ((NANN-1)*NSEC+1)		/* number of cells */

#define MAXCELLS 85

#if MAXCELLS != ((NANN-1)*NSEC/2 + 1)
#  error Please update the definition of MAXCELLS --- NANN or NSEC has changed
#endif

#define NSYNCANN 6			/* array index of boundary radius of
					   		 sync-shifted region */

/*
 * Define the region that should be sync shifted.
 *
 * SYNCRAD is the radial index of the outer edge of the outermost annulus
 * in the sync-shifted region, and must equal anndex[NSYNCANN]-1
 * (n.b. that this is the _radial_ not the array index -- see discussion
 * of anndex[])
 *
 * The sync region definitions are set up as follows:
 *                                  central
 *                                   pixel
 *                                    ||
 *                                    \/
 * |              |    |             |  |             |    |              |
 * |              |    |             |  |             |    |              |
 * <-- SYBELL-1 -->    <-- SYNCRAD -->  <-- SYNCRAD -->    <-- SYBELL-1 -->
 *                <----> SYNCEXTRA                    <----> SYNCEXTRA
 *                     <-------- SYNC_REG_SIZE ------->
 * <---------------------------- SYNC_REG_SIZEI -------------------------->
 *
 */
#define SYBELL 7			/* sync filter is 2*SYBELL-1 long */
#define SYNCRAD 11
#define SYNCEXTRA 4			/* extra size of sinc region */
#define SYNC_REG_SIZE (2*SYNCRAD+1)	/* size of region to be sinc-shifted */
#define SYNC_REG_SIZEI (SYNC_REG_SIZE + 2*(SYNCEXTRA + SYBELL-1))

/*****************************************************************************/
/*
 * This structure contains the quantities derived for a cell
 */
struct pstats {
    int ntot;				/* total # elements */
    float area;				/* area used to find mean (in pixels)*/
    float qt[3];			/* quartiles */
    float mean;				/* mean */    
    float sig;				/* sigma--real or from iqr */
    int min;				/* min of kept data in histogram */
    int rng;				/* range of kept data in histogram */
    int nel;				/* 'relevant' # of elements */
    float sum;				/* sum of 'relevant' points */
    PIX *data;				/* pointer to data */
    enum {				/* processing flags */
       EXTRACT_NONE = 0,		/* no flags set, that is:
					   cell didn't overlap edge of region
					   sigma is real (not from IQR)
					   histogram wasn't clipped.
					   The type of sort used is given by
					      (flg & EXTRACT_SORT_MASK) */
       EXTRACT_INS_SORT = 0x1,		/* data sorted with Insertion sort */
       EXTRACT_SHELL_SORT = 0x2,	/* data sorted with Shell sort */
       EXTRACT_HIST_SORT = 0x3,		/* data sorted with Histogram sort */
       EXTRACT_SORT_MASK = 0x7,		/* mask for type of sort field */
       EXTRACT_EDGE = 0x8,		/* cell overlaps edge of region */
       EXTRACT_BAD = 0x10,		/* data in cell was bad */
       EXTRACT_SIGIQR = 0x20,		/* sigma evaluated from iqr */
       EXTRACT_FAKEQT = 0x40,		/* quartiles derived from sigma */
       EXTRACT_CLIPPED = 0x80,		/* histogram was clipped; data
					   used was 1st percentile to
					   +PROFILE_NSIGMA sigma */
       EXTRACT_SINC = 0x100		/* mean was found by integration over
					   sinc-shifted function */
    } flg;
};

/*
 * this one describes the geometry of a cell
 */
struct cellgeom {
   int ann;				/* annulus index of cell */
   int sec;				/* sector index of cell */
   float n;				/* number of pixels in cell */
   float inner;				/* inner float radius of annulus */
   float outer;				/* outer float radius of annulus */
   float cw;				/* angle of clockwise boundary */
   float ccw;				/* angle of anti-clockwise boundary */
};

/*
 * and this one describes the geometry of a cell, as weighted
 * by some intensity distribution.
 */
struct cellmod {    
   float col;				/* mean col of cell   */
   float row;				/* mean row of cell   */
   float col2;				/* mean col^2 in cell */
   float row2;				/* mean row^2 in cell */
   float colrow;			/* mean col*row in cell  */
   float Q;				/* <(col^2 - row^2)/r^2> in cell */
   float U;				/* <col*row/r^2> in cell  */
   float rmean;				/* harmonic mean radius of cell */
   float rvar;				/* radius variance in cell */
};

/*****************************************************************************/
/*
 * This is the information returned to the user
 */
typedef struct {
   int id;				/* id of OBJECT1 that this CELL_STATS
					   refers to */
   int ncell;				/* number of cells */
   int nannuli;				/* number of annuli */
   int nannuli_c;			/* number of complete annuli */
   int annular;				/* true if a annular profile */
   float col_c, row_c;			/* centre used to extract profile,
					   or starting point of ridgeline */
   float col_1, row_1;			/* end point of ridgeline */
   int orad;				/* outer radius of profile */
   const REGION *syncreg;		/* sync-shifted central region */
   const float *radii;			/* radii of inner edges of annuli */
   const float *area;			/* areas of annuli */
	struct pstats *cells;		/* the data cells */
   struct cellgeom *geom;		/* geometry of cells */
   struct cellmod *mgeom;		/* (model-dependent) cell geometry */
} CELL_STATS;				/* pragma SCHEMA */

typedef struct {
   int ncell;				/* number of cells in {data,sig} */
   int is_median;			/* is data median profile? */
   float flux;				/* == sum(data*area) */
   float data[MAXCELLS];	        /* mean or median profile */
   float sig[MAXCELLS];			/* errors in data */
   float area[MAXCELLS];		/* number of pixels in data[] */
} CELL_PROF;				/* pragma SCHEMA */

/*****************************************************************************/
/*
 * aperture photometry, including calculation of Stokes parameters
 */
#define NCOEFF 35			/* Max number of coeffs in a row */
#define NMAX 35				/* Max number of rows */

struct run_T {				/* used in COEFF */
   int col0;				/* starting column */
   int n;				/* number of coefficients */
   float coeffs[NCOEFF];		/* coefficients */
};					/* pragma IGNORE */

typedef struct {			/* Coefficients for photometry */
   float sum;				/* Sum of coefficients */
   int cmin, cmax;			/* Min and max cols for needed pixels*/
   int rmin, rmax;			/* Min and max rows for needed pixels*/
   struct run_T run[NMAX];
} COEFF;				/* pragma IGNORE */

/******************* FUNCTION PROTOTYPES *********************************/

/* in extract.c */
void phInitProfileExtract(void);
void phFiniProfileExtract(void);
int phProfileIsInitialised(void);

CELL_PROF *phCellProfNew(int n);
void phCellProfRealloc(CELL_PROF *cprof, int n);
void phCellProfDel(CELL_PROF *cprof);

const CELL_STATS *phProfileSetFromCellprof(const CELL_PROF *cprof);
CELL_STATS *phProfileExtract(int id, int band,
			     const REGION *in, double rowc, double colc,
			     int rad, double sky, double skysig, int keep);
CELL_STATS *phProfileExtractLinear(int id, const REGION *in,
				   double row0, double col0,
				   double row1, double col1,
				   int hwidth, int bin,
				   double sky, double skysig);
CELL_STATS *phProfileGetLast(void);
int phProfileExtractOk(const REGION *in, double fyc, double fxc);
const CELL_STATS *phProfileGeometry(void);
void phProfileSkySubtract(CELL_STATS *cstats, float sky);
float
phSigma2GetFromProfile(const CELL_STATS *stats_obj,
		       float *sigma_rr, float *sigma_cc);
float phProfileMedian(const CELL_STATS *prof, int ann, int clip,
		      int local_mean, float *sig);
float phProfileMean(const CELL_STATS *prof, int ann, int clip, int local_mean,
		    float *sig);
int phProfileMeanFromSincApertures(const CELL_STATS *prof, int ann,
				   int clip, int local_mean,
				   float *mean, float *sig);
int phSincApertureMean(const REGION *reg, int c, int bkgd, int rc, int cc,
		       float *val, float *ntot);
int phSincApertureEval(const REGION *reg, int c, int bkgd, int rc, int cc,
		       float *val);
int phSincApertureEvalFromCoeffs(const REGION *reg, const COEFF *coeffs,
				 int bkgd, int rowc, int colc, float *val);
int phApoApertureEval(const REGION *reg, int ann, float asigma, float bkgd,
		      float rowc, float colc, float *val);
int phApoApertureMomentsEval(const REGION *reg, int ann, float asigma,
			     float bkgd, float rowc,
			     float colc, float *m_r, float *m_c);
int phApoApertureEvalNaive(const REGION *reg, float r, float asigma,float bkgd,
			   float rowc, float colc, float *val);

float *phXsectionExtract(const REGION *in,
		       double fy0, double fx0, double fy1, double fx1,
		       int hwidth, int *nval);
OBJMASK *phObjmaskSetFromProfile(int ncol, int nrow, const CELL_STATS *cstats,
				 float thresh, int nsec, int clip);
OBJMASK *phObjmaskSetFromProfileLinear(int ncol, int nrow,
				       const CELL_STATS *cs, float thresh);
void phObjmaskSetLinear(OBJMASK *mask, int ncol,int nrow, float fx0, float fy0,
			     float fx1, float fy1, int hwidth_l, int hwidth_r);

const REGION *phConvolveSyncregWithGaussian(float dsigma);

int phGetCellid(int r, int c);

/* in extract_utils.c */
int reduce_shift(float, float *);
void get_sync_with_cosbell(float *,int,double);
void region_maxmean_crude(struct pstats *);
void get_quartiles_from_array(U16 *,struct pstats *);
void get_quartiles_from_array_clipped(U16 *,struct pstats *,
				      int band, float fxc, float fyc,
				      const struct cellgeom *cgeom);
void shinstats(struct pstats *);
void simplestats(struct pstats *);
void badstats(struct pstats *);

#endif
