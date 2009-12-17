#ifndef SHCDEMO_H
#define SHCDEMO_H

#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

void shRegStatsFind (const REGION *reg, double *mean, double *sigma,
                     double *high,      int *hrow,    int *hcol, 
                     double *low,       int *lrow,    int *lcol);

void shRegClippedStatsFind (const REGION *reg, 
                     const double lbound, const double ubound, 
                     double *mean, double *sigma,
                     double *high,      int *hrow,    int *hcol, 
                     double *low,       int *lrow,    int *lcol);

double shD_RegSkyFind (const REGION *reg);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif

