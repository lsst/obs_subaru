#if !defined(PHUTILS_H)
#define PHUTILS_H

#include "dervish.h"
#include "phConsts.h"

const char *phPhotoName(void);

int phStrategicMemoryReserveSet(const size_t size);
int phStrategicMemoryReserveIsEmpty(void);

int get_FtclOpts(Tcl_Interp *interp, int *ac, char **av, ftclArgvInfo *opts);

RET_CODE shMasksMerge(MASK *mask1, MASK *mask2, int value);
int shChainDestroy(CHAIN *pChain, void (*pDelFunc)(void *));

int shMasksOverlap(MASK *mask1, MASK *mask2, int dcol, int drow);
REGION *shRegGetEnlarged(char *name, REGION *region, int row, int col,
           int nrow, int ncol, REGION_FLAGS flags);
int shRegIntCopy(REGION *out, const REGION *in);
int shRegIntSetVal(REGION *reg, const float val);
#if defined(PHSPANUTIL_H)
void phRegIntSetValInObjmask(REGION *reg, const OBJMASK *om, const float val);
void phRegIntClipValInObjmask(REGION *reg, const OBJMASK *om,
			      const int drow, const int dcol, const int val);
#endif
int shRegIntConstAdd(REGION *reg, const float val, const int dither);
int shRegIntAdd(REGION *reg1, REGION *reg2);
int shRegIntConstMult(
		      REGION *reg,	/* The region ... */
		      const float val	/* the constant to multiply by */
		      );
int shRegIntConstMultAndShift(
			      REGION *reg, /* The region ... */
			      const int val, /* the constant to multiply by */
			      const int shift /* how much to down-shift */
			      );
typedef enum {
    LINCOM_EXACT = 0, LINCOM_INTERSECT, LINCOM_UNION, LINCOM_NFLAG
} LINCOM_FLAGS;
REGION *shRegIntLincom(REGION *reg1, const REGION *reg2,
		       float a, float b, float c, LINCOM_FLAGS flag);
int
shRegIntLogand(REGION *reg,		/* The region ... */
	       const unsigned int mask); /* the mask to be ANDed */
REGION *phRegS32ToU16Convert(REGION *reg_s32, const REGION *reg_u16);
int phRegStatsSigmaClip(REGION *reg, char mask_flag, int iter, float clipsig,
                           float *mean, float *stdev);

int phPtrArrayGetFromTclArrayOfHandles(Tcl_Interp *interp,
				       char *arrayName,
				       char *list,
				       char *strtype,
				       void ***array,
				       int *nele);
int
phFltArrayGetFromTclList(Tcl_Interp *interp, char *list, float **vals, int nel);
int
phFltArrayGetFromTclArray(Tcl_Interp *interp,
			  char *arrayName, /* name of tcl array */
			  char *indices, /* indices of tcl array */
			  float **vals); /* array to allocate and return */

int
phTclArrayOfHandlesGetFromCArray(Tcl_Interp *interp, void **Carray,
				 int nele, const char *type,
				 const char *TCLarray, const char **indices);

int phRegStatsFromQuartiles(const REGION *reg, int coarse, float clip,
			    float *mean, float *med, float *sigma,
			    float *iqr, float *q1, float *q3,
			    float *cmed, float *csigma,
			    PIX *minpix, PIX *maxpix, float *counts );

#if defined(PH_VARIABLE_PSF_H)
ACOEFF *
phPolynomialFit(float *val,		/* values to be fit */
		float *valErr,		/* errors in values; or NULL */
		float *row,		/* row positions of val[] */
		float *col,		/* column positions of val[] */
		int n,			/* number of points in {val,row,col} */
		int nterm_row,		/* number of terms; in row and  */
		int nterm_col,		/*     column direction. linear == 2 */
		float *mean,		/* return mean of fit (or NULL) */
		float *sig);		/* return s.d. of fit (or NULL) */
float
phPolynomialEval(const ACOEFF *acoeff,	/* ACOEFF to evaluate */
		 float row, float col);	/* desired position */
#endif

void
phMaskSetFromCircle(MASK *mask,		/* the mask to set */
		    char val,		/* to this value */
		    float rowc, float colc, /* desired centre */
		    float r);		/* desired radius */
void
shChainQsort(CHAIN *chain,
	     int (*compar)(const void *, const void *));

REGION *
phRegIntShift(REGION *out,		/* output region; may be NULL */
	      const REGION *in,		/* input region */
	      REGION *scr,		/* scratch space; may be NULL */
	      int filtsize,		/* size of sinc filter (must be odd) */
	      float dr,			/* shift by this far in row... */
	      float dc);		/*      and this far in column */

float
phQuartilesGetFromArray(const void *arr, /* the data values */
			int type,	/* type of data */
			int n,		/* number of points */
			int clip,	/* should we clip histogram? */
			float *qt,	/* quartiles (may be NULL) */
			float *mean,	/* mean (may be NULL) */
			float *sig);	/* s.d. (may be NULL) */
long
phCrcCalc(long crc,			/* initial value of CRC (e.g. 0) */
	  const char *buff,		/* buffer to be CRCed */
	  int n);			/* number of chars in buff */
long
phCrcCalcFromFile(const char *file,	/* file to be CRCed */
		  int nbyte);		/* number of chars to process */
FILE *phTmpfile(void);

void
phRegFloor(REGION *reg,			/* the region */
	   float min);			/* desired minimum value */

void
phTchebyfit(const VECTOR_TYPE *x,
	    const VECTOR_TYPE *y,
	    const VECTOR_TYPE *w,
	    int n, int m,
	    const VECTOR_TYPE *fitx,
	    VECTOR_TYPE *fity, 
            int fitn);

REGION *
phRegionExtrapolate(REGION *out,	     /* output region, or NULL */
		    const REGION *in);	     /* input region */

PIXDATATYPE phTypenameToType(const char *typeStr);

/*
 * A type to allow us to get at the meaning PIX -- note that this file
 * is processed specially in the Makefile to expand PIX
 */
typedef struct {
    PIX pixtype;
} PHOTO_INFO;                           /* pragma SCHEMA */

#endif
