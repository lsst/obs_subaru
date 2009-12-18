/* <AUTO>
   FILE: atHgMath
<HTML>
   Routines which operate on HGs AFs and VECTORs
</HTML>
   </AUTO>
*/

/* Generally usful HG plotting */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include "smaValues.h"

#include "slalib.h"
#include "dervish.h"
#include "sdssmath.h"
#include "atTrans.h"
#include "atSlalib.h"
#include "cpgplot.h"
#include "atConversions.h"
#include "atHgMath.h"

#define minimum(x,y) (((x) < (y)) ? (x) : (y))
#define maximum(x,y) (((x) > (y)) ? (x) : (y))

typedef double MATRIX[3][3]; 

/************** static functions *************/


static RET_CODE atTransMultiply(TRANS *ta_ptr, TRANS *tb_ptr, TRANS *tc_ptr);
static RET_CODE atTransToMatrix(TRANS *t_ptr, MATRIX m);
static RET_CODE atMatrixToTrans(MATRIX m, TRANS *t_ptr);

static void atPalett(int cmode);

static void atPalett(int cmode) {
  float gl[2] = {0.0, 1.0};
  float gr[2] = {0.0, 1.0};
  float gg[2] = {0.0, 1.0};
  float gb[2] = {0.0, 1.0};

  float inv_gl[2] = {0.0, 1.0};
  float inv_gr[2] = {1.0, 0.0};
  float inv_gg[2] = {1.0, 0.0};
  float inv_gb[2] = {1.0, 0.0};

  float rl[9] = {-0.5, 0.0, 0.17, 0.33, 0.50, 0.67, 0.83, 1.0, 1.7};
  float rr[9] = { 0.0, 0.0,  0.0,  0.0,  0.6,  1.0,  1.0, 1.0, 1.0};
  float rg[9] = { 0.0, 0.0,  0.0,  1.0,  1.0,  1.0,  0.6, 0.0, 1.0};
  float rb[9] = { 0.0, 0.3,  0.8,  1.0,  0.3,  0.0,  0.0, 0.0, 1.0};
  
  float inv_rl[9] = {-0.5, 0.0, 0.17, 0.33, 0.50, 0.67, 0.83, 1.0, 1.7};
  float inv_rr[9] = { 1.0, 1.0,  1.0,  1.0,  0.6,  0.0,  0.0, 0.0, 0.0};
  float inv_rg[9] = { 1.0, 0.0,  0.6,  1.0,  1.0,  1.0,  0.0, 0.0, 0.0};
  float inv_rb[9] = { 1.0, 0.0,  0.0,  0.0,  0.3,  1.0,  0.8, 0.3, 0.0};

  float hl[5] = {0.0, 0.2, 0.4, 0.6, 1.0};
  float hr[5] = {0.0, 0.5, 1.0, 1.0, 1.0};
  float hg[5] = {0.0, 0.0, 0.5, 1.0, 1.0};
  float hb[5] = {0.0, 0.0, 0.0, 0.3, 1.0};
  
  float inv_hl[5] = {0.0, 0.2, 0.4, 0.6, 1.0};
  float inv_hr[5] = {1.0, 1.0, 1.0, 0.5, 0.0};
  float inv_hg[5] = {1.0, 1.0, 0.5, 0.0, 0.0};
  float inv_hb[5] = {1.0, 0.3, 0.0, 0.0, 0.0};
  
  float wl[10] = {0.0, 0.5, 0.5, 0.7, 0.7, 0.85, 0.85, 0.95, 0.95, 1.0};
  float wr[10] = {0.0, 1.0, 0.0, 0.0, 0.3,  0.8,  0.3,  1.0,  1.0, 1.0};
  float wg[10] = {0.0, 0.5, 0.4, 1.0, 0.0,  0.0,  0.2,  0.7,  1.0, 1.0};
  float wb[10] = {0.0, 0.0, 0.0, 0.0, 0.4,  1.0,  0.0,  0.0, 0.95, 1.0};
  
  float inv_wl[10] = {0.0, 0.5, 0.5, 0.7, 0.7, 0.85, 0.85, 0.95, 0.95, 1.0};
  float inv_wr[10] = {1.0, 1.0, 1.0, 0.3, 0.8, 0.3,  0.0,  0.0,  1.0,  0.0};
  float inv_wg[10] = {1.0, 1.0, 0.7, 0.2, 0.0, 0.0,  1.0,  0.4,  0.5,  0.0};
  float inv_wb[10] = {1.0, 0.95,0.0, 0.0, 1.0, 0.4,  0.0,  0.0,  0.0,  0.0};
  
  float al[20] = {0.0, 0.1, 0.1, 0.2, 0.2, 0.3, 0.3, 0.4, 0.4, 0.5, 0.5, 0.6, 0.6, 0.7, 0.7, 0.8, 0.8, 0.9, 0.9, 1.0};
  float ar[20] = {0.0, 0.0, 0.3, 0.3, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
  float ag[20] = {0.0, 0.0, 0.3, 0.3, 0.0, 0.0, 0.0, 0.0, 0.8, 0.8, 0.6, 0.6, 1.0, 1.0, 1.0, 1.0, 0.8, 0.8, 0.0, 0.0};
  float ab[20] = {0.0, 0.0, 0.3, 0.3, 0.7, 0.7, 0.7, 0.7, 0.9, 0.9, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  
  float inv_al[20] = {0.0, 0.1, 0.1, 0.2, 0.2, 0.3, 0.3, 0.4, 0.4, 0.5, 0.5, 0.6, 0.6, 0.7, 0.7, 0.8, 0.8, 0.9, 0.9, 1.0};
  float inv_ar[20] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.5, 0.5, 0.3, 0.3, 0.0, 0.0};
  float inv_ag[20] = {0.0, 0.0, 0.8, 0.8, 1.0, 1.0, 1.0, 1.0, 0.6, 0.6, 0.8, 0.8, 0.0, 0.0, 0.0, 0.0, 0.3, 0.3, 0.0, 0.0};
  float inv_ab[20] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.9, 0.9, 0.7, 0.7, 0.7, 0.7, 0.3, 0.3, 0.0, 0.0};
  
  float tl[4] = {0.0, 0.5, 0.5, 1.0};
  float tr[4] = {0.2, 0.6, 0.6, 1.0};
  float tg[4] = {0.0, 0.0, 0.5, 1.0};
  float tb[4] = {1.0, 0.0, 0.0, 0.0};
  
  float inv_tl[4] = {0.0, 0.5, 0.5, 1.0};
  float inv_tr[4] = {1.0, 0.6, 0.6, 0.2};
  float inv_tg[4] = {1.0, 0.5, 0.0, 0.0};
  float inv_tb[4] = {0.0, 0.0, 0.0, 0.1};
  
  float redl[2] = {0.0, 1.0};
  float redr[2] = {0.0, 1.0};
  float redg[2] = {0.0, 0.0};
  float redb[2] = {0.0, 0.0};

  float inv_redl[2] = {0.0, 1.0};
  float inv_redr[2] = {1.0, 0.0};
  float inv_redg[2] = {0.0, 0.0};
  float inv_redb[2] = {0.0, 0.0};

  float greenl[2] = {0.0, 1.0};
  float greenr[2] = {0.0, 0.0};
  float greeng[2] = {0.0, 1.0};
  float greenb[2] = {0.0, 0.0};

  float inv_greenl[2] = {0.0, 1.0};
  float inv_greenr[2] = {0.0, 0.0};
  float inv_greeng[2] = {1.0, 0.0};
  float inv_greenb[2] = {0.0, 0.0};

  float bluel[2] = {0.0, 1.0};
  float bluer[2] = {0.0, 0.0};
  float blueg[2] = {0.0, 0.0};
  float blueb[2] = {0.0, 1.0};

  float inv_bluel[2] = {0.0, 1.0};
  float inv_bluer[2] = {0.0, 0.0};
  float inv_blueg[2] = {0.0, 0.0};
  float inv_blueb[2] = {1.0, 0.0};

  float yelll[2] = {0.0, 1.0};
  float yellr[2] = {0.0, 1.0};
  float yellg[2] = {0.0, 1.0};
  float yellb[2] = {0.0, 0.0};

  float inv_yelll[2] = {0.0, 1.0};
  float inv_yellr[2] = {1.0, 0.0};
  float inv_yellg[2] = {1.0, 0.0};
  float inv_yellb[2] = {0.0, 0.0};

  float cyanl[2] = {0.0, 1.0};
  float cyanr[2] = {0.0, 0.0};
  float cyang[2] = {0.0, 1.0};
  float cyanb[2] = {0.0, 1.0};

  float inv_cyanl[2] = {0.0, 1.0};
  float inv_cyanr[2] = {0.0, 0.0};
  float inv_cyang[2] = {1.0, 0.0};
  float inv_cyanb[2] = {1.0, 0.0};

  float magl[2] = {0.0, 1.0};
  float magr[2] = {0.0, 1.0};
  float magg[2] = {0.0, 0.0};
  float magb[2] = {0.0, 1.0};

  float inv_magl[2] = {0.0, 1.0};
  float inv_magr[2] = {1.0, 0.0};
  float inv_magg[2] = {0.0, 0.0};
  float inv_magb[2] = {1.0, 0.0};

  float r2l[5] = { 0.00, 0.25, 0.50, 0.75, 1.00};
  float r2r[5] = { 0.00, 0.00, 1.00, 1.00, 1.00};
  float r2g[5] = { 0.00, 0.00, 0.00, 1.00, 1.00};
  float r2b[5] = { 0.00, 1.00, 0.00, 0.00, 1.00};
  
  float inv_r2l[5] = { 0.00, 0.25, 0.50, 0.75, 1.00};
  float inv_r2r[5] = { 1.00, 1.00, 1.00, 0.00, 0.00};
  float inv_r2g[5] = { 1.00, 1.00, 0.00, 0.00, 0.00};
  float inv_r2b[5] = { 1.00, 0.00, 0.00, 1.00, 0.00};
  
  switch (cmode) {
  case 1:
    cpgctab(gl, gr, gg, gb, 2, 1.0, 0.5); break;
  case -1:
    cpgctab(inv_gl, inv_gr, inv_gg, inv_gb, 2, 1.0, 0.5); break;
  case 2:
    cpgctab(rl, rr, rg, rb, 9, 1.0, 0.5); break;
  case -2:
    cpgctab(inv_rl, inv_rr, inv_rg, inv_rb, 9, 1.0, 0.5); break;
  case 3:
    cpgctab(hl, hr, hg, hb, 5, 1.0, 0.5); break;
  case -3:
    cpgctab(inv_hl, inv_hr, inv_hg, inv_hb, 5, 1.0, 0.5); break;
  case 4:
    cpgctab(wl, wr, wg, wb, 10, 1.0, 0.5); break;
  case -4:
    cpgctab(inv_wl, inv_wr, inv_wg, inv_wb, 10, 1.0, 0.5); break;
  case 5:
    cpgctab(al, ar, ag, ab, 20, 1.0, 0.5); break;
  case -5:
    cpgctab(inv_al, inv_ar, inv_ag, inv_ab, 20, 1.0, 0.5); break;
  case 6:
    cpgctab(tl, tr, tg, tb, 4, 1.0, 0.5); break;
  case -6:
    cpgctab(inv_tl, inv_tr, inv_tg, inv_tb, 4, 1.0, 0.5); break;
  case 7:
    cpgctab(redl, redr, redg, redb, 2, 1.0, 0.5); break;
  case -7:
    cpgctab(inv_redl, inv_redr, inv_redg, inv_redb, 2, 1.0, 0.5); break;
  case 8:
    cpgctab(greenl, greenr, greeng, greenb, 2, 1.0, 0.5); break;
  case -8:
    cpgctab(inv_greenl, inv_greenr, inv_greeng, inv_greenb, 2, 1.0, 0.5); break;
  case 9:
    cpgctab(bluel, bluer, blueg, blueb, 2, 1.0, 0.5); break;
  case -9:
    cpgctab(inv_bluel, inv_bluer, inv_blueg, inv_blueb, 2, 1.0, 0.5); break;
  case 10:
    cpgctab(yelll, yellr, yellg, yellb, 2, 1.0, 0.5); break;
  case -10:
    cpgctab(inv_yelll, inv_yellr, inv_yellg, inv_yellb, 2, 1.0, 0.5); break;
  case 11:
    cpgctab(cyanl, cyanr, cyang, cyanb, 2, 1.0, 0.5); break;
  case -11:
    cpgctab(inv_cyanl, inv_cyanr, inv_cyang, inv_cyanb, 2, 1.0, 0.5); break;
  case 12:
    cpgctab(magl, magr, magg, magb, 2, 1.0, 0.5); break;
  case -12:
    cpgctab(inv_magl, inv_magr, inv_magg, inv_magb, 2, 1.0, 0.5); break;
  case 13:
    cpgctab(r2l, r2r, r2g, r2b, 5, 1.0, 0.5); break;
  case -13:
    cpgctab(inv_r2l, inv_r2r, inv_r2g, inv_r2b, 5, 1.0, 0.5); break;

  default:
    cpgctab(gl, gr, gg, gb, 2, 1.0, 0.5); break;
  }
  return;
}

