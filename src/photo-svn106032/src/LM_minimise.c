/*
 * This code is converted to C from the function lmdif1 from the minpack
 * project:
 *     Argonne National Laboratory. Minpack Project. March 1980.
 *     Burton S. Garbow, Kenneth E. Hillstrom, Jorge J. More
 * This notice must be preserved on all copies of this file.
 *
 * The SDSS neither claims no copyright on this code, nor accepts any
 * liability for any direct or indirect losses arising from its use.
 *
 * The fortran original was converted to C by a combination of the f2c
 * utility, and RHL using emacs. All arrays are proper C arrays, and
 * all suitable arguments are passed by value
 */
#include <stdio.h>
#include <string.h>
#include <alloca.h>
#include "dervish.h"
#include "phMathUtils.h"

static int
lmdif(void (*fcn) (int m, int n, double *x, double *fvec, int *iflag),
      int m, int n, double x[], double fvec[],
      double ftol, double xtol, double gtol, double sumsq_tol,
      int maxfev, double epsfcn, double diag[], int mode,
      double factor, int *nfev, double *fjac[], int ipvt[], double qtf[],
      MAT *jac);
static double enorm(int n, double x[]);
static void
fdjac2(void (*fcn) (int m, int n, double x[], double fvec[], int *iflag),
       int m, int n, double x[], double fvec[], double *fjac[], int *iflag,
       double epsfcn, double wa[]);
static void
qrfac(int m, int n, double *a[], int pivot, int ipvt[],
      double rdiag[], double acnorm[], double wa[]);
static double
lmpar(int n, double *r[], int ipvt[], double diag[], double qtb[],
      double delta, double par, double x[], double sdiag[], double wa1[],
      double wa2[]);
static void
qrsolv(int n, double *r[], const int ipvt[], const double diag[],
       const double qtb[], double x[], double sdiag[], double wa[]);

/*
 * <AUTO EXTRACT>
 *
 * minimize the sum of the squares of m nonlinear functions in n variables
 * by a modification of the Levenberg-Marquardt algorithm. This is done by
 * using the more general least-squares solver lmdif. the user must provide a
 * subroutine which calculates the functions. the jacobian is then calculated
 * by a forward-difference approximation.
 *
 * The arguments are:
 *
 *       fcn is the name of the user-supplied subroutine which which
 *         calculates the residuals at x[] and returns them in fvec[]:
 *         void fcn(int m, int n, double x[], double fvec[], int *iflag);
 *
 *         the value of iflag should not be changed by fcn unless
 *         the user wants to terminate execution of lmdif.
 *         in this case set iflag to a negative int.
 *
 *       m is the number of data points
 * 
 *       n is the number of paramters being estimated; n <= m
 *
 *       x is an array of length n. on input x must contain
 *         an initial estimate of the parameters; on output x
 *         contains the final parameter estimate.
 *
 *       norm is the norm of the the functions evaluated at the output x.
 *
 *       nfev is the number of calls to fcn()
 *
 *       tol is the desired tolerance; termination occurs
 *         when the algorithm estimates either that the relative
 *         error in the sum of squares is at most tol or that
 *         the relative error between x and the solution is at
 *         most tol.
 *
 *       jac is the parameter's Jacobian; can be NULL if you don't
 *         want to see it. If non-Null, should be initialised as
 *            fjac = shMalloc(n*sizeof(double *));
 *            fjac[0] = shMalloc(n*n*sizeof(double));
 *            for(i = 0;i < n;i++) { fjac[i] = fvec[0] + i*n; }
 *         or equivalent.
 *
 * Note that the parameters will be varied by _small_ amounts between
 * calls to fcn; the amounts are chosen assuming that you are working
 * in double precision (i.e. don't unpack fvec[] into floats!). The most
 * likely symptoms are returning status code 9.
 *
 * The function returns a status; if the user has
 *         terminated execution, info is set to the (negative)
 *         value of iflag (see description of fcn), otherwise,
 *         info is set as follows:
 *         0  improper input parameters.
 *
 * The lowest order 3 bits describe success conditions:
 *         01  algorithm estimates that the relative error
 *            in the sum of squares is at most tol.
 *         02  algorithm estimates that the relative error
 *            between x and the solution is at most tol.
 *         04 the sum of squares is less than sumsq_tol
 *
 * Other codes indicate failure:
 *         8  number of calls to fcn has reached or
 *            exceeded 200*(n+1).
 *         9  tol is too small. no further reduction in
 *            the sum of squares is possible.
 *         10  tol is too small. no further improvement in
 *            the approximate solution x is possible.
 *         12  fvec is orthogonal to the columns of the
 *            jacobian to machine precision.
 */
int
phLMMinimise(void (*fcn) (int m, int n, double *x, double *norm, int *iflag),
	     int m,			/* number of data points */
	     int n,			/* number of parameters */
	     double x[],		/* the parameters */
	     double *norm,		/* the norm of the residuals,
					   "sqrt(chi^2)" */
	     int *nfev,			/* number of function evaluations */
	     double tol,		/* desired tolerance */
	     double sumsq_tol,		/* tolerance in norm^2 */
	     float factor,		/* scale for bound on LM steps;
					   use default if <= 0 */
	     MAT *covar)		/* parameters' covariance matrix */
{
    int mode;
    double gtol;
    double epsfcn;
    int maxfev;
    int i;
    int info;                           /* return status */
    int *ipvt;
    double **fjac;
    double *fvec;
    double *diag;
    double *qtf;
/*
 * check the input parameters for errors.
 */
    if (n <= 0 || m < n || tol < 0.0) {
       return(0);
    }
/*
 * set magic parameters
 */
    maxfev = (n + 1)*200;
    if(factor <= 0) {
       factor = 100;
    }
/* ZI: gtol is double, so take "smallest double", not "smallest float" */
#if 0
    gtol = epsfcn = EPSILON_f;
#else
    gtol = EPSILON;
    epsfcn = EPSILON_f;
#endif

    mode = 1;
/*
 * allocate memory for lmdif itself
 */
    ipvt = shMalloc(n*sizeof(int));
    fjac = shMalloc(n*sizeof(double *));
    fvec = shMalloc((m + 2*n + n*m)*sizeof(double));

    diag = fvec + m;
    qtf = diag + n;
    fjac[0] = qtf + n;
    for(i = 0;i < n;i++) {
       fjac[i] = fjac[0] + i*m;
    }
/*
 * do the work
 */
    info = lmdif(fcn, m, n, x, fvec, tol, tol, gtol, sumsq_tol,
		 maxfev, epsfcn, diag,
                 mode, factor, nfev, fjac, ipvt, qtf, covar);
    if (info == 11) {
       info = 12;
    }
    *norm = enorm(m,fvec);
/*
 * clean up
 */
    shFree(ipvt);
    shFree(fjac);
    shFree(fvec);

    return(info);
}

