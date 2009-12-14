/**************************************************************************
**
** Copyright (C) 1993 David E. Steward & Zbigniew Leyk, all rights reserved.
**
**			     Meschach Library
** 
** This Meschach Library is provided "as is" without any express 
** or implied warranty of any kind with respect to this software. 
** In particular the authors shall not be liable for any direct, 
** indirect, special, incidental or consequential damages arising 
** in any way from use of the software.
** 
** Everyone is granted permission to copy, modify and redistribute this
** Meschach Library, provided:
**  1.  All copies contain this copyright notice.
**  2.  All modified copies shall carry a notice stating who
**      made the last modification and the date of such modification.
**  3.  No charge is made for this software or works derived from it.  
**      This clause shall not be construed as constraining other software
**      distributed on the same medium as this software, nor is a
**      distribution fee considered a charge.
**
**
** Robert Lupton (rhl@astro.princeton.edu) modified this software in
** March 1997. Specifically, I reduced it to a minimum number of routines
** needed to do an SVD decomposition of a matrix, and put these routines
** together in the file.
**
** At the same time, I disabled and/or removed much of the memory management
** code in meschach, changed the error handling, and took function names
** out of the ANSI reserved namespace (e.g. __ip__ ==> inner_product)
**
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <alloca.h>
#include "dervish.h"
#include "atConversions.h"		/* for M_PI, M_SQRT2 */
#include "phMeschach.h"

static VEC *get_col(MAT *mat, unsigned int col, VEC *vec);
static VEC *get_row(MAT *mat, unsigned int row, VEC *vec);
static void givens(double x, double y, Real *c, Real *s);
static MAT *hhtrcols(MAT *M, unsigned int i0, unsigned int j0,
		     VEC *hh, double beta);
static MAT *hhtrrows(MAT *M, unsigned int i0, unsigned int j0,
		     VEC *hh, double beta);
static VEC *hhtrvec(VEC *hh, double beta, unsigned int i0,VEC *in, VEC *out);
static VEC *hhvec(VEC *vec, unsigned int i0, Real *beta, VEC *out,
		  Real *newval);
static double in_prod(VEC *a, VEC *b, unsigned int i0);
static double inner_product(register Real *dp1, register Real *dp2, int len);
static void mltadd(register Real *dp1, register Real *dp2,
		   register double s, register int len);
static MAT *m_copy(const MAT *in, MAT *out);
static MAT *m_ident(MAT *A);
static MAT *m_resize(MAT *A, int new_m, int new_n);
static MAT *rot_cols(MAT *mat, unsigned int i, unsigned int k,
		     double c, double s, MAT *out);
static MAT *rot_rows(MAT *mat, unsigned int i, unsigned int k,
		     double c, double s, MAT *out);
static VEC *v_copy(VEC *in, VEC *out, unsigned int i0);
static double v_norm_inf(VEC *x);
static VEC *v_resize(VEC *x, int new_dim);

#define	MAX(a,b)	((a) > (b) ? (a) : (b))
#define	MIN(a,b)	((a) > (b) ? (b) : (a))
#define	SGN(x)	((x) >= 0 ? 1 : -1)

/*****************************************************************************/
/*
 * first a stub to force the loading of this library
 */
void
phMeschachLoad(void)
{
   ;
}

/*****************************************************************************/
/*
 * Memory allocation.  Experiment shows that the PSP makes very Very heavy
 * use of ph{Mat,Vec}New, and that this causes very significant memory
 * fragmentation
 *
 * In consequence, we'll maintain our own free list for these objects
 */
#define MAT_FREELIST 1			/* use a freelist for MATs? */
#define VEC_FREELIST 1			/* use a freelist for VECs? */

#if MAT_FREELIST
   static MAT *mat_freelist = NULL;
#endif
#if VEC_FREELIST
   static VEC *vec_freelist = NULL;
#endif

void
phMeschachInit(void)
{
   ;
}

void
phMeschachFini(void)
{
#if MAT_FREELIST
   {
      MAT *tmp;
      MAT *mat = mat_freelist;		/* dealias mat_freelist */
      
      while(mat != NULL) {
	 tmp = mat;
	 mat = mat->link;		/* next MAT on chain */
	 
	 if(tmp->base != NULL) {
	    shFree(tmp->base);
	 }
	 if(tmp->me != NULL) {
	    shFree(tmp->me);
	 }
	 
	 shFree(tmp);
      }
      
      mat_freelist = NULL;
   }
#endif

#if VEC_FREELIST
   {
      VEC *tmp;
      VEC *vec = vec_freelist;		/* dealias vec_freelist */
      
      while(vec != NULL) {
	 tmp = vec;
	 vec = vec->link;		/* next VEC on chain */
	 
	 if(tmp->ve != NULL) {
	    shFree(tmp->ve);
	 }
	 
	 shFree(tmp);
      }
      
      vec_freelist = NULL;
   }
#endif
}


/*
   File containing routines for computing the SVD of matrices
*/

#define	MAX_STACK	100

/* fixsvd -- fix minor details about SVD
	-- make singular values non-negative
	-- sort singular values in decreasing order
	-- variables as for bisvd()
	-- no argument checking */
static void
fixsvd(VEC *d,
       MAT *U,
       MAT *V)
{
    int		i, j, k, l, r, stack[MAX_STACK], sp;
    Real	tmp, v;

    /* make singular values non-negative */
    for ( i = 0; i < d->dim; i++ )
	if ( d->ve[i] < 0.0 )
	{
	    d->ve[i] = - d->ve[i];
	    if ( U != NULL )
		for ( j = 0; j < U->m; j++ )
		    U->me[i][j] = - U->me[i][j];
	}

    /* sort singular values */
    /* nonrecursive implementation of quicksort due to R.Sedgewick,
       "Algorithms in C", p. 122 (1990) */
    sp = -1;
    l = 0;	r = d->dim - 1;
    for ( ; ; )
    {
	while ( r > l )
	{
	    /* i = partition(d->ve,l,r) */
	    v = d->ve[r];

	    i = l - 1;	    j = r;
	    for ( ; ; )
	    {	/* inequalities are "backwards" for **decreasing** order */
		while ( d->ve[++i] > v )
		    ;
		while ( j > 0 && d->ve[--j] < v )
		    ;
		if ( i >= j )
		    break;
		/* swap entries in d->ve */
		tmp = d->ve[i];	  d->ve[i] = d->ve[j];	d->ve[j] = tmp;
		/* swap rows of U & V as well */
		if ( U != NULL )
		    for ( k = 0; k < U->n; k++ )
		    {
			tmp = U->me[i][k];
			U->me[i][k] = U->me[j][k];
			U->me[j][k] = tmp;
		    }
		if ( V != NULL )
		    for ( k = 0; k < V->n; k++ )
		    {
			tmp = V->me[i][k];
			V->me[i][k] = V->me[j][k];
			V->me[j][k] = tmp;
		    }
	    }
	    tmp = d->ve[i];    d->ve[i] = d->ve[r];    d->ve[r] = tmp;
	    if ( U != NULL )
		for ( k = 0; k < U->n; k++ )
		{
		    tmp = U->me[i][k];
		    U->me[i][k] = U->me[r][k];
		    U->me[r][k] = tmp;
		}
	    if ( V != NULL )
		for ( k = 0; k < V->n; k++ )
		{
		    tmp = V->me[i][k];
		    V->me[i][k] = V->me[r][k];
		    V->me[r][k] = tmp;
		}
	    /* end i = partition(...) */
	    if ( i - l > r - i )
	    {	stack[++sp] = l;    stack[++sp] = i-1;	l = i+1;    }
	    else
	    {	stack[++sp] = i+1;  stack[++sp] = r;	r = i-1;    }
	}
	if ( sp < 0 )
	    break;
	r = stack[sp--];	l = stack[sp--];
    }
}


