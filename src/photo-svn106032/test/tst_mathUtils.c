/*
 * Test photo's Mathematical Utilities
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "phMathUtils.h"

#define shMalloc(PTR) malloc(PTR)
#define shFree(PTR) free(PTR)

#if !defined(M_PI)
#  if 0
#     include "astrotools.h"		/* for M_PI, but pulls in too much
					   stuff for a simple test suite */
#  else
#     define M_PI 3.14159265358979323846     
#  endif
#endif

/*****************************************************************************/

static int
test_brentMinimum(
		  char *msg,		/* description of test */
		  double (*f)(double x, const void *user), /* Function under
							      examination */
		  double a, double b,	/* Range the root is seeked for */
		  double ans		/* the correct answer */
		  )
{
   double minloc;

   if(phBrentMinimum(&minloc,a,b,f,NULL,100,SQRT_EPSILON) < 0) {
      fprintf(stderr,"TEST-ERR: No minimum found for test \"%s\"\n",msg);
      return(1);
   }
   if(fabs(minloc - ans) > 1e-6) {
      fprintf(stderr,"TEST-ERR: "
	      "Expected %g, saw %g (|error| = %g) for test \"%s\"\n",
	      ans, minloc, fabs(minloc - ans),msg);
      return(1);
   }
   
   return(0);
}

static int
test_zeroFind(
	      char *msg,		/* description of test */
	      double (*f)(double x, const void *user), /* Function under
							  examination */
	      double a, double b,	/* Range the root is seeked for */
	      double ans		/* the correct answer */
	      )
{
   double zero;

   if(phZeroFind(a,b,f,NULL,EPSILON,&zero) < 0) {
      if(ans < -1000) {			/* don't expect an answer */
	 return(0);
      }
      fprintf(stderr,"TEST-ERR: No zero found for test \"%s\"\n",msg);
      return(1);
   }
   if(ans < -1000) {			/* don't expect an answer */
      fprintf(stderr,"TEST-ERR: No zero expected for test \"%s\"\n",msg);
      return(1);
   }


   if(fabs(zero - ans) > 1e-6) {
      fprintf(stderr,"TEST-ERR: "
	      "Expected %g, saw %g (|error| = %g) for test \"%s\"\n",
	      ans, zero, fabs(zero - ans),msg);
      return(1);
   }
   
   return(0);
}

/*****************************************************************************/

static double
f1(double x,				/* Test of the Forsythe book */
   const void *user)			/* NOTUSED */
{
  return (pow(x,2)-2.0)*x - 5.0;
}

static double
f2(double x,				/* Modified test 1 */
   const void *user)			/* NOTUSED */
{
  return pow( (pow(x,2)-2.0)*x - 5.0, 2 );
}

static double
f3(double x,				/* My test */
   const void *user)			/* NOTUSED */
{
  return pow( cos(x) - x,2 ) - 2;
}

static double
f4(double x,				/* My test */
   const void *user)			/* NOTUSED */
{
  return sqrt(fabs(sin(x) - x)) + 1;
}

/*****************************************************************************/

static double
f5(double x,				/* My test*/
   const void *user)			/* NOTUSED */
{
  return cos(x) - x;
}

static double
f6(double x,				/* My test */
   const void *user)			/* NOTUSED */
{
  return sin(x) - x;
}

/*****************************************************************************/
/*
 * Test the spline code
 */
#define NPOINT 100

static int
test_spline_interp(float *x,		/* ordinates */
		   float *y,		/* true y values */
		   float *dydx,		/* true dy/dx values */
		   int n,		/* number of values (only half are
					   used) for fitting spline */
		   float gamma,		/* control tautness */
		   char *msg		/* describe this test */
	    )
{
   int i;
   int nerr = 0;			/* number of errors */
   SPLINE *sp;
   float sum;				/* used in finding RMS error */
   float xx[NPOINT], yy[NPOINT], values[NPOINT];
/*
 * Take every other point
 */
   for(i = 0;i < n/2;i++) {
      xx[i] = x[2*i];
      yy[i] = y[2*i];
   }

   if((sp = phSplineFindTaut(xx,yy,n/2,gamma)) == NULL) {
      fprintf(stderr,"TEST-ERR: Cannot fit taut spline for test \"%s\"\n",msg);
      return(1);
   }
/*
 * test interpolation
 */
   phSplineInterpolate(sp,x,values,n);

   sum = 0;
   for(i = 0;i < n;i++) {
      sum += pow(y[i] - values[i],2);
   }
   sum = sqrt(sum/n);
   
   if(sum > 2e-5) {
      nerr++;
      fprintf(stderr,"TEST-ERR: RMS error = %g for y, test \"%s\"\n",
	      sum,msg);
   }
/*
 * and differentiation
 */
   phSplineDerivative(sp,x,values,n);

   sum = 0;
   for(i = 0;i < n;i++) {
      sum += pow(dydx[i] - values[i],2);
   }
   sum = sqrt(sum/n);
   
   if(sum > 2e-4) {
      nerr++;
      fprintf(stderr,"TEST-ERR: RMS error = %g for dydx, test \"%s\"\n",
	      sum,msg);
   }

   phSplineDel(sp);

   return(nerr);
}

/*****************************************************************************/
/*
 * test the spline root-finding code
 */
static int
test_spline_roots(float *x,		/* ordinates */
		  float *y,		/* true y values */
		  int n,		/* number of values */
		  float x0, float x1,	/* range to search */
		  float val,		/* solve y[] = val */
		  float *roots,		/* true positions of roots */
		  int nroot,		/* number of roots */
		  float gamma,		/* control tautness */
		  char *msg		/* describe this test */
		  )
{
   int i;
   int nerr = 0;			/* number of errors */
   int nfound;				/* number of roots found */
   float found[NPOINT];
   SPLINE *sp;

   if((sp = phSplineFindTaut(x,y,n,gamma)) == NULL) {
      fprintf(stderr,"TEST-ERR: Cannot fit taut spline for test \"%s\"\n",msg);
      return(1);
   }
/*
 * test interpolation
 */
   nfound = phSplineRoots(sp, val, x0, x1, found, NPOINT);

   if(nfound < 0) {
      fprintf(stderr,"TEST-ERR: too many roots, test \"%s\"\n",msg);
      nerr++;
      nfound = -nfound;
   }

   if(nfound != nroot) {
      fprintf(stderr,"TEST-ERR: found %d roots (expected %d), test \"%s\"\n",
	      nfound, nroot, msg);
      nerr++;
   }

   for(i = 0;i < (nfound < nroot ? nfound : nroot);i++) {
      if(fabs(roots[i] - found[i]) > 2.5e-3) {
	 fprintf(stderr,"TEST-ERR: root[%d] = %g (expected %g), test \"%s\"\n",
		 i, found[i], roots[i], msg);
	 nerr++;
      }
   }

   phSplineDel(sp);

   return(nerr);
}