/*****************************************************************************/
/*
 *     This function provides double precision machine parameters
 *     when the appropriate initialisation is selected.
 *
 *       i is an int input variable set to 1, 2, or 3 which
 *         selects the desired machine parameter. If the machine has
 *         t base b digits and its smallest and largest exponents are
 *         emin and emax, respectively, then these parameters are:
 *
 *         dpmpar(1) = b**(1 - t), the machine precision,
 *
 *         dpmpar(2) = b**(emin - 1), the smallest magnitude,
 *
 *         dpmpar(3) = b**emax*(1 - b**(-t)), the largest magnitude.
 */
static double
dpmpar(int i)
{
#if 0
/*
 * Machine constants for the VAX-11.
 */
   union cvt {
      int i[2];
      double d;
   };
   union cvt macheps = { 9472, 0 };
   union cvt minmag = { 128, 0 };
   union cvt maxmag = { -32769, -1 };

   switch(i) {
    case 1:
      return(macheps.d);
    case 2:
      return(minmag.d);
    case 3:
      return(maxmag.d);
    default:
      fprintf(stderr,"Impossible condition in dpmpar()\n");
   }
#else
   switch(i) {
    case 1:
      return(EPSILON_f);
    case 2:
      return(2.23e-308);
    default:
      fprintf(stderr,"Impossible condition in dpmpar()\n");
      abort();
   }

   return(0.0);				/* NOTREACHED */
#endif
}

/*****************************************************************************/
/*
 * given an n-vector x, this function calculates the euclidean norm of x.
 * 
 *     the euclidean norm is computed by accumulating the sum of
 *     squares in three different sums. the sums of squares for the
 *     small and large components are scaled so that no overflows
 *     occur. non-destructive underflows are permitted. underflows
 *     and overflows do not occur in the computation of the unscaled
 *     sum of squares for the intermediate components.
 *     the definitions of small, intermediate and large components
 *     depend on two constants, rdwarf and rgiant. the main
 *     restrictions on these constants are that rdwarf**2 not
 *     underflow and rgiant**2 not overflow. the constants
 *     given here are suitable for every known computer.
 */
static double
enorm(int n,				/* dimension of x[] */
      double x[])			/* the vector in question */
{
   const double rdwarf = 3.834e-20;
   const double rgiant = 1.304e19;
   double xabs, x1max, x3max;
   int i, swift;
   double s1, s2, s3, agiant, floatn;
   double tmp;				/* temp used for evaluating expr^2 */
/*
 * enorm is slow. So first try a quick sloppy job, and only if that fails
 * go back to the code as grabbed from the net (and described above)
 */

   /* see if we have any dwarfs or giants around */
   swift = 0;
   for (i = 0; i < n; ++i) {
      tmp = fabs(x[i]);
      if (tmp < rdwarf || tmp > rgiant) {
          swift = 1;
      }
   }

   if (!swift) {
      s2 = 0;
      for (i = 0; i < n; ++i) {
         tmp = x[i];
         s2 += tmp*tmp;
      }
      if(s2 == s2) {			/* OK */
         return(sqrt(s2));
      }
   }
/*
 * s2 != s2, so it must be a NaN or Inf. Do it the hard way
 */
   s1 = s2 = s3 = x1max = x3max = 0.0;
   floatn = (double)n;
   agiant = rgiant / floatn;
   for (i = 0; i < n; ++i) {
	xabs = fabs(x[i]);
	if (xabs > rdwarf && xabs < agiant) {
	   s2 += xabs*xabs;		/* sum for intermediate components. */
	} else {
	   if (xabs <= rdwarf) {	/* sum for small components. */
	      if (xabs <= x3max) {
		 if (xabs != 0.0) {
		    double tmp = xabs / x3max;
		    s3 += tmp*tmp;
		 }
	      } else {
		 tmp = x3max / xabs;
		 s3 = 1.0 + s3 * tmp*tmp;
		 x3max = xabs;
	      }
	   } else {			/* sum for large components. */
	      if (xabs <= x1max) {
		 tmp = xabs/x1max;
		 s1 += tmp*tmp;
	      } else {
		 tmp = x1max / xabs;
		 s1 = 1.0 + s1 * tmp*tmp;
		 x1max = xabs;
	      }
	   }
	}
    }
/*
 * calculation of norm.
 */
    if (s1 != 0.0) {
       return(x1max * sqrt(s1 + s2 / x1max / x1max));
    }

    if (s2 == 0.0) {
       return(x3max * sqrt(s3));
    } else {
       if (s2 >= x3max) {
	  return(sqrt(s2 * (1.0 + x3max / s2 * (x3max * s3))));
       } else {
	  return(sqrt(x3max * (s2 / x3max + x3max * s3)));
       }
    }
}

/*****************************************************************************/
/*
 * compute a forward-difference approximation to the m by n jacobian
 * matrix associated with a specified problem of m functions in n variables.
 *
 * Arguments: 
 *       fcn is the name of the user-supplied subroutine which which
 *         calculates the functions at x[] and returns them in fvec[]:
 *         void fcn(int m, int n, double x[], double fvec[], int *iflag);
 *
 *         the value of iflag should not be changed by fcn unless
 *         the user wants to terminate execution of fdjac2
 *         in this case set iflag to a negative int.
 *       m is the number of functions.
 *       n is the number of variables. n must not exceed m.
 *       x is an input array of length n.
 *       fvec is an input array of length m which must contain the
 *         functions evaluated at x.
 *       fjac is an output n by m array which contains the
 *         approximation to the jacobian matrix evaluated at x.
 *       iflag is an int variable which can be used to terminate
 *         the execution of fdjac2. see description of fcn.
 *       epsfcn is an input variable used in determining a suitable
 *         step length for the forward-difference approximation. this
 *         approximation assumes that the relative errors in the
 *         functions are of the order of epsfcn. if epsfcn is less
 *         than the machine precision, it is assumed that the relative
 *         errors in the functions are of the order of the machine
 *         precision.
 *       wa is a work array of length m.
 */