/************** science routines *************/

/*<AUTO EXTRACT>

  ROUTINE: atHgGaussianFit

  DESCRIPTION:
<HTML>
	Fits a Gaussian to an HG histogram and returns a FUNC 
	with the parameters of the Gaussian in it.  maxit
        controls the maximum number of iterations for fiting.
        iprt is a flag for printing fit status at each iteration,
        but hasn't been implemented in this version and does nothing.<BR>

        Calls the SLATEC routine smaDNLS1.
</HTML>

  RETURN VALUES:
<HTML>
	Returns a FUNC struct of Gaussian type.
</HTML>
</AUTO>*/
FUNC *atHgGaussianFit(
		       HG *hg, 
		       int maxit,
		       int iprt
		       ) {
  FUNC* fitFunc=NULL;
 
  int ct;
  int iopt, m, n, ldfjac, maxfev, mode, nprint, info, nfev, njev;
  int *ipvt; 
  double ftol, xtol, gtol, epsfcn, factor;
  double *x, *fvec, *diag, *qtf; 
  double *wa1, *wa2, *wa3, *wa4;
  double *fjac;
  VECTOR *xValues, *yValues, *wValues;    /* the actual (x,y) data and weight */
  VECTOR **funcValue=NULL;
  METH *funcMeth=NULL; 

/* some error checking  */
  shAssert( hg != NULL );
  shAssert( hg->nbin > 0);
  shAssert( maxit > 0 );

/* set up some constants and allocate space */

  m   = hg->nbin;     /* number of data points */
  n   = 3;            /* number of unknowns */

  ipvt = (int *) shMalloc( n * sizeof(int) );
  x    = (double *) shMalloc( n * sizeof(double) );
  fvec = (double *) shMalloc( m * sizeof(double) );
  diag = (double *) shMalloc( n * sizeof(double) );
  qtf  = (double *) shMalloc( n * sizeof(double) );
  wa1  = (double *) shMalloc( n * sizeof(double) );
  wa2  = (double *) shMalloc( n * sizeof(double) );
  wa3  = (double *) shMalloc( n * sizeof(double) );
  wa4  = (double *) shMalloc( m * sizeof(double) );
  fjac = (double *) shMalloc( (m*n) * sizeof(double) );  

  funcValue = (VECTOR **)shMalloc( sizeof(VECTOR *) );
  funcValue[0] = shVectorNew(3);

  xValues = shVectorNew( m );
  yValues = shVectorNew( m ); 
  wValues = shVectorNew( m ); 

  iopt   = 1;                  /* flag indicating Jacobian not supplied by user */
  ldfjac = n;                  /* leading dimension of Jacobian */
  ftol   = sqrt(smaR1MACH(4));  /* tolerance parameters */
  xtol   = sqrt(smaR1MACH(4));
  gtol   = 0.0;
  maxfev = maxit;              /* maximum number of iterations */
  epsfcn = 0.0;                /* adjusts step size for Jacobian calculation */
  mode   = 1;                  /* internal scaling flag, 1 => no scaling */
  factor = 100.0;              /* determines initial step bound */
  nprint = iprt;               /* printing flag, 0 => no printing */

/* initial guess of the fit parameters */
  x[0] = shHgMean( hg );  
  x[1] = shHgSigma( hg );
  x[2] = hg->wsum * ( hg->maximum - hg->minimum ) / hg->nbin;

/* create the gaussian FUNC that will hold the answer */
  fitFunc        = smFuncNew("gauss", funcValue, 1);
  fitFunc->cov   = shVectorNew( 3*3 );
  fitFunc->sigma = shVectorNew( 3 );
  shAssert( fitFunc != NULL );

/*
 * Get the actual data and let global pointers access them
 * by calling smaGlobalSet.  This is needed because the callback 
 * function does not have the data passed to it * directly.  Also, 
 * smaCovFromFit uses these globals to compute * the errors.
 */

  for ( ct = 0; ct < m; ct++ ) {
    xValues->vec[ct] = hg->binPosition[ct];
    yValues->vec[ct] = hg->contents[ct];  
    wValues->vec[ct] = 1;                      /* weights not used  */
  }

  smaGlobalSet( xValues, yValues, wValues, fitFunc, NULL);

/* create a chi2 METH */
  funcMeth = smMethNew( "chi2" );
  shAssert( funcMeth != NULL );

/* actually call the SLATEC routine  */
    smaDNLS1( funcMeth->ma, iopt, m, n, x, fvec, fjac, ldfjac, ftol, xtol, 
           gtol, maxfev, epsfcn, diag, mode, factor, nprint, &info,
           &nfev, &njev, ipvt, qtf, wa1, wa2, wa3, wa4);

/* put the answers into the FUNC */
  funcValue[0]->vec[0] = x[0];     /* peak position */
  funcValue[0]->vec[1] = x[1];     /* sigma */
  funcValue[0]->vec[2] = x[2];     /* normalization */

  fitFunc->cov = NULL;
  fitFunc->sigma->vec[0] = 0.0;
  fitFunc->sigma->vec[1] = 0.0;
  fitFunc->sigma->vec[2] = 0.0;
  fitFunc->chi2 = 0.0;

/* fill in the 'cov' and 'sigma' arrays in the FUNC */
  smaCovFromFit( funcMeth, fitFunc, 0.0);

/* clean up and go home */

  shFree(x);
  shFree(fvec);
  shFree(diag);
  shFree(qtf); 
  shFree(wa1);
  shFree(wa2);
  shFree(wa3);
  shFree(wa4);
  shFree(fjac);
  smMethDel( funcMeth );

  return fitFunc;
}

/**************************************************************/

/*<AUTO EXTRACT>

  ROUTINE: atHgPolyFit

  DESCvRIPTION:
<HTML>
	Fits a Polynomial to an HG histogram and returns a FUNC.
        Calls the SLATEC routines smaDPOLFT and smaDPCOEF.
</HTML>

  RETURN VALUES:
<HTML>
	Returns a FUNC of type dpol and fills in tcVec with the coefficients
        from a Taylor Series expansion of the best fit polynomial.
</HTML>
</AUTO>*/
FUNC *atHgPolyFit(HG *hg, int order, VECTOR *tcVec) {

  FUNC* fitFunc = NULL;

  int ct, n, maxdeg;
  int ndeg, ierr;
  int l, c;
  double *x, *y, *w, eps;
/*  double tc;  unused */
  VECTOR *a, *r;
  VECTOR **funcValue;

/* check the inputs */
  shAssert( hg != NULL );
  shAssert( hg->nbin > 0 );
  shAssert( order > 0 );
  shAssert( tcVec != NULL );
  shAssert( tcVec->dimen > 0 );

/* for smaDPOLFT */
  n      = hg->nbin;      /* number of data points */
  maxdeg = order;         /* if eps = 0 this is the maximum degree of the fit */
  eps    = 0.0;           /* flag for how max degree of fit determined */
/* for smaDPCOEF */
  l      = order;         /* max degree of polynomial for coefficients to be found */
  c      = 0;             /* determine coefficients around (x-c) */

  x = shMalloc( n * sizeof(double) );
  y = shMalloc( n * sizeof(double) );
  w = shMalloc( n * sizeof(double) );
  funcValue = shMalloc( 2 * sizeof(VECTOR *));

  a     = shVectorNew( 3 * n + 3 * maxdeg + 3);
  r     = shVectorNew( n );
  funcValue[1] = shVectorNew(1);
 
/* get the actual data to be fit */

  for ( ct = 0; ct < n; ct++ ) {
    x[ct] = hg->binPosition[ct];
    y[ct] = hg->contents[ct];
    w[ct] = 1.0;
  } 

  smaDPOLFT( n, x, y, w, maxdeg, &ndeg, &eps, r->vec, &ierr, a->vec);

  funcValue[0] = a;
  funcValue[1]->vec[0] = ndeg;

  fitFunc = smFuncNew( "dpol", funcValue, 2);
  shAssert( fitFunc != NULL );

/* place the Taylor series coeficients into tcVec and return the FUNC */
  smaDPCOEF( l, c, tcVec->vec, a->vec);

/* clean up and go home */
  shFree( x );
  shFree( y );
  shFree( w );
  shVectorDel( a );
  shVectorDel( r );

  return fitFunc;
}