/*****************************************************************************/
/*
 * test the spline smoothing code
 */
static int
test_smoothing_spline(float *x,
		      float *y,
		      float *dy,
		      int nval,
		      char *test)
{
   float chisq;
   int i, j;
   int nerr = 0;
   SPLINE *sp;

   if((sp = phSplineFindSmoothed(x,y,dy,nval,1,&chisq,NULL)) == NULL) {
      fprintf(stderr,"TEST-ERR: "
	      "Cannot fit smoothed spline for test \"%s\"\n",test); nerr++;
   }

   for(i = 0; i < sp->nknot; i++) {
      for(j = 0; j < 4; j++) {
	 if(sp->coeffs[j][i] != sp->coeffs[j][i]) {		/* i.e. NaN */
	    nerr++;
	    fprintf(stderr,"TEST-ERR: "
		    "sp->coeffs[%d][%d] is NaN for test \"%s\"\n", i, j, test);
	    nerr++;
	 }
      }
   }

   phSplineDel(sp); sp = NULL;
   
   if(chisq > 2*nval) {
      fprintf(stderr,"TEST-ERR: "
	      "chisq too large (%g) for smoothed spline test \"%s\"\n",
	      chisq,test);
      nerr++;
   }

   return(nerr);
}

static int
test_splines_main(void)
{
   int i;
   int nerr = 0;			/* number of errors */
   int nval = NPOINT;
   float x[NPOINT], y[NPOINT], dydx[NPOINT], dy[NPOINT];
   float roots[NPOINT];
   int nroot;
/*
 * first a cubic
 */
   nval = NPOINT/2;
   for(i = 0;i < nval;i++) {
      x[i] = i/(float)(nval - 1);
      y[i] = 0 + x[i]*(2 + x[i]*(3 + x[i]));
      dydx[i] = 2 + x[i]*(6 + 3*x[i]);
   }
   nerr += test_spline_interp(x,y,dydx,nval,0,"cubic; gamma = 0");
   nerr += test_spline_interp(x,y,dydx,nval,2.5,"cubic; gamma = 2.5");
/*
 * Then a sine curve
 */
   nval = NPOINT/2;
   for(i = 0;i < nval;i++) {
      x[i] = (M_PI/2)*i/(float)(nval - 1);
      y[i] = sin(x[i]);
      dydx[i] = cos(x[i]);
   }

   nerr += test_spline_interp(x,y,dydx,nval,2.5,"sin(0..pi/2), gamma = 2.5");
/*
 * Look for a root that doesn't require extrapolation
 */
   nroot = 0;
   roots[nroot++] = asin(0.5);
   nerr += test_spline_roots(x,y,nval,0,M_PI/2,0.5,roots,nroot,
				   2.5,"sin(0..pi/2), search 0 -- pi for 0.5");
/*
 * and look for one that does, extrapolating to the right
 */
   nroot = 0;
   roots[nroot++] = asin(0.95);
   roots[nroot++] = M_PI - asin(0.95);
   nerr += test_spline_roots(x,y,nval,0,M_PI,0.95,roots,nroot,
				  2.5,"sin(0..pi/2), search 0 -- pi for 0.95");
/*
 * and to the left, searching extrapolated area and where we have data
 */
   nroot = 0;
   roots[nroot++] = asin(-0.05);
   nerr += test_spline_roots(x,y,nval,-1,0.1,-0.05,roots,nroot,
			       2.5,"sin(0..pi/2), search -1 -- 0.1 for -0.05");
/*
 * and to the left, _only_ searching the extrapolated region
 */
   nroot = 0;
   roots[nroot++] = asin(-0.05);
   nerr += test_spline_roots(x,y,nval,-1,-1e-5,-0.05,roots,nroot,
				 2.5,"sin(0..pi/2), search -1 -- 0 for -0.05");
/*
 * and to the right, _only_ searching the extrapolated region
 */
   nroot = 0;
   roots[nroot++] = M_PI - asin(0.95);
   nerr += test_spline_roots(x,y,nval,M_PI/2+1e-4,M_PI,0.95,roots,nroot,
			       2.5,"sin(0..pi/2), search pi/2 -- pi for 0.95");
/*
 * Then a longer section of a sine curve
 */
   nval = NPOINT;
   for(i = 0;i < nval;i++) {
      x[i] = 3*i/(float)(nval - 1);
      y[i] = sin(x[i]);
      dydx[i] = cos(x[i]);
   }

   nerr += test_spline_interp(x,y,dydx,nval,2.5,"sin(0..3), gamma = 2.5");
/*
 * Now a cubic
 */
   nval = NPOINT;
   for(i = 0;i < nval;i++) {
      x[i] = 5*i/(float)(nval - 1);
      y[i] = (x[i] - 1)*(x[i] - 2)*(x[i] - 3);
   }
   nroot = 0;
   roots[nroot++] = 1;
   roots[nroot++] = 2;
   roots[nroot++] = 3;
   nerr += test_spline_roots(x,y,nval,0,5,0,roots,nroot,
						   2.5,"cubic, search 0 -- 5");

   nerr += test_spline_roots(x,y,nval,0.999,3.001,0,roots,nroot,
					   2.5,"cubic, search 0.999 -- 3.001");
/*
 * and a nearly-quadratic taken from a case that gave measure objects trouble
 */
   {
      float as_r[11] = {
	 0, 0.537876248, 1.29704654, 1.67850423, 2.18885279, 2.71326518,
	 3.14395905, 3.61617303, 4.04516554, 4.51106596, 4.94895411 };
      float as_cumul[11] = {
	 0, 8.93106079, 11.0915947, 11.8795805, 12.8245687, 13.6166325, 
	 14.117857, 14.4799118, 14.6381674, 14.685607, 14.6904516};
      float val = 14.5853577;

      nval = 11;
      nroot = 0;
      roots[nroot++] = 3.8522954;
      nerr += test_spline_roots(as_r,as_cumul,nval,0,4.66,val,roots,nroot,
				2.5,"cumulative profile from MO");
   }
/*
 * Now test spline smoothing
 */
   nval = NPOINT;
   for(i = 0;i < nval;i++) {
      x[i] = 5*i/(float)(nval - 1);
      y[i] = (x[i] - 1)*(x[i] - 2)*(x[i] - 3);
      dy[i] = 1;
   }
   nerr += test_smoothing_spline(x,y,dy,nval,"cubic");
   
   {
      float xx[16] = {
	 -28.5515614, -18.5840321, -11.5762024, -7.5060544, -4.40646172, 
	 -2.58544159, -1.69256878, -0.564189613, 0.564189613, 1.69256878,
	 2.58544159, 4.40646172, 7.506054, 11.576202, 18.584032, 28.551561};
      float yy[16] = {0, 0, 0, 0, 0, 0, 12, 32, 32, 12, 0, 0, 0, 0, 0, 0};
      float dy[16] = {
	 1.19208998e-07, 1.19208998e-07, 1.19208998e-07, 1.19208998e-07, 
	 1.19208998e-07, 1.19208998e-07, 1.13707054, 7.41619825, 7.41619825,
	 1.13707054, 1.19208998e-07, 1.19208998e-07, 1.19208e-07,
	 1.19209e-07, 1.19208998e-07, 1.19208998e-07};
      
      nerr += test_smoothing_spline(xx,yy,dy,16,"test_mo object 1");
   }
   
   {
      float xx[] =
	{-10.00363255, -5.39846182, -4.94895411, -4.51106596, -4.04516554, 
	   -3.61617303, -3.14395905, -2.71326518, -2.18885279, -1.67850423, 
	   -1.29704654, -0.537876248, 0.537876248, 1.29704654, 1.67850423,
	   2.18885279, 2.71326518, 3.14395905, 3.61617303, 4.04516554,
	   4.51106596, 4.94895411, 5.39846182, 10.00363255};
      float yy[] =
	{0, -0.0912534669, 0.426261514, 0.808568776, 0.028453052, 1.02861381, 
	   4.31957531, 35.8719788, 53.5069656, -3.49599028, 1.52845299,
	   -1.40498197, -1.40498197, 1.52845299, -3.49599028, 53.5069656,
	   35.8719788, 4.31957531, 1.02861381, 0.028453052, 0.808568776,
	   0.426261514, -0.0912534669, 0};
      float dy[] =
	{1e-10, 0.0473796315, 0.0722261816, 0.0492737442, 0.00115720253, 
	   0.0112569127, 0.0171896983, 7.07682323, 16.2657204, 1.19208998e-07, 
	   1.19208998e-07, 6.43143368, 6.43143368, 1.19208998e-07,
	   1.19208998e-07, 16.2657204, 7.07682323, 0.0171896983,
	   0.0112569127, 0.00115720253, 0.0492737442, 0.0722261816,
	   0.0473796315, 1e-10};
      
      nerr += test_smoothing_spline(xx,yy,dy,24,"test_mo first light obj 1");
   }

   {
      float xx[] =
	{-8.650335312, -4.04516554, -3.61617303, -3.14395905, -2.71326518, 
	   -2.18885279, -1.67850423, -1.29704654, -0.537876248, 0.537876248, 
	   1.29704654, 1.67850423, 2.18885279, 2.71326518, 3.14395905,
	   3.61617303, 4.04516554, 8.650335312};
      float yy[] =
	{0, 0.0528428815, -0.111347489, 0.00882108137, -0.0401232503, 
	   0.0324035659, -0.00267537427, 0.00169372559, -0.00256347633,
	   -0.00256347633, 0.00169372559, -0.00267537427, 0.0324035659,
	   -0.0401232503, 0.00882108137, -0.111347489, 0.0528428815, 0};
      float dy[] =
	{1e-10, 0.122332022, 0.0194231682, 0.00665452797, 0.0273593403,
	   0.0371408202, 1.19208998e-07, 1.19208998e-07, 6.75683212,
	   6.75683212, 1.19208998e-07, 1.19208998e-07, 0.0371408202,
	   0.0273593403, 0.00665452797, 0.0194231682, 0.122332022, 1e-10};
      
      nerr += test_smoothing_spline(xx,yy,dy,18,"test_mo first light obj 2");
   }

   {
      float x[20] = {
	 -70.5101547, -45.5039101, -28.5515614, -18.5840321, -11.5762024,
	 -7.5060544, -4.40646172, -2.58544159, -1.69256878, -0.564189613,
	 0.564189613, 1.69256878, 2.58544159, 4.40646172, 7.5060544,
	 11.5762024, 18.5840321, 28.5515614, 45.5039101, 70.5101547};
      float y[20] = {
	 0, 0, 0, 0, 3.83538818, 96.4849854, 450.05249, 1455.31885,
	 4888.68066, 9189.49512, 9189.49512, 4888.68066, 1455.31885,
	 450.05249, 96.4849854, 3.83538818, 0, 0, 0, 0};
      float dy[20] = {
	 1.19208998e-07, 1.19208998e-07, 1.19208998e-07, 1.19208998e-07,
	 0.590509593, 9.17930412, 60.4429092, 91.4584427, 419.311157,
	 39.4767761, 39.4767761, 419.311157, 91.4584427, 60.4429092,
	 9.17930412, 0.590509593, 1.19208998e-07, 1.19208998e-07,
	 1.19208998e-07, 1.19208998e-07};
      
      nerr += test_smoothing_spline(x,y,dy,20,"test_mo object 2");
      nerr += test_smoothing_spline(&x[4],&y[4],&dy[4],12,"test_mo object 3");
   }

   {
      float x[20] = {
	-4.948954,-4.511066,-4.045166,-3.616173,-3.143959,-2.713265,-2.188853,
	-1.678504,-1.297047,-0.537876,0.537876,1.297047,1.678504,2.188853,
	2.713265,3.143959,3.616173,4.045166,4.511066,4.948954};
      float y[20] = {
	-0.074066,-0.217947,-0.299868,-0.070560,-0.477365,-0.672416,
	-0.716309,-1.738342,-1.930054,6.994751,6.994751,-1.930054,
	-1.738342,-0.716309,-0.672416,-0.477365,-0.070560,-0.299868,
	-0.217947,-0.074066};
      float dy[20] = {
	0.058507,0.129137,0.306029,0.375248,0.370458,0.582576,
	0.988932,1.348868,0.982093,7.877751,7.877751,0.982093,
	1.348868,0.988932,0.582576,0.370458,0.375248,0.306029,
	0.129137,0.058507};

      nerr += test_smoothing_spline(x,y,dy,20,"test_mo object 4");
   }

   {
      float x[18] = {
	 -7.01737022, -6.6119051, -3.61617303, -3.14395905, -2.71326518, 
	 -2.18885279, -1.67850423, -1.29704654, -0.537876248, 0.537876248, 
	 1.29704654, 1.67850423, 2.18885279, 2.71326518, 3.14395905,
	 3.61617303, 6.6119051, 7.01737022};	 
      float y[18] = {
	 0, 0, -0.287600428, 0.157347381, -0.653167963,
	 0.0555431433, -0.123519182, -0.301484585, 0.638308465, 0.638308465,
	 -0.301484585, -0.123519182, 0.0555431433, -0.653167963, 0.157347381,
	 -0.287600428, 0, 0};
      float dy[18] = {
	 0.100000001, 0.100000001, 0.124452673, 0.262768477, 0.418929845, 
	 0.471176654, 0.866025388, 0.569952369, 3.99751282, 3.99751282,
	 0.569952369, 0.866025388, 0.471176654, 0.418929845, 0.262768477,
	 0.124452673, 0.100000001, 0.100000001};
      nerr += test_smoothing_spline(x,y,dy,18,"col 6 field 409");
   }
   
   {
      float x[16] = {
	 -6.54515648, -6.13969135, -3.14395905, -2.71326518, -2.18885279, 
	 -1.67850423, -1.29704654, -0.537876248, 0.537876248, 1.29704654,
	 1.67850423, 2.18885279, 2.71326518, 3.14395905, 6.13969135,
	 6.54515648};
      float y[16] = {
	 0, 0, -0.187195078, -0.317154706, 0.870106041,
	 2.29141283, -0.185876369, 0.812441707, 0.812441707, -0.185876369,
	 2.29141283, 0.870106041, -0.317154706, -0.187195078, 0,
	 0};
      float dy[16] = {
	 0.100000001, 0.100000001, 0.310043216, 0.272643179, 0.508951366, 
	 1.62731314, 1.01354003, 5.2996192, 5.2996192, 1.01354003,
	 1.62731314, 0.508951366, 0.272643179, 0.310043216, 0.100000001,
	 0.100000001};
      nerr += test_smoothing_spline(x,y,dy,16,"PR 1369");
   }
   
   return(nerr);
}