/* bisvd -- svd of a bidiagonal m x n matrix represented by d (diagonal) and
			f (super-diagonals)
	-- returns with d set to the singular values, f zeroed
	-- if U, V non-NULL, the orthogonal operations are accumulated
		in U, V; if U, V == I on entry, then SVD == U^T.A.V
		where A is initial matrix
	-- returns d on exit */
static VEC	*
bisvd(VEC *d, VEC *f, MAT *U, MAT *V)
{
	int	i, j, n;
	int	i_min, i_max, split;
	Real	c, s, shift, size, z;
	Real	d_tmp, diff, t11, t12, t22, *d_ve, *f_ve;

	shAssert(d != NULL && f != NULL && d->dim == f->dim + 1);
	shAssert(U == NULL || (U->m == U->n && U->n >= d->dim));
	shAssert(V == NULL || (V->m == V->n && V->m >= d->dim));

	n = d->dim;

	if ( n == 1 )
		return d;
	d_ve = d->ve;	f_ve = f->ve;

	size = v_norm_inf(d) + v_norm_inf(f);

	i_min = 0;
	while ( i_min < n )	/* outer while loop */
	{
	    /* find i_max to suit;
		submatrix i_min..i_max should be irreducible */
	    i_max = n - 1;
	    for ( i = i_min; i < n - 1; i++ )
		if ( d_ve[i] == 0.0 || f_ve[i] == 0.0 )
		{   i_max = i;
		    if ( f_ve[i] != 0.0 )
		    {
			/* have to ``chase'' f[i] element out of matrix */
			z = f_ve[i];	f_ve[i] = 0.0;
			for ( j = i; j < n-1 && z != 0.0; j++ )
			{
			    givens(d_ve[j+1],z, &c, &s);
			    s = -s;
			    d_ve[j+1] =  c*d_ve[j+1] - s*z;
			    if ( j+1 < n-1 )
			    {
				z         = s*f_ve[j+1];
				f_ve[j+1] = c*f_ve[j+1];
			    }
			    if ( U )
				rot_rows(U,i,j+1,c,s,U);
			}
		    }
		    break;
		}
	    if ( i_max <= i_min )
	    {
		i_min = i_max + 1;
		continue;
	    }

	    split = 0;
	    while ( ! split )
	    {
		/* compute shift */
		t11 = d_ve[i_max-1]*d_ve[i_max-1] +
			(i_max > i_min+1 ? f_ve[i_max-2]*f_ve[i_max-2] : 0.0);
		t12 = d_ve[i_max-1]*f_ve[i_max-1];
		t22 = d_ve[i_max]*d_ve[i_max] + f_ve[i_max-1]*f_ve[i_max-1];
		/* use e-val of [[t11,t12],[t12,t22]] matrix
				closest to t22 */
		diff = (t11-t22)/2;
		shift = t22 - t12*t12/(diff +
			SGN(diff)*sqrt(diff*diff+t12*t12));

		/* initial Givens' rotation */
		givens(d_ve[i_min]*d_ve[i_min]-shift,
			d_ve[i_min]*f_ve[i_min], &c, &s);

		/* do initial Givens' rotations */
		d_tmp         = c*d_ve[i_min] + s*f_ve[i_min];
		f_ve[i_min]   = c*f_ve[i_min] - s*d_ve[i_min];
		d_ve[i_min]   = d_tmp;
		z             = s*d_ve[i_min+1];
		d_ve[i_min+1] = c*d_ve[i_min+1];
		if ( V )
		    rot_rows(V,i_min,i_min+1,c,s,V);
		/* 2nd Givens' rotation */
		givens(d_ve[i_min],z, &c, &s);
		d_ve[i_min]   = c*d_ve[i_min] + s*z;
		d_tmp         = c*d_ve[i_min+1] - s*f_ve[i_min];
		f_ve[i_min]   = s*d_ve[i_min+1] + c*f_ve[i_min];
		d_ve[i_min+1] = d_tmp;
		if ( i_min+1 < i_max )
		{
		    z             = s*f_ve[i_min+1];
		    f_ve[i_min+1] = c*f_ve[i_min+1];
		}
		if ( U )
		    rot_rows(U,i_min,i_min+1,c,s,U);

		for ( i = i_min+1; i < i_max; i++ )
		{
		    /* get Givens' rotation for zeroing z */
		    givens(f_ve[i-1],z, &c, &s);
		    f_ve[i-1] = c*f_ve[i-1] + s*z;
		    d_tmp     = c*d_ve[i] + s*f_ve[i];
		    f_ve[i]   = c*f_ve[i] - s*d_ve[i];
		    d_ve[i]   = d_tmp;
		    z         = s*d_ve[i+1];
		    d_ve[i+1] = c*d_ve[i+1];
		    if ( V )
			rot_rows(V,i,i+1,c,s,V);
		    /* get 2nd Givens' rotation */
		    givens(d_ve[i],z, &c, &s);
		    d_ve[i]   = c*d_ve[i] + s*z;
		    d_tmp     = c*d_ve[i+1] - s*f_ve[i];
		    f_ve[i]   = c*f_ve[i] + s*d_ve[i+1];
		    d_ve[i+1] = d_tmp;
		    if ( i+1 < i_max )
		    {
			z         = s*f_ve[i+1];
			f_ve[i+1] = c*f_ve[i+1];
		    }
		    if ( U )
			rot_rows(U,i,i+1,c,s,U);
		}
		/* should matrix be split? */
		for ( i = i_min; i < i_max; i++ )
		    if ( fabs(f_ve[i]) <
				MACHEPS*(fabs(d_ve[i])+fabs(d_ve[i+1])) )
		    {
			split = 1;
			f_ve[i] = 0.0;
		    }
		    else if ( fabs(d_ve[i]) < MACHEPS*size )
		    {
			split = 1;
			d_ve[i] = 0.0;
		    }
		}
	}
	fixsvd(d,U,V);

	return d;
}

/* bifactor -- perform preliminary factorisation for bisvd
	-- updates U and/or V, which ever is not NULL */
