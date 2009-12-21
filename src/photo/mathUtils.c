/*
 * Here are a selection of mathematical utilities used by photo, that
 * it seems foolish to get from cernlib
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <alloca.h>
#include "dervish.h"
//#include "astrotools.h"			/* for M_PI */
#include "phMathUtils.h"
#include "phConsts.h"

/*
 * <AUTO EXTRACT>
 * one-dimensional search for a function minimum over the given range
 *
 * The position of the minimum is determined with an accuracy of
 *	3*SQRT_EPSILON*abs(x) + tol.
 * The function always obtains a local minimum which coincides with
 * the global one only if a function under investigation being unimodular.
 *
 * Algorithm
 *	G.Forsythe, M.Malcolm, C.Moler, Computer methods for mathematical
 *	computations. M., Mir, 1980, p.202 of the Russian edition
 *
 *	The function makes use of the "gold section" procedure combined with
 *	the parabolic interpolation.
 *	At every step program operates three abscissae - x,v, and w.
 *	x - the last and the best approximation to the minimum location,
 *	    i.e. f(x) <= f(a) or/and f(x) <= f(b)
 *	    (if the function f has a local minimum in (a,b), then the both
 *	    conditions are fulfiled after one or two steps).
 *	v,w are previous approximations to the minimum location. They may
 *	coincide with a, b, or x (although the algorithm tries to make all
 *	u, v, and w distinct). Points x, v, and w are used to construct
 *	interpolating parabola whose minimum will be treated as a new
 *	approximation to the minimum location if the former falls within
 *	[a,b] and reduces the range enveloping minimum more efficient than
 *	the gold section procedure. 
 *	When f(x) has a second derivative positive at the minimum location
 *	(not coinciding with a or b) the procedure converges superlinearly
 *	at a rate order about 1.324
 *
 * Returns:
 *	0				routine converged
 *	1				otherwise
 *
 */
int
phBrentMinimum(
	       double *xp,		/* O: position of maximum */
	       double a,		/* Left border | of the range */
	       double b,		/* Right border| the min is seeked*/
	       double (*f)(double x, const void *user), /* Function under
							   investigation */
	       const void *user,	/* parameters to pass to (*f)() */
	       int niter,		/* max number of iterations */
	       double tol		/* Acceptable tolerance	for minimum,
					   must be > 0 */
	       )
{
   int i;				/* iteration counter */
   double x,v,w;			/* Abscissae, descr. see above */
   double fx;				/* f(x) */
   double fv;				/* f(v) */
   double fw;				/* f(w)*/
   const double r = (3.-sqrt(5.0))/2;	/* Gold section ratio */
   
   shAssert(tol > 0 && b >= a);
   
   v = a + r*(b-a);  fv = (*f)(v, user); /* First step - always gold section*/
   x = v;  w = v;
   fx=fv;  fw=fv;
   
   for(i = 0;i < niter;i++) {		/* Main iteration loop	*/
      double range = b-a;		/* Range in which the minimum
					   is sought */
      double middle_range = (a+b)/2;
      double tol_act = SQRT_EPSILON*fabs(x) + tol/3; /* Actual tolerance */
      double new_step;      		/* Step at this iteration */
	
      if(fabs(x-middle_range) + range/2 <= 2*tol_act) {
	 *xp = x;
	 return(0);			/* Acceptable approx. is found */
      }
	
      /* Obtain the gold section step	*/
      new_step = r*(x < middle_range ? b-x : a-x );
	
      /* Decide if the interpolation can be tried */
      if(fabs(x-w) >= tol_act) {	/* If x and w are distinct, try
					   interpolatiom */
	 register double p,q; 		/* Interpolation step is calculated
					   as p/q; division is delayed until
					   the last moment */
	 register double t;
	 
	 t = (x-w)*(fx-fv);
	 q = (x-v)*(fx-fw);
	 p = (x-v)*q - (x-w)*t;
	 q = 2*(q-t);
	 
	 if(q > (double)0) {		/* make q positive and assign */
	    p = -p;			/* possible minus to p */
	 } else {				
	    q = -q;
	 }
/*
 * If x+p/q falls in [a,b] not too close to a and b, and isn't too large,
 * it is accepted.
 *
 * If p/q is too large then the gold section procedure can reduce [a,b]
 * range to more extent
 */
	 
	 if(fabs(p) < fabs(new_step*q) && p > q*(a-x+2*tol_act) &&
					  p < q*(b-x-2*tol_act)) {
	    new_step = p/q;
	 }
      }
      
      if(fabs(new_step) < tol_act) {	/* Adjust the step to be not less
					   than tolerance */
	 new_step = (new_step > (double)0) ? tol_act : -tol_act;
      }
      
/*
 * Obtain the next approximation to min and reduce the enveloping range
 */
      {
	 register double t = x + new_step; /* guess for the min */
	 register double ft = (*f)(t, user);
	 if(ft <= fx) {			/* t is a better approximation */
	    if(t < x) {			/* Reduce the range so that */
	       b = x;			/* t falls within it */
	    } else {
	       a = x;
	    }
	    
	    v = w;  w = x;  x = t;	/* Assign the best approx to x */
	    fv=fw;  fw=fx;  fx=ft;
	 } else {			/* x remains the better approx */
	    if(t < x) {		/* Reduce the range enclosing x */
	       a = t;                   
	    } else {
	       b = t;
	    }
	    
	    if(ft <= fw || w==x) {
	       v = w;  w = t;
	       fv=fw;  fw=ft;
	    } else if(ft<=fv || v==x || v==w) {
	       v = t;
	       fv=ft;
	    }
	 }
      }
   }

   *xp = x;
   return(-1);				/* no convergence */
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Find a function's root within a specified range
 *
 * Output
 *      0		A root is found
 *      -1		otherwise
 *
 * An estimate for the root (if it exists) is found with accuracy
 * 4*EPSILON*abs(x) + tol
 *
 * Algorithm
 *	G.Forsythe, M.Malcolm, C.Moler, Computer methods for mathematical
 *	computations. M., Mir, 1980, p.180 of the Russian edition
 *
 * The function makes use of the bisection procedure combined with
 * the linear or quadratic inverse interpolation.
 * At every step program operates on three abscissae - a, b, and c.
 * b - the last and the best approximation to the root
 * a - the last but one approximation
 * c - the last but one or even earlier approximation than a that
 * 	1) |f(b)| <= |f(c)|
 * 	2) f(b) and f(c) have opposite signs, i.e. b and c confine
 * 	   the root
 * At every step selects one of the two new approximations, the
 * former being obtained by the bisection procedure and the latter
 * resulting in the interpolation (if a,b, and c are all different
 * the quadratic interpolation is utilized, otherwise the linear one).
 * If the latter (i.e. obtained by the interpolation) point is 
 * reasonable (i.e. lies within the current interval [b,c] not being
 * too close to the boundaries) it is accepted. The bisection result
 * is used in the other case. Therefore, the range of uncertainty is
 * ensured to be reduced at least by the factor 1.6
 */
