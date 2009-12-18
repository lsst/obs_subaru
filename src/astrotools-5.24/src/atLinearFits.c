/* <AUTO>
   FILE: atLinearFits
<HTML>
   Routines that perform a linear fit and solve linear equantions.
</HTML>
   </AUTO>
*/
#include <float.h>
#include <math.h>
#include <string.h>
#include "sdssmath.h"
#include "dervish.h"
#include "pgplot_c.h"
#include "cpgplot.h"
#include "atLinearFits.h"

void
bess_(int *nmax, int *nmodels, int *nPt, double *X, double *Xerr, 
	double *Y, double *Yerr, double *CErr, double *a, double *b,
	double *avar, double *bvar, double *bvar_ifab, double *xi,
	double *zeta);
void
bootspbec_(int *nMax, int *nPt, int *seed, double *X, double *XErr,
	     double *Y, double *YErr, double*CErr,
	     double *y1sim, double *y1errsim, double *y2sim,
	     double *y2errsim, double *cerrsim);

#define isgood(x)   ( ( ((x) < FLT_MAX)) && ((x) > -FLT_MAX) ? (1) : (0))

/*<AUTO EXTRACT>

  ROUTINE: atVLinEqn

  DESCRIPTION:
<HTML>
     Solve a set of linear equations: A X = B, where A is a matrix
     of independent coefficients, X is a vector of unknown, and B
     is a vector of dependent variables.  The independent vectors
     A are passed as a list of vectors.
     <P>
     Note, unlike the old CERLIB-based version, this version of 
     atVLinFit does not deal with errors in the dependent variable.
</HTML>

  RETURN VALUES:
<HTML>
	Returns the parameters of the linear fit in a FUNC struct
        of type "unknown".   Unlike other FUNCs, this returns 0 for 
        all values and so is not useful for FUNC based plotting, etc.).  
	Return the FUNC as NULL if there was trouble.
</HTML>
</AUTO>*/

FUNC *atVLinEqn( VECTOR *depValue,  /* dependent variable */
                 VECTOR **value,    /* VECTORs of points to be fit */
                 int nVar           /* number of VECTORs to fit */
  ) {
  int i,j,ct;
/*  int n, iDim, iFail, nPt, itask, ind;      don't use all these */
  int iDim, nPt, itask, ind;
  int *iwork;
  double *aTemp, *b, *work;
  double **a=NULL;
  VECTOR **values; 
  FUNC *fitFunc=NULL;
  
  itask = 1;      /* factor the matrix first */
  ind = 0;        /* will hold error code from smaDGEFS */
  iDim = nVar;
  nPt = value[0]->dimen;

/** some error checking **/
  shAssert( iDim > 0 );
  shAssert( value != NULL );
  for (i = 0; i < nVar; i++) {
    shAssert ( (nPt == value[i]->dimen) )
  }

  a = (double **) shMalloc( iDim * sizeof(double *));
  aTemp  = shMalloc(iDim * nPt * sizeof(double));
  b      = shMalloc(nPt * sizeof(double));
  work   = shMalloc(nPt * sizeof(double));
  iwork  = shMalloc(nPt * sizeof(int));
  values = (VECTOR **)shMalloc( iDim * sizeof(VECTOR *) ); 

/*  a[i][j] also ends up as aTemp[i], suitable for passing to smaDGEFS */
  for (i=0; i < nVar; i++) {
    a[i] = aTemp + (i)*nPt;
  }

/* create the A matrix */
  for ( i = 0; i < iDim ; i++ ) { 
    for ( j = 0; j < nPt; j++ ) {
      a[i][j] = value[i]->vec[j];
    }
  }

/* create B vector */
  for ( i = 0; i < nPt; i++ ) {
    b[i] = depValue->vec[i];
  }

/* actually make the library call */
 smaDGEFS(aTemp, nPt, iDim, b, itask, &ind, work, iwork); 

/*  if call succeeds create a FUNC of type "unknown" */

  if ( ind > 0 ) {

    values[0] = shVectorNew(nVar);
    for ( i =  0; i < iDim; i++ ) {
      values[0]->vec[i] = b[i]; 
    }

    fitFunc = smFuncNew( "unknown", values, 1);

    fitFunc->cov = shVectorNew(nVar*nVar);
    fitFunc->sigma = shVectorNew(nVar);

/*    for ( i =  0; i < iDim; i++ ) {
      values[0]->vec[i] = b[i]; 
        fitFunc->va[0]->vec[i] = b[i];
    }*/

/* 
 *  smaDGEFS, unlike its CERNLIB equivalent does not return a 
 *  covariant matrix for A, so fill these parts of the FUNC with 0 
 */

    for ( i = 0; i < iDim; i++ ) {
      fitFunc->sigma->vec[i] = 0;
    }
    ct = 0;
    for ( i = 0; i < iDim; i++ ) {
      for ( j = 0; j < iDim; j++ ) {
        fitFunc->cov->vec[ct] = 0;
        ct++; 
      }
    }
    fitFunc->chi2  = 0;

  } else {

/* an error occured, see if smaDGEFS has identified it */

    switch ( ind ) {
      case -1: 
          shErrStackPush("atVLinFit: the number of independent vectors exceeds the number of elements in each vector");
          break;
      case -2: 
          shErrStackPush("atVLinFit: the number of independent vectors is less than 1");
          break;
      case -3:
          shErrStackPush("atVLinFit: itask is less than 1");
          break;
      case -4:
          shErrStackPush("atVLinFit: the matrix A is singular, no solution has been determined");
          break;
      case -10:
          shErrStackPush("atVLinFit: the solution has no apparent significance and may be inaccurate or the matrix A may be poorly scaled");
          break; 
      default:
          shErrStackPush("atVLinFit: an error of unknown type occured");
    }

    fitFunc = NULL; 
  }

  shFree(a);
  shFree(aTemp);
  shFree(b);
  shFree(work);
  shFree(iwork);
  shFree(values); 

  return fitFunc;
}

