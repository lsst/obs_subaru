#if !defined(PHUMEDIAN_H)
#define PHUMEDIAN_H

int phHistU16StatsFit(PHHIST *hist, float *quartiles, float *sigma, int *error);
int phRegMeanFitByCol(REGION *input, int bias_scale, REGION *output);
int phGaussFitIsInitialised(void);
int phInitGaussFit(void);
int phFiniGaussFit(void);
#endif