/**************************************************************/

/* proco.c */

#define MAXPRO      10                               /* number of projections */
#define TWOPI       M_PI * 2.0                     /* twice number PI */
#define PIHALF      M_PI * 0.5                       /* number PI divided by 2 */
#define degrad(x) (at_rad2Deg*(x))          /* radians -> degrees */
#define raddeg(x) ( at_deg2Rad*(x))          /* degrees -> radians */

static struct {
   double a0;                                  /* longitude projection centre */
   double sina0;                       /* sine of longitude projection centre */
   double cosa0;                     /* cosine of longitude projection centre */
   double d0;                                   /* latitude projection centre */
   double sind0;                        /* sine of latitude projection centre */
   double cosd0;                      /* cosine of latitude projection centre */
   double rho;                                              /* rotation angle */
   double sinrho;                                   /* sine of rotation angle */
   double cosrho;                                 /* cosine of rotation angle */
} pro[MAXPRO] = {
   { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0 },
   { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0 },
   { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0 },
   { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0 },
   { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0 },
   { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0 },
   { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0 },
   { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0 },
   { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0 },
   { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0 }
};

static struct {                     /* buffer for aitoff projection constants */
   double a0;                       /* longitude projection centre in degrees */
   double d0;                        /* latitude projection centre in degrees */
   double dx;                              /* grid separation in x in degrees */
   double dy;                              /* grid separation in y in degrees */
   double fa;                                  /* scaling factor in longitude */
   double fd;                                   /* scaling factor in latitude */
   double m0;                                                     /* constant */
   double rho;
} ait = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

static struct {                   /* buffer for mercator projection constants */
   double a0;                       /* longitude projection centre in degrees */
   double d0;                        /* latitude projection centre in degrees */
   double dx;                              /* grid separation in x in degrees */
   double dy;                              /* grid separation in y in degrees */
   double fa;                                  /* scaling factor in longitude */
   double fd;                                   /* scaling factor in latitude */
   double m0;                                                     /* constant */
   double rho;
} mer = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

/*

        Copyright (c) Kapteyn Laboratorium Groningen 1990
        All Rights Reserved.

#>            proco.dc2
*/

/**************************************************************/

/*<AUTO EXTRACT>

ROUTINE:       atProco

DESCRIPTION:   Function which does the conversion of grid to sky
               coordinates and vice versa.

Author:       K.G. Begeman; copied and installed in ASTROTOOLS by Chris S.

RETURN VALUES:
                       0: successful conversion
                       1: unknown projection
                       2: unknown mode
                       3: CROTA2 = 90.0 for mode 1 and 2
                       4: CDELT1 or CDELT2 equal to zero


              xin      Input X coordinate (Longitude in degrees or
                       X grids).
              yin      Input Y coordinate (Latitude in degrees or
                       Y grids).
              xou      Output X coordinate (X grids or Longitude
                       in degrees).
              you      Output Y coordinates (Y grids or Latitude
                       in degrees).
              a0       Projection centre longitude (Longitude in
                       degrees).
              d0       Projection centre latitude (Latitude in
                       degrees).
              dx       X grid separation in degrees.
              dy       Y grid separation in degrees.
              rho      Rotation angle in degrees.
              sys       Type of projection:
                        1    AITOFF equal area projection
                        2    Equivalent Cylindrical projection
                        3    Flat projection
                        4    Gnomonic projection
                        5    Orthographic projection
                        6    Rectangular projection
                        7    Global Sinusoidal projection
                        8    North Celestial Pole projection
                        9    Stereographic projection
                       10    Mercator projection
              mode     Mode determines what type of input/output
                       coordinates are given/wanted.
                       MODE       XIN     YIN   XOUT   YOUT
                         0        LON     LAT      X      Y
                         1          X     LAT    LON      Y
                         2        LON       Y      X    LAT
                         3          X       Y    LON    LAT

Updates:      Jan  5, 1990: KGB, Document created.
              Apr 19, 1996: Chris S. installed in Astrotools
</auto>*/


int atProco(  double *xin  ,	/* input */
              double *yin  ,	/* input */
              double *xou  ,	/* output */
              double *you  ,	/* output */
              double *a0   ,	/* input */
              double *d0   ,	/* input */
              double *dx   ,	/* input */
              double *dy   ,	/* input */
              double *rho  ,	/* input */
              int   *sys  ,	/* input */
              int   *mode	/* input */
	    )
     
