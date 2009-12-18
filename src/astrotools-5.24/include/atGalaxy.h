#ifndef ATGALAXY_H
#define ATGALAXY_H

#ifdef __cplusplus
extern "C" {
#endif

void atStarCountsFind(double l,     /* galactic longitude, in radians */
                double b,       /* galactic latitude, in radians */
                double m,       /* limiting magnitude in V */
                double *density /* returned stellar density, in stars/sq. deg.*/
                );

#ifdef __cplusplus
}
#endif

#endif
