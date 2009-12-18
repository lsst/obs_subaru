#ifndef AT_QUASARS_H
#define AT_QUASARS_H

#ifdef __cplusplus
extern "C" {
#endif

#define ATQU_UPPER_LIMIT -100
#define ATQU_LOWER_LIMIT -200
#define ATQU_UNKNOWN	 -300

int atQuLocusRemove(double *xt,
                double *yt,
                double *zt,
                double *kx,
                double *ky,
                double *kz,
                double *distmax,
                double *distmin,
                double *theta,
                int npts,
                double x,
                double y,
                double z,
                double xvar,
                double xyvar,
                double xzvar,
                double yvar,
                double yzvar,
                double zvar,
                double backk,
                double backkdist,
                double forwardk,
                double forwardkdist,
		double nsigma,
		int *closestpoint,
		double *ellipsedist);

void atQuLocusKlmFromXyz(double *xt,
                double *yt,
                double *zt,
                double *kx,
                double *ky,
                double *kz,
                double *theta,
                int npts,
                double x,
                double y,
                double z,
                double *k,
                double *l,
                double *m);

#ifdef __cplusplus
}
#endif

#endif