{
   int p = (*sys - 1);
   int r = 0;

   *xou = *xin; *you = *yin;                                         /* reset */
   if ((p < MAXPRO) && (p > -1)) {                 /* legal projection system */
      if (*a0 != pro[p].a0) {                  /* put in buffer for later use */
         pro[p].a0 = *a0;                   /* longitude of projection centre */
         pro[p].sina0 = sin( raddeg( *a0 ) );            /* sine of longitude */
         pro[p].cosa0 = cos( raddeg( *a0 ) );          /* cosine of longitude */
      }
      if (*d0 != pro[p].d0) {                  /* put in buffer for later use */
         pro[p].d0 = *d0;                    /* latitude of projection centre */
         pro[p].sind0 = sin( raddeg( *d0 ) );             /* sine of latitude */
         pro[p].cosd0 = cos( raddeg( *d0 ) );           /* cosine of latitude */
      }
      if (*rho != pro[p].rho) {                /* put in buffer for later use */
         pro[p].rho = *rho;                                 /* rotation angle */
         pro[p].sinrho = sin( raddeg( *rho ) );     /* sine of rotation angle */
         pro[p].cosrho = cos( raddeg( *rho ) );   /* cosine of rotation angle */
      }
   }
   switch(*sys) {                                 /* select projection system */
      case 1: {                               /* Aitoff equal area projection */
         if ((*dx != ait.dx) || (*dy != ait.dy) || (*a0 != ait.a0) || (*d0 != ait.d0) || (*rho != ait.rho)) {
            double t1, t2, t3, dda, ddd;

            ait.dx = *dx;
            ait.dy = *dy;
            ait.a0 = *a0;
            ait.d0 = *d0;
            ait.rho = *rho;
            dda = raddeg( *dx * pro[p].cosrho - *dy * pro[p].sinrho );
            ddd = raddeg( *dy * pro[p].cosrho + *dy * pro[p].sinrho );
            t1 = sqrt( ( 1.0 + pro[p].cosd0 * cos( dda / 2.0 ) ) / 2.0 );
            t2 = 2.0 * pro[p].cosd0 * sin( dda / 2.0 );
            ait.fa = dda * t1 / t2;
            t1 = sin( pro[p].d0 + ddd );
            t2 = sqrt( ( 1.0 + cos( pro[p].d0 + ddd ) ) / 2.0 );
            t3 = sin( pro[p].d0 ) / sqrt( ( 1.0 + cos( pro[p].d0 ) ) / 2.0 );
            ait.fd = ddd / ( t1 / t2 - t3 );
            ait.m0 = ait.fd * pro[p].sind0 / sqrt( ( 1.0 + pro[p].cosd0 ) / 2.0 );
         }
         switch(*mode) {                                 /* which way to go ? */
            case 0: {                                       /* (a,d) -> (x,y) */
               double d, l, m, z, da, cosd, sind, cosa, sina;

               da = raddeg( ( *xin - pro[p].a0 ) / 2.0 );
               cosa = cos( da );
               sina = sin( da );
               d = raddeg( *yin );
               cosd = cos( d );
               sind = sin( d );
               z = sqrt( ( 1.0 + cosd * cosa ) / 2.0 );
               l = 2.0 * ait.fa * cosd * sina / z;
               m = ait.fd * sind / z - ait.m0;
               *xou = degrad( l * pro[p].cosrho + m * pro[p].sinrho ) / *dx;
               *you = degrad( m * pro[p].cosrho - l * pro[p].sinrho ) / *dy;
               break;
            }
            case 1: {                                       /* (x,d) -> (a,y) */
               double a, b, c, x, d, z, da, daold, cosd, sind;
	       daold = 0.0; /* Chris S.  19-Apr-96 */
               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               x = raddeg( *xin * *dx );
               d = raddeg( *yin );
               cosd = cos( d );
               sind = sin( d );
               da = 0.0;
               do {
                  daold = da;
                  z = sqrt( ( 1.0 + cosd * cos( da ) ) / 2.0 );
                  a = x * z;
                  b = pro[p].sinrho * ( ait.m0 * z - ait.fd * sind );
                  c = 2.0 * ait.fa * cosd * pro[p].cosrho;
                  da = asin( ( a + b ) / c );
               } while (fabs( da - daold ) > 0.0000000001);
               a = ait.fd * sind / z;
               b = x * pro[p].sinrho;
               *xou = pro[p].a0 + degrad( 2.0 * da );
               *you = degrad( ( a - b - ait.m0 ) / pro[p].cosrho ) / *dy;
               break;
            }
            case 2: {                                       /* (a,y) -> (x,d) */
               double d, q, r, s, t, x, y, z, da, dold, cosd, cosa, sina;
	       dold = 0.0; /* Chris S.  19-Apr-96 */

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               da = raddeg( ( *xin - pro[p].a0 ) / 2.0 );
               cosa = cos( da );
               sina = sin( da );
               y = raddeg( *yin * *dy );
               d = raddeg( pro[p].d0 );
               do {
                  dold = d;
                  cosd = cos( d );
                  z = sqrt( ( 1.0 + cosd * cosa ) / 2.0 );
                  q = y * z;
                  r = ait.m0 * z * pro[p].cosrho;
                  s = 2 * ait.fa * cosd * pro[p].sinrho * sina;
                  t = ait.fd * pro[p].cosrho;
                  d = asin( ( q + r + s ) / t );
               } while (fabs( d - dold ) > 0.0000000001);
               x = ( 2.0 * ait.fa * sina * cosd / z + y * pro[p].sinrho );
               *xou = degrad( x / pro[p].cosrho ) / *dx;
               *you = degrad( d );
               break;
            }
            case 3: {                                       /* (x,y) -> (a,d) */
               double d, l, m, s, t, x, y, z, da;

               x = raddeg( *xin * *dx );
               y = raddeg( *yin * *dy );
               l = x * pro[p].cosrho - y * pro[p].sinrho;
               m = y * pro[p].cosrho + x * pro[p].sinrho;
               s = l / ait.fa / 2.0;
               t = ( m + ait.m0 ) / ait.fd;
               z = 0.5 * sqrt( 4.0 - s * s - t * t );
               d = asin( t * z );
               da = 2.0 * asin( s * z / cos( d ) );
               *xou = pro[p].a0 + degrad( da );
               *you = degrad( d );
               break;
            }
            default: {                                        /* unknown mode */
               r = 2;
               break;
            }
         }
         break;
      }
      case 2: {                                     /* Cylindrical projection */
         switch(*mode) {                                 /* which way to go ? */
            case 0: {                                       /* (a,d) -> (x,y) */
               double l, m;

               l = *xin - pro[p].a0;
               m = degrad( sin( raddeg( *yin - pro[p].d0 ) ) );
               *xou = ( l * pro[p].cosrho + m * pro[p].sinrho ) / *dx;
               *you = ( m * pro[p].cosrho - l * pro[p].sinrho ) / *dy;
               break;
            }
            case 1: {                                       /* (x,d) -> (a,y) */
               double l, sind;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               sind = degrad( sin( raddeg( *yin - pro[p].d0 ) ) );
               l = ( *xin * *dx - sind * pro[p].sinrho ) / pro[p].cosrho;
               *xou = pro[p].a0 + l;
               *you = ( sind * pro[p].cosrho - l * pro[p].sinrho ) / *dy;
               break;
            }
            case 2: {                                       /* (a,y) -> (x,d) */
               double da, m;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               da = (*xin - pro[p].a0);
               m = ( *yin * *dy + da * pro[p].sinrho ) / pro[p].cosrho;
               *xou = ( da * pro[p].cosrho + m * pro[p].sinrho ) / *dx;
               *you = pro[p].d0 + degrad( asin( raddeg( m ) ) );
               break;
            }
            case 3: {                                       /* (x,y) -> (a,d) */
               double l, m, x, y;

               x = *xin * *dx;
               y = *yin * *dy;
               l = x * pro[p].cosrho - y * pro[p].sinrho;
               m = y * pro[p].cosrho + x * pro[p].sinrho;
               *xou = pro[p].a0 + l;
               *you = pro[p].d0 + degrad( asin( raddeg( m ) ) );
               break;
            }
            default: {                                        /* unknown mode */
               r = 2;
               break;
            }
         }
         break;
      }
      case 3: {                                            /* Flat projection */
         switch(*mode) {                                 /* which way to go ? */
            case 0: {                                       /* (a,d) -> (x,y) */
               double l, m;

               l = *xin - pro[p].a0;
               m = *yin - pro[p].d0;
               *xou = ( l * pro[p].cosrho + m * pro[p].sinrho ) / *dx;
               *you = ( m * pro[p].cosrho - l * pro[p].sinrho ) / *dy;
               break;
            }
            case 1: {                                       /* (x,d) -> (a,y) */
               double dd, l;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               dd = *yin - pro[p].d0;
               l = ( *xin * *dx - dd * pro[p].sinrho ) / pro[p].cosrho;
               *xou = pro[p].a0 + l;
               *you = ( dd * pro[p].cosrho - l * pro[p].sinrho ) / *dy;
               break;
            }
            case 2: {                                       /* (a,y) -> (x,d) */
               double da, m;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               da = *xin - pro[p].a0;
               m = ( *yin * *dy + da * pro[p].sinrho ) / pro[p].cosrho;
               *xou = ( da * pro[p].cosrho + m * pro[p].sinrho ) / *dx;
               *you = pro[p].d0 + m;
               break;
            }
            case 3: {                                       /* (x,y) -> (a,d) */
               double l, m, x, y;

               x = *xin * *dx;
               y = *yin * *dy;
               l = x * pro[p].cosrho - y * pro[p].sinrho;
               m = y * pro[p].cosrho + x * pro[p].sinrho;
               *xou = pro[p].a0 + l;
               *you = pro[p].d0 + m;
               break;
            }
            default: {                                        /* unknown mode */
               r = 2;
               break;
            }
         }
         break;
      }
      case 4: {                                        /* Gnomonic projection */
         switch(*mode) {                                 /* which way to go ? */
            case 0: {                                       /* (a,d) -> (x,y) */
               double l, m, da, cosa, sina, cosd, sind, t;

               da = raddeg( *xin - pro[p].a0 );
               cosa = cos( da );
               sina = sin( da );
               cosd = cos( raddeg( *yin ) );
               sind = sin( raddeg( *yin ) );
               
               t = ( sind * pro[p].sind0 + cosd * pro[p].cosd0 * cosa );
               l = cosd * sina / t;               
               m = ( sind * pro[p].cosd0 - cosd * pro[p].sind0 * cosa ) / t;
               *xou = degrad( l * pro[p].cosrho + m * pro[p].sinrho ) / *dx;
               *you = degrad( m * pro[p].cosrho - l * pro[p].sinrho ) / *dy;
               break;
            }
            case 1: {                                       /* (x,d) -> (a,y) */
               double a, b, c, d, s, t, x, da, cosa, sina, cosd, sind, tand;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               x = raddeg( *xin * *dx);
               a = pro[p].cosrho;
               b = x * pro[p].cosd0 + pro[p].sind0 * pro[p].sinrho;
               c = x * pro[p].sind0 - pro[p].cosd0 * pro[p].sinrho;
               d = raddeg( *yin );
               cosd = cos( d );
               sind = sin( d );
               tand = sind / cosd;
               da = atan( b / a ) + asin( tand * c / sqrt( a * a + b * b ) );
               cosa = cos( da );
               sina = sin( da );
               a = sind * pro[p].cosd0 * pro[p].cosrho;
               b = cosd * pro[p].sind0 * cosa * pro[p].cosrho;
               c = cosd * sina * pro[p].sinrho;
               s = sind * pro[p].sind0;
               t = cosd * pro[p].cosd0 * cosa;
               *xou = pro[p].a0 + degrad( da );
               *you = degrad( ( a - b - c ) / ( s + t ) ) / *dy;
               break;
            }
            case 2: {                                       /* (a,y) -> (x,d) */
               double a, b, c, s, t, y, da, cosa, sina, tand;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               da = raddeg( *xin - pro[p].a0 );
               cosa = cos( da );
               sina = sin( da );
               y = raddeg( *yin * *dy);
               a = pro[p].sind0 * cosa * pro[p].cosrho;
               b = sina * pro[p].sinrho;
               c = y * pro[p].cosd0 * cosa;
               s = pro[p].cosd0 * pro[p].cosrho;
               t = y * pro[p].sind0;
               tand = ( a + b + c ) / ( s - t );
               a = sina * pro[p].cosrho;
               b = tand * pro[p].cosd0 * pro[p].sinrho;
               c = pro[p].sind0 * cosa * pro[p].sinrho;
               s = tand * pro[p].sind0;
               t = pro[p].cosd0 * cosa;
               *xou = degrad( ( a + b - c ) / ( s + t ) ) / *dx;
               *you = degrad( atan( tand ) );
               break;
            }
            case 3: {                                       /* (x,y) -> (a,d) */
               double l, m, s, t, x, y, da;

               x = *xin * *dx;
               y = *yin * *dy;
               l = raddeg( x * pro[p].cosrho - y * pro[p].sinrho );
               m = raddeg( y * pro[p].cosrho + x * pro[p].sinrho );
               s = ( m * pro[p].cosd0 + pro[p].sind0 );
               t = ( pro[p].cosd0 - m * pro[p].sind0 );
               da = atan( l / t );
               *xou = pro[p].a0 + degrad( da );
               *you = degrad( atan( cos( da ) * s / t ) );
               if (((*you) * pro[p].d0 ) < 0.0) {
                  if ((*xou)>180.0) (*xou) -= 180.0; else (*xou) += 180.0;
                  (*you) *= -1.0;
               }
             
               break;
            }
            default: {                                        /* unknown mode */
               r = 2;
               break;
            }
         }
         break;
      }
      case 5: {                                    /* Orthographic projection */
         switch(*mode) {                                 /* which way to go ? */
            case 0: {                                       /* (a,d) -> (x,y) */
               double l, m, d, da, cosa, sina, cosd, sind;

               da = raddeg( *xin - pro[p].a0 );
               cosa = cos( da );
               sina = sin( da );
               d = raddeg( *yin );
               cosd = cos( d );
               sind = sin( d );
               l = cosd * sina;
               m = sind * pro[p].cosd0 - cosd * pro[p].sind0 * cosa;
               *xou = degrad( l * pro[p].cosrho + m * pro[p].sinrho ) / *dx;
               *you = degrad( m * pro[p].cosrho - l * pro[p].sinrho ) / *dy;
               break;
            }
            case 1: {                                       /* (x,d) -> (a,y) */
               double a, b, c, d, s, t, x, da, cosd, sind, cosa, sina;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               a = pro[p].cosrho;
               b = pro[p].sind0 * pro[p].sinrho;
               d = raddeg( *yin );
               x = raddeg( *xin * *dx);
               cosd = cos( d );
               sind = sin( d );
               s = x - sind * pro[p].cosd0 * pro[p].sinrho;
               t = cosd * sqrt( a * a + b * b );
               da = atan( b / a ) + asin( s / t );
               cosa = cos( da );
               sina = sin( da );
               a = sind * pro[p].cosd0 * pro[p].cosrho;
               b = cosd * pro[p].sind0 * cosa * pro[p].cosrho;
               c = cosd * sina * pro[p].sinrho;
               *xou = pro[p].a0 + degrad( da );
               *you = degrad( a - b - c ) / *dy;
               break;
            }
            case 2: {                                       /* (a,y) -> (x,d) */
               double a, b, c, d, y, da, cosa, sina, cosd, sind;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               da = raddeg( *xin - pro[p].a0 );
               cosa = cos( da );
               sina = sin( da );
               y = raddeg( *yin * *dy );
               a = pro[p].cosd0 * pro[p].cosrho;
               b = pro[p].sind0 * cosa * pro[p].cosrho + sina * pro[p].sinrho;
               d = atan( b / a ) + asin( y / sqrt( a * a + b * b ) );
               cosd = cos( d );
               sind = sin( d );
               a = cosd * sina * pro[p].cosrho;
               b = sind * pro[p].cosd0 * pro[p].sinrho;
               c = cosd * pro[p].sind0 * cosa * pro[p].sinrho;
               *xou = degrad( a + b - c ) / *dx;
               *you = degrad( d );
               break;
            }
            case 3: {                                       /* (x,y) -> (a,d) */
               double l, m, t, x, y, da;

               x = *xin * *dx;
               y = *yin * *dy;
               l = raddeg( x * pro[p].cosrho - y * pro[p].sinrho );
               m = raddeg( y * pro[p].cosrho + x * pro[p].sinrho );
               t = sqrt( 1 - l * l - m * m );
               da = atan( l / ( pro[p].cosd0 * t - m * pro[p].sind0 ) );
               *xou = pro[p].a0 + degrad( da );
               *you = degrad( asin( m * pro[p].cosd0 + pro[p].sind0 * t ) );
               break;
            }
            default: {                                        /* unknown mode */
               r = 2;
               break;
            }
         }
         break;
      }
      case 6: {                                     /* Rectangular projection */
         switch(*mode) {                                 /* which way to go ? */
            case 0: {                                       /* (a,d) -> (x,y) */
               double a, d, l, m, s, t, da, cosa, sina, cosd, sind;

               da = raddeg( *xin - pro[p].a0 );
               cosa = cos( da );
               sina = sin( da );
               d = raddeg( *yin );
               cosd = cos( d );
               sind = sin( d );
               s = sind * pro[p].sind0 + cosd * pro[p].cosd0 * cosa;
               if (s > 1.0) s = 1.0; else if (s < -1.0) s = -1.0;
               t = acos( s );
               if (t == 0.0) a = 1.0; else a = t / sin( t );
               l = a * cosd * sina;
               m = a * ( sind * pro[p].cosd0 - cosd * pro[p].sind0 * cosa );
               *xou = degrad( l * pro[p].cosrho + m * pro[p].sinrho ) / *dx;
               *you = degrad( m * pro[p].cosrho - l * pro[p].sinrho ) / *dy;
               break;
            }
            case 1: {                                       /* (x,d) -> (a,y) */
               double a, b, d, f, x, t, y, da, yold, cosd, sind;
	       yold = 0.0; /* Chris S.  19-Apr-96 */

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               x = raddeg( *xin * *dx );
               d = raddeg( *yin );
               cosd = cos( d );
               sind = sin( d );
               a = x * pro[p].sinrho * pro[p].cosd0;
               b = pro[p].cosrho * pro[p].cosd0;
               y = 0.0;
               do {
                  yold = y;
                  t = sqrt( x * x + y * y );
                  if (t == 0.0) f = 1.0; else f = sin( t ) / t;
                  y = ( sind - pro[p].sind0 * cos( t ) - a * f ) / b / f;
               } while (fabs( y - yold ) > 0.0000000001 );
               t = sqrt( x * x + y * y );
               if (t == 0.0) f = 1.0; else f = sin( t ) / t;
               da = asin( f * ( x * pro[p].cosrho - y * pro[p].sinrho ) / cosd );
               *xou = pro[p].a0 + degrad( da );
               *you = degrad( y ) / *dy;
               break;
            }
            case 2: {                                       /* (a,y) -> (x,d) */
               double a, b, c, d, s, t, x, y, da, xold, cosa, sina;
	       xold = 0.0; /* Chris S.  19-Apr-96 */

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               da = raddeg( *xin - pro[p].a0 );
               cosa = cos( da );
               sina = sin( da );
               y = raddeg( *yin * *dy );
               a = atan( pro[p].sind0 / pro[p].cosd0 / cosa );
               b = sqrt( 1.0 - pro[p].cosd0 * pro[p].cosd0 * sina * sina );
               x = 0.0;
               do {
                  xold = x;
                  t = sqrt( x * x + y * y );
                  s = cos( t ) / b;
                  if (s > 1.0) s = 1.0; else if (s < -1.0) s = 1.0;
                  c = acos( s );
                  if (y < 0.0) d = a - c; else d = a + c;
                  if (t == 0.0) {
                     x = ( y * pro[p].sinrho + cos( d ) * sina ) / pro[p].cosrho;                     
                  } else {
                     x = ( y * pro[p].sinrho + cos( d ) * sina * t / sin( t ) ) / pro[p].cosrho;
                  }
               } while (fabs( x - xold ) > 0.0000000001);
               *xou = degrad( x ) / *dx;
               *you = degrad( d );
               break;
            }
            case 3: {                                       /* (x,y) -> (a,d) */
               double a, d, l, m, t, x, y;

               x = *xin * *dx;
               y = *yin * *dy;
               l = raddeg( x * pro[p].cosrho - y * pro[p].sinrho );
               m = raddeg( y * pro[p].cosrho + x * pro[p].sinrho );
               t = sqrt( l * l + m * m );
               if (t == 0.0) a = 1.0; else a = sin( t ) / t;
               d = asin( a * m * pro[p].cosd0 + pro[p].sind0 * cos( t ) );
               *xou = pro[p].a0 + degrad( asin( a * l / cos( d ) ) );
               *you = degrad( d );
               break;
            }
            default: {                                        /* unknown mode */
               r = 2;
               break;
            }
         }
         break;
      }
      case 7: {                               /* Global sinusoidal projection */
         switch(*mode) {                                 /* which way to go ? */
            case 0: {                                       /* (a,d) -> (x,y) */
               double l, m;

               l = ( *xin - pro[p].a0 ) * cos( raddeg( * yin ) );
               m = ( *yin - pro[p].d0 );
               *xou = ( l * pro[p].cosrho + m * pro[p].sinrho ) / *dx;
               *you = ( m * pro[p].cosrho - l * pro[p].sinrho ) / *dy;
               break;
            }
            case 1: {                                       /* (x,d) -> (a,y) */
               double x, y, da, cosd;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               x = *xin * *dx;
               y = ( *yin - pro[p].d0 - x * pro[p].sinrho ) / pro[p].cosrho;
               cosd = cos( raddeg( *yin ) );
               da = ( x * pro[p].cosrho - y * pro[p].sinrho ) / cosd;
               *xou = pro[p].a0 + da;
               *you = y / *dy;
               break;
            }
            case 2: {                                       /* (a,y) -> (x,d) */
               double d, dold, t, x, y, da, d0;
	       dold = 0.0; /* Chris S.  19-Apr-96 */

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               d0 = raddeg( pro[p].d0 );
               da = raddeg( *xin - pro[p].a0 );
               y = raddeg( *yin * *dy );
               t = da * pro[p].sinrho;
               d = d0 + y / pro[p].cosrho;
               do {
                  dold = d;
                  d = d0 + ( y + t * cos( dold ) ) / pro[p].cosrho;
               } while (fabs( d - dold ) > 0.0000000001 );
               x = da * cos( d ) * pro[p].cosrho + (d - d0 ) * pro[p].sinrho;
               *xou = degrad( x ) / *dx;
               *you = degrad( d );
               break;
            }
            case 3: {                                       /* (x,y) -> (a,d) */
               double d, l, m, x, y, cosd;

               x = *xin * *dx;
               y = *yin * *dy;
               l = x * pro[p].cosrho - y * pro[p].sinrho;
               m = y * pro[p].cosrho + x * pro[p].sinrho;
               d = pro[p].d0 + m;
               cosd = cos( raddeg( d ) );
               *xou = pro[p].a0 + l / cosd;
               *you = d;
               break;
            }
            default: {                                        /* unknown mode */
               r = 2;
               break;
            }
         }
         break;
      }
      case 8: {                            /* North Celestial Pole projection */
         switch(*mode) {                                 /* which way to go ? */
            case 0: {                                       /* (a,d) -> (x,y) */
               double l, m, da, cosd;

               da = raddeg( *xin - pro[p].a0 );
               cosd = cos( raddeg( *yin ) );
               l = cosd * sin( da );
               m = ( pro[p].cosd0 - cosd * cos( da ) ) / pro[p].sind0;
               *xou = degrad( l * pro[p].cosrho + m * pro[p].sinrho ) / *dx;
               *you = degrad( m * pro[p].cosrho - l * pro[p].sinrho ) / *dy;
               break;
            }
            case 1: {                                       /* (x,d) -> (a,y) */
               double a, b, c, s, t, x, da, cosd;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               x = raddeg( *xin * *dx );
               cosd = cos( raddeg( *yin ) );
               a = pro[p].cosrho;
               b = pro[p].sinrho / pro[p].sind0;
               s = x * pro[p].sind0 - pro[p].cosd0 * pro[p].sinrho;
               t = cosd * pro[p].sind0 * sqrt( a * a + b * b );
               da = atan( b / a ) + asin( s / t );
               a = pro[p].cosrho * pro[p].cosd0 / pro[p].sind0;
               b = pro[p].cosrho * cosd * cos( da ) / pro[p].sind0;
               c = pro[p].sinrho * cosd * sin( da );
               *xou = pro[p].a0 + degrad( da );
               *you = degrad( a - b - c ) / *dy;
               break;
            }
            case 2: {                                       /* (a,y) -> (x,d) */
               double a, b, c, d, q, s, t, y, da, cosa, sina, cosd;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               y = raddeg( *yin * *dy );
               da = raddeg( *xin - pro[p].a0 );
               cosa = cos( da );
               sina = sin( da );
               s = pro[p].cosd0 * pro[p].cosrho - y * pro[p].sind0;
               t = cosa * pro[p].cosrho + sina * pro[p].sinrho * pro[p].sind0;
               q = s / t;
               if (q > 1.0) q = 1.0; else if (q < -1.0) q = -1.0;
               d = acos( q );
               if (pro[p].d0 > 0.0) d = fabs( d ); else d = -fabs( d );
               cosd = cos( d );
               a = cosd * sina * pro[p].cosrho;
               b = pro[p].cosd0 * pro[p].sinrho / pro[p].sind0;
               c = cosd * cosa * pro[p].sinrho / pro[p].sind0;
               *xou = degrad( a + b - c ) / *dx;
               *you = degrad( d );
               break;
            }
            case 3: {                                       /* (x,y) -> (a,d) */
               double d, l, m, x, y, da, cosa;

               x = *xin * *dx;
               y = *yin * *dy;
               l = raddeg( x * pro[p].cosrho - y * pro[p].sinrho );
               m = raddeg( y * pro[p].cosrho + x * pro[p].sinrho );
               da = atan( l / ( pro[p].cosd0 - m * pro[p].sind0 ) );
               cosa = cos( da );
               d = acos( ( pro[p].cosd0 - m * pro[p].sind0 ) / cosa );
               if (pro[p].d0 > 0.0) d = fabs( d ); else d = - fabs( d );
               *xou = pro[p].a0 + degrad( da );
               *you = degrad( d );
               break;
            }
            default: {                                        /* unknown mode */
               r = 2;
               break;
            }
         }
         break;
      }
      case 9: {                                   /* Stereographic projection */
         switch(*mode) {                                 /* which way to go ? */
            case 0: {                                       /* (a,d) -> (x,y) */
               double l, m, d, t, da, cosa, sina, cosd, sind;

               da = raddeg( *xin - pro[p].a0 );
               cosa = cos( da );
               sina = sin( da );
               d = raddeg( *yin );
               cosd = cos( d );
               sind = sin( d );
               t = 1.0 + sind * pro[p].sind0 + cosd * pro[p].cosd0 * cosa;
               l = 2.0 * ( cosd * sina ) / t;
               m = 2.0 * ( sind * pro[p].cosd0 - cosd * pro[p].sind0 * cosa ) / t;
               *xou = degrad( l * pro[p].cosrho + m * pro[p].sinrho ) / *dx;
               *you = degrad( m * pro[p].cosrho - l * pro[p].sinrho ) / *dy;
               break;
            }
            case 1: {                                       /* (x,d) -> (a,y) */
               double a, b, c, d, s, t, x, da, cosa, sina, cosd, sind;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               x = raddeg( *xin * *dx );
               d = raddeg( *yin );
               cosd = cos( d );
               sind = sin( d );
               a = 2.0 * cosd * pro[p].cosrho;
               b = 2.0 * cosd * pro[p].sind0 * pro[p].sinrho + x * cosd * pro[p].cosd0;
               c = x + x * sind * pro[p].sind0 - 2.0 * sind * pro[p].cosd0 * pro[p].sinrho;
               da = atan( b / a ) + asin( c / sqrt( a * a + b * b ) );
               cosa = cos( da );
               sina = sin( da );
               a = sind * pro[p].cosd0 * pro[p].cosrho;
               b = cosd * pro[p].sind0 * cosa * pro[p].cosrho;
               c = cosd * sina * pro[p].sinrho;
               s = sind * pro[p].sind0;
               t = cosd * pro[p].cosd0 * cosa;
               *xou = pro[p].a0 + degrad( da );
               *you = degrad( 2.0 * ( a - b - c ) / ( 1.0 + s + t ) ) / *dy;
               break;
            }
            case 2: {                                       /* (a,y) -> (x,d) */
               double a, b, c, d, s, t, y, da, cosa, sina, cosd, sind;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               da = raddeg( *xin - pro[p].a0 );
               cosa = cos( da );
               sina = sin( da );
               y = raddeg( * yin * *dy );
               a = 2.0 * pro[p].cosd0 * pro[p].cosrho - y * pro[p].sind0;
               b = cosa * ( y * pro[p].cosd0 + 2.0 * pro[p].sind0 * pro[p].cosrho ) + 2.0 * sina * pro[p].sinrho;
               c = y;
               d = atan( b / a ) + asin( c / sqrt( a * a + b * b ) );
               cosd = cos( d );
               sind = sin( d );
               a = sind * pro[p].cosd0 * pro[p].sinrho;
               b = cosd * pro[p].sind0 * cosa * pro[p].sinrho;
               c = cosd * sina * pro[p].cosrho;
               s = sind * pro[p].sind0;
               t = cosd * pro[p].cosd0 * cosa;
               *xou = degrad( 2.0 * ( a - b + c ) / ( 1.0 + s + t ) ) / *dx;
               *you = degrad( d );
               break;
            }
            case 3: {                                       /* (x,y) -> (a,d) */
               double d, l, m, x, y, da, cosd, cost;

               x = *xin * *dx;
               y = *yin * *dy;
               l = raddeg( x * pro[p].cosrho - y * pro[p].sinrho );
               m = raddeg( y * pro[p].cosrho + x * pro[p].sinrho );
               cost = ( 4.0 - l * l - m * m ) / ( 4.0 + l * l + m * m );
               d = asin( cost * pro[p].sind0 + 0.5 * m * pro[p].cosd0 * ( 1.0 + cost ) );
               cosd = cos( d );
               da = asin( 0.5 * l * ( 1.0 + cost ) / cosd );
               *xou = pro[p].a0 + degrad( da );
               *you = degrad( d );
               break;
            }
            default: {                                        /* unknown mode */
               r = 2;
               break;
            }
         }
         break;
      }
      case 10: {                                       /* Mercator projection */
         if ((*dx != mer.dx) || (*dy != mer.dy) || (*a0 != mer.a0) || (*d0 != mer.d0) || (*rho != mer.rho)) {
            double d, t1, t2, ddd;

            mer.dx = *dx;
            mer.dy = *dy;
            mer.a0 = *a0;
            mer.d0 = *d0;
            mer.rho = *rho;
            ddd = raddeg( *dy * pro[p].cosrho + *dy * pro[p].sinrho );
            d = raddeg( pro[p].d0 );
            t1 = log( tan( ( d + ddd ) / 2.0 + M_PI / 4.0 ) );
            t2 = log( tan( d / 2.0 + M_PI / 4.0 ) );
            mer.fa = pro[p].cosd0;
            mer.fd = ddd / ( t1 - t2 );
            mer.m0 = mer.fd * t2;
         }
         switch(*mode) {                                 /* which way to go ? */
            case 0: {                                       /* (a,d) -> (x,y) */
               double d, l, m, da;

               da = raddeg( *xin - pro[p].a0 );
               d = raddeg( *yin );
               l = mer.fa * da;
               m = mer.fd * log( tan( d / 2.0 + M_PI / 4.0 ) ) - mer.m0;
               *xou = degrad( l * pro[p].cosrho + m * pro[p].sinrho ) / *dx;
               *you = degrad( m * pro[p].cosrho - l * pro[p].sinrho ) / *dy;
               break;
            }
            case 1: {                                       /* (x,d) -> (a,y) */
               double d, m, l, x;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               x = raddeg( *xin * *dx );
               d = raddeg( *yin );
               m = mer.fd * log( tan( d / 2.0 + M_PI / 4.0 ) ) - mer.m0;
               l = ( x - m * pro[p].sinrho ) / pro[p].cosrho;
               *xou = pro[p].a0 + degrad( l / mer.fa );
               *you = degrad( m * pro[p].cosrho - l * pro[p].sinrho ) / *dy;
               break;
            }
            case 2: {                                       /* (a,y) -> (x,d) */
               double l, m, y;

               if (pro[p].cosrho == 0.0) { r = 3; break; }            /* exit */
               y = raddeg( *yin * *dy );
               l = mer.fa * raddeg( *xin - pro[p].a0 );
               m = ( y + l * pro[p].sinrho ) / pro[p].cosrho;
               *xou = degrad( l * pro[p].cosrho + m * pro[p].sinrho ) / *dx;
               *you = degrad( 2 * atan( exp( ( m + mer.m0 ) / mer.fd ) ) - M_PI / 2.0 );
               break;
            }
            case 3: {                                       /* (x,y) -> (a,d) */
               double l, m, x, y;

               x = *xin * *dx;
               y = *yin * *dy;
               l = raddeg( x * pro[p].cosrho - y * pro[p].sinrho );
               m = raddeg( y * pro[p].cosrho + x * pro[p].sinrho );
               *xou = pro[p].a0 + degrad( l / mer.fa );
               *you = degrad( 2.0 * atan( exp( ( m + mer.m0 ) / mer.fd ) ) - M_PI / 2.0 );
               break;
            }
            default: {                                        /* unknown mode */
               r = 2;
               break;
            }
         }
         break;
      }
      default: {                                        /* Unknown projection */
         r = 1;
         break;
      }
   }
   return( r );
}