/*****************************************************************************/
/*
 * test matrix utilities
 */
typedef double REAL;

static int
test_svd_matrix_inversion(REAL **a, int n, REAL *b, REAL *x, int singular);

static int
test_matrix_inversion(const REAL **a,	/* the matrix to test */
		      int n,		/* dimension */
		      const REAL *cb,	/* RHS in a*x = cb */
		      REAL *x,		/* true value of x */
		      int singular)	/* is matrix thought to be singular? */
{
   REAL *b = NULL;			/* copy of cb */
   REAL eps = 1e-5;			/* tolerance */
   REAL **ia;				/* inverse of a */
   int is_singular;			/* is the matrix really singular? */
   int i, j, k;
   int nerr = 0;			/* number of errors */
   REAL val;

   ia = shMalloc(n*sizeof(REAL *));
   ia[0] = shMalloc(n*n*sizeof(REAL));
   if(cb != NULL) {
      b = shMalloc(n*sizeof(Real));
      for(i = 0;i < n;i++) {
	 b[i] = cb[i];
      }
      cb = b;
   }

   for(i = 0;i < n;i++) {
      ia[i] = ia[0] + i*n;
   }
   memcpy(ia[0],a[0],n*n*sizeof(REAL));

   is_singular = phDMatrixInvert(ia,b,n);
   if(singular) {
      if(!is_singular) {
	 fprintf(stderr,"TEST-ERR: "
		 "expected singular matrix, but inversion succeeded\n");
	 nerr++;
      }
   } else {
/*
 * check result by multiplying a by its inverse
 */
      if(is_singular) {
	 fprintf(stderr,"TEST-ERR: matrix is unexpectedly singular\n");
	 nerr++;
      } else {
	 for(i = 0;i < n;i++) {
	    for(j = 0;j < n;j++) {
	       REAL sum = 0.0;
	       for(k = 0;k < n;k++) {
		  sum += a[i][k]*ia[k][j];
	       }

	       val = (i == j) ? 1 : 0;	/* correct value */
	       if(fabs(sum - val) > eps) {
		  fprintf(stderr,"TEST-ERR: "
			  "A*A^{-1}: [%d][%d] == %11.4f != %g\n",
			  i,j,sum,val);
		  nerr++;
	       }
	    }
	 }
	 
	 if(b != NULL) {
	    for(i = 0;i < n;i++) {
	       if(fabs(b[i] - x[i]) > eps) {
		  fprintf(stderr,"TEST-ERR: "
			  "x = A^{-1}b: x[%d] == %11.4f != %g\n",i,b[i],x[i]);
		  nerr++;
	       }
	    }
	 }
      }
   }
   shFree(ia[0]); shFree(ia);
   if(b != NULL) { shFree(b); }
   
   return(nerr);
}

