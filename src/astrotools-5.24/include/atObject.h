#ifndef ATOBJECT_H
#define ATOBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

int
atObjectCenterFind(const REGION *reg, /* region containing object */
                   int x0, int y0,      /* initial estimate of centre */
                   int sky,             /* level of background */
                   float sigma,         /* sigma of Gaussian to smooth image
                                           with (ignore if <= 0) */
                   float dark_variance, /* variance of the background */
                   float gain,          /* gain of amplifiers */
                   float *xc, float *yc, /* centre of image */
                   float *dxc, float *dyc, /* errors in xc, yc */
                   float *peak,         /* peak value */
                   float *sigsqx, float *sigsqy, /* estimates of the width^2
                                                    of the object in x and y */
                   float *sigsqp, float *sigsqm);

int atCentroidFind(REGION *reg,
        int row0, int col0,
        double dbl_sky,
        double dbl_sigma,
        double dbl_dark_variance,
        double dbl_gain,
	double minval, double maxval, /* for scaling images to U16 */
        float *fl_xc, float *fl_yc,
        float *fl_dxc, float *fl_dyc,
        float *fl_peak, float *fl_counts,
        float *fl_siga, float *fl_sigb, float *fl_pa);

int atGaussianWidthFind(REGION *reg,
        int row0, int col0,
        double dbl_sky,
        double minval, double maxval, /* for scaling images to U16 */
        float *fl_siga, float *fl_sigb, float *fl_pa, float *fl_peak,
        float *fl_counts, float *fl_xf, float *fl_yf);

int at2ndMoments(
	REGION *reg,
        double row, double col, double sky,
        int rmin, int cmin, int rmax, int cmax,
        double *pa, double *a, double *b);

int atDACentroidFind(REGION *reg,
        int row0, int col0,
        double dbl_sky,
        double dbl_sigma,
        double dbl_dark_variance,
        double dbl_gain,
        double minval, double maxval, /* for scaling images to U16 */
        float *fl_rowc, float *fl_colc,
        float *fl_drowc, float *fl_dcolc,
        float *fl_peak, float *fl_counts,
        float *fl_siga, float *fl_sigb,
        float *fl_pa);

#ifdef __cplusplus
}
#endif

#endif