#if defined(TESTBED)
main()
{
   double rho = 45.0;
   double a0  = 45.0;
   double d0  = 45.0;
   double a;
   double d;
   double x;
   double y;
   double dx = -0.01;
   double dy = 0.02;
   int   mode;
   int   p;
   int   pro;
   int   r;


   for (p = 0; p < MAXPRO; p++) {
      double as, ds, xs, ys;
      pro = p + 1;
      printf( "Projection # %ld\n", pro );

      mode = 0; a = 44.5; d = 45.5;
      r = proco_c( &a, &d, &x, &y, &a0, &d0, &dx, &dy, &rho, &pro, &mode );
      as = a; ds = d; xs = x; ys = y;
      printf( "proco = %ld, a =%12.8f, d =%12.8f, x =%12.8f, y =%12.8f\n", r, a, d, x, y );

      mode = 1; x = xs; d = ds;
      r = proco_c( &x, &d, &a, &y, &a0, &d0, &dx, &dy, &rho, &pro, &mode );
      printf( "proco = %ld, a =%12.8f, d =%12.8f, x =%12.8f, y =%12.8f\n", r, a, d, x, y );

      mode = 2; a = as; y = ys;
      r = proco_c( &a, &y, &x, &d, &a0, &d0, &dx, &dy, &rho, &pro, &mode );
      printf( "proco = %ld, a =%12.8f, d =%12.8f, x =%12.8f, y =%12.8f\n", r, a, d, x, y );

      mode = 3; x = xs; y = ys;
      r = proco_c( &x, &y, &a, &d, &a0, &d0, &dx, &dy, &rho, &pro, &mode );
      printf( "proco = %ld, a =%12.8f, d =%12.8f, x =%12.8f, y =%12.8f\n", r, a, d, x, y );

   }
}
#endif