static int
test_matrix_main(void)
{
   int i, n;
   int nerr = 0;			/* number of errors */
/*
 * Nice stable matrix
 */
   {
      REAL *a[3], s_a[3][3] = {
	 {10,  2, 0},
	 { 3,  4, 0},
	 { 0,  0, 590}
      }, b[3] = {
	 14, 11, 1770,
      }, x[3] = {
	 1, 2, 3,
      };
      
      n = sizeof(a)/sizeof(*a);
      for(i = 0;i < n;i++) {
	 a[i] = s_a[i];
      }
      
      nerr += test_matrix_inversion((const REAL **)a,n,b,x,0);
      nerr += test_svd_matrix_inversion(a,n,b,x,0);
   }
/*
 * singular matrix from composite profile fitting.
 */
   {
      REAL *a[11], s_a[11][11] = {
	 {129.415787, 0, 0, 0, 0, 0, 129.415787, 0, 0, 0, 0},
	 {0, 772.927124, 0, 0, 0, 0, 0, 0, 0, 772.927124, 0},
	 {0, 0, 796.160645, 0, 0, 0, 0, 242.034668, 159.161469,
						       209.412643, 185.551865},
	 {0, 0, 0, 3521.06714, 0, 0, 0, 803.969238, 902.772278,
							720.63031, 1093.69531},
	 {0, 0, 0, 0, 3923.12134, 0, 0, 2371.02515, 527.356018,
						       397.676025, 627.064148},
	 {0, 0, 0, 0, 0, 1140.65137, 0, 352.807587, 391.158264, 0, 396.685577},
	 {129.415787, 0, 0, 0, 0, 0, 132.414536, 0, 0, 0, 0},
	 {0, 0, 242.034668, 803.969238, 2371.02515, 352.807587,
						       0, 3769.83667, 0, 0, 0},
	 {0, 0, 159.161469, 902.772278, 527.356018, 391.158264,
							 0, 0, 1980.448, 0, 0},
	 {0, 772.927124, 209.412643, 720.63031, 397.676025, 0,
							 0, 0, 0, 2100.646, 0},
	 {0, 0, 185.551865, 1093.69531, 627.064148, 396.685577,
						      0, 0, 0, 0, 2302.99683}};
      
      n = sizeof(a)/sizeof(*a);
      for(i = 0;i < n;i++) {
	 a[i] = s_a[i];
      }
      
      nerr += test_matrix_inversion((const REAL **)a,n,NULL,NULL,0);
      nerr += test_svd_matrix_inversion(a,n,NULL,NULL,0);
   }
/*
 * singular matrix from deblending
 */
   {
      REAL *a[3], s_a[3][3] = {
	 {18344, 18344, 590},
	 {18344, 18344, 590},
	 {590, 590, 0}};
      
      n = sizeof(a)/sizeof(*a);
      for(i = 0;i < n;i++) {
	 a[i] = s_a[i];
      }
      
      nerr += test_svd_matrix_inversion(a,n,NULL,NULL,1);
   }

   return(nerr);
}