static void
fdjac2(
       void (*fcn) (int m, int n, double x[], double fvec[], int *iflag),
       int m,
       int n,
       double x[],
       double fvec[],
       double *fjac[],
       int *iflag,
       double epsfcn,
       double wa[])
{
   double temp, h;
   int i, j;
   double epsmch = dpmpar(1);           /* epsmch is the machine precision. */
   double eps;
   double *fjac_j;			/* == fjac[j], unaliased for speed */
   double ih;				/* == 1/h to replace / by * */
   
   eps = sqrt(epsfcn > epsmch ? epsfcn : epsmch);
   
   for (j = 0; j < n; ++j) {
      temp = x[j];			/* save x[j] */
      h = eps * fabs(temp);
      if (h == 0.0) {
	 h = eps;
      }
      x[j] += h;
      (*fcn)(m, n, x, wa, iflag);
      if (*iflag < 0) {
	 return;
      }
      x[j] = temp;
      fjac_j = fjac[j];
      ih = 1.0/h;
      for (i = 0; i < m; ++i) {
	 fjac_j[i] = ih*(wa[i] - fvec[i]);
      }
   }
}

/*****************************************************************************/
/*
 * Calculate the covariance given the derivatives of chi^2 with respect
 * to each parameter at each data point
 */
static void
calc_covar(MAT *covar,
	   double *fjac_save,
	   int n,
	   int m)
{
   double **fjac;			/* pointers to rows of fjac_save */
   double *fjac_i, *fjac_j;		/* unaliased from fjac[i] and [j] */
   int i, j, k;
   VEC *lambda = phVecNew(n);		/* eigen values */
   MAT *Q = phMatNew(n, n);		/* eigen vectors */
   double sum;
/*
 * set up pointers
 */
   fjac = alloca(n*sizeof(double *));
   for(i = 0;i < n;i++) {
      fjac[i] = fjac_save + m*i;
   }
/*
 * evaluate curvature matrix; c.f. Numerical Recipes (2nd ed) eq. 15.5.11
 */
   for(i = 0;i < n;i++) {
      for(j = 0;j < n;j++) {
	 fjac_i = fjac[i]; fjac_j = fjac[j]; /* unalias */

	 sum = 0;
	 for(k = 0;k < m;k++) {
	    sum += fjac_i[k]*fjac_j[k];
	 }
	 covar->me[i][j] = covar->me[j][i] = 0.5*sum;
      }
   }
/*
 * and invert to get covariance.
 */
   (void)phEigen(covar, Q, lambda);
   
   for(i = 0;i < n;i++) {
      if(fabs(lambda->ve[i]) < 1e-5) {
	 lambda->ve[i] = 0;
      } else {
	 lambda->ve[i] = 1/lambda->ve[i];
      }
   }
   phEigenInvert(covar, Q, lambda);

   phMatDel(Q); phVecDel(lambda);
}

/*****************************************************************************/
/*
 *     the purpose of lmdif is to minimize the sum of the squares of
 *     m nonlinear functions in n variables by a modification of
 *     the levenberg-marquardt algorithm. the user must provide a
 *     subroutine which calculates the functions. the jacobian is
 *     then calculated by a forward-difference approximation.
 *
 * Arguments are:
 *       fcn is the name of the user-supplied subroutine which which
 *         calculates the functions at x[] and returns them in fvec[]:
 *         void fcn(int m, int n, double x[], double fvec[], int *iflag);
 *
 *         the value of iflag should not be changed by fcn unless
 *         the user wants to terminate execution of lmdif.
 *         in this case set iflag to a negative int.
 *
 *       m is the number of functions.
 *       n is the number of variables, n <= m.
 *       x is an array of length n. on input x must contain 
 *         an initial estimate of the solution vector. on output x
 *         contains the final estimate of the solution vector.
 *       fvec is an output array of length m which contains
 *         the functions evaluated at the output x.
 *       ftol is nonnegative; termination occurs when both the actual
 *         and predicted relative reductions in the sum of squares are
 *         at most ftol. therefore, ftol measures the relative error desired
 *         in the sum of squares.
 *       xtol is nonnegative; termination occurs when the relative error
 *         between two consecutive iterates is at most xtol. therefore, xtol
 *         measures the relative error desired in the approximate solution.
 *       gtol is nonnegative; termination occurs when the cosine of the angle
 *         between fvec and any column of the jacobian is at most gtol in
 *         absolute value. therefore, gtol measures the orthogonality 
 *         desired between the function vector and the columns of the jacobian.
 *       maxfev is positive; termination occurs when the number of calls to
 *         fcn is at least maxfev by the end of an iteration.
 *       epsfcn is used in determining a suitable step length for the
 *         forward-difference approximation. this approximation assumes that
 *         the relative errors in the functions are of the order of epsfcn.
 *         if epsfcn is less than the machine precision, it is assumed that
 *         the relative errors in the functions are of the order of the machine
 *         precision.
 *       diag is an array of length n. if mode = 1 (see
 *         below), diag is internally set. if mode = 2, diag
 *         must contain positive entries that serve as
 *         multiplicative scale factors for the variables.
 *       mode is an int. if mode = 1, the
 *         variables will be scaled internally. if mode = 2,
 *         the scaling is specified by the input diag. other
 *         values of mode are equivalent to mode = 1.
 *       factor is a positive input variable used in determining the
 *         initial step bound. this bound is set to the product of
 *         factor and the euclidean norm of diag*x if nonzero, or else
 *         to factor itself. in most cases factor should lie in the
 *         interval (.1,100.). 100. is a generally recommended value.
 *       nfev is set to the number of calls to fcn.
 *       fjac is an output n by m array. the upper n by n submatrix
 *         of fjac contains an upper triangular matrix r with
 *         diagonal elements of nonincreasing magnitude such that
 *
 *                t     t           t
 *               p *(jac *jac)*p = r *r,
 *
 *         where p is a permutation matrix and jac is the final
 *         calculated jacobian. column j of p is column ipvt(j)
 *         (see below) of the identity matrix. the lower trapezoidal
 *         part of fjac contains information generated during
 *         the computation of r.
 *       ipvt is an int output array of length n. ipvt
 *         defines a permutation matrix p such that jac*p = q*r,
 *         where jac is the final calculated jacobian, q is
 *         orthogonal (not stored), and r is upper triangular
 *         with diagonal elements of nonincreasing magnitude.
 *         column j of p is column ipvt(j) of the identity matrix.
 *       qtf is an output array of length n which contains
 *         the first n elements of the vector (q transpose)*fvec.
 *
 *  The function returns an error code info; if the user has
 *         terminated execution, info is set to the (negative)
 *         value of iflag. see description of fcn. otherwise,
 *         info is set as follows:
 *         0  improper input parameters.
 *
 * The lowest order 3 bits describe success conditions:
 *         01  algorithm estimates that the relative error
 *            in the sum of squares is at most tol.
 *         02  algorithm estimates that the relative error
 *            between x and the solution is at most tol.
 *         04 the sum of squares is less than sumsq_tol
 *
 * Other codes indicate failure:
 *
 *         8  number of calls to fcn has reached or
 *            exceeded maxfev.
 *         9  ftol is too small. no further reduction in
 *            the sum of squares is possible.
 *         10  xtol is too small. no further improvement in
 *            the approximate solution x is possible.
 *         11  gtol is too small. fvec is orthogonal to the
 *            columns of the jacobian to machine precision.
 *         12  the cosine of the angle between fvec and any
 *            column of the jacobian is at most gtol in
 *            absolute value.
 */