/* ================================================================== */

/*<AUTO EXTRACT>

  ROUTINE: atVLinFitBces

  DESCRIPTION:
  <HTML>
  BCES (Bivariate Correlated Errors and intrinsic Scatter) is a linear
  regression algorithm that allows for:
  <UL>
  <li> measurement errors on both variables
  <li> measurement errors that depend on the measurement
  <li> correlated measurement errors
  <li> scatter intrinsic to the relation being fitted, beyond measument error
  </UL>
  The routine performs four fits: y regressed on x, x regressed on y,
  the bisector, and orthogonal errors. Which answer is the "right" one
  depends on the situation. (A simplified guide would be: if you wish
  to predict a y given an x, use y regressed on x. If you wish to learn
  about the relationship of two quantities, use the bisector. See
  Feigelson and Babu, ApJ 397, 55 1992 for details.)
  <p>
  The algorithm and the base fortran code are from Akritas and Bershady,
  ApJ 470, ? 1996.
 <p> Also returned are the results of a bootstrap ananlysis.
  <p> The "slopeErr" and "slopeErrBoot" lists have two extra elements on them.
        These are variences for the bisector and orthogonal slopes calculated
        using a technique of wider applicability than the usual one
        (which assumes that the residuals in Y about a line are independant
        of the value of X; see Isobe, Feigelson, Akritas, and Babu, ApJ 364, 104 1990)
  <p> The covarience vector may be all zeros.
  <p> James Annis, June 14, 1996 <p>

</HTML>
</AUTO>*/