/*<AUTO EXTRACT>
  ROUTINE: atRegPlot

  DESCRIPTION:  Plot a region in a PGPLOT window defined in a PGSTATE

  RETURN VALUES: SH_SUCCESS

</AUTO>*/
RET_CODE atRegPlot(
		   PGSTATE *pg, /* the PGPSTATE to plot in */
		   REGION *reg,	/* the REGION to plot */
		   float fg,	/* minimum pixel value to show */
		   float bg,	/* maximum pixel value to show */
		   TRANS *trans, /* TRANS to get from row,col to display */
		   int mode,	/* 0=linear; 1=log; 2-sqrt */
		   char *xlab,	/* x label string */
		   char *ylab,	/* y label string */
		   int nobox    /* 0=plot box, 1=no plotting of box */
		   ) {
  int i, j, idim, jdim, i1, i2, j1, j2, icSave;
  float *a = NULL;
  float tr[6], xmin, xmax, ymin, ymax, xborder, yborder;
  float yminFinal, ymaxFinal;
  float v;
  TRANS *tc, *tb;
  double coeff[6], xe, ye;

  idim = reg->ncol;
  jdim = reg->nrow;

  a = (float *) shMalloc(idim*jdim*sizeof(float));
  for (i=0; i<idim; i++) {
    for (j=0; j<jdim; j++) {
      /* a[i + (j*jdim)] = reg->rows_fl32[j][i]; */
      v = shRegPixGetAsDbl(reg, j, i);
      switch (mode) {

	/* log scaling */
      case 1:
	v = maximum(v,fg);
	v = minimum(v,bg);
	v = log10(1.0+9.0*(v-fg)/(bg-fg));
	break;

	/* sqrt scaling */
      case 2: 
	v = maximum(v,fg);
	v = minimum(v,bg);
	v = sqrt(100*(v-fg)/(bg-fg));
	break;
      }
      a[i + (j*idim)] = v;
    }
  }

  switch (mode) {
    /* log scaling */
  case 1:
    fg = 0;
    bg = 1;
    break;
    /* sqrt scaling */
  case 2:
    fg = 0;
    bg = 10;
    break;
  }

/** this never referenced, so comment out to stop IRIX6 compiler warning
  scale = 0.5*(
    sqrt(pow(trans->b,2) + pow(trans->c,2)) + 
    sqrt(pow(trans->e,2) + pow(trans->f,2)));
**/
  tb = atTransNew();
  tb->a = -0.5;
  tb->d = -0.5;
  tc = atTransNew();
  atTransMultiply(trans, tb, tc);
  tr[0] = tc->a;
  tr[1] = tc->b;
  tr[2] = tc->c;
  tr[3] = tc->d;
  tr[4] = tc->e;
  tr[5] = tc->f;

  coeff[0] = trans->a;
  coeff[1] = trans->b;
  coeff[2] = trans->c;
  coeff[3] = trans->d;
  coeff[4] = trans->e;
  coeff[5] = trans->f;
  slaXy2xy(0.0, 0.0, coeff, &xe, &ye);
  xmin = xe; ymin = ye;
  xmax = xe; ymax = ye;
  slaXy2xy(0.0, jdim, coeff, &xe, &ye);
  xmin = minimum(xmin,xe); ymin = minimum(ymin,ye);
  xmax = maximum(xmax,xe); ymax = maximum(ymin,ye);
  slaXy2xy(idim, 0.0, coeff, &xe, &ye);
  xmin = minimum(xmin,xe); ymin = minimum(ymin,ye);
  xmax = maximum(xmax,xe); ymax = maximum(ymin,ye);
  slaXy2xy(idim, jdim, coeff, &xe, &ye);
  xmin = minimum(xmin,xe); ymin = minimum(ymin,ye);
  xmax = maximum(xmax,xe); ymax = maximum(ymin,ye);

  atTransDel(tb);
  atTransDel(tc);
  if (pg->isNewplot == 1) {
    shPgstateNextWindow(pg);

    xborder = 0.05*(xmax-xmin);
    yborder = 0.05*(ymax-ymin);

    if (xmin > 90) {
      /* in the south */
      yminFinal = ymin-yborder;
      ymaxFinal = ymax+yborder;
    } else {
      yminFinal = ymax+yborder;
      ymaxFinal = ymin-yborder;
    }
    if (pg->just == 0) {
      cpgswin(xmin-xborder, xmax+xborder, yminFinal, ymaxFinal);
    } else {
      cpgwnad(xmin-xborder, xmax+xborder, yminFinal, ymaxFinal);
    }
  }
  if (!nobox) {
  	cpgbox(pg->xopt, pg->xtick, pg->nxsub,
	 	pg->yopt, pg->ytick, pg->nysub);
  }

  i1 = 1;
  i2 = idim;
  j1 = 1;
  j2 = jdim;

  cpggray(a, idim, jdim, i1, i2, j1, j2, bg, fg, tr);

  /* put the text titles */
  if (pg->isNewplot == 1) {
    cpgqci(&icSave);
    cpgsci(pg->icLabel);
    cpglab(xlab,ylab,reg->name);
    cpgsci(icSave);
  }
  shFree(a);

  return SH_SUCCESS;
}