static int
lmdif(void (*fcn) (int m, int n, double *x, double *fvec, int *iflag),
      int m,
      int n,
      double x[],
      double fvec[],
      double ftol,
      double xtol,
      double gtol,
      double sumsq_tol,
      int maxfev,
      double epsfcn,
      double diag[],
      int mode,
      double factor,
      int *nfev,                        /* number of function evaluations */
      double *fjac[],
      int ipvt[],
      double qtf[],
      MAT *covar)			/* parameters' covariance matrix,
					   if non-NULL */
{
   int iter;
   double temp, temp1, temp2;
   int i, j, l, iflag;
   int info;				/* information on convergence */
   double delta;
   double *fjac_save;			/* saved value of fjac */
   double ratio = 0;			/* initialise to pacify irix 5.2 cc */
   double fnorm, gnorm;
   double pnorm, xnorm, fnorm1, actred, dirder, prered;
   double par, sum;
   double epsmch = dpmpar(1);           /* the machine precision */
   double *wa1, *wa2, *wa3, *wa4;       /* work space */

/* 
 * if gtol implies double precision, take corresponding machine accuracy 
 */
   if (gtol < EPSILON_f) {
       epsmch = EPSILON;
   }
 
/*
 * allocate scratch space
 */
   wa1 = alloca((3*n + m)*sizeof(double));
   wa2 = wa1 + n;
   wa3 = wa2 + n;
   wa4 = wa3 + n;
   if(covar == NULL) {
      fjac_save = NULL;
   } else {
      shAssert(fjac[1] == fjac[0] + m);	/* i.e. memory is contiguous */
      fjac_save = alloca(n*m*sizeof(double));
   }

   info = 0;
   iflag = 0;
   *nfev = 0;
/*
 * check the input parameters
 */
   if (n <= 0 || m < n || ftol < 0.0 || xtol < 0.0 || 
                                gtol < 0.0 || maxfev <= 0 || factor <= 0.0) {
      return(iflag < 0 ? iflag : info);
   }

   if(mode == 1) {
      ;                                 /* OK */
   } else if(mode == 2) {
      for (j = 0; j < n; ++j) {
         if (diag[j] <= 0.0) {
            return(iflag < 0 ? iflag : info);
         }
      }
   } else {
      mode = 1;                         /* all other modes are treated as 1 */
   }
/*
 * evaluate the function at the starting point
 * and calculate its norm.
 */
   iflag = 1;
   (*fcn)(m, n, x, fvec, &iflag);
   *nfev = 1;
   if(iflag < 0) {
      return(iflag);
   }
   fnorm = enorm(m, fvec);
/*
 * initialize levenberg-marquardt parameter and iteration counter.
 */
   par = 0.0;
   iter = 1;
   xnorm = delta = 0;			/* else gcc doesn't realise that
					   they are initialised for iter==1 */

   for(;;) {                            /* beginning of the outer loop. */
/*
 * calculate the jacobian matrix, or rather the n*m matrix d(chi^2_i)/d(param)
 * with chi^2_i the contribution to chi^2 from the i'th data point
 */
      iflag = 2;
      fdjac2(fcn, m, n, x, fvec, fjac, &iflag, epsfcn, wa4);
      *nfev += n;
      if(iflag < 0) {
         return(iflag);
      }
/*
 * We may want the jacobian, and I (RHL) see no way to extract it from
 * the parameters that this routine keeps; so copy the fjac vector to
 * a safe place
 */
      if(covar != NULL) {
	 memcpy(fjac_save, fjac[0], n*m*sizeof(double));
      }
/*
 * compute the qr factorization of the jacobian.
 */
      qrfac(m, n, fjac, 1, ipvt, wa1, wa2, wa3);
/*
 * on the first iteration and if mode is 1, scale according 
 * to the norms of the columns of the initial jacobian.
 */
      if (iter == 1) {
         if (mode == 1) {
            for (j = 0; j < n; ++j) {
               diag[j] = wa2[j];
               if (wa2[j] == 0.0) {
                  diag[j] = 1.0;
               }
            }
         }
/*
 * on the first iteration, calculate the norm of the scaled x
 * and initialize the step bound delta.
 */
         for (j = 0; j < n; ++j) {
            wa3[j] = diag[j] * x[j];
         }
         xnorm = enorm(n, wa3);
         delta = factor*xnorm;
         if (delta == 0.0) {
            delta = factor;
         }
      }
/*
 * form (q transpose)*fvec and store the first n components in qtf.
 */
      for (i = 0; i < m; ++i) {
         wa4[i] = fvec[i];
      }
      for (j = 0; j < n; ++j) {
         if (fjac[j][j] != 0.0) {
            sum = 0.0;
            for (i = j; i < m; ++i) {
               sum += fjac[j][i] * wa4[i];
            }
            temp = -sum / fjac[j][j];
            for (i = j; i < m; ++i) {
               wa4[i] += fjac[j][i] * temp;
            }
         }
         fjac[j][j] = wa1[j];
         qtf[j] = wa4[j];
      }
/*
 * compute the norm of the scaled gradient.
 */
      gnorm = 0.0;
      if (fnorm != 0.0) {
         for (j = 0; j < n; ++j) {
            l = ipvt[j];
            if (wa2[l] != 0.0) {
               sum = 0.0;
               for (i = 0; i <= j; ++i) {
                  sum += fjac[j][i] * (qtf[i] / fnorm);
               }
               temp1 = fabs(sum/wa2[l]);
               if(temp1 > gnorm) {
                  gnorm = temp1;
               }
            }
         }
      }
/*
 * test for convergence of the gradient norm.
 */
      if (gnorm <= gtol) {
         info = 12;
	 if(iflag >= 0 && covar != NULL) {
	    calc_covar(covar,fjac_save,n,m);
	 }
         return(iflag < 0 ? iflag : info);
      }
/*
 * rescale if necessary.
 */
      if (mode == 1) {
         for (j = 0; j < n; ++j) {
            if(wa2[j] > diag[j]) {
               diag[j] = wa2[j];
            }
         }
      }
      
      do {                                /* beginning of inner loop */
/*
 * determine the levenberg-marquardt parameter.
 */
         par = lmpar(n, fjac, ipvt, diag, qtf, delta, par, wa1, wa2, wa3, wa4);
/*
 * store the direction p and x + p. calculate the norm of p.
 */
         for (j = 0; j < n; ++j) {
            wa1[j] = -wa1[j];
            wa2[j] = x[j] + wa1[j];
            wa3[j] = diag[j] * wa1[j];
         }
         pnorm = enorm(n, wa3);
/*
 * on the first iteration, adjust the initial step bound.
 */
         if (iter == 1) {
	    if(pnorm < delta) {
	       delta = pnorm;
	    }
         }
/*
 * evaluate the function at x + p and calculate its norm.
 */
         iflag = 1;
         (*fcn)(m, n, wa2, wa4, &iflag);
         ++(*nfev);
         if (iflag < 0) {
            return(iflag);
         }
         fnorm1 = enorm(m, wa4);
/*
 * compute the scaled actual reduction.
 */
         if (0.1*fnorm1 < fnorm) {
            actred = 1.0 - pow(fnorm1 / fnorm, 2);
         } else {
            actred = -1.0;
         }
/*
 * compute the scaled predicted reduction and the scaled
 * directional derivative.
 */
         for (j = 0; j < n; ++j) {
            wa3[j] = 0.0;
            l = ipvt[j];
            temp = wa1[l];
            for (i = 0; i <= j; ++i) {
               wa3[i] += fjac[j][i] * temp;
            }
         }
         temp1 = enorm(n, wa3) / fnorm;
         temp2 = sqrt(par) * pnorm / fnorm;
         prered = pow(temp1, 2) + pow(temp2, 2) / 0.5;
         dirder = -(pow(temp1, 2) + pow(temp2, 2));
/*
 * compute the ratio of the actual to the predicted reduction.
 */
         if (prered == 0.0) {
	    ratio = 0.0;
	 } else {
            ratio = actred / prered;
         }
/*
 * update the step bound.
 */
         if (ratio > 0.25) {
            if (par == 0.0 || ratio >= 0.75) {
               delta = pnorm / 0.5;
               par = 0.5 * par;
            }
         } else {
            if (actred >= 0.0) {
               temp = 0.5;
            } else {
               temp = 0.5 * dirder / (dirder + 0.5 * actred);
            }
            if (0.1 * fnorm1 >= fnorm || temp < 0.1) {
               temp = 0.1;
            }
            temp1 = pnorm / 0.1;
            delta = temp*(delta < temp1 ? delta : temp1);
            par /= temp;
         }
/*
 * test for successful iteration.
 */
         if (ratio >= 1e-4) {
/*
 * successful iteration. update x, fvec, and their norms.
 */
            for (j = 0; j < n; ++j) {
               x[j] = wa2[j];
               wa2[j] = diag[j] * x[j];
            }
            for (i = 0; i < m; ++i) {
               fvec[i] = wa4[i];
            }
            xnorm = enorm(n, wa2);
            fnorm = fnorm1;
            ++iter;
         }
/*
 * tests for convergence.
 */
	 info = 0;
         if (fabs(actred) <= ftol && prered <= ftol && 0.5*ratio <= 1.0) {
            info |= 01;
         }
         if (delta <= xtol*xnorm) {
            info |= 02;
         }
	 if(fnorm*fnorm < sumsq_tol) {
	    info |= 04;
	 }
         if (info != 0) {
	    if(iflag >= 0 && covar != NULL) {
	       calc_covar(covar,fjac_save,n,m);
	    }
            return(iflag < 0 ? iflag : info);
         }
/*
 * tests for termination and stringent tolerances.
 */
         if (*nfev >= maxfev) {
            info = 8;
         }
         if (fabs(actred) <= epsmch && prered <= epsmch && 0.5*ratio <= 1.0) {
            info = 9;
         }
         if (delta <= epsmch * xnorm) {
            info = 10;
         }
         if (gnorm <= epsmch) {
            info = 11;
         }
         if (info != 0) {
	    if(iflag >= 0 && covar != NULL) {
	       calc_covar(covar,fjac_save,n,m);
	    }
            return(iflag < 0 ? iflag : info);
         }
      } while (ratio < 1e-4);
   }

   return(0);				/* NOTREACHED */
}