/*****************************************************************************/
/*
 * Test SVD code from meschach
 */
static int
check_svd_lsq(void)
{
   VEC *b;				/* vector on RHS */
   float ca = 2, cb = 6, cc = -1, cd = 5; /* parameters to fit */
   float eps = 1e-3;			/* allowed tolerance in params */
   float sigma = 0.01;			/* sigma of measurements */
   int i, j;
   int m = 20;				/* number of data points */
   int n = 4;				/* number of parameters */
   int nerr = 0;			/* number of errors */
   MAT *A;				/* input matrix; m*n */
   MAT *U;				/* may be NULL; m*m */
   MAT *V;				/* may be NULL; n*n */
   VEC *w;				/* singular values; n */
   VEC *x;

   A = phMatNew(m,n);
   b = phVecNew(m);

   for(i = 0;i < m;i++) {
      A->me[i][0] = 1/sigma;
      A->me[i][1] = i/sigma;
      A->me[i][2] = i*i/sigma;
      A->me[i][3] = i*i*i/sigma;

      b->ve[i] = ca*A->me[i][0] + cb*A->me[i][1] + cc*A->me[i][2] + \
	cd*A->me[i][3] + 1/sqrt(12)*0.5*(rand()/(float)RAND_MAX*2 - 1);
   }

   U = phMatNew(m,m); V = phMatNew(n,n);
   w = phSvd(A,U,V,NULL);
/*
 * See if there are any zeros among the singular values
 */
   for(i = 0;i < n;i++) {
      if(fabs(w->ve[i]) < 1e-6) {
	 w->ve[i] = 0.0;
      }
   }
/*
 * U is really an n*m matrix, but the values beyond m*m correspond to
 * zero singular values. Adjust dimensions to get rid of these extra
 * entries, and allow us to do matrix multiplications
 */
   if(m > n) {
      U->m = n;
   } else {
      V->m = m;
   }
/*
 * use SVD to solve system of equations
 */
   x = phSvdBackSub(U, V, w, b);
/*
 * reuse U for the covariances
 */
   if(m < n) {
      fprintf(stderr,"m > n in LSQ problem\n");
      return(1);
   }
   U->m = U->n = n;

   phSvdCovarGet(U,V,w);

   printf("SVD Least Squares:\n");

   printf("Singular values:\n");
   for(i = 0;i < w->dim;i++) {
      printf("%10.5f ",w->ve[i]);
   }
   printf("\n");

   printf("Best estimate of parameters %g, %g, %g, %g:\n", ca, cb, cc, cd);
   for(i = 0;i < x->dim;i++) {
      printf("%10.5f ",x->ve[i]);
   }
   printf("\n");

   printf("covariance:\n");
   for(i = 0;i < U->m;i++) {
      for(j = 0;j < U->n;j++) {
	 printf("%10.5f ",U->me[i][j]);
      }
      printf("\n");
   }

   printf("Principal components:\n");
   for(i = 0;i < V->m;i++) {
      for(j = 0;j < V->n;j++) {
	 printf("%10.5f ",V->me[i][j]);
      }
      printf("\n");
   }

   if(fabs((x->ve[0] - ca)/ca) > eps ||
      fabs((x->ve[1] - cb)/cb) > eps || 
      fabs((x->ve[2] - cc)/cc) > eps || 
      fabs((x->ve[3] - cd)/cd) > eps) {
      fprintf(stderr,"TEST-ERR: error in SVD param estimation\n");
      nerr++;
   }

   return(nerr);
}

