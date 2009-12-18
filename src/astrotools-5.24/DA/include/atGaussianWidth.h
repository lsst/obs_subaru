#ifndef ATGAUSSIANWIDTH_H
#define ATGAUSSIANWIDTH_H

#ifdef __cplusplus
extern "C" {
#endif

/* We probably don't need the first ifdef; the second
was created to enable Russ Owen's standalone version */
#if defined(VXWORKS)  /* online da computers  */
#define U16  unsigned short int
#endif
#ifndef U16
#define U16 unsigned short int
#endif

/* The 'focus moment' is the SUM of g_xmom and g_ymom */

typedef struct gausmom_T{
    float g_xmom;         /* normalized moment of 2(x/sig)^2 - 1 */
    float g_ymom;         /* normalized moment of 2(y/sig)^2 - 1 */
    float g_pmom;         /* normalized moment of ((x-y)/sig)^2 - 1 */
    float g_mmom;         /* normalized moment of ((x+y)/sig)^2 - 1 */
    float g_filval;       /* value of filter */
    float g_xf;           /* float coordinates */
    float g_yf;           /* float coordinates */
} GAUSSMOM;

int atFindFocMom(const U16 **p,
          int xsz,
          int ysz,
          int x,
          int y,
          int sky,
          GAUSSMOM *ps);

int atSetFSigma(double sigma);

int atSigmaFind(const U16 **p,
          int xsz,
          int ysz,
          int x,
          int y,
          int sky,
          struct gausmom_T *ps,
	  double *sig);

#ifdef __cplusplus
}
#endif

#endif