/*****************************************************************************/
/*
 *     given an m by n matrix a, an n by n nonsingular diagonal
 *     matrix d, an m-vector b, and a positive number delta,
 *     the problem is to determine a value for the parameter
 *     par such that if x solves the system
 *
 *           a*x = b ,     sqrt(par)*d*x = 0 ,
 *
 *     in the least squares sense, and dxnorm is the euclidean
 *     norm of d*x, then either par is zero and
 *
 *           (dxnorm-delta) <= 0.1*delta ,
 *
 *     or par is positive and
 *
 *           fabs(dxnorm-delta) <= 0.1*delta .
 *
 *     this subroutine completes the solution of the problem
 *     if it is provided with the necessary information from the
 *     qr factorization, with column pivoting, of a. that is, if
 *     a*p = q*r, where p is a permutation matrix, q has orthogonal
 *     columns, and r is an upper triangular matrix with diagonal
 *     elements of nonincreasing magnitude, then lmpar expects
 *     the full upper triangle of r, the permutation matrix p,
 *     and the first n components of (q transpose)*b. on output
 *     lmpar also provides an upper triangular matrix s such that
 *
 *            t   t                   t
 *           p *(a *a + par*d*d)*p = s *s .
 *
 *     s is employed within lmpar and may be of separate interest.
 *
 *     only a few iterations are generally needed for convergence
 *     of the algorithm. if, however, the limit of 10 iterations
 *     is reached, then the output par will contain the best
 *     value obtained so far.
 *
 *     The arguments are:
 *
 *       n is the order of r.
 *       r is an n by n array. on input the full upper triangle
 *         must contain the full upper triangle of the matrix r.
 *         on output the full upper triangle is unaltered, and the
 *         strict lower triangle contains the strict upper triangle
 *         (transposed) of the upper triangular matrix s.
 *       ipvt is an int input array of length n which defines the
 *         permutation matrix p such that a*p = q*r. column j of p
 *         is column ipvt(j) of the identity matrix.
 *       diag is an input array of length n which must contain the
 *         diagonal elements of the matrix d.
 *       qtb is an input array of length n which must contain the first
 *         n elements of the vector (q transpose)*b.
 *       delta is a positive input variable which specifies an upper
 *         bound on the euclidean norm of d*x.
 *       par is a nonnegative variable. on input par contains an
 *         initial estimate of the levenberg-marquardt parameter.
 *         on output par contains the final estimate.
 *       x is an output array of length n which contains the least
 *         squares solution of the system a*x = b, sqrt(par)*d*x = 0,
 *         for the output par.
 *       sdiag is an output array of length n which contains the
 *         diagonal elements of the upper triangular matrix s.
 *       wa1 and wa2 are work arrays of length n.
 */