static int
test_svd_matrix_inversion(REAL **a,	/* the matrix to test */
			  int n,	/* dimension */
			  REAL *b,	/* RHS in a*x = b */
			  REAL *x,	/* true value of x */
			  int singular)	/* is matrix thought to be singular? */
{
   MAT *A = phMatNew(n, n);		/* matrix == a */
   VEC *bv = NULL;			/* vector == b, if b != NULL */
   REAL eps = 1e-5;			/* tolerance */
   MAT *inv = phMatNew(n,n);		/* inverse of A */
   int is_singular;			/* is the matrix really singular? */
   int i, j, k;
   int nerr = 0;			/* number of errors */
   MAT *U = phMatNew(n, n);
   MAT *V = phMatNew(n, n);
   VEC *w, *iw;				/* singular values, and 1/w */
   REAL val;

   printf("Inversion using SVD:\n");
/*
 * copy values into a MAT
 */
   for(i = 0;i < n;i++) {
      for(j = 0;j < n;j++) {
	 inv->me[i][j] = A->me[i][j] = a[i][j];
      }
   }

   if(b != NULL) {
      bv = phVecNew(n);
      for(i = 0;i < n;i++) {
	 bv->ve[i] = b[i];
      }
   }
/*
 * SVD and inversion
 */
   w = phSvd(inv, U, V, NULL);
   iw = phVecNew(w->dim);

   is_singular = 0;
   for(i = 0;i < n;i++) {
      if(fabs(w->ve[i]) < 1e-5) {
	 is_singular++;
	 iw->ve[i] = 0;
      } else {
	 iw->ve[i] = 1/w->ve[i];
      }
   }
   phSvdInvert(inv, U, V, iw);
/*
 * check results
 */
   if(singular) {
      if(!is_singular) {
	 fprintf(stderr,"TEST-ERR: "
		 "expected singular matrix, but inversion succeeded\n");
	 nerr++;
      }
   } else {
/*
 * check result by multiplying A by its inverse
 */
      if(is_singular) {
	 fprintf(stderr,"TEST-ERR: matrix is unexpectedly singular\n");
	 nerr++;
      } else {
	 for(i = 0;i < n;i++) {
	    for(j = 0;j < n;j++) {
	       REAL sum = 0.0;
	       for(k = 0;k < n;k++) {
		  sum += A->me[i][k]*inv->me[k][j];
	       }

	       val = (i == j) ? 1 : 0;	/* correct value */
	       if(fabs(sum - val) > eps) {
		  fprintf(stderr,"TEST-ERR: "
			  "A*A^{-1}: [%d][%d] == %11.4f != %g\n",
			  i,j,sum,val);
		  nerr++;
	       }
	    }
	 }
	 
	 if(b != NULL) {
	    VEC *xv = phSvdBackSub(U, V, w, bv);

	    for(i = 0;i < n;i++) {
	       if(fabs(xv->ve[i] - x[i]) > eps) {
		  fprintf(stderr,"TEST-ERR: "
			  "x = A^{-1}b: x[%d] == %11.4f != %g\n",
			  i, xv->ve[i], x[i]);
		  nerr++;
	       }
	    }

	    phVecDel(xv);
	 }
      }
   }

   phMatDel(inv); phMatDel(A); phMatDel(U); phMatDel(V);
   phVecDel(w); phVecDel(iw); phVecDel(bv);

   return(nerr);
}

static int
test_svd_main(void)
{
   int nerr = 0;

   nerr += check_svd_lsq();

   return(nerr);
}

/*****************************************************************************/
/*
 * Test eigen value/vector code from meschach
 */
static int
check_eigen_decomp(void)
{
   float eps = 1e-3;			/* allowed tolerance in params */
   int i, j, k;
   const int m = 4;
   int nerr = 0;			/* number of errors */
   MAT *A;				/* input matrix; m*m */
   MAT *Q;				/* eigenvectors; m*m */
   VEC *lambda;				/* eigenvalues; m */
   MAT *prod = NULL, *mtmp = NULL;	/* tmp space */
   float sum;

   A = phMatNew(m,m);
   for(i = 0;i < m;i++) {
      for(j = i;j < m;j++) {
	 A->me[i][j] = A->me[j][i] = sin((i+1)*(j+2));
      }
   }

   printf("A:\n");
   for(i = 0;i < A->m;i++) {
      for(j = 0;j < A->n;j++) {
	 printf("%10.5f ",A->me[i][j]);
      }
      printf("\n");
   }
   
   Q = phMatNew(m,m);
   lambda = phEigen(A,Q,NULL);
#if 0
   printf("Eigen values:\n");
   for(i = 0;i < lambda->dim;i++) {
      printf("%10.5f ",lambda->ve[i]);
   }
   printf("\n");

   printf("Q:\n");
   for(i = 0;i < Q->m;i++) {
      for(j = 0;j < Q->n;j++) {
	 printf("%10.5f ",Q->me[i][j]);
      }
      printf("\n");
   }
#endif
/*
 * Check that we can recover A from lambda and Q
 */
   mtmp = phMatNew(m,m);
   prod = phMatNew(m,m);
   for(i = 0;i < m;i++) {
      for(j = 0;j < m;j++) {
	 mtmp->me[i][j] = lambda->ve[i]*Q->me[j][i];
      }
   }
   for(i = 0;i < m;i++) {
      for(j = 0;j < m;j++) {
	 sum = 0;
	 for(k = 0;k < m;k++) {
	    sum += Q->me[i][k]*mtmp->me[k][j];
	 }
	 prod->me[i][j] = sum;
      }
   }

   printf("Q Lambda Q^T:\n");
   for(i = 0;i < prod->m;i++) {
      for(j = 0;j < prod->n;j++) {
	 printf("%10.5f ",prod->me[i][j]);
      }
      printf("\n");
   }
/*
 * check recovery of initial matrix
 */
   for(i = 0;i < prod->m;i++) {
      for(j = 0;j < prod->n;j++) {
	 if(fabs(prod->me[i][j]/A->me[i][j] - 1) > eps) {
	    printf("Q Lambda Q^T: [%d][%d] is %10.5f should be %10.5f\n",
		   i,j,prod->me[i][j],A->me[i][j]);
	    nerr++;
	 }
      }
      printf("\n");
   }
   
   return(nerr);
}

/*
 * Solve an LSQ problem by eigen function decomposition
 */
