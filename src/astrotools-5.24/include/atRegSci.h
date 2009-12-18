#ifndef ATCREGSCI_H
#define ATCREGSCI_H

#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

RET_CODE atRegMap(const REGION *source, REGION *target,      const double rowoff,
              const double coloff,  const double theta,  const double rcenter,
              const double ccenter, const double rowmag, const double colmag);

RET_CODE atRegMedianFindAsDbl(const REGION *reg, double *median);

RET_CODE atRegModeFind(const REGION *reg, double min,  double max,
		       double *mode);

RET_CODE atRegStatsFind (const REGION *reg, double *mean, double *sigma,
                     double *high,      int *hrow,    int *hcol, 
                     double *low,       int *lrow,    int *lcol, double *total);

RET_CODE atRegClippedStatsFind (const REGION *reg, 
		     const double lbound, const double ubound, 
		     double *mean, double *sigma,
                     double *high,      int *hrow,    int *hcol, 
                     double *low,       int *lrow,    int *lcol);

RET_CODE atRegGaussianAdd(REGION *reg, const double rpos, const double cpos,
                      const double amplitude, const double rsig,
		      const double csig, const double radius);

/** not used 
void atRegGS(const REGION *reg1, const REGION *reg2, double *factor);
**/

RET_CODE atRegConvolve(const REGION * const reg, const REGION * const tmplt, REGION * const target);

double atRegSkyFind (const REGION *reg);

RET_CODE atRegBin(const REGION *reg, REGION *regtarget, const int rowfactr, 
	      const int colfactr, const int normalize); 

RET_CODE atRegMedianFind (const REGION * const * const reglist, 
		      const int n_reg, 
		      REGION * const regtarget);
RET_CODE atRegClippedMeanFind (const REGION * const * const reglist, 
		           const int n_reg, 
                           const int niter,
                           const double nsigma,
		           REGION * const regtarget,
                           int const clipCorrectFlag);

RET_CODE atRegSigmaClip(
                REGION *regPtr,                 /* the data */
                double sigma_clip,              /* how many sigma */
                int number_of_iterations,       /* how many times */
                double *sigma_mean,             /* the sigma-clipped  mean */
                double *sigma_avgdev            /* the sigma-clipped sigma */
        );


RET_CODE atRegMedianFindByColumn(REGION *input, REGION *output);

RET_CODE atRegMedianFindByRow(REGION *input, REGION *output);

RET_CODE atRegMedianSmooth(REGION *input, int m, int n, REGION *output);

RET_CODE atRegPixValSubstitute(REGION *reg, double oldval, double newval, int compare);

RET_CODE atRegPixRangeSubstitute(REGION *reg, double minval, double maxval, double newval);

RET_CODE atRegCorrectFL32 (REGION *input, double *corr, int ncorr, float firstC, float lastC);

RET_CODE atRegDistortFL32 (REGION *input, double *corr, int ncorr, float firstC, float lastC);

RET_CODE atRegCorrectINT (REGION *input, double *corr, int ncorr, int firstC);

RET_CODE atRegCorrectQuick (REGION *input, double *corr, int ncorr, int firstC,
                     int lastC);

double atErfc(double x);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