/* This routine is meant as a c-clone of the routine
	bces_regress.f from Akritas, Bird, and Bershady.
	Only that routine, basically a driver, has been replaced.
	All the subroutines, which do the actual work, are
	retained. The variable names are retained as well,
	including the scintillating "a" and "b".
						JTA June 1996
*/
int atBcesFit (
	double *X,		/* a DERVISH vector of x values */
	double *Y,		/* a DERVISH vector of y values */
	double *XErr,		/* a DERVISH vector of x error values */
	double *YErr,		/* a DERVISH vector of y error values */
	double *CErr,		/* a DERVISH vector of xy covariance errors */
	int nPt,		/* number of data points */
	int nsim,		/* number of simulations */
	int seed,		/* seed for random number generator */
	double *b,		/* bces fit */
	double *bvar,		/* bces fit sigma */
	double *a,		/* bces fit */
	double *avar,		/* bces fit sigma */
	double *bavg,		/* bootstrap fit */
	double *sdb,		/* bootstrap fit sigma */
	double *aavg,		/* bootstrap fit */
	double *sda,		/* bootstrap fit sigma */
	double *bvar_ifab,	/* isobe et al. alt. slope variance method */
	double *bvar_ifabsim	/* bootstrap of that */
				/* only calculated for bisector and orthog */
  ) 
{
	int nMax = 300000;	/* parameters from the fortran */
	int nmodels = ATLFNMODELS;	/* parameters from the fortran */
  	int nmodelsBig;
	int nPtBig;

	double *asim, *bsim, *avarsim, *bvarsim;
	double *asum, *bsum;
	double *assum, *bssum;
	double sdtest, rnsim;

	double *zeta, *xi;
	double *y1sim, *y1errsim;
	double *y2sim, *y2errsim;
	double *cerrsim;

	int i, j;

  /* make sure everything makes sense */
	if (nPt > nMax) {
    	   shErrStackPush("atVLinFitBces: max vector size exceeded, currently 10000");
    	   return 0;
  	}

  /* make all of the memory the routine requires */
	nmodelsBig = nmodels+1;
	nPtBig = nMax;	/* for some reason the fortran wants the arrays this size */
	y1sim = (double *) shMalloc (nPtBig * sizeof(double));
	y1errsim = (double *) shMalloc (nPtBig * sizeof(double));
	y2sim = (double *) shMalloc (nPtBig * sizeof(double));
	y2errsim = (double *) shMalloc (nPtBig * sizeof(double));
	cerrsim = (double *) shMalloc (nPtBig * sizeof(double));

	asim = (double *) shMalloc (nmodelsBig * sizeof(double));
	avarsim = (double *) shMalloc (nmodelsBig * sizeof(double));
	bsim = (double *) shMalloc (nmodelsBig * sizeof(double));
	bvarsim = (double *) shMalloc (nmodelsBig * sizeof(double));
	asum = (double *) shMalloc (nmodelsBig * sizeof(double));
	bsum = (double *) shMalloc (nmodelsBig * sizeof(double));
	assum = (double *) shMalloc (nmodelsBig * sizeof(double));
	bssum = (double *) shMalloc (nmodelsBig * sizeof(double));
	zeta = (double *) shMalloc (nmodelsBig * nPtBig * sizeof(double));
	xi = (double *) shMalloc (nmodelsBig * nPtBig * sizeof(double));

/* be paranoid about clearing space */
	for (i = 0; i < nmodels; i++) {
		y1sim[i] = y1errsim[i] = 0;
		y2sim[i] = y2errsim[i] = 0;
		cerrsim[i] = 0;
		asim[i] = bsim[i] = asum[i] = bsum[i] = 0;
		avarsim[i] = bvarsim[i] = assum[i] = bssum[i] = 0;
	}
	for (i = 0; i < nmodels*nPt; i++) {
		zeta[i] = xi[i] = 0;
	}

/*
  	Calculate nominal fits
 */

	bess_(&nMax, &nmodels, &nPt, X, XErr, Y, YErr, CErr, 
		a, b,avar, bvar, bvar_ifab, xi, zeta);

/*
 *     Make Bootstrap simulated datasets, and compute averages
 *     and standard deviations of regression coefficients
 */
	rnsim = (double )nsim;
	for (i=0; i< nmodels; i++) {
		asum[i] = 0.0;
		bsum[i] = 0.0;
		assum[i] = 0.0;
		bssum[i] = 0.0;
		sda[i] = 0.0;
		sdb[i] = 0.0;
	}
	for (i=0; i< nsim; i++) {
		if (i == 0) seed = -1*abs(seed);
		bootspbec_(&nMax, &nPt, &seed, X, XErr, Y, YErr, CErr,
			y1sim, y1errsim, y2sim, y2errsim, cerrsim);
		bess_(&nMax, &nmodels, &nPt, y1sim, y1errsim, y2sim, y2errsim,
			cerrsim, asim, bsim, avarsim, bvarsim, bvar_ifabsim, xi, zeta);

		for (j=0; j < nmodels; j++) {
			asum[j] = asum[j] + asim[j];
			bsum[j] = bsum[j] + bsim[j];
			assum[j] = assum[j] + pow(asim[j], 2);
			bssum[j] = bssum[j] + pow(bsim[j], 2);
		}
	}
	for (i=0; i< nmodels; i++) {
		aavg[i] = asum[i]/rnsim;
		sdtest = assum[i] - rnsim*pow(aavg[i], 2);
		if (sdtest > 0) sda[i] = sqrt(sdtest/(rnsim-1.0));
		bavg[i] = bsum[i]/rnsim;
		sdtest = bssum[i] - rnsim*pow(bavg[i], 2);
		if (sdtest > 0) sdb[i] = sqrt(sdtest/(rnsim-1.0));
		bvar[i] = sqrt(bvar[i]);
		avar[i] = sqrt(avar[i]);
		bvar_ifab[i] = sqrt(bvar_ifab[i]);
		bvar_ifabsim[i] = sqrt(bvar_ifabsim[i]);
	}

	shFree(y1sim);
	shFree(y1errsim);
	shFree(y2sim);
	shFree(y2errsim);
	shFree(cerrsim);
	shFree(asim);
	shFree(avarsim);
	shFree(bsim);
	shFree(bvarsim);
	shFree(asum);
	shFree(bsum);
	shFree(assum);
	shFree(bssum);
	shFree(zeta);
	shFree(xi);

	return(0);
}





