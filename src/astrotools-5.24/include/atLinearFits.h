#ifndef ATLINEARFITS_H
#define ATLINEARFITS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dervish.h"
#include "atSlalib.h"
#include "sdssmath.h"

#define SM_MAX_LINFIT_VAR 100

FUNC *atVLinEqn( VECTOR *depValue,
                   VECTOR **value,
		   int nVar
		 );


# define ATLFNMODELS 4
int atBcesFit (
        double *X,              /* a vector of x values */
        double *Y,              /* a vector of y values */
        double *XErr,           /* a vector of x error values */
        double *YErr,           /* a vector of y error values */
        double *CErr,           /* a vector of xy covariance errors */
        int nPt,                /* number of data points */
        int nsim,               /* number of simulations */
        int seed,               /* seed for random number generator */
        double *b,              /* bces fit */
        double *bvar,           /* bces fit sigma */
        double *a,              /* bces fit */
        double *avar,           /* bces fit sigma */
        double *bavg,           /* bootstrap fit */
        double *sdb,            /* bootstrap fit sigma */
        double *aavg,           /* bootstrap fit */
        double *sda,            /* bootstrap fit sigma */
        double *bavg_ifab,      /* isobe, feigulson something */
        double *bavg_ifabsim    /* bootstrap of that */
  );

#ifdef __cplusplus
}
#endif

#endif