static double
lmpar(int n,
      double *r[],
      int ipvt[],
      double diag[],
      double qtb[],
      double delta,
      double par,			/* the desired L-M parameter */
      double x[],
      double sdiag[],
      double wa1[],
      double wa2[])
{
   double parc, parl;
   int iter;
   double temp, paru;
   int i, j, l;
   double dwarf;
   int nsing;                           /* rank of matrix */
   double gnorm, fp;
   double dxnorm;
   int jm1, jp1;
   double sum;
   
   dwarf = dpmpar(2);			/* the smallest positive magnitude. */
/*
 * compute and store in x the gauss-newton direction. if the jacobian
 * is rank-deficient, obtain a least squares solution.
 */
   nsing = n;
   for (j = 0; j < n; ++j) {
      wa1[j] = qtb[j];
      if (r[j][j] == 0.0 && nsing == n) {
	 nsing = j;
      }
      if (nsing < n) {
	 wa1[j] = 0.0;
      }
   }
   for (j = nsing - 1; j >= 0; j--) {
      wa1[j] /= r[j][j];
      temp = wa1[j];
      jm1 = j - 1;
      if (jm1 >= 0) {
         for (i = 0; i <= jm1; ++i) {
            wa1[i] -= r[j][i] * temp;
         }
      }
   }
   for (j = 0; j < n; ++j) {
      l = ipvt[j];
      x[l] = wa1[j];
   }
/*
 * initialize the iteration counter. evaluate the function at the origin,
 * and test for acceptance of the gauss-newton direction.
 */
   for (j = 0; j < n; ++j) {
      wa2[j] = diag[j] * x[j];
   }
   dxnorm = enorm(n, wa2);
   fp = dxnorm - delta;
   if (fp <= 0.1 * delta) {
      return(0.0);
   }
/*
 * if the jacobian is not rank deficient, the newton step provides a
 * lower bound, parl, for the zero of the function. otherwise set this
 * bound to zero.
 */
   parl = 0.0;
   if (nsing == n) {
      for (j = 0; j < n; ++j) {
         l = ipvt[j];
         wa1[j] = diag[l] * (wa2[l] / dxnorm);
      }
      for (j = 0; j < n; ++j) {
         sum = 0.0;
         jm1 = j - 1;
         if (jm1 >= 0) {
            for (i = 0; i <= jm1; ++i) {
               sum += r[j][i] * wa1[i];
            }
         }
         wa1[j] = (wa1[j] - sum) / r[j][j];
      }
      temp = enorm(n, wa1);
      parl = fp / delta / temp / temp;
   }
/*
 * calculate an upper bound, paru, for the zero of the function.
 */
   for (j = 0; j < n; ++j) {
      sum = 0.0;
      for (i = 0; i <= j; ++i) {
         sum += r[j][i] * qtb[i];
      }
      l = ipvt[j];
      wa1[j] = sum / diag[l];
   }
   gnorm = enorm(n, wa1);
   paru = gnorm/delta;
   if (paru == 0.0) {
      if(delta < 0.1) {
         paru = dwarf/delta;
      } else {
         paru = dwarf/0.1;
      }
   }
/*
 * if the input par lies outside of the interval (parl,paru), set par to
 * the closer endpoint.
 */
   if(parl > par) {
      par = parl;
   }
   if(paru < par) {
      par = paru;
   }
   if (par == 0.0) {
      par = gnorm / dxnorm;
   }
   
   for(iter = 1;;iter++) {
/*
 * evaluate the function at the current value of par.
 */
      if (par == 0.0) {
	 temp = 0.001 * paru;
	 par = (dwarf > temp ? dwarf : temp);
      }
      temp = sqrt(par);
      for (j = 0; j < n; ++j) {
	 wa1[j] = temp * diag[j];
      }
      qrsolv(n, r, ipvt, wa1, qtb, x, sdiag, wa2);
      for (j = 0; j < n; ++j) {
	 wa2[j] = diag[j] * x[j];
      }
      dxnorm = enorm(n, wa2);
      temp = fp;
      fp = dxnorm - delta;
/*
 * if the function is small enough, accept the current value of par.
 * also test for the exceptional cases where parl is zero or the number
 * of iterations has reached 10.
 */
      if (fabs(fp) <= 0.1*delta ||
	  (parl == 0.0 && fp <= temp && temp < 0.0) ||
	  iter == 10) {
	 return(par);
      }
/*
 * compute the newton correction.
 */
      for (j = 0; j < n; ++j) {
         l = ipvt[j];
         wa1[j] = diag[l] * (wa2[l] / dxnorm);
      }
      for (j = 0; j < n; ++j) {
         wa1[j] /= sdiag[j];
         temp = wa1[j];
         jp1 = j + 1;
         if (n > jp1) {
            for (i = jp1; i < n; ++i) {
               wa1[i] -= r[j][i] * temp;
            }
         }
      }
      temp = enorm(n, wa1);
      parc = fp / delta / temp / temp;
/*
 * depending on the sign of the function, update parl or paru.
 */
      if (fp > 0.0) {
         if(par > parl) {
            parl = par;
         }
      } else if (fp < 0.0) {
         if(par < paru) {
            paru = par;
         }
      }
/*
 * compute an improved estimate for par.
 */
      temp = par + parc;
      par = (parl > temp) ? parl : temp;
   }

   return(0.0);				/* NOTREACHED */
}

