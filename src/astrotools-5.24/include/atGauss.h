#if !defined(ATGAUSS_H)
#define ATGAUSS_H
#include "dervish.h"
int atGaussVectorMake(const double sigma, int *nvec, float **gvec);
int atRegConvolveWithGaussian(REGION *inpug, REGION *temp, REGION *output,
			      int nelem, float *gvec);
#endif
