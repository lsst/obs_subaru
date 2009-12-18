#ifndef ATCENTROID_H
#define ATCENTROID_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(VXWORKS)  /* online da computers  */
#define U16  unsigned short int
#endif


  int 
  atEllipseFind( float sigsqx, float sigsqy, float sigsqp, float sigsqm,
		      float sigma, float convpeak, float *siga, float *sigb, 
		      float *pa, float *peak, float *counts);

int
atObjectCenterFind_Ptr(const U16 **rows, /* data */
                    int nrow, int ncol, /* size of data array */
                    int x0, int y0,   /* initial estimate of centre */
                    int sky,          /* level of background */
                    float sigma,      /* sigma of Gaussian to smooth image
                                         with (ignore if <= 0) */
                    float dark_variance, /* variance of the background */
                    float gain,       /* gain of amplifiers */
                    float *xc, float *yc, /* centre of image */
                    float *dxc, float *dyc, /* errors in xc, yc */
                    float *peak,      /* peak value */
                    float *sigsqx, float *sigsqy, /* estimates of the width^2
                                                  of the object in x and y */
                    float *sigsqp, float *sigsqm);

int atDACentroid( const U16 **rows, /* data */
                      int nx, int ny, /* size of data array */
                      int x0, int y0,   /* initial estimate of centre */
                                        /* x is col, and y is row */
                      int sky,          /* level of background */
                      float sigma,      /* sigma of Gaussian to smooth image
                                           with (ignore if <= 0) */
                      float dark_variance, /* variance of the background */
                      float gain,       /* gain of amplifiers */
                      float *xc, float *yc, /* center of image */
                      float *dxc, float *dyc, /* errors in xc, yc */
                      float *peak,      /* peak value */
                      float *counts, float *siga,
                      float *sigb, float *pa);

#ifdef __cplusplus
}
#endif

#endif