/*<AUTO EXTRACT>
  ROUTINE: atRegColorPlot

  DESCRIPTION:  Plot a region in a PGPLOT window defined in a PGSTATE

  RETURN VALUES: SH_SUCCESS

</AUTO>*/
RET_CODE atRegColorPlot(
			PGSTATE *pg,    /* the PGPSTATE to plot in */
			REGION *reg,	/* the REGION to plot */
			float fg,	/* minimum pixel value to show */
			float bg,	/* maximum pixel value to show */
			TRANS *trans,   /* TRANS: row,col to display */
			int smode,	/* 0=linear; 1=log; 2-sqrt */
			int cmode,      /* 1=gray; 2=rainbow; 3=heat; 
					   4=weird IRAF;  5=AIPS; 6=TJP;
					  -1=inverse gray */
			char *xlab,	/* x label string */
			char *ylab,	/* y label string */
		   	int nobox    /* 0=plot box, 1=no plotting of box */
			) {
  int i, j, idim, jdim, i1, i2, j1, j2, icSave;
  float *a = NULL;
  float tr[6], xmin, xmax, ymin, ymax, xborder, yborder;
  float yminFinal, ymaxFinal;
  float v;
  TRANS *tc, *tb;
  double coeff[6], xe, ye;

  idim = reg->ncol;
  jdim = reg->nrow;

  a = (float *) shMalloc(idim*jdim*sizeof(float));
  for (i=0; i<idim; i++) {
    for (j=0; j<jdim; j++) {
      /* a[i + (j*jdim)] = reg->rows_fl32[j][i]; */
      v = shRegPixGetAsDbl(reg, j, i);
      switch (smode) {

	/* log scaling */
      case 1:
	v = maximum(v,fg);
	v = minimum(v,bg);
	v = log10(1.0+9.0*(v-fg)/(bg-fg));
	break;

	/* sqrt scaling */
      case 2: 
	v = maximum(v,fg);
	v = minimum(v,bg);
	v = sqrt(100*(v-fg)/(bg-fg));
	break;
      }
      a[i + (j*idim)] = v;
    }
  }

  switch (smode) {
    /* log scaling */
  case 1:
    fg = 0;
    bg = 1;
    break;
    /* sqrt scaling */
  case 2:
    fg = 0;
    bg = 10;
    break;
  }

/** scale never references, so comment this out to prevent compiler warning
  scale = 0.5*(
    sqrt(pow(trans->b,2) + pow(trans->c,2)) + 
    sqrt(pow(trans->e,2) + pow(trans->f,2)));
**/
  tb = atTransNew();
  tb->a = -0.5;
  tb->d = -0.5;
  tc = atTransNew();
  atTransMultiply(trans, tb, tc);
  tr[0] = tc->a;
  tr[1] = tc->b;
  tr[2] = tc->c;
  tr[3] = tc->d;
  tr[4] = tc->e;
  tr[5] = tc->f;

  coeff[0] = trans->a;
  coeff[1] = trans->b;
  coeff[2] = trans->c;
  coeff[3] = trans->d;
  coeff[4] = trans->e;
  coeff[5] = trans->f;
  slaXy2xy(0.0, 0.0, coeff, &xe, &ye);
  xmin = xe; ymin = ye;
  xmax = xe; ymax = ye;
  slaXy2xy(0.0, jdim, coeff, &xe, &ye);
  xmin = minimum(xmin,xe); ymin = minimum(ymin,ye);
  xmax = maximum(xmax,xe); ymax = maximum(ymin,ye);
  slaXy2xy(idim, 0.0, coeff, &xe, &ye);
  xmin = minimum(xmin,xe); ymin = minimum(ymin,ye);
  xmax = maximum(xmax,xe); ymax = maximum(ymin,ye);
  slaXy2xy(idim, jdim, coeff, &xe, &ye);
  xmin = minimum(xmin,xe); ymin = minimum(ymin,ye);
  xmax = maximum(xmax,xe); ymax = maximum(ymin,ye);

  atTransDel(tb);
  atTransDel(tc);
  if (pg->isNewplot == 1) {
    shPgstateNextWindow(pg);

    xborder = 0.05*(xmax-xmin);
    yborder = 0.05*(ymax-ymin);

    if (xmin > 90) {
      /* in the south */
      yminFinal = ymin-yborder;
      ymaxFinal = ymax+yborder;
    } else {
      yminFinal = ymax+yborder;
      ymaxFinal = ymin-yborder;
    }
    if (pg->just == 0) {
      cpgswin(xmin-xborder, xmax+xborder, yminFinal, ymaxFinal);
    } else {
      cpgwnad(xmin-xborder, xmax+xborder, yminFinal, ymaxFinal);
    }
  }
  if (!nobox) {
  	cpgbox(pg->xopt, pg->xtick, pg->nxsub,
	 	pg->yopt, pg->ytick, pg->nysub);
  }

  i1 = 1;
  i2 = idim;
  j1 = 1;
  j2 = jdim;

  atPalett(cmode);
  cpgimag(a, idim, jdim, i1, i2, j1, j2, fg, bg, tr);

  /* put the text titles */
  if (pg->isNewplot == 1) {
    cpgqci(&icSave);
    cpgsci(pg->icLabel);
    cpglab(xlab,ylab,reg->name);
    cpgsci(icSave);
  }
  shFree(a);

  return SH_SUCCESS;
}

/**************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atHg2New

  DESCRIPTION:
<HTML>
	Make a new HG2
</HTML>

  RETURN VALUES:
<HTML>
	pointer to the HG2
</HTML>
</AUTO>*/
HG2 *atHg2New(
	      double xmin,	/* minimum x value */
	      double xmax,	/* maximum x value */
	      int nx,		/* number of bins in x */
	      double ymin,	/* minimum y value */
	      double ymax,	/* maximum y value */
	      double ny,	/* number of bins in y */
	      char *xlab,	/* label for x axis */
	      char *ylab,	/* label for y axis */
	      char *name	/* name of HG2 */
	      ) {
  HG2 *hg2 = (HG2 *) shMalloc(sizeof(HG2));
  hg2->reg = shRegNew("hg2Name", ny, nx, TYPE_FL32);
  shRegSetWithDbl(0.0, hg2->reg);
  if (xlab != NULL) strncpy(hg2->xlab, xlab, 99);
  if (ylab != NULL) strncpy(hg2->ylab, ylab, 99);
  if (name != NULL) strncpy(hg2->name, name, 99);
  hg2->delx = (xmax-xmin)/nx;
  hg2->dely = (ymax-ymin)/ny;

  hg2->xmin = xmin;
  hg2->xmax = xmax;
  hg2->nx = nx;
  hg2->ymin = ymin;
  hg2->ymax = ymax;
  hg2->ny = ny;

  return hg2;
}

/**************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atHg2Del

  DESCRIPTION:
<HTML>
Delete a HG2 structure
</HTML>

  RETURN VALUES:
<HTML>
SH_SUCCESS
</HTML>
</AUTO>*/
RET_CODE atHg2Del(
		  HG2 *hg2	/* the HG2 to delete */
		  ) {
  shRegDel(hg2->reg);
  shFree(hg2);
  return SH_SUCCESS;
}


