#if !defined(AT_MATH_UTILS)
#define AT_MATH_UTILS 1
#include <math.h>


/*****************************************************************************/
/*
 * Matrices
 */
int atMatrixInvert(float **arr, float *b, int n);
int atDMatrixInvert(double **arr, double *b, int n);

#endif