/*****************************************************************************/
/*
 *     this subroutine uses householder transformations with column
 *     pivoting (optional) to compute a qr factorization of the
 *     m by n matrix a. that is, qrfac determines an orthogonal
 *     matrix q, a permutation matrix p, and an upper trapezoidal
 *     matrix r with diagonal elements of nonincreasing magnitude,
 *     such that a*p = q*r. the householder transformation for
 *     column k, k = 1,2,...,min(m,n), is of the form
 *
 *                           t
 *           i - (1/u(k))*u*u
 *
 *     where u has zeros in the first k-1 positions. the form of
 *     this transformation and the method of pivoting first
 *     appeared in the corresponding linpack subroutine.
 *
 *     Arguments are:
 *       m is the number of rows of a.
 *       n is the number of columns of a.
 *       a is an n by m array. on input a contains the matrix for
 *         which the qr factorization is to be computed. on output
 *         the strict upper trapezoidal part of a contains the strict
 *         upper trapezoidal part of r, and the lower trapezoidal
 *         part of a contains a factored form of q (the non-trivial
 *         elements of the u vectors described above).
 *       pivot controls pivoting; if true, then column pivoting is
 *         enforced, otherwise no column pivoting is done.
 *       ipvt is an int output array of length n. ipvt
 *         defines the permutation matrix p such that a*p = q*r.
 *         column j of p is column ipvt(j) of the identity matrix.
 *         if pivot is false, ipvt is not referenced.
 *       rdiag is an output array of length n which contains the
 *         diagonal elements of r.
 *       acnorm is an output array of length n which contains the
 *         norms of the corresponding columns of the input matrix a.
 *         if this information is not needed, then acnorm can coincide
 *         with rdiag.
 *       wa is a work array of length n. if pivot is false, then wa
 *         can coincide with rdiag.
 */
static void
qrfac(int m,
      int n,
      double *a[],
      int pivot,
      int ipvt[],
      double rdiag[],
      double acnorm[],
      double wa[])
{
   int kmax;
   double temp;
   int i, j, k, minmn;
   double epsmch = dpmpar(1);           /* the machine precision. */
   double ajnorm;
   int jp1;
   double sum;
/*
 * compute the initial column norms and initialize several arrays.
 */
   for (j = 0; j < n; ++j) {
      acnorm[j] = enorm(m, a[j]);
      rdiag[j] = acnorm[j];
      wa[j] = rdiag[j];
      if (pivot) {
         ipvt[j] = j;
      }
   }
/*
 * reduce a to r with householder transformations.
 */
   minmn = (m < n) ? m : n;
   for (j = 0; j < minmn; ++j) {
      if (pivot) {
/*
 * bring the column of largest norm into the pivot position.
 */
         kmax = j;
         for (k = j; k < n; ++k) {
	    if (rdiag[k] > rdiag[kmax]) {
               kmax = k;
	    }
         }
         if (j != kmax) {
            for (i = 0; i < m; ++i) {
               temp = a[j][i];
               a[j][i] = a[kmax][i];
               a[kmax][i] = temp;
            }
            rdiag[kmax] = rdiag[j];
            wa[kmax] = wa[j];
            k = ipvt[j];
            ipvt[j] = ipvt[kmax];
            ipvt[kmax] = k;
         }
      }
/*
 * compute the householder transformation to reduce the j-th column of a
 * to a multiple of the j-th unit vector.
 */
      {
	 double *a_j = a[j];
	 double iajnorm;		/* == 1/ajnorm */
	 
	 ajnorm = enorm(m - j, &a_j[j]);
	 if (ajnorm == 0.0) {
	    rdiag[j] = 0.0;
	    continue;
	 }
	 if (a_j[j] < 0.0) {
	    ajnorm = -ajnorm;
	 }
	 iajnorm = 1.0/ajnorm;
	 for (i = j; i < m; ++i) {
	    a_j[i] *= iajnorm;
	 }
	 a_j[j] += 1.0;
      }
/*
 * apply the transformation to the remaining columns and update the norms.
 */
	jp1 = j + 1;
	if (jp1 >= n) {
           rdiag[j] = -ajnorm;
           continue;
	}
	for (k = jp1; k < n; ++k) {
	    sum = 0.0;
	    for (i = j; i < m; ++i) {
		sum += a[j][i] * a[k][i];
	    }
	    temp = sum / a[j][j];
	    for (i = j; i < m; ++i) {
		a[k][i] -= temp * a[j][i];
	    }
	    if (pivot && rdiag[k] != 0.0) {
               temp = 1.0 - pow(a[k][j] / rdiag[k],2);
               if(temp <= 0.0) {
                  rdiag[k] = 0.0;
               } else {
                  rdiag[k] *= sqrt(temp);
               }
               if (0.05 * pow(rdiag[k] / wa[k],2) <= epsmch) {
                  rdiag[k] = enorm(m - j - 1, &a[k][jp1]);
                  wa[k] = rdiag[k];
               }
            }
	}
	rdiag[j] = -ajnorm;
    }
}

