#ifndef ATHGMATH_H
#define ATHGMATH_H

#include <dervish.h>
#include "atTrans.h"
#include "sdssmath.h"


typedef struct Hg2 {
  REGION *reg;
  double xmin, xmax, delx;
  double ymin, ymax, dely;
  int nx, ny;
  char xlab[100], ylab[100], name[100];
} HG2;

FUNC *atHgGaussianFit(
		       HG *hg, 
		       int maxit,
		       int iprt
		       );

FUNC *atHgPolyFit(
		   HG *hg,       /* the HG to fit */
		   int order,    /* 0 for constant, 1 for line, etc. */
                   VECTOR *coef  /* coefficients of the fit */
		   );

int atProco(  
	    double *xin  ,
	    double *yin  ,
	    double *xou  ,
	    double *you  ,
	    double *a0   ,
	    double *d0   ,
	    double *dx   ,
	    double *dy   ,
	    double *rho  ,
	    int   *sys  ,
	    int   *mode 
	    );

RET_CODE atRegPlot(
		   PGSTATE *pg, /* the PGPSTATE to plot in */
		   REGION *reg,	/* the REGION to plot */
		   float fg,	/* minimum pixel value to show */
		   float bg,	/* maximum pixel value to show */
		   TRANS *trans, /* TRANS to get from row,col to display */
		   int mode,	/* 0=linear; 1=log; 2-sqrt */
		   char *xlab,	/* x label string */
		   char *ylab,	/* y label string */
		   int nobox    /* 0 to plot box, 1 to not plot box */
		   );

RET_CODE atRegColorPlot(
			PGSTATE *pg, /* the PGPSTATE to plot in */
			REGION *reg,	/* the REGION to plot */
			float fg,	/* minimum pixel value to show */
			float bg,	/* maximum pixel value to show */
			TRANS *trans, /* TRANS: row,col to display */
			int smode,	/* 0=linear; 1=log; 2-sqrt */
			int cmode,   /* 1=gray; 2=rainbow; 3=heat; 
					4=weird IRAF;  5=AIPS; 6=TJP*/
			char *xlab,	/* x label string */
			char *ylab,	/* y label string */
		   	int nobox    /* 0 to plot box, 1 to not plot box */
			);

HG2 *atHg2New(
	      double xmin,
	      double xmax,
	      int nx,
	      double ymin,
	      double ymax,
	      double ny,
	      char *xlab,
	      char *ylab,
	      char *name
	      );

RET_CODE atHg2Del(
		  HG2 *hg2
		  );

RET_CODE atHg2Fill(
		 HG2 *hg2,
		 double x,
		 double y,
		 double w
		 );

double atHg2Get(
	      HG2 *hg2,
	      double x,
	      double y
	      );

RET_CODE atHg2VFill(
		  HG2 *hg2,
		  VECTOR *vx,
		  VECTOR *vy,
		  VECTOR *weight
		  );

RET_CODE atHg2Cont(
		 PGSTATE *pg, 
		 HG2 *hg2,
		 VECTOR *cVec
		 );

RET_CODE atHg2Imag(
		 PGSTATE *pg, 
		 HG2 *hg2,
		 float a1,
		 float a2
		 );

RET_CODE atHg2Gray(
		 PGSTATE *pg, 
		 HG2 *hg2,
		 float fg,
		 float bg
		 );

RET_CODE atRegHistEq(
		     REGION *reg,
		     HG *hg,
		     double min,
		     double max
		     );
#endif


