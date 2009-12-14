#if !defined(PHLIBRARY_H)
#define PHLIBRARY_H

#include "region.h"
#include "phHist.h"
#include "phSpanUtil.h"

PHHIST *phHistU16FromRegion(REGION *reg, CHAIN *mask);
PHHIST *phHistU16FromArray(int nelem, PIX *array);
int      phNtile(PHHIST *hist, int ntile_n, float *array);
int      phHistU16Stats(PHHIST *hist, double nsigma, float *quartiles, 
                        float *mean, float *mode, float *sigma);
int      phRegStats(REGION *reg, double nsigma, CHAIN *mask, 
                    float *quartiles, float *mean,
                    float *mode, float *sigma);
#endif
