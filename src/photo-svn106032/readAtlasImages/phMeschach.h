#if !defined(PHMESCHACH_H)
#define	PHMESCHACH_H

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
** Robert Lupton (rhl@astro.princeton.edu) modified this software in
** March 1997. Specifically, I reduced it to a minimum number of routines
** needed to do an SVD decomposition of a matrix, and put these routines
** together in the file.
**
** At the same time, I disabled and/or removed much of the memory management
** code in meschach, and changed the error handling.
**
***************************************************************************/
#if 0					/* included at _bottom_ of file*/
#  include "phMathUtils.h"
#endif

#if 0					/* single precision */
   typedef float Real;
#  define MACHEPS EPSILON_f
#else					/* double precision */
   typedef double Real;
#  define MACHEPS EPSILON
#endif

/*****************************************************************************/
/*
 * from matrix.h
 */
typedef	struct vec {
   unsigned int dim, max_dim;
   Real *ve;
   struct vec *link;			/* used for maintaining a free list */
} VEC;

typedef	struct mat {
   unsigned int	m, n;
   unsigned int	max_m, max_n, max_size;
   Real	**me,*base;			/* base is base of alloc'd mem */
   struct mat *link;			/* used for maintaining a free list */
} MAT;

/*
 * prototypes
 */
void phMeschachLoad(void);

void phMeschachInit(void);
void phMeschachFini(void);

MAT *phMatNew(int m, int n);
void phMatDel(MAT *mat);
MAT *phMatDelRowCol(MAT *mat, int row, int col);
void phMatClear(MAT *A);
VEC *phVecNew(int);
void phVecDel(VEC *vec);
VEC *phVecDelElement(VEC *vec, int el);
void phVecClear(VEC *vec);


VEC *phEigen(const MAT *A,		/* input matrix */
	     MAT *Q,			/* eigen vectors (can be NULL) */
	     VEC *eigen);		/* eigen values (can be NULL) */
VEC *phEigenBackSub(const MAT *Q,
		    const VEC *lambda,
		    const VEC *b);
void phEigenCovarGet(MAT *covar,
		     const MAT *Q,
		     const VEC *lambda);

VEC *phSvd(MAT *A, MAT *U, MAT *V, VEC *d);
VEC *phSvdBackSub(const MAT *U,		/* SVD */
		  const MAT *V,		/*    decomposition */
		  const VEC *w,		/*        of design matrix */
		  const VEC *b);	/* RHS */
void phSvdCovarGet(MAT *covar,
		   const MAT *V,
		   const VEC *w);
MAT *
phSvdInvert(MAT *inv,			/* inverse matrix (or NULL) */
	    const MAT *U,		/* SVD */
	    const MAT *V,		/*    decomposition */
	    const VEC *iw);		/*        of design matrix
					   n.b. _inverse_ of singular values */
MAT *
phEigenInvert(MAT *inv,			/* inverse matrix (or NULL) */
	      const MAT *Q,		/* eigen vectors */
	      const VEC *ilambda);	/* inverse eigen values */


#include "phMathUtils.h"

#endif


