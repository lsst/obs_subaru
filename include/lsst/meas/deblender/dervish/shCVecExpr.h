#if !defined(SHCVECEXPR_H)
#define SHCVECEXPR_H

/*
 * Be careful with the define for VECTOR_TYPE; if it is changed, 
 * the typedef for VECTOR must also be changed to match, but 
 * can _not_ contain the VECTOR_TYPE define, because schemaGet
 * will stop working on it.
 */


#define VECTOR_TYPE double
#define VECTOR_NAME_LENGTH 80

typedef struct {
  char name[VECTOR_NAME_LENGTH];
  int dimen;				/* dimension of vector */
  double *vec;				/* storage for elements */
} VECTOR;				/*
					   pragma SCHEMA
					 */

VECTOR *shVectorNew(int dimen);
void shVectorDel(VECTOR *vec);
VECTOR *shVectorResize(VECTOR *vec, int dimen);
void shVectorPrint(VECTOR *x);
int
shVectorsReadFromFile(FILE *fil,	/* file descriptor to read from */
		      VECTOR **vector,	/* vectors to read */
		      int *col,		/* from these columns */
		      int nvec,		/* size of vecs and ncol */
		      const char *type,	/* type specifier for desired rows */
		      int nline);	/* max number of lines to read, or 0 */

VECTOR *p_shVectorGet(char *name, int *alloc);	/* return a VECTOR given name */
VECTOR *shVectorStringEval(char *str);	/* evaluate a string */
/*
 * Misc operations
 */
VECTOR *shVectorDo(VECTOR *x1, VECTOR *x2, VECTOR *dx);
VECTOR *shVectorSubscript(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorIf(VECTOR *v1, VECTOR *v2);
/*
 * arithmetic operations
 */
VECTOR *shVectorAbs(VECTOR *v1);
VECTOR *shVectorAcos(VECTOR *v1);
VECTOR *shVectorAdd(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorAsin(VECTOR *v1);
VECTOR *shVectorAtan(VECTOR *v1);
VECTOR *shVectorAtan2(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorConcat(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorCos(VECTOR *v1);
VECTOR *shVectorDimen(VECTOR *v1);
VECTOR *shVectorDivide(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorExp(VECTOR *v1);
VECTOR *shVectorInt(VECTOR *v1);
VECTOR *shVectorLg(VECTOR *v1);
VECTOR *shVectorLn(VECTOR *v1);
VECTOR *shVectorMap(VECTOR *v1, VECTOR *v2, VECTOR *v3);
VECTOR *shVectorMultiply(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorNegate(VECTOR *v1);
VECTOR *shVectorNtile(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorPow(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorRand(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorRandN(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorSelect(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorSin(VECTOR *v1);
VECTOR *shVectorSort(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorSqrt(VECTOR *v1);
VECTOR *shVectorSubtract(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorSum(VECTOR *v1);
VECTOR *shVectorTan(VECTOR *v1);
/*
 * Bitwise operators
 */
VECTOR *shVectorLAnd(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorLOr(VECTOR *v1, VECTOR *v2);
/*
 * Logical operators
 */
VECTOR *shVectorAnd(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorEq(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorGe(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorGt(VECTOR *v1, VECTOR *v2);
VECTOR *shVectorLternary(VECTOR *logical, VECTOR *expr1, VECTOR *expr2);
VECTOR *shVectorNot(VECTOR *v1);
VECTOR *shVectorOr(VECTOR *v1, VECTOR *v2);

#endif
