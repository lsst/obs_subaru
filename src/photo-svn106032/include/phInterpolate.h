#if !defined(PHINTERPOLATE_H)
#define PHINTERPOLATE_H

/*
 * Interpolate over a single-column defect. PTR is a pointer to the
 * pixel to be interpolated over; it's the user's responsibility to
 * ensure that PTR[-2] and PTR[2] are valid.
 */
#define PH_INTERP_1(PTR) \
   ((-((PTR)[-2] + (PTR)[2]) + 3*((PTR)[-1] + (PTR)[1]))/4)

#endif