/*****************************************************************************/
/*
 *     given an m by n matrix a, an n by n diagonal matrix d,
 *     and an m-vector b, the problem is to determine an x which
 *     solves the system
 *
 *           a*x = b ,     d*x = 0 ,
 *
 *     in the least squares sense.
 *
 *     this subroutine completes the solution of the problem
 *     if it is provided with the necessary information from the
 *     qr factorization, with column pivoting, of a. that is, if
 *     a*p = q*r, where p is a permutation matrix, q has orthogonal
 *     columns, and r is an upper triangular matrix with diagonal
 *     elements of nonincreasing magnitude, then qrsolv expects
 *     the full upper triangle of r, the permutation matrix p,
 *     and the first n components of (q transpose)*b. the system
 *     a*x = b, d*x = 0, is then equivalent to
 *
 *                  t       t
 *           r*z = q *b ,  p *d*p*z = 0 ,
 *
 *     where x = p*z. if this system does not have full rank,
 *     then a least squares solution is obtained. on output qrsolv
 *     also provides an upper triangular matrix s such that
 *
 *            t   t               t
 *           p *(a *a + d*d)*p = s *s .
 *
 *     s is computed within qrsolv and may be of separate interest.
 *
 *     the arguments are:
 *       n is the order of r.
 *       r is an n by n array. on input the full upper triangle
 *         must contain the full upper triangle of the matrix r.
 *         on output the full upper triangle is unaltered, and the
 *         strict lower triangle contains the strict upper triangle
 *         (transposed) of the upper triangular matrix s.
 *       ipvt is an integer input array of length n which defines the
 *         permutation matrix p such that a*p = q*r. column j of p
 *         is column ipvt[j] of the identity matrix.
 *       diag is an input array of length n which must contain the
 *         diagonal elements of the matrix d.
 *       qtb is an input array of length n which must contain the first
 *         n elements of the vector (q transpose)*b.
 *       x is an output array of length n which contains the least
 *         squares solution of the system a*x = b, d*x = 0.
 *       sdiag is an output array of length n which contains the
 *         diagonal elements of the upper triangular matrix s.
 *       wa is a work array of length n.
 */
static void
qrsolv(int n,
       double *r[],
       const int ipvt[],
       const double diag[],
       const double qtb[],
       double x[],
       double sdiag[],
       double wa[]
       )
{
   double temp;
   int i, j, k, l;
   double cotan;
   int nsing;
   double qtbpj;
   int jp1, kp1;
   double tan_, cos_, sin_, sum;
/*
 * copy r and (q transpose)*b to preserve input and initialize s.
 * in particular, save the diagonal elements of r in x.
 */
   for (j = 0; j < n; ++j) {
      for (i = j; i < n; ++i) {
         r[j][i] = r[i][j];
      }
      x[j] = r[j][j];
      wa[j] = qtb[j];
   }
/*
 * eliminate the diagonal matrix d using a givens rotation.
 */
   for (j = 0; j < n; ++j) {
/*
 * prepare the row of d to be eliminated, locating the diagonal element
 * using p from the qr factorization.
 */
      l = ipvt[j];
      if (diag[l] == 0.0) {
         sdiag[j] = r[j][j];           /* store diagonal element of s, */
         r[j][j] = x[j];               /* and restore it in r[][] */
         
         continue;
      }
      for (k = j; k < n; ++k) {
         sdiag[k] = 0.0;
      }
      sdiag[j] = diag[l];
/*
 * the transformations to eliminate the row of d modify only a single element
 * of (q transpose)*b beyond the first n, which is initially 0.0.
 */
      qtbpj = 0.0;
      for (k = j; k < n; ++k) {
/*
 * determine a givens rotation which eliminates the appropriate element in
 * the current row of d.
 */
         if (sdiag[k] != 0.0) {
            if (fabs(r[k][k]) >= fabs(sdiag[k])) {
               tan_ = sdiag[k] / r[k][k];
               cos_ = 0.5 / sqrt(0.25 + 0.25 * pow(tan_,2));
               sin_ = cos_ * tan_;
            } else {
               cotan = r[k][k] / sdiag[k];
               sin_ = 0.5 / sqrt(0.25 + 0.25 * pow(cotan,2));
               cos_ = sin_ * cotan;
            }
/*
 * compute the modified diagonal element of r and the modified element
 * of ((q transpose)*b,0).
 */
            r[k][k] = cos_ * r[k][k] + sin_ * sdiag[k];
            temp = cos_ * wa[k] + sin_ * qtbpj;
            qtbpj = -sin_ * wa[k] + cos_ * qtbpj;
            wa[k] = temp;
/*
 * accumulate the tranformation in the row of s.
 */
            kp1 = k + 1;
            if (kp1 < n) {
               for (i = kp1; i < n; ++i) {
                  temp = cos_ * r[k][i] + sin_ * sdiag[i];
                  sdiag[i] = -sin_ * r[k][i] + cos_ * sdiag[i];
                  r[k][i] = temp;
               }
            }
         }
      }
      sdiag[j] = r[j][j];              /* store diagonal element of s, */
      r[j][j] = x[j];                  /* and restore it in r[][] */
   }
/*
 * solve the triangular system for z. if the system is singular, then
 * obtain a least squares solution.
 */
   nsing = n;
   for (j = 0; j < n; ++j) {
      if (sdiag[j] == 0.0 && nsing == n) {
         nsing = j;
      }
      if (nsing < n) {
         wa[j] = 0.0;
      }
   }
   if (nsing >= 1) {
      for(j = nsing - 1;j >= 0; j--) {
         sum = 0.0;
         jp1 = j + 1;
         for (i = jp1; i < nsing; ++i) {
            sum += r[j][i] * wa[i];
         }
         wa[j] = (wa[j] - sum) / sdiag[j];
      }
   }
/*
 * permute the components of z back to components of x.
 */
   for (j = 0; j < n; ++j) {
      l = ipvt[j];
      x[l] = wa[j];
   }
} 

