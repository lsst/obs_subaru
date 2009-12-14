#if !defined(PHINTERP_H)
#define PHINTERP_H

#include "phUtils.h"

int      phFindIndexFloat(int npt, float *array, float x, int *index);
int      phLinearInterpFloat(int npt, float *x, float *y, 
                             float xval, float *yval);

#endif
