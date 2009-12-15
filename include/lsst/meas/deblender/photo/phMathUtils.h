#if !defined(PH_MATH_UTILS)
#define PH_MATH_UTILS 1
#include <math.h>
#include "phMeschach.h"

#define MAXLOG 709			/* ~ ln(2^1024);
					   log of largest double */
#define EPSILON	2.22045e-16		/* smallest double such that
					   1 + EPSILON != 1 */
#define SQRT_EPSILON	1.49012e-08
#define EPSILON_f	1.19209e-07	/* smallest float such that
					   1 + EPSILON_f != 1 */
#define SQRT_EPSILON_f	0.000345267

#if !defined(M_SQRT2)			/* should be in math.h */
#  define M_SQRT2    1.4142135623730950488E0
#endif

/*****************************************************************************/
/*
 * Zero of a function
 */
int
phZeroFind(double ax,			/* zero being sought is between */
	   double bx,			/*                           a and b */
	   double (*f)(double x, const void *user), /* Function under
						       investigation */
	   const void *user,		/* parameters to pass to (*f)() */
	   double tol,			/* Acceptable tolerance */
	   double *root);			/* desired root */

/*****************************************************************************/
/*
 * Minimisation. phBrentMinimum minimises a function of one variable using
 * Brent's algorithm; phLMMinimise uses the Levenberg-Marquardt method to
 * solve a non-linear least squares problem.
 */
int
phBrentMinimum(double *x,		/* O: the position of the minimum */
	       double a,		/* Left border of the range */
	       double b,		/* Right border where min is sought */
	       double (*f)(double x, const void *user), /* Function under
							   investigation */
	       const void *user,	/* parameters to pass to (*f)() */
	       int niter,		/* max number of iterations */
	       double tol);		/* Acceptable tolerance	 */
int
phLMMinimise(void (*fcn) (int m, int n, double *x, double *fvec, int *iflag),
	     int m,
	     int n,
	     double x[],
	     double *norm,
	     int *nfunc,
	     double tol,
	     double sumsq_tol,
	     float factor,		/* scale for bound on LM steps */
	     MAT *covar);

/*****************************************************************************/
/*
 * Splines
 */
typedef struct {
   int nknot;				/* number of output knots */
   float *knots;			/* positions of knots */
   float *coeffs[4];			/* and associated coefficients */
} SPLINE;

SPLINE *phSplineNew(int nknot,  float *knots,  float *coeffs[4]);
void phSplineDel(SPLINE *sp);

SPLINE *
phSplineFindTaut(const float *tau,	/* points where function's specified */
		 const float *gtau,	/* values of function at tau[] */
		 int ntau,		/* size of tau and gtau */
		 float gamma		/* control extra knots */
		 );
SPLINE *
phSplineFindTautEven(const float *tau,	/* points where function's specified */
		     const float *gtau,	/* values of function at tau[] */
		     int ntau,		/* size of tau and gtau */
		     float gamma	/* control extra knots */
		     );
SPLINE *
phSplineFindTautOdd(const float *tau,	/* points where function's specified */
		    const float *gtau,	/* values of function at tau[] */
		    int ntau,		/* size of tau and gtau */
		    float gamma		/* control extra knots */
		    );
SPLINE *
phSplineFindSmoothed(const float x[],   /* absissae */
                     const float y[],   /* ordinates */
                     const float dy[],  /* errors in y */
                     int npoint,        /* number of points in x, y, dy */
                     float s,           /* desired chisq */
                     float *chisq,	/* final chisq */
		     float *errs);	/* error estimates, if non-NULL*/
void
phSplineInterpolate(const SPLINE *sp,	/* spline coefficients/knots */
		    const float *x,	/* points to interpolate at */
		    float *y,		/* values of spline interpolation */
		    int n		/* dimension of x[] and y[] */
		    );
void
phSplineDerivative(const SPLINE *sp,	/* spline coefficients/knots */
		   const float *x,	/* points to interpolate at */
		   float *y,		/* values of spline interpolation */
		   int n		/* dimension of x[] and y[] */
		   );

int
phSplineRoots(const SPLINE *sp,		/* spline coefficients/knots */
	      float value,		/* desired value */
	      float x0,			/* specify desired range is [x0,x1) */
	      float x1,
	      float *roots,		/* the roots found */
	      int roots_size		/* dimension of roots[] */
	      );

/*****************************************************************************/
/*
 * Matrices
 */
int phMatrixInvert(float **arr, float *b, int n);
int phDMatrixInvert(double **arr, double *b, int n);

/*
 * Gamma functions and chisq probabilities
 */
double phIGamma(double a, double x);
double phIGammaC(double a, double x);
double phLnGamma(double z);
double phChisqProb(double x2, double nu, double L_soft);

/*****************************************************************************/
/*
 * a?sinh() and erfc?()
 */
double sinh_ph(double x);
double asinh_ph(double x);
double erf_ph(double x);
double erfc_ph(double x);

/*****************************************************************************/
/*
 * Numerical integration
 */
int phIntegrate(float (*f)(float),		/* integrate f() */
		float a,			/* from a ... */
		float b,			/*            to b */
		float *ans);			/* resulting in *ans */

/*****************************************************************************/
/*
 * Statistics
 */
void
phFloatArrayStats(float *arr,			/* the array */
		  int n,			/* number of elements */
		  int sorted,			/* is the array sorted? */
		  float *median,		/* desired median */
		  float *iqr,			/* desired IQR */
                  float *q25,                   /* 25% quartile */
                  float *q75);                  /* 75% quartile */
void
phFloatArrayStatsErr(const float *arr,	/* the array */
		     const float *da,	/* error in arr */
		     int n,		/* number of elements */
		     float *median,	/* desired median */
		     float *iqr,	/* desired IQR */
		     float *q25,	/* 25% quartile */
		     float *q75);	/* 75% quartile */
float
phLinearFit(const float *t,		/* "x" values */
	    const float *y_i,		/* input "y" values */
	    const float *yErr,		/* errors in y */
	    int n,			/* number of points to fit */
	    float *a,			/* estimated y-value at t == 0 */
	    float *aErr,		/* error in a */
	    float *covar,		/* covariance of a and b (not sqrt) */
	    float *b,			/* value of slope */
	    float *bErr);		/* error in b */
#endif
