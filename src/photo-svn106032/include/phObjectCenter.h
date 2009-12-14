#if !defined(PH_OBJECT_CENTER_H)
#define PH_OBJECT_CENTER_H

int
phObjectCenterFind(const REGION *reg,	/* region containing object */
		   int x0, int y0,	/* initial estimate of centre */
		   float sky,		/* number of counts in the sky */
		   float sigma,		/* sigma of Gaussian to smooth image
					   with (ignore if <= 0) */
		   float bkgd,		/* level of background to subtract */
		   float dark_variance,	/* variance of the background */
		   float gain,		/* gain of amplifiers */
		   float *xc, float *yc, /* centre of image */
		   float *dxc, float *dyc, /* errors in xc, yc */
		   float *peak,		/* peak value */
		   float *sigsqx, float *sigsqy); /* estimates of the width^2
						     of the object in x and y*/
int
phRegIntMaxPixelFind(const REGION *reg,	/* region to search */
		     float bkgd,	/* background level */
		     int row0, int col0, /* LLC of rectangle to search */
		     int row1, int col1, /* URC of rectangle to search */
		     int *rpeak, int *cpeak, /* position of maximum */
		     float *rowc, float *colc);	/* region centroid */
#endif