int
phZeroFind(double ax,			/* zero being sought is between */
	   double bx,			/*                           a and b */
	   double (*f)(double x, const void *user), /* Function under
						    investigation */
	   const void *user,		/* parameters to pass to (*f)() */
	   double tol,			/* Acceptable tolerance. If specified
					   as 0.0, the root will be found
					   to machine precision */
	   double *root)		/* the desired root */
{
   double a,b,c;			/* Abscissae, descr. see above */
   double fa;				/* f(a) */
   double fb;				/* f(b) */
   double fc;				/* f(c) */

   a = ax; fa = (*f)(a, user);
   b = bx; fb = (*f)(b, user);
   c = a; fc = fa;

   if(fa*fb > 0) {			/* root isn't bracketed */
      return(-1);
   }
   
   for(;;) {				/* main iteration loop */
      double prev_step = b-a;		/* Distance from the last but one
					   to the last approximation */
      double tol_act;			/* Actual tolerance */
      double p;      			/* Interpolation step is calculated */
      double q;      			/* in the form p/q; division is
					   delayed until the last moment */
      double new_step;      		/* Step at this iteration */
      
      if(fabs(fc) < fabs(fb)) {		/* make b the best approximation */
	 a = b;  b = c;  c = a;
	 fa=fb;  fb=fc;  fc=fa;
      }
      tol_act = 2*EPSILON*fabs(b) + tol/2;
      new_step = (c-b)/2;
      
      if(fb == (double)0.0) {		/* found it */
	 *root = b;
	 return(0);
      }
      if(fabs(new_step) <= tol_act) {
	 *root = b;
	 return(0);			/* Acceptable approx. is found */
      }
/*
 * Decide if the interpolation can be tried; do so if prev_step was large
 * enough and was in true direction
 */
      if(fabs(prev_step) >= tol_act && fabs(fa) > fabs(fb)) {
	 register double t1,cb,t2;
	 cb = c-b;
	 if(a == c) {			/* We have only two distinct points,
					   so use only linear interpolation */
	    t1 = fb/fa;
	    p = cb*t1;
	    q = 1.0 - t1;
	 } else {			/* Quadratic inverse interpolation */
	    q = fa/fc;  t1 = fb/fc;  t2 = fb/fa;
	    p = t2 * ( cb*q*(q-t1) - (b-a)*(t1-1.0) );
	    q = (q-1.0) * (t1-1.0) * (t2-1.0);
	 }
/*
 * if p was calculated with the opposite sign, make p positive
 * and assign minus to q
 */
	 if(p > (double)0.0) {
	    q = -q;
	 } else {
	    p = -p;
	 }
/*
 * Accept the new estimate b + p/q if it falls in [b,c] the step
 * isn't too large; if p/q is too large then bisection does a better
 * job of reducing the range [b,c] 
 */
	 if(p < (0.75*cb*q-fabs(tol_act*q)/2) && p < fabs(prev_step*q/2)) {
	    new_step = p/q;
	 }
      }
/*
 * Adjust the step to be not less than tolerance
 */
      if(fabs(new_step) < tol_act) {
	 new_step = (new_step > (double)0.0) ? tol_act : -tol_act;
      }
      
      a = b;  fa = fb;			/* Save the previous approx. */
      b += new_step;  fb = (*f)(b, user); /* Do step to a new approxim. */
/*
 * if the step is such that the zero lies in [a,b], adjust definitions
 * to make it lie in [b,c]
 */
      if((fb > 0 && fc > 0) || (fb < 0 && fc < 0) ) {
	 c = a;  fc = fa;
      }
   }

   return(0);				/* NOTREACHED */
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * 
 * Adapted from
 *      A Practical Guide to Splines
 * by
 *      C. de Boor (N.Y. : Springer-Verlag, 1978)
 * (His routine tautsp converted to C by Robert Lupton)
 *
 * Construct cubic spline interpolant to given data.
 *
 * If gamma .gt. 0., additional knots are introduced where needed to
 * make the interpolant more flexible locally. this avoids extraneous
 * inflection points typical of cubic spline interpolation at knots to
 * rapidly changing data. Values for gamma are:
 *
 *          = 0., no additional knots
 *          in (0.,3.), under certain conditions on the given data at
 *                points i-1, i, i+1, and i+2, a knot is added in the
 *                i-th interval, i=1,...,ntau-3. see description of method
 *                below. the interpolant gets rounded with increasing
 *                gamma. a value of  2.5  for gamma is typical.
 *          in (3.,6.), same , except that knots might also be added in
 *                intervals in which an inflection point would be permitted.
 *                a value of  5.5  for gamma is typical.
 *
 * Returns:
 *          a SPLINE giving knots and coefficients, if all was OK
 *          NULL otherwise (and pushes message on error stack)
 *
 *  ------  m e t h o d  ------
 *  on the i-th interval, (tau[i], tau[i+1]), the interpolant is of the
 *  form
 *  (*)  f(u(x)) = a + b*u + c*h(u,z) + d*h(1-u,1-z) ,
 *  with u = u(x) = (x - tau[i])/dtau[i].  Here,
 *       z = z(i) = addg(i+1)/(addg(i) + addg(i+1))
 *  (= .5, in case the denominator vanishes). with
 *       ddg(j) = dg(j+1) - dg(j),
 *       addg(j) = abs(ddg(j)),
 *       dg(j) = divdif(j) = (gtau[j+1] - gtau[j])/dtau[j]
 *  and
 *       h(u,z) = alpha*u**3 + (1 - alpha)*(max(((u-zeta)/(1-zeta)),0)**3
 *  with
 *       alpha(z) = (1-gamma/3)/zeta
 *       zeta(z) = 1 - gamma*min((1 - z), 1/3)
 *  thus, for 1/3 .le. z .le. 2/3,  f  is just a cubic polynomial on
 *  the interval i. otherwise, it has one additional knot, at
 *         tau[i] + zeta*dtau[i] .
 *  as  z  approaches  1, h(.,z) has an increasingly sharp bend  near 1,
 *  thus allowing  f  to turn rapidly near the additional knot.
 *     in terms of f(j) = gtau[j] and
 *       fsecnd[j] = 2*derivative of f at tau[j],
 *  the coefficients for (*) are given as
 *       a = f(i) - d
 *       b = (f(i+1) - f(i)) - (c - d)
 *       c = fsecnd[i+1]*dtau[i]**2/hsecnd(1,z)
 *       d = fsecnd[i]*dtau[i]**2/hsecnd(1,1-z)
 *  hence can be computed once fsecnd[i],i=0,...,ntau-1, is fixed.
 *
 *   f  is automatically continuous and has a continuous second derivat-
 *  ive (except when z = 0 or 1 for some i). we determine fscnd(.) from
 *  the requirement that also the first derivative of  f  be continuous.
 *  in addition, we require that the third derivative be continuous
 *  across  tau[1] and across tau[ntau-2] . this leads to a strictly
 *  diagonally dominant tridiagonal linear system for the fsecnd[i]
 *  which we solve by gauss elimination without pivoting.
 *
 *  There must be at least 4 interpolation points for us to fit a taut
 * cubic spline, but if you provide fewer we'll fit a quadratic or linear
 * polynomial (but you must provide at least 2)
 *
 * See also phSplineFindTautEven and phSplineFindTautOdd for splining
 * functions with known symmetries
 */
SPLINE *
phSplineFindTaut(
		 const float *tau,	/* points where function's specified;
					   must be strictly increasing */
		 const float *gtau,	/* values of function at tau[] */
		 int ntau,		/* size of tau and gtau, must be >= 2*/
		 float gamma		/* control extra knots */
       )
{
    float *s[6];			/* scratch space */
    float zeta, temp, c, d;
    int i, j;
    float alpha, z, denom, ratio, entry_, factr2, onemg3;
    float z_half;			/* == z - 0.5 */
    float entry3, divdif, factor;
    int method;
    float onemzt, zt2, del;
    float *knots, *coeffs[4];		/* returned as part of SPLINE */
    int nknot;				/* number of knots */

    if(ntau < 4) {			/* use a single quadratic */
       if(ntau < 2) {
	  shErrStackPush("phSplineFindTaut: ntau = %d, should be >= 2\n",ntau);
	  return(NULL);
       }
       nknot = ntau;
       knots = shMalloc(nknot*sizeof(float));
       coeffs[0] = shMalloc(4*nknot*sizeof(float));
       coeffs[1] = coeffs[0] + nknot;
       coeffs[2] = coeffs[1] + nknot;
       coeffs[3] = coeffs[2] + nknot;

       knots[0] = tau[0];
       for(i = 1;i < nknot;i++) {
	  knots[i] = tau[i];
	  if(tau[i - 1] >= tau[i]) {
	     shErrStackPush("phSplineFindTaut: "
			    "point %d and the next, %f %f, are out of order\n",
			    i - 1,tau[i - 1],tau[i]);
	     shFree(knots); shFree(coeffs[0]);
	     return(NULL);
	  }
       }

       if(ntau == 2) {
	  coeffs[0][0] = gtau[0];
	  coeffs[1][0] = (gtau[1] - gtau[0])/(tau[1] - tau[0]);
	  coeffs[2][0] = coeffs[3][0] = 0;

	  coeffs[0][1] = gtau[1];
	  coeffs[1][1] = (gtau[1] - gtau[0])/(tau[1] - tau[0]);
	  coeffs[2][1] = coeffs[3][1] = 0;
       } else {				/* must be 3 */
	  float tmp = (tau[2]-tau[0])*(tau[2]-tau[1])*(tau[1]-tau[0]);
	  coeffs[0][0] = gtau[0];
	  coeffs[1][0] = ((gtau[1] - gtau[0])*pow(tau[2] - tau[0],2) -
			  (gtau[2] - gtau[0])*pow(tau[1] - tau[0],2))/tmp;
	  coeffs[2][0] = -2*((gtau[1] - gtau[0])*(tau[2] - tau[0]) -
			    (gtau[2] - gtau[0])*(tau[1] - tau[0]))/tmp;
	  coeffs[3][0] = 0;

	  coeffs[0][1] = gtau[1];
	  coeffs[1][1] = coeffs[1][0] + (tau[1] - tau[0])*coeffs[2][0];
	  coeffs[2][1] = coeffs[2][0];
	  coeffs[3][1] = 0;

	  coeffs[0][2] = gtau[2];
	  coeffs[1][2] = coeffs[1][0] + (tau[2] - tau[0])*coeffs[2][0];
	  coeffs[2][2] = coeffs[2][0];
	  coeffs[3][2] = 0;
       }
       
       return(phSplineNew(nknot,knots,coeffs));
    }
/*
 * Allocate scratch space
 *     s[0][...] = dtau = tau(.+1) - tau
 *     s[1][...] = diag = diagonal in linear system
 *     s[2][...] = u = upper diagonal in linear system
 *     s[3][...] = r = right side for linear system (initially)
 *               = fsecnd = solution of linear system , namely the second
 *                          derivatives of interpolant at  tau
 *     s[4][...] = z = indicator of additional knots
 *     s[5][...] = 1/hsecnd(1,x) with x = z or = 1-z. see below.
 */
    s[0] = shMalloc(6*ntau*sizeof(float));
    for(i = 0;i < 6;i++) {
       s[i] = s[0] + i*ntau;
    }
/*
 * Construct delta tau and first and second (divided) differences of data 
 */
    for (i = 0; i < ntau - 1; i++) {
	s[0][i] = tau[i + 1] - tau[i];
	if(s[0][i] <= 0.) {
	   shErrStackPush("phSplineFindTaut: "
			  "point %d and the next, %f %f, are out of order\n",
			  i,tau[i],tau[i+1]);
	   shFree(s[0]);
           return(NULL);
        }
	s[3][i + 1] = (gtau[i + 1] - gtau[i])/s[0][i];
    }
    for (i = 1; i < ntau - 1; ++i) {
       s[3][i] = s[3][i + 1] - s[3][i];
    }

/*
 * Construct system of equations for second derivatives at tau. At each
 * interior data point, there is one continuity equation, at the first
 * and the last interior data point there is an additional one for a
 * total of ntau equations in ntau unknowns.
 */
    s[1][1] = s[0][0]/3;

    if(gamma <= 0) {
	method = 1;
    } else if(gamma > 3) {
       gamma -= 3;
       method = 3;
    } else {
       method = 2;
    }
    onemg3 = 1 - gamma/3;

    nknot = ntau;			/* count number of knots */
/*
 * Some compilers don't realise that the flow of control always initialises
 * these variables; so initialise them to placate e.g. gcc
 */
    zeta = alpha = ratio = 0;
    z_half = entry3 = factor = onemzt = zt2 = 0;
    for(i = 1;i < ntau - 2;i++) {
 /*
 * construct z[i] and zeta[i]
 */
       z = .5;
       if((method == 2 && s[3][i]*s[3][i + 1] >= 0) || method == 3) {
          temp = fabs(s[3][i + 1]);
          denom = fabs(s[3][i]) + temp;
          if(denom != 0) {
             z = temp/denom;
             if (fabs(z - 0.5) <= 1.0/6.0) {
                z = 0.5;
             }
          }
       }

       if (fabs(1 - z) < EPSILON_f) {
	   z = 1.0;
       }
       s[4][i] = z;
/*
  set up part of the i-th equation which depends on the i-th interval
  */
       z_half = z - 0.5;
       if(z_half < 0) {
	  zeta = gamma*z;
	  onemzt = 1 - zeta;
	  zt2 = zeta*zeta;
	  temp = onemg3/onemzt;
	  alpha = (temp < 1 ? temp : 1);
	  factor = zeta/(alpha*(zt2 - 1) + 1);
	  s[5][i] = zeta*factor/6;
	  s[1][i] += s[0][i]*((1 - alpha*onemzt)*factor/2 - s[5][i]);
/*
 * if z = 0 and the previous z = 1, then d[i] = 0. Since then
 * also u[i-1] = l[i+1] = 0, its value does not matter. Reset
 * d[i] = 1 to insure nonzero pivot in elimination.
 */
	  if (s[1][i] <= 0.) {
	     s[1][i] = 1;
	  }
	  s[2][i] = s[0][i]/6;

	  if(z != 0) {			/* we'll get a new knot */
	     nknot++;
	  }
       } else if(z_half == 0) {
	  s[1][i] += s[0][i]/3;
	  s[2][i] = s[0][i]/6;
       } else {
	  onemzt = gamma*(1 - z);
	  zeta = 1 - onemzt;
	  temp = onemg3/zeta;
	  alpha = (temp < 1 ? temp : 1);
	  factor = onemzt/(1 - alpha*zeta*(onemzt + 1));
	  s[5][i] = onemzt*factor/6;
	  s[1][i] += s[0][i]/3;
	  s[2][i] = s[5][i]*s[0][i];

	  if(onemzt != 0) {			/* we'll get a new knot */
	     nknot++;
	  }
       }
       if (i == 1) {
	  s[4][0] = 0.5;
/*
 * the first two equations enforce continuity of the first and of
 * the third derivative across tau[1].
 */
	  s[1][0] = s[0][0]/6;
	  s[2][0] = s[1][1];
	  entry3 = s[2][1];
	  if(z_half < 0) {
	     factr2 = zeta*(alpha*(zt2 - 1.) + 1.)/
	       (alpha*(zeta*zt2 - 1.) + 1.);
	     ratio = factr2*s[0][1]/s[1][0];
	     s[1][1] = factr2*s[0][1] + s[0][0];
	     s[2][1] = -factr2*s[0][0];
	  } else if (z_half == 0) {
	     ratio = s[0][1]/s[1][0];
	     s[1][1] = s[0][1] + s[0][0];
	     s[2][1] = -s[0][0];
	  } else {
	     ratio = s[0][1]/s[1][0];
	     s[1][1] = s[0][1] + s[0][0];
	     s[2][1] = -s[0][0]*6*alpha*s[5][1];
	  }
/*
 * at this point, the first two equations read
 *              diag[0]*x0 +    u[0]*x1 + entry3*x2 = r[1]
 *       -ratio*diag[0]*x0 + diag[1]*x1 +   u[1]*x2 = 0
 * set r[0] = r[1] and eliminate x1 from the second equation
 */
	  s[3][0] = s[3][1];

	  s[1][1] += ratio*s[2][0];
	  s[2][1] += ratio*entry3;
	  s[3][1] = ratio*s[3][1];
       } else {
/*
 * the i-th equation enforces continuity of the first derivative
 * across tau[i]; it reads
 *         -ratio*diag[i-1]*x_{i-1} + diag[i]*x_i + u[i]*x_{i+1} = r[i]
 * eliminate x_{i-1} from this equation
 */
	  s[1][i] += ratio*s[2][i - 1];
	  s[3][i] += ratio*s[3][i - 1];
       }
/*
 * Set up the part of the next equation which depends on the i-th interval.
 */
       if(z_half < 0) {
	  ratio = -s[5][i]*s[0][i]/s[1][i];
	  s[1][i + 1] = s[0][i]/3;
       } else if(z_half == 0) {
	  ratio = -(s[0][i]/6)/s[1][i];
	  s[1][i + 1] = s[0][i]/3;
       } else {
	  ratio = -(s[0][i]/6)/s[1][i];
	  s[1][i + 1] = s[0][i]*((1 - zeta*alpha)*factor/2 - s[5][i]);
       }
    }

    s[4][ntau - 2] = 0.5;
/*
 * last two equations, which enforce continuity of third derivative and
 * of first derivative across tau[ntau - 2]
 */
    entry_ = ratio*s[2][ntau - 3] + s[1][ntau - 2] + s[0][ntau - 2]/3;
    s[1][ntau - 1] = s[0][ntau - 2]/6;
    s[3][ntau - 1] = ratio*s[3][ntau - 3] + s[3][ntau - 2];
    if (z_half < 0) {
       ratio = s[0][ntau - 2]*6*s[5][ntau - 3]*alpha/s[1][ntau - 3];
       s[1][ntau - 2] = ratio*s[2][ntau - 3] + s[0][ntau - 2] + s[0][ntau - 3];
       s[2][ntau - 2] = -s[0][ntau - 3];
    } else if (z_half == 0) {
       ratio = s[0][ntau - 2]/s[1][ntau - 3];
       s[1][ntau - 2] = ratio*s[2][ntau - 3] + s[0][ntau - 2] + s[0][ntau - 3];
       s[2][ntau - 2] = -s[0][ntau - 3];
    } else {
       factr2 = onemzt*(alpha*(onemzt*onemzt - 1) + 1)/
	 (alpha*(onemzt*onemzt*onemzt - 1) + 1);
       ratio = factr2*s[0][ntau - 2]/s[1][ntau - 3];
       s[1][ntau - 2] = ratio*s[2][ntau - 3] + factr2*s[0][ntau - 3] +
								s[0][ntau - 2];
       s[2][ntau - 2] = -factr2*s[0][ntau - 3];
    }
/*
 * at this point, the last two equations read
 *             diag[i]*x_i +      u[i]*x_{i+1} = r[i]
 *      -ratio*diag[i]*x_i + diag[i+1]*x_{i+1} = r[i+1]
 *     eliminate x_i from last equation
 */
    s[3][ntau - 2] = ratio*s[3][ntau - 3];
    ratio = -entry_/s[1][ntau - 2];
    s[1][ntau - 1] += ratio*s[2][ntau - 2];
    s[3][ntau - 1] += ratio*s[3][ntau - 2];

/*
 * back substitution
 */
    shAssert(i == ntau - 2);
    s[3][ntau - 1] /= s[1][ntau - 1];
    do {
       s[3][i] = (s[3][i] - s[2][i]*s[3][i + 1])/s[1][i];
    } while(--i > 0);

    s[3][0] = (s[3][0] - s[2][0]*s[3][1] - entry3*s[3][2])/s[1][0];
/*
 * construct polynomial pieces; first allocate space for the coefficients
 */
#if 1
/*
 * Start by counting the knots
 */
    j = nknot;
    nknot = ntau;
    
    for(i = 0; i < ntau - 1; ++i) {
       z = s[4][i];
       if((z < 0.5 && z != 0.0) || (z > 0.5 && (1 - z) != 0.0)) {
	   nknot++;
       }
    }
    shAssert(nknot == j);
#endif
    knots = shMalloc(nknot*sizeof(float));
    coeffs[0] = shMalloc(4*nknot*sizeof(float));
    coeffs[1] = coeffs[0] + nknot;
    coeffs[2] = coeffs[1] + nknot;
    coeffs[3] = coeffs[2] + nknot;

    knots[0] = tau[0];
    j = 0;
    for(i = 0; i < ntau - 1; ++i) {
	coeffs[0][j] = gtau[i];
	coeffs[2][j] = s[3][i];
	divdif = (gtau[i + 1] - gtau[i])/s[0][i];
	z = s[4][i];
	z_half = z - 0.5;
	if (z_half < 0) {
	   if (z == 0) {
	      coeffs[1][j] = divdif;
	      coeffs[2][j] = 0;
	      coeffs[3][j] = 0;
	   } else {
	      zeta = gamma*z;
	      onemzt = 1 - zeta;
	      c = s[3][i + 1]/6;
	      d = s[3][i]*s[5][i];
	      j++;
	      
	      del = zeta*s[0][i];
	      knots[j] = tau[i] + del;
	      zt2 = zeta*zeta;
	      temp = onemg3/onemzt;
	      alpha = (temp < 1 ? temp : 1);
	      factor = onemzt*onemzt*alpha;
	      temp = s[0][i];
	      coeffs[0][j] = gtau[i] + divdif*del +
                temp*temp*(d*onemzt*(factor - 1) + c*zeta*(zt2 - 1));
	      coeffs[1][j] = divdif +
				    s[0][i]*(d*(1 - 3*factor) + c*(3*zt2 - 1));
	      coeffs[2][j] = (d*alpha*onemzt + c*zeta)*6;
	      coeffs[3][j] = (c - d*alpha)*6/s[0][i];
	      if(del*zt2 == 0) {
		 coeffs[1][j - 1] = 0;	/* would be NaN in an */
		 coeffs[3][j - 1] = 0;	/*              0-length interval */
	      } else {
		 coeffs[3][j - 1] = coeffs[3][j] - d*6*(1 - alpha)/(del*zt2);
		 coeffs[1][j - 1] = coeffs[1][j] -
				   del*(coeffs[2][j] - del/2*coeffs[3][j - 1]);
	      }
	   }
	} else if (z_half == 0) {
	   coeffs[1][j] = divdif - s[0][i]*(s[3][i]*2 + s[3][i + 1])/6;
	   coeffs[3][j] = (s[3][i + 1] - s[3][i])/s[0][i];
	} else {
	   onemzt = gamma*(1 - z);
	   if (onemzt == 0) {
	      coeffs[1][j] = divdif;
	      coeffs[2][j] = 0;
	      coeffs[3][j] = 0;
	   } else {
	      zeta = 1 - onemzt;
	      temp = onemg3/zeta;
	      alpha = (temp < 1 ? temp : 1);
	      c = s[3][i + 1]*s[5][i];
	      d = s[3][i]/6;
	      del = zeta*s[0][i];
	      knots[j + 1] = tau[i] + del;
	      coeffs[1][j] = divdif - s[0][i]*(2*d + c);
	      coeffs[3][j] = (c*alpha - d)*6/s[0][i];
	      j++;

	      coeffs[3][j] = coeffs[3][j - 1] +
			      (1 - alpha)*6*c/(s[0][i]*(onemzt*onemzt*onemzt));
	      coeffs[2][j] = coeffs[2][j - 1] + del*coeffs[3][j - 1];
	      coeffs[1][j] = coeffs[1][j - 1] +
			       del*(coeffs[2][j - 1] + del/2*coeffs[3][j - 1]);
	      coeffs[0][j] = coeffs[0][j - 1] +
                del*(coeffs[1][j - 1] +
			    del/2*(coeffs[2][j - 1] + del/3*coeffs[3][j - 1]));
	   }
	}

	j++;
	knots[j] = tau[i + 1];
    }
    
    shFree(s[0]);
/*
 * If there are discontinuities some of the knots may be at the same
 * position; in this case we generated some NaNs above. As they only
 * occur for 0-length segments, it's safe to replace them by 0s
 *
 * Due to the not-a-knot condition, the last set of coefficients isn't
 * needed (the last-but-one is equivalent), but it makes the book-keeping
 * easier if we _do_ generate them
 */
    del = tau[ntau - 1] - knots[nknot - 2];
    
    coeffs[0][nknot - 1] = coeffs[0][nknot - 2] +
      del*(coeffs[1][nknot - 2] + del*(coeffs[2][nknot - 2]/2 +
				       del*coeffs[3][nknot - 2]/6));
    coeffs[1][nknot - 1] = coeffs[1][nknot - 2] +
      del*(coeffs[2][nknot - 2] + del*coeffs[3][nknot - 2]/2);
    coeffs[2][nknot - 1] = coeffs[2][nknot - 2] + del*coeffs[3][nknot - 2];
    coeffs[3][nknot - 1] = coeffs[3][nknot - 2];

    shAssert(j + 1 == nknot);
    return(phSplineNew(nknot,knots,coeffs));
}

/*****************************************************************************/
/*
 * Here's the code to fit smoothing splines through data points
 */
typedef float REAL642;			/* floating type; originally double */

static void
spcof1(const REAL642 x[], REAL642 avh, const REAL642 y[], const REAL642 dy[],
       int n, REAL642 p, REAL642 q, REAL642 a[], REAL642 *c[3], REAL642 u[],
       const REAL642 v[]);

static void
sperr1(const REAL642 x[], REAL642 avh, const REAL642 dy[], int n,
       REAL642 *r[3], REAL642 p, REAL642 var, REAL642 se[]);

static REAL642
spfit1(const REAL642 x[], REAL642 avh, const REAL642 dy[], int n,
       REAL642 rho, REAL642 *p, REAL642 *q, REAL642 var, REAL642 stat[],
       const REAL642 a[], REAL642 *c[3], REAL642 *r[3], REAL642 *t[2],
       REAL642 u[], REAL642 v[]);

static REAL642
spint1(const REAL642 x[], REAL642 *avh, const REAL642 y[], REAL642 dy[], int n,
       REAL642 a[], REAL642 *c[3], REAL642 *r[3], REAL642 *t[2]);

/*
 * Algorithm 642 collected algorithms from ACM.
 *     Algorithm appeared in Acm-Trans. Math. Software, vol.12, no. 2,
 *     Jun., 1986, p. 150.
 *
 * Translated from fortran by a combination of f2c and RHL
 *
 *   Author              - M.F.Hutchinson
 *                         CSIRO Division of Mathematics and Statistics
 *                         P.O. Box 1965
 *                         Canberra, ACT 2601
 *                         Australia
 *
 *   latest revision     - 15 August 1985
 *
 *   purpose             - cubic spline data smoother
 *
 * arguments:
 *   x: array of length n containing the abscissae of the n data points
 * (x(i),f(i)) i=0..n-1.  x must be ordered so that x(i) .lt. x(i+1)
 *
 *   f: array of length n containing the ordinates (or function values)
 * of the n data points
 *
 *   df: array of length n; df[i] is the relative standard deviation of
 * the error associated with data point i; each df[] must be positive.
 * If the absolute standard deviations are known, these should be provided
 * in df and the error variance parameter var (see below) should be set
 * to 1; if the relative standard deviations are unknown, set each df[i]=1.
 *
 *   n: number of data points (input). Must be >= 3.
 *
 *   y,c: spline coefficients (output). y is an array of length n; c is
 * an n-1 by 3 matrix. The value of the spline approximation at t is
 *    s(t) = c[2][i]*d^3 + c[1][i]*d^2 + c[0][i]*d + y[i]
 * where x[i] <= t < x[i+1] and d = t - x[i].
 *
 *   var: error variance. If var is negative (i.e. unknown) then the
 * smoothing parameter is determined by minimizing the generalized
 * cross validation and an estimate of the error variance is returned.
 * If var is non-negative (i.e. known) then the smoothing parameter is
 * determined to minimize an estimate, which depends on var, of the true
 * mean square error. In particular, if var is zero, then an interpolating
 * natural cubic spline is calculated. Set var to 1 if absolute standard
 * deviations have been provided in df (see above).
 *
 * Notes:
 *
 * Additional information on the fit is available in the stat array.
 on normal exit the values are assigned as follows:
 *   stat[0] = smoothing parameter (= rho/(rho + 1))
 *   stat[1] = estimate of the number of degrees of freedom of the
 * residual sum of squares; this reduces to the usual value of n-2
 * when a least squares regression line is calculated.
 *   stat[2] = generalized cross validation
 *   stat[3] = mean square residual
 *   stat[4] = estimate of the true mean square error at the data points
 *   stat[5] = estimate of the error variance; chi^2/nu in the case
 *             of linear regression
 *
 * If stat[0]==0 (rho==0) an interpolating natural cubic spline has been
 * calculated; if stat[0]==1 (rho==infinite) a least squares regression
 * line has been calculated.
 *
 * Returns stat[4], an estimate of the true rms error
 *
 * precision/hardware  - REAL642 (originally VAX double)
 *
 * the number of arithmetic operations required by the subroutine is
 * proportional to n.  The subroutine uses an algorithm developed by
 * M.F. Hutchinson and F.R. de Hoog, 'Smoothing Noisy Data with Spline
 * Functions', Numer. Math. 47 p.99 (1985)
 */
SPLINE *
phSplineFindSmoothed(const float x[],	/* absissae */
                     const float f[],	/* ordinates */
                     const float df[],	/* relative errors in f */
                     int n,		/* number of points in x, y, dy */
                     float var,		/* scaling for the df[] */
                     float *chisq,	/* final chisq, or NULL */
		     float *errs)	/* Bayesian error estimates.
					   Must have size >= n if not NULL */
{
   REAL642 *y;
   REAL642 *c[3];
   float *coeffs[4];			/* coeffs that end up in SPLINE */
   REAL642 *sdf;			/* scaled values of df */
   const REAL642 ratio = 2.0;
   const REAL642 tau = 1.618033989;	/* golden ratio */
   REAL642 avdf, avar, stat[6];
   REAL642 p, q, delta, r1, r2, r3, r4;
   REAL642 gf1, gf2, gf3, gf4, avh, err;
   REAL642 *r[3], *t[2], *u, *v;
   SPLINE *sp;				/* the desired spline */
   int set_r3;				/* was the value of r3 set? */
/*
 * The following assertion is required for the argument lists to be
 * correct, and so that we don't have to copy the arrays y and c into
 * the SPLINE. It is not required by the algorithm; indeed the original
 * data type for REAL642 was double
 */
   shAssert(sizeof(REAL642) == sizeof(float));
/*
 * allocate scratch space
 */
   sdf = shMalloc(n*sizeof(REAL642));
   coeffs[0] = shMalloc(4*n*sizeof(REAL642)); /* memory belongs to the SPLINE*/
   coeffs[1] = coeffs[0] + n;		/*        so isn't really scratch */
   coeffs[2] = coeffs[1] + n;
   coeffs[3] = coeffs[2] + n;

   y = coeffs[0];
   c[0] = coeffs[1];
   c[1] = coeffs[2];
   c[2] = coeffs[3];

   r[0] = shMalloc(7*(n+2)*sizeof(REAL642));
   r[0]++;				/* we want indices -1..n */
   r[1] = r[0] + (n + 2);
   r[2] = r[1] + (n + 2);
   t[0] = r[2] + (n + 2);
   t[1] = t[0] + (n + 2);
   u = t[1] + (n + 2);
   v = u + (n + 2);
/*
 * and so to work.
 */
   memcpy(sdf, df, n*sizeof(REAL642));

   avdf = spint1(x, &avh, f, sdf, n, y, c, r, t);
   avar = var;
   if (var > 0) {
      avar *= avdf*avdf;
   }

   if (var == 0) {			/* simply find a natural cubic spline*/
      r1 = 0;

      gf1 = spfit1(x, avh, sdf, n, r1, &p, &q, avar, stat, y, c, r, t, u, v);
   } else {				/* Find local minimum of gcv or the
					   expected mean square error */
      r1 = 1;
      r2 = ratio*r1;
      gf2 = spfit1(x, avh, sdf, n, r2, &p, &q, avar, stat, y, c, r, t, u, v);
      for(;;) {
	 gf1 = spfit1(x, avh, sdf, n, r1, &p, &q, avar, stat, y, c, r, t, u,v);
	 if (gf1 > gf2) {
	    break;
	 }
	 
	 if (p <= 0) {
	    break;
	 }
	 r2 = r1;
	 gf2 = gf1;
	 r1 /= ratio;
      }
      
      if(p <= 0) {
	 set_r3 = 0;
	 r3 = 0;			/* placate compiler */
      } else {
	 r3 = ratio * r2;
	 set_r3 = 1;
	 
	 for(;;) {
 	    gf3 = spfit1(x, avh, sdf, n, r3, &p, &q, avar,
						       stat, y, c, r, t, u, v);
	    if (gf3 >= gf2) {
	       break;
	    }
	    
	    if (q <= 0) {
	       break;
	    }
	    r2 = r3;
	    gf2 = gf3;
	    r3 = ratio*r3;
	 }
      }
      
      if(p > 0 && q > 0) {
	 shAssert(set_r3);
	 r2 = r3;
	 gf2 = gf3;
	 delta = (r2 - r1) / tau;
	 r4 = r1 + delta;
	 r3 = r2 - delta;
	 gf3 = spfit1(x, avh, sdf, n, r3, &p, &q, avar, stat, y, c, r, t, u, v);
	 gf4 = spfit1(x, avh, sdf, n, r4, &p, &q, avar, stat, y, c, r, t, u, v);
/*
 * Golden section search for local minimum
 */
	 do {
	    if (gf3 <= gf4) {
	       r2 = r4;
	       gf2 = gf4;
	       r4 = r3;
	       gf4 = gf3;
	       delta /= tau;
	       r3 = r2 - delta;
	       gf3 = spfit1(x, avh, sdf, n, r3, &p, &q, avar, stat,
							     y, c, r, t, u, v);
	    } else {
	       r1 = r3;
	       gf1 = gf3;
	       r3 = r4;
	       gf3 = gf4;
	       delta /= tau;
	       r4 = r1 + delta;
	       gf4 = spfit1(x, avh, sdf, n, r4, &p, &q, avar, stat,
							     y, c, r, t, u, v);
	    }
	    
	    err = (r2 - r1) / (r1 + r2);
	 } while(err*err + 1 > 1 && err > 1e-6);

	 r1 = (r1 + r2) * .5;
	 gf1 = spfit1(x, avh, sdf, n, r1, &p, &q, avar, stat, y, c, r, t, u,v);
      }
   }
/*
 * Calculate spline coefficients
 */
   spcof1(x, avh, f, sdf, n, p, q, y, c, u, v);

   stat[2] /= avdf*avdf;		/* undo scaling */
   stat[3] /= avdf*avdf;
   stat[4] /= avdf*avdf;
/*
 * Optionally calculate standard error estimates
 */
   if(errs != NULL) {
      sperr1(x, avh, sdf, n, r, p, avar, errs);
   }
/*
 * convert the measured coefficients to a spline. Note that we asserted
 * that REAL642 == float, so we only need to repackage coeffs
 */
   {
      float *knots = shMalloc(n*sizeof(float));
      memcpy(knots, x, n*sizeof(float));
      sp = phSplineNew(n, knots, coeffs);
   }
/*
 * clean up
 */
   shFree(sdf); shFree(r[0] - 1);

   if(chisq != NULL) {
      *chisq = n*stat[4];
   }

   return(sp);
}

/*****************************************************************************/
/*
 * initializes the arrays c, r and t for one dimensional cubic
 * smoothing spline fitting by subroutine spfit1. The values
 * df[i] are scaled so that the sum of their squares is n.
 * The average of the differences x[i+1] - x[i] is calculated
 * in avh in order to avoid underflow and overflow problems in spfit1.
 *
 * Return the initial rms value of dy.
 */
static REAL642
spint1(const REAL642 x[],
       REAL642 *avh,
       const REAL642 f[],
       REAL642 df[],
       int n,
       REAL642 a[],
       REAL642 *c[3],
       REAL642 *r[3],
       REAL642 *t[2])
{
   REAL642 avdf;
   REAL642 e, ff, g, h;
   int i;

   shAssert(n >= 3);
/*
 * Get average x spacing in avh
 */
   g = 0;
   for (i = 0; i < n - 1; ++i) {
      h = x[i + 1] - x[i];
      shAssert(h > 0);

      g += h;
   }
   *avh = g/(n - 1);
/*
 * Scale relative weights
 */
   g = 0;
   for(i = 0; i < n; ++i) {
      shAssert(df[i] > 0);

      g += df[i]*df[i];
   }
   avdf = sqrt(g / n);

   for (i = 0; i < n; ++i) {
      df[i] /= avdf;
   }
/*
 * Initialize h,f
 */
   h = (x[1] - x[0])/(*avh);
   ff = (f[1] - f[0])/h;
/*
 * Calculate a,t,r
 */
   for (i = 1; i < n - 1; ++i) {
      g = h;
      h = (x[i + 1] - x[i])/(*avh);
      e = ff;
      ff = (f[i + 1] - f[i])/h;
      a[i] = ff - e;
      t[0][i] = (g + h) * 2./3.;
      t[1][i] = h / 3.;
      r[2][i] = df[i - 1]/g;
      r[0][i] = df[i + 1]/h;
      r[1][i] = -df[i]/g - df[i]/h;
   }
/*
 * Calculate c = r'*r
 */
   r[1][n-1] = 0;
   r[2][n-1] = 0;
   r[2][n] = 0;

   for (i = 1; i < n - 1; i++) {
      c[0][i] = r[0][i]*r[0][i]   + r[1][i]*r[1][i] + r[2][i]*r[2][i];
      c[1][i] = r[0][i]*r[1][i+1] + r[1][i]*r[2][i+1];
      c[2][i] = r[0][i]*r[2][i+2];
   }

   return(avdf);
}

/*****************************************************************************/
/*
 * Fits a cubic smoothing spline to data with relative
 * weighting dy for a given value of the smoothing parameter
 * rho using an algorithm based on that of C.H. Reinsch (1967),
 * Numer. Math. 10, 177-183.
 *
 * The trace of the influence matrix is calculated using an
 * algorithm developed by M.F.Hutchinson and F.R.De Hoog (numer.
 * math., 47 p.99 (1985)), enabling the generalized cross validation
 * and related statistics to be calculated in order n operations.
 *
 * The arrays a, c, r and t are assumed to have been initialized
 * by the routine spint1.  overflow and underflow problems are
 * avoided by using p=rho/(1 + rho) and q=1/(1 + rho) instead of
 * rho and by scaling the differences x[i+1] - x[i] by avh.
 *
 * The values in dy are assumed to have been scaled so that the
 * sum of their squared values is n.  the value in var, when it is
 * non-negative, is assumed to have been scaled to compensate for
 * the scaling of the values in dy.
 *
 * the value returned in fun is an estimate of the true mean square
 * when var is non-negative, and is the generalized cross validation
 * when var is negative.
 */
static REAL642
spfit1(const REAL642 x[],
       REAL642 avh,
       const REAL642 dy[],
       int n,
       REAL642 rho,
       REAL642 *pp,
       REAL642 *pq,
       REAL642 var,
       REAL642 stat[],
       const REAL642 a[],
       REAL642 *c[3],
       REAL642 *r[3],
       REAL642 *t[2],
       REAL642 u[],
       REAL642 v[])
{
   const REAL642 eps = (sizeof(REAL642)==sizeof(float)) ? EPSILON_f : EPSILON;
   double e, f, g, h;
   REAL642 fun;
   int i;
   REAL642 p, q;				/* == *pp, *pq */
/*
 * Use p and q instead of rho to prevent overflow or underflow
 */
   if(fabs(rho) < eps) {
      p = 0; q = 1;
   } else if(fabs(1/rho) < eps) {
      p = 1; q = 0;
   } else {
      p = rho/(1 + rho);
      q =   1/(1 + rho);
   }
/*
 * Rational Cholesky decomposition of p*c + q*t
 */
   f = 0;
   g = 0;
   h = 0;
   r[0][-1] = r[0][0] = 0;
   
   for(i = 1; i < n - 1; ++i) {
      r[2][i-2] = g*r[0][i - 2];
      r[1][i-1] = f*r[0][i - 1];
      {
	 double tmp = p*c[0][i] + q*t[0][i] - f*r[1][i-1] - g*r[2][i-2];
	 if(tmp == 0.0) {
	    r[0][i] = 1e30;
	 } else {
	    r[0][i] = 1/tmp;
	 }
      }
      f = p*c[1][i] + q*t[1][i] - h*r[1][i-1];
      g = h;
      h = p*c[2][i];
   }
/*
 * Solve for u
 */
   u[-1] = u[0] = 0;
   for(i = 1; i < n - 1; i++) {
      u[i] = a[i] - r[1][i-1]*u[i-1] - r[2][i-2]*u[i-2];
   }
   u[n-1] = u[n] = 0;
   for(i = n - 2; i > 0; i--) {
      u[i] = r[0][i]*u[i] - r[1][i]*u[i+1] - r[2][i]*u[i+2];
   }
/*
 * Calculate residual vector v
 */
   e = h = 0;
   for(i = 0; i < n - 1; i++) {
      g = h;
      h = (u[i + 1] - u[i])/((x[i+1] - x[i])/avh);
      v[i] = dy[i]*(h - g);
      e += v[i]*v[i];
   }
   v[n-1] = -h*dy[n-1];
   e += v[n-1]*v[n-1];
/*
 * Calculate upper three bands of inverse matrix
 */
   r[0][n-1] = r[1][n-1] = r[0][n] = 0;
   for (i = n - 2; i > 0; i--) {
      g = r[1][i];
      h = r[2][i];
      r[1][i] = -g*r[0][i+1] - h*r[1][i+1];
      r[2][i] = -g*r[1][i+1] - h*r[0][i+2];
      r[0][i] = r[0][i] - g*r[1][i] - h*r[2][i];
   }
/*
 * Calculate trace
 */
   f = g = h = 0;
   for (i = 1; i < n - 1; ++i) {
      f += r[0][i]*c[0][i];
      g += r[1][i]*c[1][i];
      h += r[2][i]*c[2][i];
   }
   f += 2*(g + h);
/*
 * Calculate statistics
 */
   stat[0] = p;
   stat[1] = f*p;
   stat[2] = n*e/(f*f + 1e-20);
   stat[3] = e*p*p/n;
   stat[5] = e*p/((f < 0) ? f - 1e-10 : f + 1e-10);
   if (var >= 0) {
      fun = stat[3] - 2*var*stat[1]/n + var;

      stat[4] = fun;
   } else {
      stat[4] = stat[5] - stat[3];
      fun = stat[2];
   }
   
   *pp = p; *pq = q;

   return(fun);
}

/*
 * Calculates coefficients of a cubic smoothing spline from
 * parameters calculated by spfit1()
 */
static void
spcof1(const REAL642 x[],
       REAL642 avh,
       const REAL642 y[],
       const REAL642 dy[],
       int n,
       REAL642 p,
       REAL642 q,
       REAL642 a[],
       REAL642 *c[3],
       REAL642 u[],
       const REAL642 v[])
{
   REAL642 h;
   int i;
   REAL642 qh;
   
   qh = q/(avh*avh);
   
   for (i = 0; i < n; ++i) {
      a[i] = y[i] - p * dy[i] * v[i];
      u[i] = qh*u[i];
   }
/*
 * calculate c
 */
   for (i = 0; i < n - 1; ++i) {
      h = x[i+1] - x[i];
      c[2][i] = (u[i + 1] - u[i])/(3*h);
      c[0][i] = (a[i + 1] - a[i])/h - (h*c[2][i] + u[i])*h;
      c[1][i] = u[i];
   }
}

/*****************************************************************************/
/*
 * Calculates Bayesian estimates of the standard errors of the fitted
 * values of a cubic smoothing spline by calculating the diagonal elements
 * of the influence matrix.
 */
static void
sperr1(const REAL642 x[],
       REAL642 avh,
       const REAL642 dy[],
       int n,
       REAL642 *r[3],
       REAL642 p,
       REAL642 var,
       REAL642 se[])
{
   REAL642 f, g, h;
   int i;
   REAL642 f1, g1, h1;
/*
 * Initialize
 */
   h = avh/(x[1] - x[0]);
   se[0] = 1 - p*dy[0]*dy[0]*h*h*r[0][1];
   r[0][0] = r[1][0] = r[2][0] = 0;
/*
 * Calculate diagonal elements
 */
   for (i = 1; i < n - 1; ++i) {
      f = h;
      h = avh/(x[i+1] - x[i]);
      g = -(f + h);
      f1 = f*r[0][i-1] + g*r[1][i-1] + h*r[2][i-1];
      g1 = f*r[1][i-1] + g*r[0][i] + h*r[1][i];
      h1 = f*r[2][i-1] + g*r[1][i] + h*r[0][i+1];
      se[i] = 1 - p*dy[i]*dy[i]*(f*f1 + g*g1 + h*h1);
   }
   se[n-1] = 1 - p*dy[n-1]*dy[n-1]*h*h*r[0][n-2];
/*
 * Calculate standard error estimates
 */
    for(i = 0; i < n; ++i) {
       REAL642 tmp = se[i]*var;
       se[i] = (tmp >= 0) ? sqrt(tmp)*dy[i] : 0;
    }
}

/*****************************************************************************/
/*
 * <EXTRACT AUTO>
 *
 * Fit a taut spline to a set of data, forcing the resulting spline to
 * obey S(x) = S(-x). The input points must have tau[] >= 0.
 *
 * See phSplineFindTaut() for a discussion of the alogorithm, and
 * the meaning of the parameter gamma
 *
 * This is done by duplicating the input data for -ve x, so consider
 * carefully before using this function on many-thousand-point datasets
 */
SPLINE *
phSplineFindTautEven(
		     const float *tau,	/* points where function's specified;
					   must be strictly increasing, >= 0 */
		     const float *gtau,	/* values of function at tau[] */
		     int ntau,		/* size of tau and gtau, must be >= 2*/
		     float gamma	/* control extra knots */
		     )
{
   int i;
   int i0;				/* index of knots[] > 0 in spline */
   float *knots, *coeffs[4];		/* returned as part of SPLINE */
   int nknot;				/* number of knots */
   int np;				/* the number of points in x and y */
   SPLINE *sp, *sp2;
   float *x, *y;			/* tau and gtau, extended to -ve tau */

   if(tau[0] == 0.0f) {
      np = 2*ntau - 1;
      x = alloca(2*np*sizeof(float)); y = x + np;

      x[ntau - 1] =  tau[0]; y[ntau - 1] = gtau[0];
      for(i = 1;i < ntau;i++) {
	 x[ntau - 1 + i] =  tau[i]; y[ntau - 1 + i] = gtau[i];
	 x[ntau - 1 - i] = -tau[i]; y[ntau - 1 - i] = gtau[i];
      }
   } else {
      np = 2*ntau;
      x = alloca(2*np*sizeof(float)); y = x + np;

      for(i = 0;i < ntau;i++) {
	 x[ntau + i] =      tau[i]; y[ntau + i] =     gtau[i];
	 x[ntau - 1 - i] = -tau[i]; y[ntau - 1 - i] = gtau[i];
      }
   }

   sp = phSplineFindTaut(x,y,np,gamma);
/*
 * Now repackage that spline to reflect the original points
 */
   for(i = sp->nknot - 1;i >= 0;i--) {
      if(sp->knots[i] < 0.0f) {
	 break;
      }
   }
   i0 = i + 1;
   nknot = sp->nknot - i0;
   
   knots = shMalloc(nknot*sizeof(float));
   coeffs[0] = shMalloc(4*nknot*sizeof(float));
   coeffs[1] = coeffs[0] + nknot;
   coeffs[2] = coeffs[1] + nknot;
   coeffs[3] = coeffs[2] + nknot;

   for(i = i0;i < sp->nknot;i++) {
      knots[i - i0] = sp->knots[i];
      coeffs[0][i - i0] = sp->coeffs[0][i];
      coeffs[1][i - i0] = sp->coeffs[1][i];
      coeffs[2][i - i0] = sp->coeffs[2][i];
      coeffs[3][i - i0] = sp->coeffs[3][i];
   }
   sp2 = phSplineNew(nknot,knots,coeffs);
   phSplineDel(sp);

   return(sp2);
}

/*****************************************************************************/
/*
 * <EXTRACT AUTO>
 *
 * Fit a taut spline to a set of data, forcing the resulting spline to
 * obey S(x) = -S(-x). The input points must have tau[] >= 0.
 *
 * See phSplineFindTaut() for a discussion of the alogorithm, and
 * the meaning of the parameter gamma
 *
 * This is done by duplicating the input data for -ve x, so consider
 * carefully before using this function on many-thousand-point datasets
 */
SPLINE *
phSplineFindTautOdd(
		    const float *tau,	/* points where function's specified;
					   must be strictly increasing, >= 0 */
		    const float *gtau,	/* values of function at tau[] */
		    int ntau,		/* size of tau and gtau, must be >= 2*/
		    float gamma	/* control extra knots */
		    )
{
   int i;
   int i0;				/* index of knots[] > 0 in spline */
   float *knots, *coeffs[4];		/* returned as part of SPLINE */
   int nknot;				/* number of knots */
   int np;				/* the number of points in x and y */
   SPLINE *sp, *sp2;
   float *x, *y;			/* tau and gtau, extended to -ve tau */

   if(tau[0] == 0.0f) {
      np = 2*ntau - 1;
      x = alloca(2*np*sizeof(float)); y = x + np;

      x[ntau - 1] =  tau[0]; y[ntau - 1] = gtau[0];
      for(i = 1;i < ntau;i++) {
	 x[ntau - 1 + i] =  tau[i]; y[ntau - 1 + i] =  gtau[i];
	 x[ntau - 1 - i] = -tau[i]; y[ntau - 1 - i] = -gtau[i];
      }
   } else {
      np = 2*ntau;
      x = alloca(2*np*sizeof(float)); y = x + np;

      for(i = 0;i < ntau;i++) {
	 x[ntau + i] =      tau[i]; y[ntau + i] =      gtau[i];
	 x[ntau - 1 - i] = -tau[i]; y[ntau - 1 - i] = -gtau[i];
      }
   }

   sp = phSplineFindTaut(x,y,np,gamma);
/*
 * Now repackage that spline to reflect the original points
 */
   for(i = sp->nknot - 1;i >= 0;i--) {
      if(sp->knots[i] < 0.0f) {
	 break;
      }
   }
   i0 = i + 1;
   nknot = sp->nknot - i0;
   
   knots = shMalloc(nknot*sizeof(float));
   coeffs[0] = shMalloc(4*nknot*sizeof(float));
   coeffs[1] = coeffs[0] + nknot;
   coeffs[2] = coeffs[1] + nknot;
   coeffs[3] = coeffs[2] + nknot;

   for(i = i0;i < sp->nknot;i++) {
      knots[i - i0] = sp->knots[i];
      coeffs[0][i - i0] = sp->coeffs[0][i];
      coeffs[1][i - i0] = sp->coeffs[1][i];
      coeffs[2][i - i0] = sp->coeffs[2][i];
      coeffs[3][i - i0] = sp->coeffs[3][i];
   }
   sp2 = phSplineNew(nknot,knots,coeffs);
   phSplineDel(sp);

   return(sp2);
}

/*****************************************************************************/
/*
 * returns index i of first element of x >= z; the input i is an initial guess
 */
static int
search_array(float z, float *x, int n, int i)
{
   register int lo, hi, mid;
   float xm;

   if(i < 0 || i >= n) {		/* initial guess is useless */
      lo = -1;
      hi = n;
   } else {
      unsigned int step = 1;		/* how much to step up/down */

      if(z > x[i]) {			/* expand search upwards */
	 if(i == n - 1) {		/* off top of array */
	    return(n - 1);
	 }

	 lo = i; hi = lo + 1;
	 while(z >= x[hi]) {
	    lo = hi;
	    step += step;		/* double step size */
	    hi = lo + step;
	    if(hi >= n) {		/* reached top of array */
	       hi = n - 1;
	       break;
	    }
	 }
      } else {				/* expand it downwards */
	 if(i == 0) {			/* off bottom of array */
	    return(-1);
	 }

	 hi = i; lo = i - 1;
	 while(z < x[lo]) {
	    hi = lo;
	    step += step;		/* double step size */
	    lo = hi - step;
	    if(lo < 0) {		/* off bottom of array */
	       lo = -1;
	       break;
	    }
	 }
      }
   }
/*
 * perform bisection
 */
   while(hi - lo > 1) {
      mid = (lo + hi)/2;
      xm = x[mid];
      if(z <= xm) {
	 hi = mid;
      } else {
	 lo = mid;
      }
   }

   if(lo == -1) {			/* off the bottom */
      return(lo);
   }
/*
 * If there's a discontinuity (many knots at same x-value), choose the
 * largest
 */
   xm = x[lo];
   while(lo < n - 1 && x[lo + 1] == xm) lo++;

   return(lo);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Interpolate a spline. The coefficients and knots are packed into the
 * SPLINE; specifically, for knots[i] <= x <= knots[i+1], the interpolant
 * has the form
 *    val = coef[0][i] +dx*(coef[1][i] + dx*(coef[2][i]/2 + dx*coef[3][i]/6))
 * with
 *    dx = x - knots[i]
 */
void
phSplineInterpolate(const SPLINE *sp,	/* spline coefficients/knots */
		    const float *x,	/* points to interpolate at */
		    float *y,		/* values of spline interpolation */
		    int n		/* dimension of x[] and y[] */
		    )
{
   float dx;				/* distance of x[i] from knot */
   int i;
   int ind;				/* index into coefficients */
   float *knots, **coeffs;		/* unpacked from sp */
   int nknot;				/*  "    "    "  " */

   shAssert(sp != NULL);

   nknot = sp->nknot;
   knots = sp->knots;
   coeffs = (float **)sp->coeffs;	/* cast for Irix 6.2 cc */

   ind = -1;				/* no idea initially */
   for(i = 0;i < n;i++) {
      ind = search_array(x[i],knots,nknot,ind);

      if(ind < 0) {			/* off bottom */
	 ind = 0;
      } else if(ind >= nknot) {		/* off top */
	 ind = nknot - 1;
      }

      dx = x[i] - knots[ind];
      y[i] = coeffs[0][ind] + dx*(coeffs[1][ind] +
				  dx*(coeffs[2][ind]/2 + dx*coeffs[3][ind]/6));
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Find the derivative of a spline. The coefficients and knots are packed
 * into the SPLINE; specifically, for knots[i] <= x <= knots[i+1], the
 * interpolant has the form
 *    val = coef[0][i] +dx*(coef[1][i] + dx*(coef[2][i]/2 + dx*coef[3][i]/6))
 * with
 *    dx = x - knots[i]
 * so the derivative is
 *    val = coef[1][i] + dx*(coef[2][i] + dx*coef[3][i]/2))
 */
void
phSplineDerivative(const SPLINE *sp,	/* spline coefficients/knots */
		   const float *x,	/* points to interpolate at */
		   float *y,		/* values of spline derivative */
		   int n		/* dimension of x[] and y[] */
		   )
{
   float dx;				/* distance of x[i] from knot */
   int i;
   int ind;				/* index into coefficients */
   float *knots, **coeffs;		/* unpacked from sp */
   int nknot;				/*  "    "    "  " */

   shAssert(sp != NULL);

   nknot = sp->nknot;
   knots = sp->knots;
   coeffs = (float **)sp->coeffs;	/* cast for Irix 6.2 cc */

   ind = -1;				/* no idea initially */
   for(i = 0;i < n;i++) {
      ind = search_array(x[i],knots,nknot,ind);

      if(ind < 0) {			/* off bottom */
	 ind = 0;
      } else if(ind >= nknot) {		/* off top*/
	 ind = nknot - 1;
      }

      dx = x[i] - knots[ind];
      y[i] = coeffs[1][ind] + dx*(coeffs[2][ind] + dx*coeffs[3][ind]/2);
   }
}

/*****************************************************************************/
/*
 * Select the roots that lie in the specified range [x0,x1); the valid
 * roots are moved to the start of the array (the others are discarded),
 * and the number surviving is returned
 */
static int
check_roots(float *roots,int nroot,float x0, float x1)
{
   int i,j;
   
   for(i = 0;i < nroot;i++) {
      if(roots[i] < x0 || roots[i] >= x1) { /* reject this root */
	 for(j = i;j < nroot - 1;j++) {	/* move others down */
	    roots[j] = roots[j + 1];
	 }
	 i--; nroot--;
      }
   }
   return(nroot);
}

/*****************************************************************************/
/*
 * find the real roots of a quadratic ax^2 + bx + c = 0, returning the number
 * of roots found (the values are in roots[], of dimen >= 2)
 */
static int
do_quadratic(double a, double b, double c, float *roots)
{
   if(fabs(a) < EPSILON) {
      if(fabs(b) < EPSILON) {
	 return(0);
      }
      roots[0] = -c/b;
      return(1);
   } else {
      double tmp = b*b - 4*a*c;
      
      if(tmp < 0) {
	 return(0);
      }
      if(b >= 0) {
	 roots[0] = (-b - sqrt(tmp))/(2*a);
      } else {
	 roots[0] = (-b + sqrt(tmp))/(2*a);
      }
      roots[1] = c/(a*roots[0]);
/*
 * sort roots
 */
      if(roots[0] > roots[1]) {
	 tmp = roots[0]; roots[0] = roots[1]; roots[1] = tmp;
      }
      
      return(2);
   }
}

/*****************************************************************************/
/*
 * find the real roots of a cubic ax^3 + bx^2 + cx + d = 0, returning the
 * number of roots found (the values are in roots[], of dimen >= 3)
 */
static int
do_cubic(double a, double b, double c, double d, float *roots)
{
   double q, r;
   double sq, sq3;			/* sqrt(q), sqrt(q)^3 */
   double tmp;

   if(fabs(a) < EPSILON) {
      return(do_quadratic(b,c,d,roots));
   }
   b /= a; c /= a; d /= a;

   q = (b*b - 3*c)/9;
   r = (2*b*b*b - 9*b*c + 27*d)/54;
/*
 * n.b. note that the test for the number of roots is carried out on the
 * same variables as are used in (e.g.) the acos, as it is possible for
 * r*r < q*q*q && r > sq*sq*sq due to rounding.
 */
   sq = (q >= 0) ? sqrt(q) : -sqrt(-q);
   sq3 = sq*sq*sq;
   if(fabs(r) < sq3) {			/* three real roots */
      double theta = acos(r/sq3);	/* sq3 cannot be zero */

      roots[0] = -2*sq*cos(theta/3) - b/3;
      roots[1] = -2*sq*cos((theta + 2*M_PI)/3) - b/3;
      roots[2] = -2*sq*cos((theta - 2*M_PI)/3) - b/3;
      /*
       * sort roots
       */
      if(roots[0] > roots[1]) {
	 tmp = roots[0]; roots[0] = roots[1]; roots[1] = tmp;
      }
      if(roots[1] > roots[2]) {
	 tmp = roots[1]; roots[1] = roots[2]; roots[2] = tmp;
      }
      if(roots[0] > roots[1]) {
	 tmp = roots[0]; roots[0] = roots[1]; roots[1] = tmp;
      }

      return(3);
   } else if(fabs(r) == sq3) {		/* no more than two real roots */
      double aa;
      
      aa = -((r < 0) ? -pow(-r,1.0/3.0) : pow(r,1.0/3.0));

      if(fabs(aa) < EPSILON) {		/* degenerate case; one real root */
	 roots[0] = -b/3;
	 return(1);
      } else {
	 roots[0] = 2*aa - b/3;
	 roots[1] = -aa - b/3;
	 
	 if(roots[0] > roots[1]) {
	    tmp = roots[0]; roots[0] = roots[1]; roots[1] = tmp;
	 }
	 return(2);
      }
   } else {				/* only one real root */
      double aa, bb;
      
      tmp = sqrt(r*r - (q > 0 ? sq3*sq3 : -sq3*sq3));
      tmp = r + (r < 0 ? -tmp : tmp);
      aa = -((tmp < 0) ? -pow(-tmp,1.0/3.0) : pow(tmp,1.0/3.0));
      bb = (fabs(aa) < EPSILON) ? 0 : q/aa;

      roots[0] = (aa + bb) - b/3;
#if 0
      roots[1] = -(aa + bb)/2 - b/3;	/* the real */
      roots[2] = sqrt(3)/2*(aa - bb);	/*          and imaginary parts of the
						    complex roots */
#endif
      return(1);
   }
}

/*****************************************************************************/
/*
 * copy the array roots_s to roots, taking due account of how much space
 * is available
 */
#define COPY_ROOTS_S_TO_ROOTS \
   if(nroot + nroot_s > roots_dimen) { \
      memcpy(&roots[nroot], roots_s, (roots_dimen - nroot)*sizeof(float)); \
      return(-roots_dimen); \
   } else { \
      memcpy(&roots[nroot], roots_s, nroot_s*sizeof(float)); \
   } \
   nroot += nroot_s

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Find the roots of
 *   SPLINE - val = 0
 * in the range [x0, x1). Return the number of roots found, or -roots_size
 * if the array roots[] wasn't large enough (in this case, roots_size roots
 * are returned).
 *
 * We know that the interpolant has the form
 *    val = coef[0][i] +dx*(coef[1][i] + dx*(coef[2][i]/2 + dx*coef[3][i]/6))
 * so we can use the usual analytic solution for a cubic. Note that the
 * cubic quoted above returns dx, the distance from the previous knot,
 * rather than x itself
 */
int
phSplineRoots(const SPLINE *sp,		/* spline coefficients/knots */
	      float value,		/* desired value */
	      float x0,			/* specify desired range is [x0,x1) */
	      float x1,
	      float *roots,		/* the roots found */
	      int roots_dimen		/* dimension of roots[] */
	      )
{
   int i,j;
   int i0, i1;				/* indices corresponding to x0, x1 */
   int nroot;				/* number of roots found */
   float roots_s[3];			/* roots in a sector */
   int nroot_s;				/* number of roots in a sector */
   float *knots, **coeffs;		/* unpacked from sp */
   int nknot;				/*  "    "    "  " */

   shAssert(sp != NULL);

   nknot = sp->nknot;
   knots = sp->knots;
   coeffs = (float **)sp->coeffs;	/* cast for Irix 6.2 cc */

   i0 = search_array(x0,knots,nknot,-1);
   i1 = search_array(x1,knots,nknot,i0);
   shAssert(i1 >= i0 && i1 <= nknot - 1);

   nroot = 0;
/*
 * Deal with special case that x0 may be off one end or the other of
 * the array of knots.
 */
   if(i0 < 0) {				/* off bottom */
      i0 = 0;
      nroot_s = do_cubic(coeffs[3][i0]/6, coeffs[2][i0]/2, coeffs[1][i0],
			 coeffs[0][i0] - value, roots_s);
      for(j = 0;j < nroot_s;j++) {
	 roots_s[j] += knots[i0];
      }
      nroot_s = check_roots(roots_s, nroot_s, x0, knots[i0]);

      COPY_ROOTS_S_TO_ROOTS;
      x0 = knots[i0];
   } else if(i0 >= nknot) {		/* off top */
      i0 = nknot - 1;
      shAssert(i0 >= 0);
      nroot_s = do_cubic(coeffs[3][i0]/6, coeffs[2][i0]/2, coeffs[1][i0],
			 coeffs[0][i0] - value, roots_s);

      for(j = 0;j < nroot_s;j++) {
	 roots_s[j] += knots[i0];
      }
      nroot_s = check_roots(roots_s, nroot_s, x0, x1);
      COPY_ROOTS_S_TO_ROOTS;

      return(nroot);
   }
/*
 * OK, now search in main body of spline. Note that i1 may be nknot - 1, and
 * in any case the right hand limit of the last segment is at x1, not a knot
 */
   for(i = i0;i <= i1;i++) {
      nroot_s = do_cubic(coeffs[3][i]/6, coeffs[2][i]/2, coeffs[1][i],
			 coeffs[0][i] - value, roots_s);
      for(j = 0;j < nroot_s;j++) {
	 roots_s[j] += knots[i];
      }
      nroot_s = check_roots(roots_s, nroot_s,
			    ((i == i0) ? x0 : knots[i]),
			    ((i == i1) ? x1 : knots[i + 1]));

      COPY_ROOTS_S_TO_ROOTS;
   }
   
   return(nroot);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Create a SPLINE, given its knots/coefficients.
 *
 * Note that we assume that memory for the coeffs is all allocated via
 * a single call like coeffs[0] = shMalloc(...).
 *
 * The dimension of coeffs[i] is nknot
 */
SPLINE *
phSplineNew(int nknot, float *knots, float *coeffs[4])
{
   SPLINE *n = shMalloc(sizeof(SPLINE));

   n->nknot = nknot;
   n->knots = knots;
   n->coeffs[0] = coeffs[0];
   n->coeffs[1] = coeffs[1];
   n->coeffs[2] = coeffs[2];
   n->coeffs[3] = coeffs[3];

   return(n);
}

/*
 * <AUTO EXTRACT>
 * 
 * Destroy a SPLINE
 */
void
phSplineDel(SPLINE *sp)
{
   if(sp != NULL) {
      shFree(sp->knots);
      shFree(sp->coeffs[0]);

      shFree(sp);
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Invert an n*n double matrix arr in place using Gaussian Elimination
 * with pivoting; return -1 for singular matrices
 *
 * Also solve the equation arr*b = x, replacing b by x (if b is non-NULL)
 *
 * The matrix arr is destroyed during the inversion
 */
int
phMatrixInvert(float **arr,		/* matrix to invert */
	       float *b,		/* Also solve arr*x = b; may be NULL */
	       int n)			/* dimension of iarr and arr */
{
   float abs_arr_jk;			/* |arr[j][k]| */
   int row, col;			/* pivotal row, column */
   int i, j, k;
   int *icol, *irow;			/* which {column,row}s we pivoted on */
   int *pivoted;			/* have we pivoted on this column? */
   float pivot;				/* value of pivot */
   float ipivot;			/* 1/pivot */
   float tmp;

   icol = alloca(3*n*sizeof(int));
   irow = icol + n;
   pivoted = irow + n;

   for(i = 0;i < n;i++) {
      pivoted[i] = 0;
   }

   for(i = 0;i < n;i++) {
/*
 * look for a pivot
 */
      row = col = -1;
      pivot = 0;
      for(j = 0;j < n;j++) {
	 if(pivoted[j] != 1) {
	    for(k = 0;k < n;k++) {
	       if(!pivoted[k]) {	/* we haven't used this column */
		  abs_arr_jk = (arr[j][k] > 0) ? arr[j][k] : -arr[j][k];
		  if(abs_arr_jk >= pivot) {
		     pivot = abs_arr_jk;
		     row = j; col = k;
		  }
	       } else if(pivoted[k] > 1) { /* we've used this column already */
		  return(-1);		/* so it's singular */
	       }
	    }
	 }
      }
      shAssert(row >= 0);
/*
 * got the pivot; now move it to the diagonal, i.e. move the pivotal column
 * to the row'th column
 *
 * We have to apply the same permutation to the solution vector, b
 */
      pivoted[col]++;			/* we'll pivot on column col */
      icol[i] = col; irow[i] = row;
      if((pivot = arr[row][col]) == 0.0) {
	 return(-1);			/* singular */
      }

      if(row != col) {
	 for(j = 0;j < n;j++) {
	    tmp = arr[row][j];
	    arr[row][j] = arr[col][j];
	    arr[col][j] = tmp;
	 }
	 if(b != NULL) {
	    tmp = b[row];
	    b[row] = b[col];
	    b[col] = tmp;
	 }
      }
/*
 * divide pivotal row by pivot
 */
      ipivot = 1/pivot;
      arr[col][col] = 1.0;
      for(j = 0;j < n;j++) {
	 arr[col][j] *= ipivot;
      }
      if(b != NULL) {
	 b[col] *= ipivot;
      }
/*
 * and reduce other rows
 */
      for(j = 0;j < n;j++) {
	 if(j == col) {			/* not the pivotal one */
	    continue;
	 }

	 tmp = arr[j][col];
	 arr[j][col] = 0.0;
	 for(k = 0;k < n;k++) {
	    arr[j][k] -= tmp*arr[col][k];
	 }
	 if(b != NULL) {
	    b[j] -= tmp*b[col];
	 }
      }
   }
/*
 * OK, we're done except for undoing a permutation of the columns
 */
   for(i = n - 1;i >= 0;i--) {
      if(icol[i] != irow[i]) {
	 for(j = 0;j < n;j++) {
	    tmp = arr[j][icol[i]];
	    arr[j][icol[i]] = arr[j][irow[i]];
	    arr[j][irow[i]] = tmp;
	 }
      }
   }
   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Invert an n*n double matrix arr in place using Gaussian Elimination
 * with pivoting; return -1 for singular matrices
 *
 * Also solve the equation arr*b = x, replacing b by x (if b is non-NULL)
 *
 * The matrix arr is destroyed during the inversion
 */
int
phDMatrixInvert(double **arr,		/* matrix to invert */
		double *b,		/* Also solve arr*x = b; may be NULL */
		int n)			/* dimension of iarr and arr */
{
   double abs_arr_jk;			/* |arr[j][k]| */
   int row, col;			/* pivotal row, column */
   int i, j, k;
   int *icol, *irow;			/* which {column,row}s we pivoted on */
   int *pivoted;			/* have we pivoted on this column? */
   double pivot;				/* value of pivot */
   double ipivot;			/* 1/pivot */
   double tmp;

   icol = alloca(3*n*sizeof(int));
   irow = icol + n;
   pivoted = irow + n;

   for(i = 0;i < n;i++) {
      pivoted[i] = 0;
   }

   for(i = 0;i < n;i++) {
/*
 * look for a pivot
 */
      row = col = -1;
      pivot = 0;
      for(j = 0;j < n;j++) {
	 if(pivoted[j] != 1) {
	    for(k = 0;k < n;k++) {
	       if(!pivoted[k]) {	/* we haven't used this column */
		  abs_arr_jk = (arr[j][k] > 0) ? arr[j][k] : -arr[j][k];
		  if(abs_arr_jk >= pivot) {
		     pivot = abs_arr_jk;
		     row = j; col = k;
		  }
	       } else if(pivoted[k] > 1) { /* we've used this column already */
		  return(-1);		/* so it's singular */
	       }
	    }
	 }
      }
      shAssert(row >= 0);
/*
 * got the pivot; now move it to the diagonal, i.e. move the pivotal column
 * to the row'th column
 *
 * We have to apply the same permutation to the solution vector, b
 */
      pivoted[col]++;			/* we'll pivot on column col */
      icol[i] = col; irow[i] = row;
      if((pivot = arr[row][col]) == 0.0) {
	 return(-1);			/* singular */
      }

      if(row != col) {
	 for(j = 0;j < n;j++) {
	    tmp = arr[row][j];
	    arr[row][j] = arr[col][j];
	    arr[col][j] = tmp;
	 }
	 if(b != NULL) {
	    tmp = b[row];
	    b[row] = b[col];
	    b[col] = tmp;
	 }
      }
/*
 * divide pivotal row by pivot
 */
      ipivot = 1/pivot;
      arr[col][col] = 1.0;
      for(j = 0;j < n;j++) {
	 arr[col][j] *= ipivot;
      }
      if(b != NULL) {
	 b[col] *= ipivot;
      }
/*
 * and reduce other rows
 */
      for(j = 0;j < n;j++) {
	 if(j == col) {			/* not the pivotal one */
	    continue;
	 }

	 tmp = arr[j][col];
	 arr[j][col] = 0.0;
	 for(k = 0;k < n;k++) {
	    arr[j][k] -= tmp*arr[col][k];
	 }
	 if(b != NULL) {
	    b[j] -= tmp*b[col];
	 }
      }
   }
/*
 * OK, we're done except for undoing a permutation of the columns
 */
   for(i = n - 1;i >= 0;i--) {
      if(icol[i] != irow[i]) {
	 for(j = 0;j < n;j++) {
	    tmp = arr[j][icol[i]];
	    arr[j][icol[i]] = arr[j][irow[i]];
	    arr[j][irow[i]] = tmp;
	 }
      }
   }

   return(0);
}

/*****************************************************************************/
/*
 * Now Gamma functions and chisq probabilities
 *
 * These functions come from netlib, and carry the copyright:
 *
 *  Cephes Math Library Release 2.0:  April, 1987
 *  Copyright 1985, 1987 by Stephen L. Moshier (moshier@world.std.com)
 *  Direct inquiries to 30 Frost Street, Cambridge, MA 02140
 *
 * When I did so, the reply was:
 *
 *  From moshier@world.std.com Wed Jul 10 15:00 EDT 1996
 *  Date: Wed, 10 Jul 1996 14:59:56 -0400 (EDT)
 *  From: Stephen L Moshier <moshier@world.std.com>
 *  To: Robert Lupton the Good <rhl@astro.Princeton.EDU>
 *
 *  Subject: Re: cephes
 *
 *  The cephes files in netlib are freely distributable.  The netlib
 *  people would appreciate a mention.
 *
 * They have been somewhat rewritten since then, so don't blame Stephen.
 */

/*
 * <AUTO EXTRACT>
 *
 * Calculate the complemented incomplete gamma integral
 *
 * The function is defined by
 *
 *
 *  phIGammaC(a,x)   =   1 - phIGamma(a,x)
 *
 *                            inf.
 *                              -
 *                     1       | |  -t  a-1
 *               =   -----     |   e   t   dt.
 *                    -      | |
 *                   | (a)    -
 *                             x
 *
 *
 * In this implementation both arguments must be positive.
 * The integral is evaluated by either a power series or
 * continued fraction expansion, depending on the relative
 * values of a and x.
 */
double
phIGammaC(double a,
	  double x)
{
   double big = 4.503599627370496e15;
   double biginv =  2.22044604925031308085e-16;
   double ans, ax, c, yc, r, t, y, z;
   double pk, pkm1, pkm2, qk, qkm1, qkm2;
   
   if(x <= 0 ||  a <= 0) {
      return(1.0);
   } else if(x < 1.0 || x < a) {
      return(1.0 - phIGamma(a,x));
   }
   
   ax = a*log(x) - x - phLnGamma(a);
   if(ax < -MAXLOG) {			/* underflow */
      return(0.0);
   }
   ax = exp(ax);
   
   /* continued fraction */
   y = 1.0 - a;
   z = x + y + 1.0;
   c = 0.0;
   pkm2 = 1.0;
   qkm2 = x;
   pkm1 = x + 1.0;
   qkm1 = z * x;
   ans = pkm1/qkm1;
   
   do {
      c += 1.0;
      y += 1.0;
      z += 2.0;
      yc = y * c;
      pk = pkm1 * z  -  pkm2 * yc;
      qk = qkm1 * z  -  qkm2 * yc;
      if( qk != 0 ) {
	 r = pk/qk;
	 t = fabs( (ans - r)/r );
	 ans = r;
      } else {
	 t = 1.0;
      }
      pkm2 = pkm1;
      pkm1 = pk;
      qkm2 = qkm1;
      qkm1 = qk;
      if( fabs(pk) > big ) {
	 pkm2 *= biginv;
	 pkm1 *= biginv;
	 qkm2 *= biginv;
	 qkm1 *= biginv;
      }
   } while( t > EPSILON );

   ans *= ax;

   return((ans < 1e-35) ? 0 : ans);	/* causes problems on alphas */
}

/*
 * <AUTO EXTRACT>
 *
 * Incomplete gamma integral
 *
 *
 * The function is defined by
 *
 *                           x
 *                            -
 *                   1       | |  -t  a-1
 *  phIGamma(a,x)  =   -----     |   e   t   dt.
 *                  -      | |
 *                 | (a)    -
 *                           0
 *
 *
 * In this implementation both arguments must be positive.
 * The integral is evaluated by either a power series or
 * continued fraction expansion, depending on the relative
 * values of a and x.
 *
 * The power series is used for the left tail of incomplete gamma function:
 *
 *          inf.      k
 *   a  -x   -       x
 *  x  e     >   ----------
 *           -     -
 *          k=0   | (a+k+1)
 */
double
phIGamma(double a,
	 double x)
{
   double ans, ax, c, r;

   if( (x <= 0) || ( a <= 0) )
     return( 0.0 );
   
   if( (x > 1.0) && (x > a ) )
     return( 1.0 - phIGammaC(a,x) );
   
   /* Compute  x**a * exp(-x) / gamma(a)  */
   ax = a * log(x) - x - phLnGamma(a);
   if(ax < -MAXLOG) {			/* underflow */
      return( 0.0 );
   }
   ax = exp(ax);
   
   /* power series */
   r = a;
   c = 1.0;
   ans = 1.0;
   
   do {
      r += 1.0;
      c *= x/r;
      ans += c;
   } while(c/ans > EPSILON);

   ans *= ax/a;

   return((ans < 1e-35) ? 0 : ans);	/* causes problems on alphas */
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Here's a log-gamma function, based on  Lanczos' version of Stirling's
 * formula (see, e.g. Press et al. --- whence this code cometh not)
 *
 * Should be good to ~ 1e-10 for all Re(z) > 0.
 */
double
phLnGamma(double z)
{
   double sum;

   z--;

   sum=1.000000000190015 +
     76.18009172947146/(z + 1) + -86.50532032941677/(z + 2) +
       24.01409824083091/(z + 3) + -1.231739572450155/(z + 4) + 
	 0.1208650973867e-2/(z + 5) + -0.539523938495e-5/(z + 6);

   return((z + 0.5)*log(z + 5.5) - (z + 5.5) + log(2*M_PI)/2 + log(sum));
}

/*****************************************************************************/
/*
 * Map ln(L) to b/asinh(b/L).
 *
 * Special cases:
 *   b == 0	Return L
 *   b >= 1	Return lnL
 *
 * N.b. This is numerically unstable for b <~ 1e-10
 */
static float
Lmap(double lnL, double b)
{
   const double L = exp(lnL);
   static int warned = 0;

   if(b == 0.0) {
      return(exp(lnL));			/* no softening */
   } else if(b >= 1.0) {
      return(lnL);			/* infinite softening */
   }
   
   if(!warned) {
      double err;

      warned++;				/* need to incr this first! */
      err = Lmap(0, b) - 1.0;
      
      if(fabs(err) > 1e-5) {
	 shError("Warning: loss of precision: Lmap(1) - 1 = %g", err);
      }
   }
   
   if(lnL < log(b)) {
      return(b/(-lnL + log(b + sqrt(b*b + L*L))));
   } else {
      return(b/log(b/L + sqrt(1 + b/L*b/L)));
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return the probability that a value at least as large as x2 should be
 * drawn from a \chi^2_nu distribution (so phChisqProb(infty, nu, infty) == 0)
 *
 * The value returned is actually
 *    L_soft/asinh(L_soft/L)
 *	== L                    for L >> L_soft
 *	== L_soft/ln(2L_soft/L) for L << L_soft.
 *
 * Special cases:
 *   L_soft == 0	Return L
 *   L_soft >= 1	Return lnL
 */
double
phChisqProb(double x2,			/* value of chi^2 */
	    double nu,			/* number of degrees of freedom */
	    double L_soft)		/* amount of softening */
{
   const double a = 0.5*nu;
   const double x = 0.5*x2;
   double lp;				/* ln(desired probability) */
   
   {
      double p = phIGammaC(a, x);
      if(p > 1e-10) {
	 return(Lmap(log(p), L_soft));	/* no underflow */
      }
   }
/*
 * Evaluate function by continued fraction, using a modified Lentz's method
 * as in Numerical Recipes (Press et al.), section 6.2
 */
   {
#define NINT 200			/* number of iterations allowed */
      double an;
      double b, c, d;
      double del;
      int i;
      
      b = x + 1 - a;
      c = 1/DBL_MIN;
      d = 1/b;
      lp = log(d);
      
      for(i = 1; i <= NINT; i++) {
	 an = -i*(i - a);
	 b += 2;

	 d = an*d + b;
	 if(fabs(d) < DBL_MIN) {
	    d = DBL_MIN;
	 }

	 c = b + an/c;
	 if(fabs(c) < DBL_MIN) {
	    c = DBL_MIN;
	 }

	 d = 1/d;
	 del = d*c;
	 lp += log(del);

	 if(fabs(del - 1) < EPSILON) {
	    lp += -x + a*log(x) - phLnGamma(a);
	    
	    return(Lmap(lp, L_soft));
	 }
      }
   }
   shError("Failed to converge in phChisqProb(%g,%g)", x2, nu);

   return(Lmap(lp, L_soft));
}

/*****************************************************************************/
/*
 * Adaptive Quadrature by Simpson Rule
 *
 * Code is from
 *     Numerical Analysis:
 *     The Mathematics of Scientific Computing
 *     D.R. Kincaid & E.W. Cheney
 *     Brooks/Cole Publ., 1990
 *
 *     Section 7.5
 *
 * Converted to C by RHL
 */
#define N 10				/* number of vectors */
/*
 * <AUTO EXTRACT>
 *
 * Adaptive Quadrature by Simpson Rule
 */
int
phIntegrate(float (*f)(float),		/* integrate f() */
	    float a,			/* from a ... */
	    float b,			/*            to b */
	    float *ans)			/* resulting in *ans */
{
   float v[N][6];
   const float delta = b - a;
   const float epsi = 5.0e-5;
   float fa, fb, fc;			/* f(a), f(b), f((a + b)/2) */
   float fv, fy, fz;			/* f(y), f(z) */
   float h;				/* current interval */
   int k;
   float S, Star, SStar;
   float sum = 0.0;			/* the desired integral */
   float tmp;
   float y, z;				/* two points in [a,b] */

   if(delta < 0) {
      return(-2);
   }
/*
 * initialize everything, in particular the first vector in the stack.
 */
   h = delta/2.0;
   
   fa = f(a);
   fb = f(b);
   fc = f(0.5*(a + b));
   S = (fa + 4.0*fc + fb)*h/3.0;
   v[0][0] = a;
   v[0][1] = h;
   v[0][2] = fa;
   v[0][3] = fc;
   v[0][4] = fb;
   v[0][5] = S;

   k = 0;
   while(k >= 0 && k < N) {
/*
 * take the last vector off the stack and process it.
 */
      h = 0.5*v[k][1];
      y = v[k][0] + h;
      fy = f(y);
      Star = (v[k][2] + 4.0*fy + v[k][3])*h/3.0;
      z = v[k][0] + 3.0*h;
      fz = f(z);
      SStar = (v[k][3] + 4.0*fz + v[k][4])*h/3.0;
      
      tmp = Star + SStar - v[k][5];
      if(fabs(tmp) < 30.0*epsi*h/delta) {
/*
 * if the tolerance is being met, add partial integral and take a new vector
 * from the bottom of the stack.
 */
         sum += Star + SStar + tmp/15.0;
         if(k-- <= 0) {
            *ans = sum;
            return(0);
         }
      } else {
         if(k >= N - 1) {
/*
 * give up if the stack has reached its maximum number of vectors (N)
 */
            return(-1);
         }
/*
 * if the tolerance is not being met, subdivide the interval and create two
 * new vectors in the stack, one of which overwrites the vector just
 * processed.
 */
	 fv = v[k][4];
	 v[k][1] = h;
	 v[k][4] = v[k][3];
	 v[k][3] = fy;
	 v[k][5] = Star;
	 
	 k++;
	 v[k][0] = v[k-1][0] + 2.0*h;
	 v[k][1] = h;
	 v[k][2] = v[k-1][4];
	 v[k][3] = fz;
	 v[k][4] = fv;
	 v[k][5] = SStar;
      }
/*
 * having created two new vectors in the stack, pick off the last one
 * and start the proces anew.
 */
   }
   shFatal("phIntegrate: you cannot get here");
   return(-1);
}

/*****************************************************************************/
/*
 * A couple of hyperbolic sine functions. asinh() isn't in the 
 * standard, and sinh(100) is a problem on an alpha...
 */
double
asinh_ph(double x)
{
   const float amax = 35.0;		/* ~ largest number that we can sinh */

   if(fabs(x) > sinh(amax)) {
      return(x > 0 ? amax : -amax);
   }
/*
 * this is equivalent to a simple
 *   return(log(x + sqrt(1 + x*x)));
 * for all values of x, but is far more numerically stable
 */
   if(x < 0) {
      return(-log(-x + sqrt(1 + x*x)));
   } else {
      return(log(x + sqrt(1 + x*x)));
   }
}

double
sinh_ph(double x)
{
   const float amax = 35.0;		/* ~ largest number that we can sinh */

   if(fabs(x) > amax) {
      return(sinh(amax));
   } else {
      return(sinh(x));
   }
}

/*****************************************************************************/
/*
 * erf[c]() are not in the ANSI C89 standard, although they are almost
 * always in libm.a.  IRIX for example omits their prototypes from math.h,
 * so we'll provide our own.
 *
 * The approximations are good everwhere, to 1.2e-7! Cf Press et al.
 */
double
erfc_ph(double x)
{
   double z = fabs(x);
   double t = 1/(1 + z/2);
   double val;				/* desired value */

   val = t*exp(-z*z - 1.26551223 + 
	       t*(1.00002368 + t*(0.37409196 + 
		 t*(0.09678418 + t*(-0.18628806 + 
		   t*(0.27886807 + t*(-1.13520398 + 
		     t*(1.48851587 + t*(-0.82215223 + t*0.17087277)))))))));

   return((x >= 0) ? val : 2 - val);
}

double
erf_ph(double x)
{
   return(1 - erfc_ph(x));
}

/**************************************************************/
/*
 * This is a private function only used by the call to qsort
 * in phFloatArrayStats
 */
static int
compar(const float *x, const float *y)
{
   if (*x > *y) {
     return(1);
   } else if (*x < *y) {
     return(-1);
   } else {
      return(0);
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * given an array, return the median and interquartile range. If sorted
 * is false, the routine will be sorted (in place)
 *
 * We assume that the cumulative distribution is linear
 */
void
phFloatArrayStats(float *arr,			/* the array */
		  int n,			/* number of elements */
		  int sorted,			/* is the array sorted? */
		  float *median,		/* desired median */
		  float *iqr,			/* desired IQR */
                  float *q25,                   /* 25% quartile */
                  float *q75)                   /* 75% quartile */
{
   float fi;				/* floating index of qn */
   int i;				/* truncated index of qn */
   int i0, i1;				/* range of tied values */
   int j;
   float q[3];				/* quartiles and median */
   float w;

   shAssert(n >= 1);

   if(n == 1) {
      *median = arr[0];
      if (iqr != NULL) {
         *iqr = 1e10;			/* Inf would be better */
      }
      return;
   }

   if(!sorted) {
      qsort(arr, n, sizeof(float),(int (*)(const void *, const void *))compar);
   }

   if(n == 1) {
      q[0] = q[1] = q[2] = arr[0];
   } else if(n == 2) {
      q[0] = arr[0];
      q[1] = 0.5*(arr[0] + arr[1]);
      q[2] = arr[1];
   } else {
      for(j = 0;j < 3;j++) {
	 fi = 0.25*(j + 1)*n - 0.499999;
	 i = (int)fi;
	 for(i0 = i; i0 > 0     && arr[i0 - 1] == arr[i]; i0--) continue;
	 for(i1 = i; i1 < n - 2 && arr[i1 + 1] == arr[i]; i1++) continue;

	 w = (fi - i0)/(i1 - i0 + 1);
	 shAssert(i >= 0 && i < n - 1 && i1 >= 0 && i1 < n - 1);
	 q[j] = (1 - w)*arr[i] + w*arr[i1 + 1];
      }
   }

   *median = q[1];
   if (iqr != NULL) *iqr = q[2] - q[0];
   if (q25 != NULL) *q25 = q[0];
   if (q75 != NULL) *q75 = q[2];
}


/*****************************************************************************/
/*
 * Working routine for phFloatArrayStatsErr()
 */
typedef struct {
   float val;				/* desired value */
   int n;				/* number of data values */
   float *arr;				/* data values */
   float *da;				/* errors in data */
} ARR_DA;

/*
 * Evaluate (the cumulative probability of a value x) - p->val
 */
static double
eval_cprob(double x,			/* point to evaluate function */
	   const void *vp)		/* user data */
{
   const ARR_DA *p = vp;
   const double val = p->val;
   const int n = p->n;
   const float *arr = p->arr;
   const float *da = p->da;
   double cprob = 0;			/* the cumulative probability of
					   a value <= x */
   int i;

   cprob = 0;
   for(i = 0; i < n; i++) {
      float p = 0.5*(1 + erf_ph((x - arr[i])/(M_SQRT2*da[i])));
      cprob += p;
   }
   cprob /= n;

   return(cprob - val);
}

/*
 * <AUTO EXTRACT>
 * Like phFloatArrayStats(), but includes error estimates for each data value
 */
void
phFloatArrayStatsErr(const float *arr,	/* the array */
		     const float *da,	/* error in arr */
		     int n,		/* number of elements */
		     float *median,	/* desired median */
		     float *iqr,	/* desired IQR */
		     float *q25,	/* 25% quartile */
		     float *q75)	/* 75% quartile */
{
   float ax = 0, bx = 1;		/* range of data */
   ARR_DA data;				/* data for zero finder */
   int i;
   double quart[3];			/* the three quartiles */
   double tol = 0;			/* desired tolerance */

   shAssert(n >= 1);

   data.n = n;
   data.arr = (float *)arr;
   data.da = (float *)da;
/*
 * Find points bounding the desired statistics 
 */
   for(i = 0; i < n; i++) {
      if(i == 0 || arr[i] - 2*da[i] < ax) {
	 ax = arr[i] - 2*da[i];
      }
      if(i == 0 || arr[i] + 2*da[i] > bx) {
	 bx = arr[i] + 2*da[i];
      }
   }
   data.val = 0;			/* so eval_cprob returns cumul. prob */
   shAssert(eval_cprob(ax, &data) <= 0.25);
   shAssert(eval_cprob(bx, &data) >= 0.75);

   for(i = 0; i < 3; i++) {
      if(i == 0) {
	 if(q25 == NULL && iqr == NULL) {
	    continue;
	 }
      } else if(i == 1) {
	 if(q25 != NULL) {
	    if(quart[0] > ax) {
	       ax = quart[0];
	    }
	 }
	 if(median == NULL) {
	    continue;
	 }
      } else {	
	 if(median != NULL) {
	    if(quart[1] > ax) {
	       ax = quart[1];
	    }
	 }

	 if(q75 == NULL && iqr == NULL) {
	    continue;
	 }
      }

      data.val = 0.25*(i + 1);
      phZeroFind(ax, bx, eval_cprob, &data, tol, &quart[i]);
   }

   if(q25 != NULL) {
      *q25 = quart[0];
   }
   if(median != NULL) {
      *median = quart[1];
   }
   if(q75 != NULL) {
      *q75 = quart[2];
   }
   if(iqr != NULL) {
      *iqr = quart[2] - quart[0];
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return the simple mean of an array
 */
void
phFloatArrayMean(const float *arr,	/* array of data */
		 int n,			/* number of elements */
		 float *meanp,		/* mean; or NULL */
		 float *sigmap)		/* s.d.; or NULL */
{
   int i;
   double mean;				/* mean value of array */
   double sum;
   
   if(n <= 0) {
      if(meanp != NULL) *meanp = 0;
      if(sigmap != NULL) *sigmap = -1;
      return;
   }
/*
 * calculate mean
 */
   mean = 0;
   for(i = 0; i < n; i++) {
      mean += arr[i];
   }
   mean /= n;

   if(meanp != NULL) {
      *meanp = mean;
   }
/*
 * and standard deviation
 */
   if(sigmap == NULL) {
      return;
   }

   if(n == 1) {
      *sigmap = 0;
      return;
   }

   sum = 0;
   for(i = 0; i < n; i++) {
      double tmp = arr[i] - mean;
      sum += tmp*tmp;
   }

   *sigmap = sqrt(sum/(n - 1));
}

/*****************************************************************************/
/*
 * fit a straight line to a set of points, y = a + b*t
 *
 * Any or all of the pointer a, aErr, covar, b, bErr may be NULL
 *
 * Return the value of chi^2 for the fit
 */
float
phLinearFit(const float *t,		/* "x" values */
	    const float *y_i,		/* input "y" values */
	    const float *yErr,		/* errors in y */
	    int n,			/* number of points to fit */
	    float *a,			/* estimated y-value at t == 0 */
	    float *aErr,		/* error in a */
	    float *covar,		/* covariance of a and b (not sqrt) */
	    float *b,			/* value of slope */
	    float *bErr)		/* error in b */
{
   double det;				/* determinant needed to find a,b */
   int i;
   float ivar;				/* == 1/yErr^2 */
   double sum, sum_t, sum_tt, sum_y, sum_ty;
   float *y = alloca(n*sizeof(float));	/* == y_i[] - ybar */
   double ybar;				/* straight mean of y */

   shAssert(n > 0);
/*
 * Did they provide pointers to return the answers?
 */
   {
      float s_a, s_aErr, s_covar, s_b, s_bErr; /* possible storage for a etc.*/

      if(a == NULL) a = &s_a;
      if(aErr == NULL) aErr = &s_aErr;
      if(covar == NULL) covar = &s_covar;
      if(b == NULL) b = &s_b;
      if(bErr == NULL) bErr = &s_bErr;
   }
/*
 * Start by subtracting off y's mean, to improve numerical stability
 */
   sum_y = 0.0;
   for(i = 0; i < n; i++) {
      sum_y += y_i[i];
   }
   ybar = sum_y/n;
/*
 * Now fit to data
 */
   sum = sum_t = sum_tt = sum_y = sum_ty = 0.0;
   for(i = 0; i < n; i++) {
      y[i] = y_i[i] - ybar;

      ivar = (yErr[i] < 1e-10) ? 1e20 : 1/(yErr[i]*yErr[i]);
      sum += ivar;
      sum_t += ivar*t[i];
      sum_tt += ivar*t[i]*t[i];
      sum_y += ivar*y[i];
      sum_ty += ivar*y[i]*t[i];
   }
   det = (sum*sum_tt - sum_t*sum_t);
   if(fabs(det) < 1e-10) {
      *a = *aErr = VALUE_IS_BAD;
      *covar = 0;
      *b = 0; *bErr = VALUE_IS_BAD;
      return(-1);
   }

   *a =  (sum_tt*sum_y - sum_t*sum_ty)/det;
   *aErr = sqrt(sum_tt/det);
   *covar = -sum_t/det;
   *b = (sum*sum_ty - sum_t*sum_y)/det;
   *bErr = sqrt(sum/det);

   if(*aErr > 1e10) { *aErr = ERROR_IS_BAD; }
   if(*bErr > 1e10) { *bErr = ERROR_IS_BAD; }
/*
 * Find chi^2
 */
   sum = 0;
   for(i = 0; i < n; i++) {
      ivar = (yErr[i] < 1e-10) ? 1e10 : 1/yErr[i];
      sum += pow(ivar*((*a) + (*b)*t[i] - y[i]), 2);
   }

   *a += ybar;

   return(sum);
}