static int
check_eigen_lsq(void)
{
   VEC *b;				/* RHS of Ax = b */
   float ca = 2, cb = 6, cc = -1, cd = 5; /* parameters to fit */
   float eps = 1e-3;			/* allowed tolerance in params */
   float sigma = 0.01;			/* sigma of measurements */
   int i, j, k;
   int m = 20;				/* number of data points */
   int n = 4;				/* number of parameters */
   int nerr = 0;			/* number of errors */
   MAT *A;				/* design matrix; m*m */
   MAT *in;				/* input matrix; m*n */
   MAT *Q;				/* may be NULL; m*m */
   double sum;				/* accumulator */
   VEC *lambda;				/* singular values; n */
   MAT *prod = NULL, *mtmp = NULL;	/* tmp space */
   VEC *x;				/* in . y */
   VEC *y;				/* vector on RHS */

   in = phMatNew(m,n);
   y = phVecNew(m);

   for(i = 0;i < m;i++) {
      in->me[i][0] = 1/sigma;
      in->me[i][1] = i/sigma;
      in->me[i][2] = i*i/sigma;
      in->me[i][3] = i*i*i/sigma;

      y->ve[i] = ca*in->me[i][0] + cb*in->me[i][1] + cc*in->me[i][2] +
							       cd*in->me[i][3];
      y->ve[i] += 1/sqrt(12)*0.5*(rand()/(float)RAND_MAX*2 - 1);
   }
/*
 * find the normal equations
 */
   A = phMatNew(n,n);
   b = phVecNew(n);
   for(i = 0; i < n; i++) {
      sum = 0;
      for(k = 0; k < m; k++) {
	 sum += in->me[k][i]*y->ve[k];
      }
      b->ve[i] = sum;
      
      for(j = i; j < n; j++) {
	 sum = 0;
	 for(k = 0; k < m; k++) {
	    sum += in->me[k][i]*in->me[k][j];
	 }
	 A->me[i][j] = A->me[j][i] = sum;
      }
   }
      
   Q = phMatNew(n,n);
   lambda = phEigen(A,Q,NULL);
/*
 * See if there are any zeros among the singular values
 */
   for(i = 0;i < n;i++) {
      if(fabs(lambda->ve[i]) < 1e-6) {
	 lambda->ve[i] = 0.0;
      }
   }
/*
 * solve system of equations
 */
   x = phEigenBackSub(Q, lambda, b);

#if 0
   phSvdCovarGet(U,V,lambda);
#endif
   printf("Eigen Least Squares:\n");

   printf("Eigen values:\n");
   for(i = 0;i < lambda->dim;i++) {
      printf("%10.5f ",lambda->ve[i]);
   }
   printf("\n");

   printf("Best estimate of parameters %g, %g, %g, %g:\n", ca, cb, cc, cd);
   for(i = 0;i < x->dim;i++) {
      printf("%10.5f ",x->ve[i]);
   }
   printf("\n");

#if 0
   printf("covariance:\n");
   for(i = 0;i < U->m;i++) {
      for(j = 0;j < U->n;j++) {
	 printf("%10.5f ",U->me[i][j]);
      }
      printf("\n");
   }
#endif

   printf("Eigen vectors:\n");
   for(i = 0;i < Q->m;i++) {
      for(j = 0;j < Q->n;j++) {
	 printf("%10.5f ",Q->me[i][j]);
      }
      printf("\n");
   }
/*
 * Check that we can recover A from lambda and Q
 */
   mtmp = phMatNew(n,n);
   prod = phMatNew(n,n);
   for(i = 0;i < n;i++) {
      for(j = 0;j < n;j++) {
	 mtmp->me[i][j] = lambda->ve[i]*Q->me[j][i];
      }
   }
   for(i = 0;i < n;i++) {
      for(j = 0;j < n;j++) {
	 sum = 0;
	 for(k = 0;k < n;k++) {
	    sum += Q->me[i][k]*mtmp->me[k][j];
	 }
	 prod->me[i][j] = sum;
      }
   }

   printf("Q Lambda Q^T:\n");
   for(i = 0;i < prod->m;i++) {
      for(j = 0;j < prod->n;j++) {
	 printf("%10.5f ",prod->me[i][j]);
      }
      printf("\n");
   }

   if(fabs((x->ve[0] - ca)/ca) > eps ||
      fabs((x->ve[1] - cb)/cb) > eps || 
      fabs((x->ve[2] - cc)/cc) > eps || 
      fabs((x->ve[3] - cd)/cd) > eps) {
      fprintf(stderr,"TEST-ERR: error in Eigen param estimation\n");
      nerr++;
   }

   return(nerr);
}

static int
test_eigen_main(void)
{
   int nerr = 0;

   nerr += check_eigen_decomp();
   nerr += check_eigen_lsq();

   return(nerr);
}

/*****************************************************************************/

static float fcos(float x) { return(cos(x)); }
static float fsqrt(float x) { return(sqrt(x)); }

static int
test_integrate_main(void)
{
   float ans;				/* the answer */
   float a,b;				/* limits of integration */
   float eps = 1e-5;			/* desired accuracy */
   int nerr = 0;			/* number of errors */
   float truth;				/* the true answer */

   a = 0; b = 10;
   truth = 2.0/3.0*(pow(b,1.5) - pow(a,1.5));
   if(phIntegrate(fsqrt,a,b,&ans) < 0 ||
      fabs(ans - truth) > eps) {
      fprintf(stderr,"TEST-ERR: failed to find int(sqrt(x),x=%g..%g)\n",a,b);
      if(ans == 0) {
	 fprintf(stderr,"          expected %g, saw %g\n",truth,ans);
      }
      nerr++;
   }

   a = 0; b = 10;
   truth = sin(b) - sin(a);
   if(phIntegrate(fcos,a,b,&ans) < 0 || fabs(ans - truth) > eps) {
      fprintf(stderr,"TEST-ERR: failed to find int(cos(x),x=%g..%g)\n",a,b);
      if(ans == 0) {
	 fprintf(stderr,"          expected %g, saw %g\n",truth,ans);
      }
      nerr++;
   }

   return(nerr);
}

/*****************************************************************************/
/*
 * Test the chi^2 probability function code
 */
