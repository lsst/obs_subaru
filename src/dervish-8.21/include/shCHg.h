#ifndef SHCHG_H
#define SHCHG_H

#include "shCErrStack.h"   
#include "shCRegUtils.h"
#include "shCAssert.h"
#include "shTclVerbs.h"
#include "shTclHandle.h"
#include "shCLink.h"
#include "shCVecExpr.h"

#define HG_OK    7000
#define HG_ERROR 7001
#define HG_DEVICE_NAMELEN 512
#define HG_TITLE_LENGTH 100
#define HG_SCHEMA_LENGTH 100
#define HG_OPT_LENGTH 12

#define FITNAME_LENGTH 20

#define MN_USE        101
#define MN_DO_NOT_USE 100
#define MN_NO_MORE     99

/* These values are used to test whether the user has entered a value (the
   corresponding bit is set) or the default value is being used. */
#define XMINOPT 1
#define XMAXOPT (XMINOPT << 1)
#define YMINOPT (XMAXOPT << 1)
#define YMAXOPT (YMINOPT << 1)

#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

#define HGLABELSIZE 80

/* HistoGram Structure */
typedef struct Hg {
  int id;
				/* info for this histogram */
  double minimum;
  double maximum;
  char name[HGLABELSIZE];
  char xLabel[HGLABELSIZE];
  char yLabel[HGLABELSIZE];
  unsigned int nbin;
  float *contents, *error;
  float *binPosition;
  double underflow;
  double overflow;
  unsigned int entries;
  double wsum, sum, sum2;
} HG;



typedef struct real {
   double value;
   double error;
   unsigned short masked;   /* true if this value is masked */
} REAL;                         /* pragma SCHEMA */
  
/* Pt Structure */
typedef struct Pt {
  int id;
  float row, col, radius;
} PT;


typedef struct Pgstate {
  int id;
				/* info for the hg plotting state */
  char device[HG_DEVICE_NAMELEN];
  int nxwindow, nywindow;	/* number of windows in x and y */
  float xfract, yfract;		/* window filling fraction in x and y */
  int ixwindow, iywindow;	/* current window in x and y */
  int just;			/* "just" of pgplot_pgenv */
  int axis;			/* "axis" of pgplot_pgenv */
  int full;			/* 0 if plot is empty; 1 if it is full */
				/* parameters used in PGBOX */
  char xopt[HG_OPT_LENGTH], yopt[HG_OPT_LENGTH];
  float xtick, ytick;
  int nxsub, nysub;
  HG *hg;			/* pointer to hg plotted */
  char xSchemaName[HG_SCHEMA_LENGTH];/* name of the element plotted in x */
  char ySchemaName[HG_SCHEMA_LENGTH]; /* name of the element plotted in y */
  char plotType[10];		/* HG or AF, depending on what is plotted */
  char plotTitle[HG_TITLE_LENGTH]; /* Title at the top of the page */
  int symb;			/* symb used in pgpt (page A-33) */
  int isLine;			/* 0 for points, 1 for line */
  int isTime;			/* 1 to enable times on axes with pgtbox */
  int icMark, icLine;		/* color index for mark and line */
  int icBox, icLabel;		/* color index for box and label */
  int icError;			/* color index for err bars;  0 for none */
  int lineWidth, lineStyle;	/* line specification (A-36 and A-37) */
  int isNewplot;		/* 1 to move to next plot or page */
} PGSTATE;

/* These are the functions that act on HG and PGSTATE structures */
HG *shHgNew(void);
RET_CODE shHgDefine(HG *hg, char *xLabel, char *yLabel, char *name,
		  double minimum, double maximum, unsigned int nbin);
RET_CODE shHgClear(HG *hg);
RET_CODE shHgFill(HG *hg, double value, double weight);
RET_CODE shHgPrint(HG *hg);
RET_CODE shHgTest(int test);
HG *shHgOper(HG *hg1, char *operation, HG *hg2);
RET_CODE shHgDel(HG *hg);
double   shHgMean(HG *hg);
double   shHgSigma(HG *hg);

PGSTATE *shPgstateNew(void);
RET_CODE shPgstateDefault(PGSTATE *hg);
RET_CODE shPgstateSet(PGSTATE *hg, int argc, char **argv, char *formalCmd);
RET_CODE shPgstatePrint(PGSTATE *hg);
RET_CODE shPgstateNextWindow(PGSTATE *hg);
RET_CODE shPgstateDel(PGSTATE *hg);
RET_CODE shPgstateOpen(PGSTATE *pgstate);
RET_CODE shPgstateTitle(PGSTATE *pgstate);
RET_CODE shHgPlot(PGSTATE *pgstate, HG *hg,
		  double xminIn,    /* min value for x */
		  double xmaxIn,    /* max value for x */
		  double yminIn,    /* min value for y */
		  double ymaxIn,    /* max value for y */
		  int xyOptMask /* XMINOPT | XMAXOPT, etc., to use above 
				   values, not calculated ones */   
		  );