static MAT *
bifactor(MAT *A,
	 MAT *U,
	 MAT *V)
{
	int	k;
	VEC	*tmp1, *tmp2;
	Real	beta;

	shAssert(A != NULL);

	shAssert(U == NULL || (U->m == U->n && U->m == A->m));
	shAssert(V == NULL || (V->m == V->n && V->m == A->n));

	tmp1 = phVecNew(A->m);
	tmp2 = phVecNew(A->n);

	if ( A->m >= A->n )
	    for ( k = 0; k < A->n; k++ )
	    {
		get_col(A,k,tmp1);
		hhvec(tmp1,k,&beta,tmp1,&(A->me[k][k]));
		hhtrcols(A,k,k+1,tmp1,beta);
		if ( U )
		    hhtrcols(U,k,0,tmp1,beta);
		if ( k+1 >= A->n )
		    continue;
		get_row(A,k,tmp2);
		hhvec(tmp2,k+1,&beta,tmp2,&(A->me[k][k+1]));
		hhtrrows(A,k+1,k+1,tmp2,beta);
		if ( V )
		    hhtrcols(V,k+1,0,tmp2,beta);
	    }
	else
	    for ( k = 0; k < A->m; k++ )
	    {
		get_row(A,k,tmp2);
		hhvec(tmp2,k,&beta,tmp2,&(A->me[k][k]));
		hhtrrows(A,k+1,k,tmp2,beta);
		if ( V )
		    hhtrcols(V,k,0,tmp2,beta);
		if ( k+1 >= A->m )
		    continue;
		get_col(A,k,tmp1);
		hhvec(tmp1,k+1,&beta,tmp1,&(A->me[k+1][k]));
		hhtrcols(A,k+1,k+1,tmp1,beta);
		if ( U )
		    hhtrcols(U,k+1,0,tmp1,beta);
	    }

	phVecDel(tmp1); phVecDel(tmp2);

	return A;
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Calculate a matrix's singular values, and updates U and/or V, if one
 * or the other is non-NULL.
 *
 * The definition employed of U and V, namely
 *              U^T*diag(w)*V == A
 * differs from Press' version (U*diag(w)*V^T == A).
 *
 * Note that the input matrix A is destroyed
 */
VEC *
phSvd(MAT *A,				/* I: input m*n matrix */
      MAT *U,				/* I: U and V matricies; m*m and n*n */
      MAT *V,				/*			 may be NULL */
      VEC *d)				/* O: singular values. If non-NULL
					   is identical to the return value */
{
   VEC	*f=NULL;
   int	i, limit;

   shAssert(A != NULL);
   
   if(U != NULL) {
      if(U->m != U->n || U->m != A->m) {
	 shError("phSvd: U must be square, wth U->m == A->m");
	 return(NULL);
      }
   }
   if(V != NULL) {
      if(V->m != V->n || V->m != A->n) {
	 shError("phSvd: V must be square, wth V->m == A->n");
	 return(NULL);
      }
   }
   
   if ( U != NULL ) m_ident(U);
   if ( V != NULL ) m_ident(V);
   
   limit = MIN(A->m,A->n);
   d = v_resize(d,limit);
   f = phVecNew(limit-1);
   
   bifactor(A,U,V);
   if ( A->m >= A->n ) {
      for ( i = 0; i < limit; i++ ) {
	 d->ve[i] = A->me[i][i];
	 if ( i+1 < limit ) f->ve[i] = A->me[i][i+1];
      }
   } else {
      for ( i = 0; i < limit; i++ ) {
	 d->ve[i] = A->me[i][i];
	 if ( i+1 < limit ) f->ve[i] = A->me[i+1][i];
      }
   }
   
   if ( A->m >= A->n )
     bisvd(d,f,U,V);
   else
     bisvd(d,f,V,U);
   
   phVecDel(f);
   
   return d;
}

/*****************************************************************************/
/*
 * This one is written by RHL, but is freely distributable
 */
/*
 * <AUTO EXTRACT>
 *
 * back substitute from the equation x = V^T*diag(1/w)*(U*b)
 * (N.b. transposed from Press et al.'s convention, cf NR Eq. 2.6.7)
 *
 * x is allocated and returned
 */
VEC *
phSvdBackSub(const MAT *U,		/* SVD */
	     const MAT *V,		/*    decomposition */
	     const VEC *w,		/*        of design matrix */
	     const VEC *b)		/* RHS */
{
   int i, j;
   int m, n;				/* == V->[mn] */
   Real *Ume_i;				/* == unalias U->me[i] */
   Real *bve;				/* == unalias b->ve*/
   Real *vtmp;				/* temp vector */
   Real val;
   Real **Vme;				/* == V->me */
   VEC *x;				/* the desired answer */

   shAssert(U != NULL && U->me != NULL && V != NULL && V->me != NULL);
   shAssert(w != NULL && w->ve != NULL && b != NULL && b->ve != NULL);
   
   m = U->m; n = U->n;
   vtmp = alloca(n*sizeof(Real));

   for(i = 0;i < m;i++) {
      if(w->ve[i] == 0.0) {
	 vtmp[i] = 0;
      } else {
	 val = 0;
	 Ume_i = U->me[i];
	 bve = b->ve;
	 for(j = 0;j < n;j++) {
	    val += Ume_i[j]*bve[j];
	 }
	 vtmp[i] = val/w->ve[i];
      }
   }

   x = phVecNew(m);
   Vme = V->me;
   for(i = 0;i < m;i++) {
      val = 0;
      for(j = 0;j < m;j++) {
	 val += Vme[j][i]*vtmp[j];
      }
      x->ve[i] = val;
   }

   return(x);
}


/*****************************************************************************/
/*
 * This routine is written by RHL, but is freely distributable 
 */
/*
 * <AUTO EXTRACT>
 *
 * Calculate the Covariance of parameters estimated via SVD of the
 * design matrix
 *
 * This result is in Press et al., (Eq. 15.4.20), but remember that our
 * V is their V^T
 */
void
phSvdCovarGet(MAT *covar,
	      const MAT *V,
	      const VEC *w)
{
   int i, j, k;
   int n;
   Real val;

   shAssert(covar != NULL && covar->me != NULL && covar->n == covar->m);
   shAssert(V != NULL && V->me != NULL && V->n == V->m);
   shAssert(w != NULL && w->ve != NULL);

   n = covar->n;

   shAssert(V->n == n && w->dim == n);

   for(i = 0;i < n;i++) {
      for(j = 0;j < n;j++) {
	 val = 0;
	 for(k = 0;k < n;k++) {
	    if(w->ve[k] != 0.0) {
	       val += V->me[i][k]*V->me[j][k]/(w->ve[k]*w->ve[k]);
	    }
	 }
	 covar->me[i][j] = val;
      }
   }
}

/*
 * <AUTO EXTRACT>
 *
 * Calculate the Covariance of parameters estimated by solving the normal
 * equations by diagonalising the left hand side
 */
void
phEigenCovarGet(MAT *covar,
		const MAT *Q,
		const VEC *lambda)
{
   phSvdCovarGet(covar, Q, lambda);
}

/*****************************************************************************/
/*
 * RHL again.
 *
 * Given the SVD decomposition of a square matrix, return its
 * inverse V^T*diag(1/w)*U
 *
 * N.b. You are expected to provide the _inverse_ singular values, and to
 * have dealt with any that are too small before calling this routine
 *
 * The inverse is returned; if inv is non-NULL it will be used for the inverse,
 * otherwise a MAT will be allocated
 */
MAT *
phSvdInvert(MAT *inv,			/* inverse matrix (or NULL) */
	    const MAT *U,		/* SVD */
	    const MAT *V,		/*    decomposition */
	    const VEC *iw)		/*        of design matrix;
					   n.b. _inverse_ of singular values */
{
   int i, j, k;
   int n;				/* == U->n */
   Real val;

   shAssert(U != NULL && U->me != NULL && U->m == U->n);
   n = U->n;
   shAssert(V != NULL && V->me != NULL && V->m == n && V->n == n);
   shAssert(iw != NULL && iw->ve != NULL && iw->dim == n);
   shAssert(inv == NULL || (inv->me != NULL && inv->m == n && inv->n == n));

   if(inv == NULL) {
      inv = phMatNew(n, n);
   }
/*
 * OK, we have all checked (or created) all the desired matrices, so do
 * the inversion
 */
   for(i = 0; i < n; i++) {
      for(j = 0; j < n; j++) {
	 val = 0;
	 for(k = 0; k < n; k++) {
	    val += V->me[k][i]*iw->ve[k]*U->me[k][j];
	 }
	 inv->me[i][j] = val;
      }
   }

   return(inv);
}

/*****************************************************************************/
/*
 * invert a matrix, specified by its eigen values and vectors,
 *   inverse = Q diag(1/lambda) Q^T
 *
 * N.b. You are expected to provide the _inverse_ eigen values, and to
 * have dealt with any that are too small before calling this routine
 *
 * The inverse is returned; if inv is non-NULL it will be used for the inverse,
 * otherwise a MAT will be allocated
 */
MAT *
phEigenInvert(MAT *inv,			/* inverse matrix (or NULL) */
	      const MAT *Q,		/* eigen vectors */
	      const VEC *ilambda)	/* inverse eigen values */
{
   int i, j, k;
   int n;				/* == U->n */
   Real val;

   shAssert(Q != NULL && Q->me != NULL && Q->m == Q->n);
   n = Q->n;
   shAssert(ilambda != NULL && ilambda->ve != NULL && ilambda->dim == n);
   shAssert(inv == NULL || (inv->me != NULL && inv->m == n && inv->n == n));

   if(inv == NULL) {
      inv = phMatNew(n, n);
   }
/*
 * OK, we have all checked (or created) all the desired matrices, so do
 * the inversion
 */
   for(i = 0; i < n; i++) {
      for(j = 0; j < n; j++) {
	 val = 0;
	 for(k = 0; k < n; k++) {
	    val += Q->me[i][k]*ilambda->ve[k]*Q->me[j][k];
	 }
	 inv->me[i][j] = val;
      }
   }

   return(inv);
}
	   
/*****************************************************************************/
/*
 * hessen.c
 */

/* Hfactor -- compute Hessenberg factorisation in compact form.
	-- factorisation performed in situ
	-- for details of the compact form see QRfactor.c and matrix2.doc */
static MAT *
Hfactor(MAT *A,
	VEC *diag,
	VEC *beta)
{
	VEC	*tmp1 = NULL;
	int	k, limit;

	shAssert(A != NULL && diag != NULL && beta != NULL);
	shAssert(A->m == A->n);
	shAssert(diag->dim >= A->m - 1 && beta->dim >= A->m - 1);
	limit = A->m - 1;

	tmp1 = v_resize(tmp1,A->m);

	for ( k = 0; k < limit; k++ )
	{
		get_col(A,k,tmp1);
		/* printf("the %d'th column = ");	v_output(tmp1); */
		hhvec(tmp1,k+1,&beta->ve[k],tmp1,&A->me[k+1][k]);
		diag->ve[k] = tmp1->ve[k+1];
		/* printf("H/h vector = ");	v_output(tmp1); */
		/* printf("from the %d'th entry\n",k+1); */
		/* printf("beta = %g\n",beta->ve[k]); */

		hhtrcols(A,k+1,k+1,tmp1,beta->ve[k]);
		hhtrrows(A,0  ,k+1,tmp1,beta->ve[k]);
		/* printf("A = ");		m_output(A); */
	}

	phVecDel(tmp1);

	return (A);
}

/* makeHQ -- construct the Hessenberg orthogonalising matrix Q;
	-- i.e. Hess M = Q.M.Q'	*/
static MAT *
makeHQ(MAT *H,
       VEC *diag,
       VEC *beta,
       MAT *Qout)
{
	int	i, j, limit, lim;
	VEC	*tmp1 = NULL, *tmp2 = NULL;

	shAssert(H != NULL && diag != NULL && beta != NULL);
	shAssert(H->m == H->n);

	limit = H->m - 1;
	shAssert(diag->dim >= limit && beta->dim >= limit);
	Qout = m_resize(Qout,H->m,H->m);

	tmp1 = v_resize(tmp1,H->m);
	tmp2 = v_resize(tmp2,H->m);

	for ( i = 0; i < H->m; i++ )
	{
		/* tmp1 = i'th basis vector */
		for ( j = 0; j < H->m; j++ )
		  tmp1->ve[j] = 0.0;
		tmp1->ve[i] = 1.0;

		/* apply H/h transforms in reverse order */
		for ( j = limit-1; j >= 0; j-- )
		{
			get_col(H,j,tmp2);
			tmp2->ve[j+1] = diag->ve[j];
			hhtrvec(tmp2,beta->ve[j],j+1,tmp1,tmp1);
		}

		/* insert into Qout */
		lim = MIN(Qout->m, tmp1->dim);
		for ( j=0; j<lim; j++ )
		  Qout->me[j][i] = tmp1->ve[j];
	}

	phVecDel(tmp1);
	phVecDel(tmp2);

	return (Qout);
}

/*
 * symmeig.c
 */

/* trieig -- finds eigenvalues of symmetric tridiagonal matrices
	-- matrix represented by a pair of vectors a (diag entries)
		and b (sub- & super-diag entries)
	-- eigenvalues in a on return */
static VEC *
trieig(VEC *a,
       VEC *b,
       MAT *Q)
{
	int	i, i_min, i_max, n, split;
	Real	*a_ve, *b_ve;
	Real	b_sqr, bk, ak1, bk1, ak2, bk2, z;
	Real	c, c2, cs, s, s2, d, mu;

	shAssert(a != NULL && b != NULL && a->dim == b->dim + 1);
	if(Q != NULL) {
	   shAssert(Q->m == a->dim && Q->m == Q->n);
	}

	n = a->dim;
	a_ve = a->ve;		b_ve = b->ve;

	i_min = 0;
	while ( i_min < n )		/* outer while loop */
	{
		/* find i_max to suit;
			submatrix i_min..i_max should be irreducible */
		i_max = n-1;
		for ( i = i_min; i < n-1; i++ )
		    if ( b_ve[i] == 0.0 )
		    {	i_max = i;	break;	}
		if ( i_max <= i_min )
		{
		    /* printf("# i_min = %d, i_max = %d\n",i_min,i_max); */
		    i_min = i_max + 1;
		    continue;	/* outer while loop */
		}

		/* printf("# i_min = %d, i_max = %d\n",i_min,i_max); */

		/* repeatedly perform QR method until matrix splits */
		split = 0;
		while ( ! split )		/* inner while loop */
		{

		    /* find Wilkinson shift */
		    d = (a_ve[i_max-1] - a_ve[i_max])/2;
		    b_sqr = b_ve[i_max-1]*b_ve[i_max-1];
		    mu = a_ve[i_max] - b_sqr/(d + SGN(d)*sqrt(d*d+b_sqr));
		    /* printf("# Wilkinson shift = %g\n",mu); */

		    /* initial Givens' rotation */
		    givens(a_ve[i_min]-mu,b_ve[i_min],&c,&s);
		    s = -s;
		    /* printf("# c = %g, s = %g\n",c,s); */
		    if ( fabs(c) < M_SQRT2 )
		    {	c2 = c*c;	s2 = 1-c2;	}
		    else
		    {	s2 = s*s;	c2 = 1-s2;	}
		    cs = c*s;
		    ak1 = c2*a_ve[i_min]+s2*a_ve[i_min+1]-2*cs*b_ve[i_min];
		    bk1 = cs*(a_ve[i_min]-a_ve[i_min+1]) +
						(c2-s2)*b_ve[i_min];
		    ak2 = s2*a_ve[i_min]+c2*a_ve[i_min+1]+2*cs*b_ve[i_min];
		    bk2 = ( i_min < i_max-1 ) ? c*b_ve[i_min+1] : 0.0;
		    z  = ( i_min < i_max-1 ) ? -s*b_ve[i_min+1] : 0.0;
		    a_ve[i_min] = ak1;
		    a_ve[i_min+1] = ak2;
		    b_ve[i_min] = bk1;
		    if ( i_min < i_max-1 )
			b_ve[i_min+1] = bk2;
		    if ( Q )
			rot_cols(Q,i_min,i_min+1,c,-s,Q);
		    /* printf("# z = %g\n",z); */
		    /* printf("# a [temp1] =\n");	v_output(a); */
		    /* printf("# b [temp1] =\n");	v_output(b); */

		    for ( i = i_min+1; i < i_max; i++ )
		    {
			/* get Givens' rotation for sub-block -- k == i-1 */
			givens(b_ve[i-1],z,&c,&s);
			s = -s;
			/* printf("# c = %g, s = %g\n",c,s); */

			/* perform Givens' rotation on sub-block */
		        if ( fabs(c) < M_SQRT2 )
		        {	c2 = c*c;	s2 = 1-c2;	}
		        else
		        {	s2 = s*s;	c2 = 1-s2;	}
		        cs = c*s;
			bk  = c*b_ve[i-1] - s*z;
			ak1 = c2*a_ve[i]+s2*a_ve[i+1]-2*cs*b_ve[i];
			bk1 = cs*(a_ve[i]-a_ve[i+1]) +
						(c2-s2)*b_ve[i];
			ak2 = s2*a_ve[i]+c2*a_ve[i+1]+2*cs*b_ve[i];
			bk2 = ( i+1 < i_max ) ? c*b_ve[i+1] : 0.0;
			z  = ( i+1 < i_max ) ? -s*b_ve[i+1] : 0.0;
			a_ve[i] = ak1;	a_ve[i+1] = ak2;
			b_ve[i] = bk1;
			if ( i < i_max-1 )
			    b_ve[i+1] = bk2;
			if ( i > i_min )
			    b_ve[i-1] = bk;
			if ( Q )
			    rot_cols(Q,i,i+1,c,-s,Q);
		        /* printf("# a [temp2] =\n");	v_output(a); */
		        /* printf("# b [temp2] =\n");	v_output(b); */
		    }

		    /* test to see if matrix should be split */
		    for ( i = i_min; i < i_max; i++ )
			if ( fabs(b_ve[i]) < MACHEPS*
					(fabs(a_ve[i])+fabs(a_ve[i+1])) )
			{   b_ve[i] = 0.0;	split = 1;	}

		    /* printf("# a =\n");	v_output(a); */
		    /* printf("# b =\n");	v_output(b); */
		}
	}

	return a;
}

/*****************************************************************************/
/*
 * fixeigen -- fix minor details about eigen value decomposition
 *  -- make eigen values non-negative
 *  -- sort eigen values in decreasing order
 *
 * Modified from fixsvd by RHL
 */
static void
fixeigen(VEC *lambda,
	 MAT *Q)
{
   int n;				/* == Q->n == Q->m == lambda->dim */
   int i, j, k, l, r, stack[MAX_STACK], sp;
   Real **Qme;				/* == Q->me */
   Real tmp, v;
   
   shAssert(lambda != NULL && Q != NULL);
   shAssert(lambda->dim == Q->m && lambda->dim == Q->n);
   n = Q->n;
   Qme = Q->me;
#if 0
/*
 * make eigenvalues non-negative
 */
   for(i = 0; i < n; i++) {
      if(lambda->ve[i] < 0.0) {
	 lambda->ve[i] = - lambda->ve[i];
	 for(j = 0; j < n; j++) {
	    Qme[j][i] = -Qme[j][i];
	 }
      }
   }
#endif
/*
 * sort eigenvalues
 *
 * nonrecursive implementation of quicksort due to R.Sedgewick,
 * "Algorithms in C", p. 122 (1990)
 */
   sp = -1;
   l = 0;	r = n - 1;
   for(;;) {
      while(r > l) {
	 /* i = partition(lambda->ve,l,r) */
	 v = lambda->ve[r];
	 
	 i = l - 1;	    j = r;
	 for(; ;) {
	    /* inequalities are "backwards" for **decreasing** order */
	    while(lambda->ve[++i] > v) continue;
	    while(j > 0 && lambda->ve[--j] < v) continue;
	    if(i >= j) {
	       break;
	    }
	    
	    /* swap entries in lambda->ve */
	    tmp = lambda->ve[i];
	    lambda->ve[i] = lambda->ve[j];
	    lambda->ve[j] = tmp;
	    /* swap columns of Q as well */
	    for(k = 0; k < n; k++) {
	       tmp = Qme[k][i];
	       Qme[k][i] = Qme[k][j];
	       Qme[k][j] = tmp;
	    }
	 }
	 tmp = lambda->ve[i];
	 lambda->ve[i] = lambda->ve[r];
	 lambda->ve[r] = tmp;
	 
	 for(k = 0; k < n; k++) {
	    tmp = Qme[k][i];
	    Qme[k][i] = Qme[k][r];
	    Qme[k][r] = tmp;
	 }
	 /* end i = partition(...) */
	 if(i - l > r - i) {
	    stack[++sp] = l;    stack[++sp] = i-1;	l = i+1;
	 } else {
	    stack[++sp] = i+1;  stack[++sp] = r;	r = i-1;
	 }
      }
      if(sp < 0) {
	 break;
      }
      r = stack[sp--];
      l = stack[sp--];
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Compute eigenvalues of a dense symmetric matrix, i.e.
 *	Q^T A Q = diag(lambda)
 *	Q Q^T = I
 *
 * A **must** be symmetric on entry; this is not checked
 *
 * The eigenvalues are returned (if out is non-NULL, they are stored in
 * this vector, otherwise a new one is allocated).
 */
VEC *
phEigen(const MAT *A,			/* input matrix; must by symmetric */
	MAT *Q,				/* columns are eigenvectors
					   may be NULL */
	VEC *out)			/* eigenvalues. May be NULL */
{
	int	i;
	MAT	*tmp = NULL;
	VEC	*b   = NULL, *diag = NULL, *beta = NULL;

	shAssert(A != NULL && A->m == A->n);
	out = v_resize(out,A->m);

	tmp  = m_copy(A,tmp);
	b    = v_resize(b,A->m - 1);
	diag = v_resize(diag,A->m);
	beta = v_resize(beta,A->m);

	Hfactor(tmp,diag,beta);
	if ( Q )
		makeHQ(tmp,diag,beta,Q);

	for ( i = 0; i < A->m - 1; i++ )
	{
		out->ve[i] = tmp->me[i][i];
		b->ve[i] = tmp->me[i][i+1];
	}
	out->ve[i] = tmp->me[i][i];
	trieig(out,b,Q);

	phMatDel(tmp);
	phVecDel(b);
	phVecDel(diag);
	phVecDel(beta);

	fixeigen(out, Q);

	return out;
}

/*****************************************************************************/
/*
 * This one is written by RHL, but is freely distributable
 */
/*
 * <AUTO EXTRACT>
 *
 * Solve the equation (Q diag(lambda) Q^T) x = b, where Q Q^T = I,
 * i.e. evaluate
 *   Q diag(1/lambda) Q^T b
 *
 * if lambda is 0.0, it's taken to be infinite (!), i.e. 1/lambda == 0
 *
 * x is allocated and returned
 */
VEC *
phEigenBackSub(const MAT *Q,
	       const VEC *lambda,
	       const VEC *b)
{
   int i, j;
   int m;				/* == U->m */
   Real **Qme, *Qme_i;			/* == unalias Q->me, Q->me[i] */
   Real *bve;				/* == unalias b->ve*/
   Real *vtmp;				/* temp vector */
   Real val;
   VEC *x;				/* the desired answer */

   shAssert(Q != NULL && Q->me != NULL && Q->m == Q->n);
   shAssert(lambda != NULL && lambda->ve != NULL && lambda->dim == Q->m);
   shAssert(b != NULL && b->ve != NULL && b->dim == Q->m);
   
   m = Q->m;
   Qme = Q->me;
   vtmp = alloca(m*sizeof(Real));

   for(i = 0;i < m;i++) {
      if(lambda->ve[i] == 0.0) {
	 vtmp[i] = 0;
      } else {
	 val = 0;
	 bve = b->ve;
	 for(j = 0;j < m;j++) {
	    val += Qme[j][i]*bve[j];
	 }
	 vtmp[i] = val/lambda->ve[i];
      }
   }

   x = phVecNew(m);
   for(i = 0;i < m;i++) {
      val = 0;
      Qme_i = Qme[i];
      for(j = 0;j < m;j++) {
	 val += Qme_i[j]*vtmp[j];
      }
      x->ve[i] = val;
   }

   return(x);
}

/*****************************************************************************/
/*
 * copy.c
 */

/* m_copy -- copies matrix into new area */
static MAT *
m_copy(const MAT *in, MAT *out)
{
	unsigned int i;

	shAssert(in != NULL);

	if ( in==out )
		return (out);
	if ( out==NULL || out->m < in->m || out->n < in->n )
		out = m_resize(out,in->m,in->n);

	for ( i=0; i < in->m; i++ )
	  memcpy(out->me[i],in->me[i],in->n*sizeof(Real));

	return (out);
}

/* v_copy -- copies vector into new area */

static VEC *
v_copy(VEC *in, VEC *out, unsigned int i0)
{
   shAssert(in != NULL && i0 < in->dim);

   if ( in==out )
     return (out);
   if ( out==NULL || out->dim < in->dim )
     out = v_resize(out,in->dim);
   
   memcpy(&out->ve[i0], &in->ve[i0], (in->dim - i0)*sizeof(Real));
   
   return (out);
}

/*
 * givens.c
 */

/*
		Files for matrix computations

	Givens operations file. Contains routines for calculating and
	applying givens rotations for/to vectors and also to matrices by
	row and by column.
*/

/* givens.c 1.2 11/25/87 */

/* givens -- returns c,s parameters for Givens rotation to
		eliminate y in the vector [ x y ]' */
static void
givens(double x, double y, Real *c, Real *s)
{
	Real	norm;

	norm = sqrt(x*x+y*y);
	if ( norm == 0.0 )
	{	*c = 1.0;	*s = 0.0;	}	/* identity */
	else
	{	*c = x/norm;	*s = y/norm;	}
}

/* rot_rows -- premultiply mat by givens rotation described by c,s */
static MAT *
rot_rows(MAT *mat, unsigned int i, unsigned int k, double c, double s,
	 MAT *out)
{
	unsigned int	j;
	Real	temp;

	shAssert(mat != NULL);

	shAssert(i < mat->m && k < mat->m);

	out = m_copy(mat,out);

	for ( j=0; j<mat->n; j++ )
	{
		temp = c*out->me[i][j] + s*out->me[k][j];
		out->me[k][j] = -s*out->me[i][j] + c*out->me[k][j];
		out->me[i][j] = temp;
	}

	return (out);
}

/* rot_cols -- postmultiply mat by givens rotation described by c,s */
static MAT	*
rot_cols(MAT *mat, unsigned int i, unsigned int k, double c, double s, MAT *out)
{
	unsigned int	j;
	Real	temp;

	shAssert(mat != NULL && i < mat->n && k < mat->n);
	out = m_copy(mat,out);

	for ( j=0; j<mat->m; j++ )
	{
		temp = c*out->me[j][i] + s*out->me[j][k];
		out->me[j][k] = -s*out->me[j][i] + c*out->me[j][k];
		out->me[j][i] = temp;
	}

	return (out);
}

/*
 * hsehldr.c
 */

/*
		Files for matrix computations

	Householder transformation file. Contains routines for calculating
	householder transformations, applying them to vectors and matrices
	by both row & column.
*/

/* hsehldr.c 1.3 10/8/87 */


/* hhvec -- calulates Householder vector to eliminate all entries after the
	i0 entry of the vector vec. It is returned as out. May be in-situ */
static VEC *
hhvec(VEC *vec, unsigned int i0, Real *beta, VEC *out, Real *newval)
{
	Real	norm;

	out = v_copy(vec,out,i0);
	norm = sqrt(in_prod(out,out,i0));
	if ( norm <= 0.0 )
	{
		*beta = 0.0;
		return (out);
	}
	*beta = 1.0/(norm * (norm+fabs(out->ve[i0])));
	if ( out->ve[i0] > 0.0 )
		*newval = -norm;
	else
		*newval = norm;
	out->ve[i0] -= *newval;

	return (out);
}

/* hhtrvec -- apply Householder transformation to vector -- may be in-situ */
static VEC	*
hhtrvec(VEC *hh,
	double beta,
	unsigned int i0,
	VEC *in,
	VEC *out)
{
	Real	scale;
	/* u_int	i; */

	shAssert(hh != NULL && in != NULL);
	shAssert(in->dim == hh->dim && i0 <= in->dim);

	scale = beta*in_prod(hh,in,i0);
	out = v_copy(in,out,0);
	mltadd(&(out->ve[i0]),&(hh->ve[i0]),-scale,(int)(in->dim-i0));
#if 0
	for ( i=i0; i<in->dim; i++ )
		out->ve[i] = in->ve[i] - scale*hh->ve[i];
#endif

	return (out);
}

/* hhtrrows -- transform a matrix by a Householder vector by rows
	starting at row i0 from column j0 -- in-situ */
static MAT *
hhtrrows(MAT *M, unsigned int i0, unsigned int j0, VEC *hh, double beta)
{
	Real	ip, scale;
	int	i /*, j */;

	shAssert(M != NULL && hh != NULL && M->n == hh->dim);
	shAssert(i0 <= M->m && j0 <= M->n);

	if ( beta == 0.0 )	return (M);

	/* for each row ... */
	for ( i = i0; i < M->m; i++ )
	{	/* compute inner product */
		ip = inner_product(&(M->me[i][j0]),&(hh->ve[j0]),(int)(M->n-j0));
		scale = beta*ip;
		if ( scale == 0.0 )
		    continue;

		/* do operation */
		mltadd(&(M->me[i][j0]),&(hh->ve[j0]),-scale,
							(int)(M->n-j0));
	}

	return (M);
}


/* hhtrcols -- transform a matrix by a Householder vector by columns
	starting at row i0 from column j0 -- in-situ */
static MAT	*
hhtrcols(MAT *M, unsigned int i0, unsigned int j0, VEC *hh, double beta)
{
	int	i;
	VEC	*w = NULL;

	shAssert(M != NULL && hh != NULL && M->m == hh->dim);
	shAssert(i0 <= M->m && j0 <= M->n);

	if ( beta == 0.0 )	return (M);

	w = phVecNew(M->n);
	phVecClear(w);

	for ( i = i0; i < M->m; i++ )
	    if ( hh->ve[i] != 0.0 )
		mltadd(&(w->ve[j0]),&(M->me[i][j0]),hh->ve[i],
							(int)(M->n-j0));
	for ( i = i0; i < M->m; i++ )
	    if ( hh->ve[i] != 0.0 )
		mltadd(&(M->me[i][j0]),&(w->ve[j0]),-beta*hh->ve[i],
							(int)(M->n-j0));
	phVecDel(w);
	return (M);
}

/*
 * init.c
 */

/*
	This is a file of routines for zero-ing, and initialising
	vectors, matrices and permutations.
	This is to be included in the matrix.a library
*/

/* phVecClear -- zero the vector x */
void
phVecClear(VEC *x)
{
   shAssert(x != NULL);

   memset(x->ve,'\0',x->dim*sizeof(Real));
   shAssert(x->ve[0] == 0.0);
}


/* phMatClear -- zero the matrix A */
void
phMatClear(MAT *A)
{
	int	i, A_m, A_n;
	Real	**A_me;

	shAssert(A != NULL);

	A_m = A->m;	A_n = A->n;	A_me = A->me;
	for ( i = 0; i < A_m; i++ )
	  memset(A_me[i],'\0',A_n*sizeof(Real));

	shAssert(A->me[0][0] == 0.0);
}

/* mat_id -- set A to being closest to identity matrix as possible
	-- i.e. A[i][j] == 1 if i == j and 0 otherwise */
static MAT *
m_ident(MAT *A)
{
	int	i, size;

	shAssert(A != NULL);

	phMatClear(A);
	size = MIN(A->m,A->n);
	for ( i = 0; i < size; i++ )
		A->me[i][i] = 1.0;

	return A;
}
/*
 * machine.c
 */

/**************************************************************************
**
** Copyright (C) 1993 David E. Stewart & Zbigniew Leyk, all rights reserved.
**
**			     Meschach Library
** 
** This Meschach Library is provided "as is" without any express 
** or implied warranty of any kind with respect to this software. 
** In particular the authors shall not be liable for any direct, 
** indirect, special, incidental or consequential damages arising 
** in any way from use of the software.
** 
** Everyone is granted permission to copy, modify and redistribute this
** Meschach Library, provided:
**  1.  All copies contain this copyright notice.
**  2.  All modified copies shall carry a notice stating who
**      made the last modification and the date of such modification.
**  3.  No charge is made for this software or works derived from it.  
**      This clause shall not be construed as constraining other software
**      distributed on the same medium as this software, nor is a
**      distribution fee considered a charge.
**
***************************************************************************/

/*
  This file contains basic routines which are used by the functions
  in meschach.a etc.
  These are the routines that should be modified in order to take
  full advantage of specialised architectures (pipelining, vector
  processors etc).
  */

/* inner_product -- inner product */
static double
inner_product(register Real *dp1, register Real *dp2, int len)
{
    register int	i;
    register Real     sum;

    sum = 0.0;
    for ( i = 0; i < len; i++ )
      sum += dp1[i]*dp2[i];
    
    return sum;
}

/* mltadd -- scalar multiply and add c.f. v_mltadd() */
static void
mltadd(register Real *dp1, register Real *dp2,
       register double s, register int len)
{
   register int	i;
   
   for ( i = 0; i < len; i++ )
     dp1[i] += s*dp2[i];
}

/*
 * memory.c
 */

/* memory.c 1.3 11/25/87 */

/* phMatNew -- gets an mxn matrix (in MAT form) by dynamic memory allocation */
MAT *
phMatNew(int m, int n)
{
   MAT	*matrix;
   int	i;

   shAssert(m >= 0 && n >= 0);

#if MAT_FREELIST			/* put it on the matrix free list */
   {
      MAT *prev = NULL;
      matrix = mat_freelist;
      while(matrix != NULL) {
	 if(matrix->m == m && matrix->n == n) {
	    break;
	 }
	 
	 prev = matrix;
	 matrix = matrix->link;
      }

      if(matrix != NULL) {
	 if(prev == NULL) {
	    mat_freelist = matrix->link;
	 } else {
 	    prev->link = matrix->link;
	 }
	 matrix->link = NULL;
	 return(matrix);
      }
   }
#endif
   
   matrix = shMalloc(sizeof(MAT));
   
   matrix->m = matrix->max_m = m;
   matrix->n = matrix->max_n = n;
   matrix->max_size = m*n;
   matrix->base = shMalloc(m*n*sizeof(Real));
   matrix->me = (Real **)shMalloc(m*sizeof(Real *));
   matrix->link = NULL;
   
   /* set up pointers */
   for ( i=0; i<m; i++ )
     matrix->me[i] = &(matrix->base[i*n]);
   
   return (matrix);
}

/* phMatDel -- returns MAT & asoociated memory back to memory heap */
void
phMatDel(MAT *mat)
{
   if(mat == NULL) {
      return;
   }
   
#if MAT_FREELIST			/* put it on the matrix free list */
#if 0
   mat->m = mat->max_m; mat->n = mat->max_n;
#endif
   mat->link = mat_freelist;
   mat_freelist = mat;
#else					/* free memory now */
   if(mat->base != NULL) {
      shFree(mat->base);
   }
   if(mat->me != NULL) {
      shFree(mat->me);
   }
   
   shFree(mat);
#endif
}

/*
 * <AUTO EXTRACT>
 *
 * Delete a single row and column from a matrix, returning the input
 * matrix, or NULL if no elements remain.
 *
 * row or col may be -1, indicating that no row/column should be deleted
 */
MAT *
phMatDelRowCol(MAT *mat,		/* the matrix in question */
	       int row,			/* row to delete, or -1 */
	       int col)			/* col to delete, or -1 */
{
   int i, j;
   
   if(mat == NULL) {
      return(NULL);
   }
   shAssert(row == -1 || (row >= 0 && row < mat->m));
   shAssert(col == -1 || (col >= 0 && col < mat->n));

   if(row >= 0) {			/* there's a row to delete */
      for(i = row; i < mat->m - 1; i++) {
	 mat->me[i] = mat->me[i + 1];
      }
      mat->m--;
   }
   
   if(col >= 0) {			/* there's a column to delete */
      for(i = 0; i < mat->m; i++) {
	 for(j = col; j < mat->n - 1; j++) {
	    mat->me[i][j] = mat->me[i][j + 1];
	 }
      }
      mat->n--;
   }
   
   if(mat->m == 0 || mat->n == 0) {	/* there's nothing left */
      phMatDel(mat);
      return(NULL);
   } else {
      return(mat);
   }
}

/* phVecNew -- gets a VEC of dimension 'dim'
   -- Note: not initialized to zero */
VEC *
phVecNew(int size)
{
   VEC	*vector;
   
   shAssert(size >= 0);

#if VEC_FREELIST			/* put it on the vector free list */
   {
      VEC *prev = NULL;
      vector = vec_freelist;
      while(vector != NULL) {
	 if(vector->dim == size) {
	    break;
	 }
	 
	 prev = vector;
	 vector = vector->link;
      }

      if(vector != NULL) {
	 if(prev == NULL) {
	    vec_freelist = vector->link;
	 } else {
 	    prev->link = vector->link;
	 }
	 vector->link = NULL;
	 return(vector);
      }
   }
#endif

   vector = shMalloc(sizeof(VEC));
   vector->dim = vector->max_dim = size;
   vector->ve = shMalloc(size*sizeof(Real));
   vector->link = NULL;
   
   return (vector);
}

/* phVecDel -- returns VEC & asoociated memory back to memory heap */

void
phVecDel(VEC *vec)
{
   if(vec == NULL) {
      return;
   }
   
#if VEC_FREELIST			/* put it on the vector free list */
#if 0
   vec->dim = vec->max_dim;
#endif
   vec->link = vec_freelist;
   vec_freelist = vec;
#else					/* free memory now */
   if(vec->ve != NULL) {
      shFree(vec->ve);
   }
   shFree(vec);
#endif
}


/*
 * <AUTO EXTRACT>
 *
 * Delete a single element from a vector, returning the input
 * vector, or NULL if no elements remain.
 *
 * the element may be -1, indicating that nothing should be deleted;
 * not very useful, but consistent with phMatDelRowCol
 */
VEC *
phVecDelElement(VEC *vec,		/* the vector in question */
		int el)			/* element to delete, or -1 */
{
   int i;
   
   if(vec == NULL) {
      return(NULL);
   }
   shAssert(el == -1 || (el >= 0 && el < vec->dim));

   if(el >= 0) {			/* there's an element to delete */
      for(i = el; i < vec->dim - 1; i++) {
	 vec->ve[i] = vec->ve[i + 1];
      }
      vec->dim--;
   }
   
   if(vec->dim == 0) {			/* there's nothing left */
      phVecDel(vec);
      return(NULL);
   } else {
      return(vec);
   }
}

/* m_resize -- returns the matrix A of size new_m x new_n; A is zeroed
   -- if A == NULL on entry then the effect is equivalent to phMatNew() */
static MAT *
m_resize(MAT *A, int new_m, int new_n)
{
   int	i;
   int	new_max_m, new_max_n, new_size, old_m, old_n;
#if !defined(NDEBUG)			/* check that 0.0 is all 0s */
   Real tmp = 1;
   memset(&tmp,'\0',sizeof(Real));
   shAssert(tmp == 0.0);
#endif
   
   shAssert(new_m >= 0 && new_n >= 0);

   if ( ! A )
     return phMatNew(new_m,new_n);

   /* nothing was changed */
   if (new_m == A->m && new_n == A->n)
     return A;

   old_m = A->m;	old_n = A->n;
   if ( new_m > A->max_m ) {		/* re-allocate A->me */
      shAssert(A->me != NULL);
      A->me = shRealloc(A->me,new_m*sizeof(Real *));
   }
   new_max_m = MAX(new_m,A->max_m);
   new_max_n = MAX(new_n,A->max_n);
   
   new_size = new_max_m*new_max_n;
   if ( new_size > A->max_size ) {	/* re-allocate A->base */
      A->base = shRealloc(A->base,new_size*sizeof(Real));
      A->max_size = new_size;
   }
   
   /* now set up A->me[i] */
   for ( i = 0; i < new_m; i++ )
     A->me[i] = &(A->base[i*new_n]);
   
   /* now shift data in matrix */
   if ( old_n > new_n ) {
      for ( i = 1; i < MIN(old_m,new_m); i++ )
	memcpy(&A->base[i*new_n], &A->base[i*old_n], sizeof(Real)*new_n);
   } else if ( old_n < new_n ) {
      for ( i = (int)(MIN(old_m,new_m))-1; i > 0; i-- )
      {   /* copy & then zero extra space */
	 memcpy(&A->base[i*new_n], &A->base[i*old_n], sizeof(Real)*old_n);
	 memset(&A->base[i*new_n+old_n],'\0',(new_n - old_n)*sizeof(Real));
      }
      memset(&A->base[old_n],'\0',(new_n - old_n)*sizeof(Real));
      A->max_n = new_n;
   }
   /* zero out the new rows.. */
   for ( i = old_m; i < new_m; i++ )
     memset(&A->base[i*new_n],'\0',new_n*sizeof(Real));
   
   A->max_m = new_max_m;
   A->max_n = new_max_n;
   A->max_size = A->max_m*A->max_n;
   A->m = new_m;	A->n = new_n;
   
   return A;
}

/* v_resize -- returns the vector x with dim new_dim
   -- x is set to the zero vector */
static VEC *
v_resize(VEC *x, int new_dim)
{
   shAssert(new_dim >= 0);

   if ( ! x )
     return phVecNew(new_dim);

   /* nothing is changed */
   if (new_dim == x->dim)
     return x;

   if ( x->max_dim == 0 )	/* assume that it's from sub_vec */
     return phVecNew(new_dim);
   
   if ( new_dim > x->max_dim ) {
      shAssert(x->ve != NULL);
      x->ve = shRealloc(x->ve,new_dim*sizeof(Real));
      x->max_dim = new_dim;
   }
   
   if ( new_dim > x->dim ) {
      memset(&x->ve[x->dim],'\0',(new_dim - x->dim)*sizeof(Real));
      shAssert(x->ve[x->dim] == 0.0);
   }
   x->dim = new_dim;
   
   return x;
}

/*
 * norm.c
 */

/*
	A collection of functions for computing norms: scaled and unscaled
*/


/* v_norm_inf -- computes (scaled) infinity-norm (supremum norm) of vectors */
static double
v_norm_inf(VEC *x)
{
	int	i, dim;
	Real	maxval, tmp;

	shAssert(x != NULL);

	dim = x->dim;

	maxval = 0.0;
	for ( i = 0; i < dim; i++ ) {
	   tmp = fabs(x->ve[i]);
	   maxval = MAX(maxval,tmp);
	}

	return maxval;
}

/*
 * submat.c
 */

/* 1.2 submat.c 11/25/87 */


/* get_col -- gets a specified column of a matrix and retruns it as a vector */
static VEC *
get_col(MAT *mat, unsigned int col, VEC *vec)
{
   unsigned int	i;
   
   shAssert(mat != NULL && col < mat->n);

   if ( vec==(VEC *)NULL || vec->dim<mat->m )
     vec = v_resize(vec,mat->m);
   
   for ( i=0; i<mat->m; i++ )
     vec->ve[i] = mat->me[i][col];
   
   return (vec);
}

/* get_row -- gets a specified row of a matrix and retruns it as a vector */
static VEC *
get_row(MAT *mat, unsigned int row, VEC *vec)
{
   unsigned int	i;
   
   shAssert(mat != NULL && row < mat->m);

   if ( vec==(VEC *)NULL || vec->dim<mat->n )
     vec = v_resize(vec,mat->n);
   
   for ( i=0; i<mat->n; i++ )
     vec->ve[i] = mat->me[row][i];
   
   return (vec);
}
/*
 * vecop.c
 */

/* vecop.c 1.3 8/18/87 */

/* in_prod -- inner product of two vectors from i0 downwards */
static double
in_prod(VEC *a, VEC *b, unsigned int i0)
{
	unsigned int limit;

	shAssert(a != NULL && b != NULL);

	limit = MIN(a->dim,b->dim);
	shAssert(i0 <= limit);

	return inner_product(&a->ve[i0],&b->ve[i0],limit - i0);
}