static int
test_chi2_main(void)
{
   struct {
      double x2, nu;			/* chi^2, nu */
      double prob;			/* correct value from maple */
      double lnprob;			/* ln(correct value from maple) */
   } data[] = {
      {    0,    0,  1.0, 0.0 },
      { 20.0,    5,  0.001249730562, -6.684827301},
      { 20.0,   13,  0.09521025607,  -2.351667611},
      { 20.0,   33,  0.9634658986,  -0.03721818501},
      { 30.0,   20,  0.06985366071, -2.661352786},
      { 50.0,   20,  0.0002214766381, -8.415193445},
      {100.0,   20,  0.1259608459e-11, -27.40022019},
      {200.0,   20,  0.1125347396e-30, -71.26204610},
      {500.0,   20,  0.0, -213.0721689},
      {1000.0,  20,  0.0, -456.8522278},
      {1000.0, 100,  0.0, -339.9470461},
      {1000.0, 400,  0.0, -120.7213307},
      {1000.0, 999,  0.4851314893, -.7233353128},
   };
   float errmax = 1e-5;
   double p;				/* a returned probability */
   int i;
   const int n = sizeof(data)/sizeof(data[0]);
   int nfail = 0;

   for(i = 0; i < n; i++) {
      p = phChisqProb(data[i].x2, data[i].nu, 0);
      if(fabs(p - data[i].prob) > errmax*data[i].prob) {
	 fprintf(stderr,"TEST-ERR: phChisqProb(%g,%g) == %g != %g\n",
		 data[i].x2, data[i].nu, p, data[i].prob);
	 nfail++;
      }

      p = phChisqProb(data[i].x2, data[i].nu, 1);
      if(fabs(p - data[i].lnprob) > errmax) {
	 fprintf(stderr,"TEST-ERR: phLnChisqProb(%g,%g) == %g != %g\n",
		 data[i].x2, data[i].nu, p, data[i].lnprob);
	 nfail++;
      }
   }

   return(nfail);
}

/*****************************************************************************/
/*
 * Test misc statistical routines
 */
static int
test_phFloatArrayStatsErr(const char *label,
			  const float *x,
			  const float *dx,
			  int n)
{
   float median = 0, iqr = 0, q25 = 0, q75 = 0;
   int nfail = 0;

   phFloatArrayStatsErr(x, dx, n, &median, &iqr, &q25, &q75);

   if(median < q25) {
      fprintf(stderr,"TEST-ERR: "
	      "%s: median = %g < q25 = %g\n", label, median, q25);
      nfail++;
   }
   if(q75 < median) {
      fprintf(stderr,"TEST-ERR: "
	      "%s: q75 = %g < median = %g\n", label, q75, median);
      nfail++;
   }

   return(nfail);
}

#define NSTATS 20

static int
test_stats_main(void)
{
   int nfail = 0;

   {
      char label[20];
      const int n = NSTATS;
      int i, j;
      float x[NSTATS], dx[NSTATS];
      
      srand(12345);
      for(i = 0; i < 100; i++) {
	 for(j = 0; j < n; j++) {
	    x[j] = 10*rand()/((1<<30) - 1.0);
	    dx[j] = 1*rand()/((1<<30) - 1.0);
	 }
	 
	 sprintf(label, "random %d", i);
	 nfail += test_phFloatArrayStatsErr(label, x, dx, n);
      }
   }

   {
      float x[] = {-0.0258975644, -0.0697945654};
      float dx[] = {0.0144217201, 0.00549825188};
      int n = sizeof(x)/sizeof(x[0]);
      
      assert(sizeof(dx) == sizeof(x));
      nfail += test_phFloatArrayStatsErr("Test 1", x, dx, n);
   }
   {
      float x[] = {-0.00240118033, -0.00322070206};
      float dx[] = {0.0020931398, 0.00294490578};
      int n = sizeof(x)/sizeof(x[0]);
      
      assert(sizeof(dx) == sizeof(x));
      nfail += test_phFloatArrayStatsErr("Test 2", x, dx, n);
   }

   {
      double x;
      double erf(double);

      for(x = -5; x <= 5; x += 0.01) {
	 if(fabs(erf(x) - erf_ph(x)) > 1e-7) {
	    printf("TEST-ERR: x = %g  erf = %.9f erf - erf_ph = %.10f\n",
		   x, erf(x), erf_ph(x) - erf(x));
	 }
      }
   }
   
   return(nfail);
}

/*****************************************************************************/

int
main(void)
{
   int nfail, dnfail;

   nfail = dnfail = 0;
/*
 * Test phBrentMinimum
 */
   dnfail += test_brentMinimum("x^3 - 2*x - 5", f1,
			      0.0,1.0, 0.816497);

   dnfail += test_brentMinimum("(x^3 - 2*x - 5)^2", f2,
			      2.0, 3.0, 2.0945514815);

   dnfail += test_brentMinimum("(cos(x)-x)^2 - 2 in [2,3)", f3, 
			      2.0, 3.0, 2.0);
  
   dnfail += test_brentMinimum("(cos(x)-x)^2 - 2 in [-1, 3)", f3, 
			       -1.0, 3.0, 0.7390851269);
  
   dnfail += test_brentMinimum("sqrt(|sin(x)-x|) + 1 in [-1, 3)", f4,
			       -1.0, 3.0, 0);

   if(dnfail > 0) {
      fprintf(stderr,"TEST-ERR: phBrentMinimum Failed %d tests\n",dnfail);
   }
   nfail += dnfail; dnfail = 0;
/*
 * Test phBrentMinimum
 */
   dnfail += test_zeroFind("x^3 - 2*x - 5", f1, 2.0, 3.0, 2.0945514815);
   
   dnfail += test_zeroFind("cos(x)-x in [2,3)",f5, 2.0, 3.0, -10000);

   dnfail += test_zeroFind("cos(x)-x in [-1,3)",f5, -1.0, 3.0, 0.7390851332);

   dnfail += test_zeroFind("sin(x)-x in [-1,3)",f6, -1.0, 3.0, 0.0);

   if(dnfail > 0) {
      fprintf(stderr,"TEST-ERR: phZeroFind Failed %d test\n",dnfail);
   }
   nfail += dnfail; dnfail = 0;
/*
 * Now splines
 */
   nfail += test_splines_main();
/*
 * and matrices
 */
   nfail += test_matrix_main();
/*
 * and SVD matrix stuff
 */
   nfail += test_svd_main();
/*
 * and matrix eigen value/vector stuff
 */
   nfail += test_eigen_main();
/*
 * and the integrator
 */
   nfail += test_integrate_main();
/*
 * and the chi^2 probability function
 */
   nfail += test_chi2_main();
/*
 * And statistics routines
 */
   nfail += test_stats_main();
/*
 * Return the good/bad news
 */
   return(nfail == 0 ? 0 : -1);
}