RET_CODE shPgstateClose(PGSTATE *pgstate);
RET_CODE shHgReg(HG *hg, REGION *reg, int maskBits);

RET_CODE shChainFromPlot(PGSTATE *pgstate, CHAIN *outputChain, 
		       CHAIN *inputChain, VECTOR *vMask);
RET_CODE shVIndexFromPlot(
			  PGSTATE *pgstate,   /* what plot? */
			  VECTOR *vIndex,
			  VECTOR *vX,
			  VECTOR *vY,
			  VECTOR *vMask       /* mask */
			 );
RET_CODE shVIndexFromHg(
			PGSTATE *pgstate,   /* what plot? */
			VECTOR *vIndex,
			VECTOR *vX,
			VECTOR *vMask       /* mask */
			);
RET_CODE shChainToSao
  (ClientData clientData,
   CHAIN *chain, char *rowName, char *colName, char *radiusName);

int shGetCoord(PGSTATE *pgstate,  PT *pt1, PT *pt2);

void *shGetClosestFromChain(CHAIN *inputChain, VECTOR *vMask,
			    PT *point, char *xName, char *yName);

RET_CODE shPtBin(HG *hg, PT *pt, char *minORmax);

unsigned int shAppendIncludedElementsToChain(CHAIN *outputChain, 
					  CHAIN *inputChain, VECTOR *vMask,
					  PT *pt1, PT *pt2, 
					  char *xName, char *yName);

int shGetClosestFromVectors(VECTOR *vX, VECTOR *vY, VECTOR *vMask,
			    PT *pt);
unsigned int shGetIncludedFromVectors(VECTOR *vIndex,VECTOR *vX, VECTOR *vY, 
				      VECTOR *vMask, PT *pt1, PT *pt2);

PT *shPtNew(void);
RET_CODE shPtDefine(PT *pt, float row, float col, float radius);
RET_CODE shPtPrint(PT *pt);
RET_CODE shPtDel(PT *pt);

int shRegFluctuateAsSqrt(
			 REGION *regPtr,    /* region */
			 double gain	    /* the gain in electrons per ADU */
			 );

float shGetValueFromItem(void *item, char *memberName);
unsigned int shGetIncludedFromVector(VECTOR *vIndex,VECTOR *vX,  
                                      VECTOR *vMask, PT *pt1, PT *pt2);

void shFitGauss(
	       int *dummy,
	       double *indVar, /* the independent variable */
	       int *nParam, /* number of paramters */
	       double *paramValue, /* the values of the parameters */
	       double *functionValue, /* return:  the fit value at indVar */
	       double *df, /* dummy for now */
	       int *mode, /* the mode, 1 if we need to calculate df here */
	       int *nReturn /* return value -- 0 is good */
	       );

void shFitPoly(
	       int dummy,
	       double *indVar, /* the independent variable */
	       int nParam, /* number of paramters */
	       double *paramValue, /* the values of the parameters */
	       double *functionValue, /* return:  the fit value at indVar */
	       double *df, /* dummy for now */
	       int mode, /* the mode, 1 if we need to calculate df here */
	       int *nReturn /* return value -- 0 is good */
	       );

void shFitGauint(
		 int *dummy,
		 double *indVar, /* the independent variable */
		 int *nParam, /* number of paramters */
		 double *paramValue, /* the values of the parameters */
		 double *functionValue, /* return:  the fit value at indVar */
		 double *df, /* dummy for now */
		 int *mode, /* the mode, 1 if we need to calculate df here */
		 int *nReturn /* return value -- 0 is good */
		 );
void shFitExp(
                 int *dummy,
                 double *indVar, /* the independent variable */
                 int *nParam, /* number of paramters */
                 double *paramValue, /* the values of the parameters */
                 double *functionValue, /* return:  the fit value at indVar */
                 double *df, /* dummy for now */
                 int *mode, /* the mode, 1 if we need to calculate df here */                 int *nReturn /* return value -- 0 is good */
                 );

void shFitVector(
                 int *dummy,
                 double *indVar, /* the independent variable */
                 int *nParam, /* number of paramters */
                 double *paramValue, /* the values of the parameters */
                 double *functionValue, /* return:  the fit value at indVar */
                 double *df, /* dummy for now */
                 int *mode, /* the mode, 1 if we need to calculate df here */                 int *nReturn /* return value -- 0 is good */
                 );

void shFitVectorSet(
                    VECTOR *xVector,
                    VECTOR *yVector
                    );


void shMnHgSet(HG *hg);
HG *shMnHgGet(void);
int doubleCompare(const void *d1, const void *d2);
#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */


#endif