/**************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atHg2Fill

  DESCRIPTION:
<HTML>
Fill the HG2 at the specified location with the weight given
</HTML>

  RETURN VALUES:
<HTML>
SH_SUCCESS
</HTML>
</AUTO>*/
RET_CODE atHg2Fill(
		   HG2 *hg2,	/* the HG2 to fill */
		   double x,	/* x position to fill */
		   double y,	/* y position to fill */
		   double w	/* weight -- increments bin by this amount */
		   ) {
  int ix, iy;
  ix = floor((x-hg2->xmin)/hg2->delx);
  if ((ix>=0) && (ix<hg2->reg->ncol)) {
    iy = floor((y-hg2->ymin)/hg2->dely);
    if ((iy>=0) && (iy<hg2->reg->nrow)) {
      hg2->reg->rows_fl32[iy][ix] += w;
    }
  }
  return SH_SUCCESS;
}

/**************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atHg2Get

  DESCRIPTION:
<HTML>
Get the value of a bin of a HG2 structure
</HTML>

  RETURN VALUES:
<HTML>
the value, or 0 if x and/or y are out of range
</HTML>
</AUTO>*/
double atHg2Get(
		HG2 *hg2,	/* the HG2 to evaluate */
		double x,	/* x position for evaluation */
		double y	/* y position for evaluation */
		) {
  int ix, iy;
  double answer = 0.0;
  ix = floor((x-hg2->xmin)/hg2->delx);
  if ((ix>=0) && (ix<hg2->reg->ncol)) {
    iy = floor((y-hg2->ymin)/hg2->dely);
    if ((iy>=0) && (iy<hg2->reg->nrow)) {
      answer = hg2->reg->rows_fl32[iy][ix];
    }
  }
  return answer;
}

/**************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atHg2VFill

  DESCRIPTION:
<HTML>
Fill the HG2 with the contents of a vector
</HTML>

  RETURN VALUES:
<HTML>
SH_SUCCESS
</HTML>
</AUTO>*/
RET_CODE atHg2VFill(
		    HG2 *hg2,	/* the HG2 to fill */
		    VECTOR *vx,	/* vector of x values */
		    VECTOR *vy,	/* vector of y values */
		    VECTOR *vw	/* vecotr of weights */
		    ) {
  int i;
  if (vw != NULL) {
    for (i=0; i<vx->dimen; i++) {
      atHg2Fill(hg2, vx->vec[i], vy->vec[i], vw->vec[i]);
    }
  } else {
    for (i=0; i<vx->dimen; i++) {
      atHg2Fill(hg2, vx->vec[i], vy->vec[i], 1.0);
    }
  }
  return SH_SUCCESS;
}


/**************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atHg2Cont

  DESCRIPTION:
<HTML>
Plot the HG2 as a contour
</HTML>

  RETURN VALUES:
<HTML>
SH_SUCCESS
</HTML>
</AUTO>*/
RET_CODE atHg2Cont(
		   PGSTATE *pg, /* where to make the plot */
		   HG2 *hg2,	/* the HG2 to plot */
		   VECTOR *cVec	/* vector of contour levels */
		   ) {
  int idim, jdim, i1, i2, j1, j2;
  float *a = NULL;
  float *c = NULL;
  int nc;
  float tr[6];
  int i,j;

  idim = hg2->reg->ncol;
  jdim = hg2->reg->nrow;
  i1 = 1;
  i2 = idim;
  j1 = 1;
  j2 = jdim;
  a = (float *) shMalloc(idim*jdim*sizeof(float));
  for (i=0; i<idim; i++) {
    for (j=0; j<jdim; j++) {
      a[i + (j*jdim)] = hg2->reg->rows_fl32[j][i];
    }
  }
  c = (float *) shMalloc(cVec->dimen*sizeof(float));
  for (i=0; i<cVec->dimen; i++) {
    c[i] = cVec->vec[i];
  }
  nc = cVec->dimen;
  tr[1] = (hg2->xmax-hg2->xmin)/idim;
  tr[0] = hg2->xmin;
  tr[2] = 0;
  tr[5] = (hg2->ymax-hg2->ymin)/jdim;
  tr[3] = hg2->ymin;
  tr[4] = 0;

  if (pg->isNewplot == 1) {
    shPgstateNextWindow(pg);
    cpgswin(hg2->xmin, hg2->xmax, hg2->ymin, hg2->ymax);
    cpgbox(pg->xopt, pg->xtick, pg->nxsub,
	   pg->yopt, pg->ytick, pg->nysub);
  }
  cpgcont(a, idim, jdim, i1, i2, j1, j2, c, nc, tr);

  /* Clean up and get out of here */
  shFree(c); shFree(a);

  return SH_SUCCESS;
}


/**************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atHg2Gray

  DESCRIPTION:
<HTML>
Plot the HG2 as gray scale image
</HTML>

  RETURN VALUES:
<HTML>
SH_SUCCESS
</HTML>
</AUTO>*/
RET_CODE atHg2Gray(
		   PGSTATE *pg, /* where to make the plot */
		   HG2 *hg2,	/* the HG2 to plot */
		   float fg,	/* foreground value */
		   float bg	/* background value */
		   ) {
  int idim, jdim, i1, i2, j1, j2;
  float *a = NULL;
  float tr[6];
  int i,j,icSave;
  float xb, yb;

  idim = hg2->reg->ncol;
  jdim = hg2->reg->nrow;
  i1 = 1;
  i2 = idim;
  j1 = 1;
  j2 = jdim;
  a = (float *) shMalloc(idim*jdim*sizeof(float));
  for (i=0; i<idim; i++) {
    for (j=0; j<jdim; j++) {
      a[i + (j*idim)] = hg2->reg->rows_fl32[j][i];
    }
  }
  tr[1] = (hg2->xmax-hg2->xmin)/idim;
  tr[0] = hg2->xmin - tr[1]/2.0;
  tr[2] = 0;
  tr[5] = (hg2->ymax-hg2->ymin)/jdim;
  tr[3] = hg2->ymin - tr[5]/2.0;
  tr[4] = 0;

  if (pg->isNewplot == 1) {
    shPgstateNextWindow(pg);

    xb = (hg2->xmax-hg2->xmin)/20.0;
    yb = (hg2->ymax-hg2->ymin)/20.0;

    if (pg->just == 0) {
      cpgswin(hg2->xmin-xb, hg2->xmax+xb, hg2->ymin-yb, hg2->ymax+yb);
    } else {
      cpgwnad(hg2->xmin-xb, hg2->xmax+xb, hg2->ymin-yb, hg2->ymax+yb);
    }
    cpgbox(pg->xopt, pg->xtick, pg->nxsub,
	   pg->yopt, pg->ytick, pg->nysub);
  }
  cpggray(a, idim, jdim, i1, i2, j1, j2, bg, fg, tr);

  /* put the text titles */
  if (pg->isNewplot == 1) {
    cpgqci(&icSave);
    cpgsci(pg->icLabel);
    cpglab(hg2->xlab, hg2->ylab, hg2->name);
    cpgsci(icSave);
  }

  /* Clean up and get out of here */
  shFree(a);

  return SH_SUCCESS;
}


RET_CODE atRegHistEq(
		     REGION *reg,
		     HG *hg,
		     double min,
		     double max
		     ) {
  int row, col, bin;
  double oldVal, newVal;
  for (row=0; row<reg->nrow; row++) {
    for (col=0; col<reg->ncol; col++) {
      oldVal = shRegPixGetAsDbl(reg, row, col);
      if (oldVal < hg->minimum) {
	newVal = min;
      } else if (oldVal >= hg->maximum) {
	newVal = max;
      } else {
	bin = (int) 
	  (hg->nbin * (oldVal-hg->minimum) / (hg->maximum - hg->minimum));
	  newVal = min + (max-min)*hg->contents[bin];
      }
      shRegPixSetWithDbl(reg, row, col, newVal);
    }
  }
  return SH_SUCCESS;
}


/*----------------------------------------------------------------------------*/

/*<AUTO EXTRACT>

  ROUTINE: atTransMultiply

  DESCRIPTION:
<HTML>
	Make the TRANS (tc) that does the same thing as applying
        tb and then ta.  Only the affine terms are used.
</HTML>

  RETURN VALUES:
<HTML>
        Returns SH_SUCCESS
</HTML>
</AUTO>*/
  RET_CODE atTransMultiply(TRANS *ta, TRANS *tb, TRANS *tc)

    {  /* Function atTransMultiply */

      int ndx_a, ndx_b, ndx_c;
      MATRIX ma, mb, mc;

/* Convert the input FITS transforms to matrices */

      atTransToMatrix(ta, ma);
      atTransToMatrix(tb, mb);

/* Multiply the matrices */

      for (ndx_a = 0; ndx_a <= 2; ndx_a += 1)
        {                               
        for (ndx_b = 0; ndx_b <= 2; ndx_b += 1)
          {
          mc[ndx_a][ndx_b] = 0.0;
          for (ndx_c = 0; ndx_c <= 2; ndx_c += 1)
            {
            mc[ndx_a][ndx_b] += ma[ndx_c][ndx_b] * mb[ndx_a][ndx_c];
            }
          }
        }

/* Convert the output matrix to a FITS transform */

      atMatrixToTrans(mc, tc);

      return (0);

    }  /* Function atTransMultiply */

/*----------------------------------------------------------------------------*/

/*<AUTO EXTRACT>

  ROUTINE: atMatrixToTrans

  DESCRIPTION:
<HTML>
	Convert a struct of type MATRIX to a struct of type TRANS.
	Only the affine terms are converted.
</HTML>

  RETURN VALUES:
<HTML>
        Returns 0
</HTML>
</AUTO>*/
  RET_CODE atMatrixToTrans(MATRIX m, TRANS *t_ptr)

    { /* Function atMatrixToTrans */

      t_ptr->a = m[2][0];
      t_ptr->b = m[0][0];
      t_ptr->c = m[1][0];
      t_ptr->d = m[2][1];
      t_ptr->e = m[0][1];
      t_ptr->f = m[1][1];
      return (0);

    } /* Function atMatrixToTrans */

/*----------------------------------------------------------------------------*/

/*<AUTO EXTRACT>

  ROUTINE: atTransToMatrix

  DESCRIPTION:
<HTML>
	Convert a struct of type TRANS to a struct of type MATRIX
	Only the affine terms are converted.
</HTML>

  RETURN VALUES:
<HTML>
        Returns 0
</HTML>
</AUTO>*/
  RET_CODE atTransToMatrix(TRANS *t_ptr, MATRIX m)

    { /* Function atTransToMatrix */

    m[0][0] = t_ptr->b;
    m[0][1] = t_ptr->e;
    m[0][2] = 0.0;
    m[1][0] = t_ptr->c;
    m[1][1] = t_ptr->f;
    m[1][2] = 0.0;
    m[2][0] = t_ptr->a;
    m[2][1] = t_ptr->d;
    m[2][2] = 1.0;
    return (0);

    } /* Function atTransToMatrix */
  
