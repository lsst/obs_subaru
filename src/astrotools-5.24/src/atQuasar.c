
/* <AUTO>                                                                         
  FILE: quLocus
<HTML>
  Routines which fits points to the stellar locus of an input data set
</HTML>
  </AUTO>
*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "atQuasar.h"
#include "atConversions.h" /* For M_PI */
#include "dervish.h"

#define EPSILON 1e-10

static void atCalcLm (const double kx, const double ky, const double kz, const double theta, double *lx, double *ly, double *lz, double *mx, double *my, double *mz) {

    double unitdenom;

    /* Unitdenom must be positive, since kz!=1 */
    unitdenom = sqrt(kx*kx+ky*ky);
    *lx = (-kx*kz*cos(theta)+ky*sin(theta))/unitdenom;
    *ly = (-ky*kz*cos(theta)-kx*sin(theta))/unitdenom;
    *lz = (kx*kx+ky*ky)*cos(theta)/unitdenom;
    *mx = (kx*kz*sin(theta)+ky*cos(theta))/unitdenom;
    *my = (ky*kz*sin(theta)-kx*cos(theta))/unitdenom;
    *mz = -(kx*kx+ky*ky)*sin(theta)/unitdenom;

}

static double atCalcEllipseDist (const double ld, const double md, const double totalmajor, const double totalminor) {

    double ellipsedist;

    /* This will bomb if kx=ky=0.  The definition of the axes also fails.
       This condition has already been caught. */
    if (totalmajor*totalminor!=0) {
      ellipsedist = ld*ld/(totalmajor*totalmajor) + md*md/(totalminor*totalminor);
    } else {
      if ((ld==0)&&(md==0)) return 0;
      if (totalmajor == 0) return 2;		/* 2 is just a number bigger than one, which */
      if (md!=0) return 2;			/* means the point is outside of the ellipse */
      ellipsedist = ld*ld/(totalmajor*totalmajor);
    }

    return ellipsedist;

}

static double atApplyLimits (const double xvar, const double xd, const double xdlimit) {

        if ( ((xvar==ATQU_LOWER_LIMIT)&&(xd<xdlimit))||
             ((xvar==ATQU_UPPER_LIMIT)&&(xd>xdlimit)) ) {
             return xdlimit;
        } else {
	     return xd;
	}

}

static double atCalcOptimalX (const double totalmajor, const double totalminor, const double lx, const double ly, const double lz, const double mx, const double my, const double mz, const double yd, const double zd) {

	double denom, xd;
        denom = totalminor*totalminor*lx*lx + totalmajor*totalmajor*mx*mx;
        if (denom > EPSILON) {
              xd = - ( (totalminor*totalminor*lx*ly + totalmajor*totalmajor*mx*my) * yd
                     +
                     (totalminor*totalminor*lx*lz + totalmajor*totalmajor*mx*mz) * zd
                   ) / denom;
        } else {

		/* Minimize l, given that m=0 */
		if (lx!=0) {
		      /* set l to zero */
		      xd = -(ly*yd+lz*zd)/lx;
		} else {
			if (mx!=0) {
				/* set m to zero */
		      		xd = -(my*yd+mz*zd)/mx;
			} else {
		      		/* hat{k} = hat{x}, so we're only moving along the axis */
		      		xd = 0;
			}
		}

	}
	return xd;

}

static double atCalcOptimalZ (const double totalmajor, const double totalminor, const double lx, const double ly, const double lz, const double mx, const double my, const double mz, const double xd, const double yd) {

	return atCalcOptimalX (totalmajor,totalminor,lz,lx,ly,mz,mx,my,xd,yd);
}

static double atCalcOptimalY (const double totalmajor, const double totalminor, const double lx, const double ly, const double lz, const double mx, const double my, const double mz, const double xd, const double zd) {

	return atCalcOptimalX (totalmajor,totalminor,ly,lz,lx,my,mz,mx,zd,xd);
}

static void atMoveToLocus (const double xdlimit, const double ydlimit, const double kx, const double ky, const double kz, const double xvar, const double yvar, const double totalmajor, const double totalminor, const double lx, const double ly, const double lz, const double mx, const double my, const double mz, double *xd, double*yd, const double zd) {

	double lambda, xdtemp;
        /* This removes the warning that kx was never used */
        lambda = kx;

	/* Calculate the values for yd and xd that will put the point exactly on the locus */
	if (kz==0) {
	  lambda = 0;
	} else {
	  lambda = zd/kz;
	}
        *yd = lambda*ky;
	*yd = atApplyLimits (yvar, *yd, ydlimit);
        xdtemp = atCalcOptimalX (totalmajor, totalminor, lx, ly, lz, mx, my, mz, *yd, zd);
        *xd = atApplyLimits (xvar, xdtemp, xdlimit);
	if (*xd!=xdtemp) {
	  /*We hit a limit in x - try finding the best y value given this limit */
	  *yd = atCalcOptimalY (totalmajor, totalminor, lx, ly, lz, mx, my, mz, *xd, zd);
	  *yd = atApplyLimits (yvar, *yd, ydlimit);
	}

}

static void atMoveXToEllipsoid (const double kx, const double ky, const double kz, const double lx,
	const double ly, const double lz, const double mx, const double my, const double mz,
	const double al2, const double am2, const double ak2, const double kzero,
	double *xd, const double yd, const double zd) {

  double denom, numerator;

  denom = am2*ak2*lx*lx + al2*ak2*mx*mx + al2*am2*kx*kx;

  if (denom==0) {

    if (am2==0) {
       if (mx!=0) {
	  /* Move as onto the m=0 plane */
	  *xd = - (my*yd+mz*zd) / mx;

       } else {
          if (al2==0) {
            /* The best thing to do is put the point as close to the center of the locus 
	    as possible (this is may not have already been done if more than one of 
	    the colors is unknown) */
		denom = lx;
           /* denom = 1-kx*kx; */
            if (denom>0) {
               *xd = kx*(ky*yd+kz*zd)/denom;
              /*  ***Second possible minimization, moves data point to l=0 plane  *xd = -(ly*yd+lz*zd)/lx;*/
            } else { *xd = 0; }

	  } else {
	    /* We can't affect m, so minimize within l,k ellipse */
            denom = ak2*lx*lx + al2*kx*kx;
	    if (denom!=0) {
               numerator = ak2*(ly*yd+lz*zd)*lx + al2*(ky*yd+kz*zd-kzero)*kx;
               *xd = -numerator/denom;
	    } else {
	       /* We must have am2=ak2=0 and mx=kx=0, so lx=1.  So, set xd to 0. */
	       *xd = 0;
	    }
	  }
       }
    } else {
       /* Since al2>=am2>0, it must be true that ak2=kx=0.  We can't affect k, so 
		minimize the l,m ellipse.  Since we can't have lx=mx=kx=0, this
		denominator must be positive. */
       denom = am2*lx*lx + al2*mx*mx;
       numerator = am2*(ly*yd+lz*zd)*lx + al2*(my*yd+mz*zd)*mx;
       *xd = -numerator/denom;
    }

  } else {

    numerator = am2*ak2*(ly*yd+lz*zd)*lx + al2*ak2*(my*yd+mz*zd)*mx
		+ al2*am2*(ky*yd+kz*zd-kzero)*kx;
    *xd = -numerator/denom;

  }

}

static void atMoveToEllipsoid (const double kx, const double ky, const double kz, const double lx, const double ly, const double lz, const double mx, const double my, const double mz, const double al2, const double am2, const double ak2, const double kzero, double *xd, double *yd, const double zd, const double xvar, const double yvar, const double xdlimit, const double ydlimit) {

       double denom, numerator, xdtemp;

       /* Calculate the values for yd and xd that will put the point as close to the end
          point as possible. */
       denom = ak2*kz*kz + am2*mz*mz + al2*lz*lz;
       if (denom != 0) {
	     numerator = ( ky*kz*ak2 + my*mz*am2 + ly*lz*al2 ) * zd
			- (lx*mz*am2 - mx*lz*al2)*kzero;
	     *yd = numerator/denom;
        } else {
	     /* Move to endpoint */
	     *yd = ky*kzero;
	}
	*yd = atApplyLimits (yvar, *yd, ydlimit);
	atMoveXToEllipsoid (kx, ky, kz, lx, ly, lz, mx, my, mz, al2, am2, ak2, kzero, &xdtemp, *yd, zd);
	*xd = atApplyLimits (xvar, xdtemp, xdlimit);
        if (*xd!=xdtemp) {
	  /* We hit a limit in x - try finding the best y value given this limit */
	  atMoveXToEllipsoid (ky, kz, kx, ly, lz, lx, my, mz, mx, al2, am2, ak2, kzero, yd, zd, *xd);
	  *yd = atApplyLimits (yvar, *yd, ydlimit);
        }

}

static void atEllipseConvolve (const double kx, const double ky, const double kz, const double theta, const double distmax, const double distmin, const double xvar, const double xyvar, const double xzvar, const double yvar, const double yzvar, const double zvar, const double nsigma, double *totalmajor, double *totalminor, double *totaltheta) {

    double vl, vm, vlm;
    double errormajorvar, errorminorvar, errortheta;
    double cos2errortheta, sin2errortheta, locusmajorvar, locusminorvar;
    double alphanum, betanum, gammanum, anum, bnum;
    /* double denom; */
    double lx, ly, lz, mx, my, mz;
    double xvaruse, yvaruse, zvaruse, xyvaruse, xzvaruse, yzvaruse;
    double smallnum = 1e-10;

    atCalcLm (kx, ky, kz, theta, &lx, &ly, &lz, &mx, &my, &mz);
    xyvaruse = xyvar; xzvaruse = xzvar; yzvaruse = yzvar;
    if (xvar<0) {xvaruse = 0.0; xyvaruse = 0.0; xzvaruse = 0.0;} else {xvaruse = xvar;}
    if (yvar<0) {yvaruse = 0.0; xyvaruse = 0.0; yzvaruse = 0.0;} else {yvaruse = yvar;}
    if (zvar<0) {zvaruse = 0.0; xzvaruse = 0.0; yzvaruse = 0.0;} else {zvaruse = zvar;}
    vl = lx*lx*xvaruse + ly*ly*yvaruse + lz*lz*zvaruse + 2*lx*ly*xyvaruse + 2*lx*lz*xzvaruse + 2*ly*lz*yzvaruse;
    vm = mx*mx*xvaruse + my*my*yvaruse + mz*mz*zvaruse + 2*mx*my*xyvaruse + 2*mx*mz*xzvaruse + 2*my*mz*yzvaruse;
    vlm = lx*mx*xvaruse + ly*my*yvaruse + lz*mz*zvaruse + (lx*my+mx*ly)*xyvaruse + 
	(lx*mz+mx*lz)*xzvaruse + (ly*mz+my*lz)*yzvaruse;



    errormajorvar = nsigma*nsigma*( vl+vm + sqrt( (vl-vm)*(vl-vm) + 4*vlm*vlm ) ) / 2.0;
    errorminorvar = nsigma*nsigma*( vl+vm - sqrt( (vl-vm)*(vl-vm) + 4*vlm*vlm ) ) / 2.0;
    if (fabs(vlm)<smallnum) {
        if (vm>vl) {errortheta = M_PI/2;} else {errortheta = 0;}
    } else {
    	errortheta = atan( ( (vm-vl) + sqrt( (vm-vl)*(vm-vl) + 4.*vlm*vlm ) )/ (2.*vlm ));
    }
    cos2errortheta = cos(errortheta)*cos(errortheta);
    sin2errortheta = sin(errortheta)*sin(errortheta);
    locusmajorvar = distmax*distmax;
    locusminorvar = distmin*distmin;

    /* Increase the size of the elliptical cross section associated with the closest point,*/

    alphanum = (errorminorvar*cos2errortheta + errormajorvar*sin2errortheta + locusminorvar);
    betanum = (-cos(errortheta)*sin(errortheta)*(errormajorvar - errorminorvar));
    gammanum = (errorminorvar*sin2errortheta+errormajorvar*cos2errortheta+locusmajorvar);
    anum = (errormajorvar+errorminorvar+locusminorvar+ locusmajorvar) / 2.0;
    bnum = (sqrt( (errormajorvar-errorminorvar)*(errormajorvar-errorminorvar) +
	2.0*(errormajorvar-errorminorvar)*
		(locusminorvar - locusmajorvar)*(sin2errortheta-cos2errortheta) + 
	(locusmajorvar-locusminorvar)*(locusmajorvar-locusminorvar) 
	) / 2.0);
    if (fabs(bnum-alphanum+anum)<smallnum) {
	if (fabs(alphanum-anum+bnum)<smallnum) {
           *totaltheta = 0;
        } else {
	   *totaltheta = M_PI/2.0;
        }
    } else {
        *totaltheta = atan2( (alphanum-gammanum+sqrt((alphanum-gammanum)*(alphanum-gammanum)+4*betanum*betanum)),(2*betanum));

	
    }
    if (errortheta < 0) *totaltheta = -*totaltheta;
    *totaltheta = theta+*totaltheta;
    if (*totaltheta>M_PI/2.0) *totaltheta -= M_PI;
    if (*totaltheta<=-M_PI/2.0) *totaltheta += M_PI;
    /* The only ways denom can be zero, given that errorminorvar <= errormajorvar
	and locusminorvar <= locusmajorvar, are when both minor axes are zero.
	Additionally, we must have errortheta=0 or one of the major axes must
	be zero.  anum and bnum must be positive.  The only way anum=bnum is if
	denom==0. */
    /*denom = errormajorvar*errorminorvar + locusminorvar*errorminorvar*sin2errortheta + 
	errormajorvar*locusminorvar*cos2errortheta + locusmajorvar*errorminorvar*cos2errortheta
	+ locusmajorvar*errormajorvar*sin2errortheta + locusmajorvar*locusminorvar; */
	if(anum-bnum < 0) {
    *totalminor = 0;
	} else {
    *totalminor = (sqrt(anum-bnum));
	}
    *totalmajor = (sqrt(anum+bnum));


}

static double atDistanceCalc(const double x, const double y, const double z,
	const double xerror, const double yerror, const double zerror,
	const double xt, const double yt, const double zt) {

       double dist;
       double xd, yd, zd;

       if (xerror<0.0) {
          if (yerror<0.0) {
             /* x and y are missing */
             zd = z-zt;
             yd = 0;
	     yd = atApplyLimits (yerror, yd, y-yt);
             xd = 0;
	     xd = atApplyLimits (xerror, xd, x-xt);
          } else {
             if (zerror<0.0) {
                /* x and z are missing */
                yd = y-yt;
                zd = 0;
	        zd = atApplyLimits (zerror, zd, z-zt);
                xd = 0;
	        xd = atApplyLimits (xerror, xd, x-xt);
             } else {
                /* Only x is missing */
                zd = z-zt;
                yd = y-yt;
                xd = 0;
	        xd = atApplyLimits (xerror, xd, x-xt);
             }
          }
       } else {
          if (yerror<0.0) {
             if (zerror<0.0) {
                /* y and z are missing */
                xd = x-xt;
                zd = 0;
	        zd = atApplyLimits (zerror, zd, z-zt);
                yd = 0;
	        yd = atApplyLimits (yerror, yd, y-yt);
             } else {
                /* Only y is missing */
                zd = z-zt;
                xd = x-xt;
                yd = 0;
	        yd = atApplyLimits (yerror, yd, y-yt);
             }
          } else {
             if (zerror<0.0) {
                /* Only z is missing */
                yd = y-yt;
                xd = x-xt;
                zd = 0;
	        zd = atApplyLimits (zerror, zd, z-zt);
             } else {
                /* x, y, and z are all there */
                zd = z-zt;
                yd = y-yt;
                xd = x-xt;
             }
          }
       }
       dist = sqrt(xd*xd + yd*yd + zd*zd);
       return dist;
}

/*<AUTO EXTRACT>

  ROUTINE: atQuLocusRemove

  DESCRIPTION: Takes a set of npts locus points described by xt,yt,zt,kx,ky,kz,distmax,
	distmin, and theta.  Also takes a single data point with errors.  If the errors 
	are unknown, set them to zero.  The variance in the x, y and z directions is either the 
	(positive) computed variance, or a flag that distinguishes between UPPER_LIMIT, 
	LOWER_LIMIT, or UNKNOWN.  If the variance flag is a limit, the corresponding value 
	is assumed to be the limiting color.  If the flag is UNKNOWN, the corresponding 
	value is ignored.  If the point is inside the locus, 1 is returned, else 0.
	If none of the colors is known (all colors are flagged), then -1 is returned.  The 
	values of distmax and distmin are negative or reversed, a flag of -2 is returned.  The
	axes are undefined if kx=ky=0, and -3 is returned if this is true for the closest
	point, or one of the endpoints.
  <P>
	The input locus parameters should describe the extent of the locus, not including
	any photometric errors.  The extent may be determined any reasonable way, including
	ntiles or some number of sigma of the intrinsic width.  The cross section of the
	locus will be assumed to have a Gaussian distribution.  The nsigma in this input
	determines the size of the error Gaussian around the input coordinates x, y, and z.
	The width of the locus will be increased by summing the error Gaussian and the
	locus width Gaussians in quadrature.  The raw coordinates (without errors) are then
	compared with this enlarged locus.
  <P>
	The ends of the locus are ellipsoids that fit smoothly onto the last locus point,
	and are axially aligned in the l,m,k directions.  The width in the l and m directions
	are the widths of the locus at the closest locus point in the allowed k range.
	The widths in the k direction are given by backkdist and forwardkdist.  By tweeking
	the input locus, we can eliminate stars inside right elliptical cylinders
	(backkdist=forwardkdist=0) and half ellipsoids; but it is especially designed to do 
	a sequence of elliptical cylinders with ellipsoids on the ends.  Errors in the k
	direction are added in quadrature with the ellipse widths in the k direction.
  <P>
	The limits associated with a variance which contains a flag is assumed to include
	any error in the knowledge of the limit.  We will assume zero additional error in that
	coordinate.  Any covariances which are unknown should be set to zero.

<HTML>
</HTML>
</AUTO>*/

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
		int *clpnt,
		double *eldist) {
  int tcnt;
  int backpoint, forwardpoint;
  double xd,yd,zd,kd,id,jd,ld,md;
  int closestpoint;
  double closestdist, dist;
  int inlocus;
  double ellipsedist;
  double *koffset;
  double lx, ly, lz, mx, my, mz;
  double kendpoint;
  double vk;
  double xvaruse, yvaruse, zvaruse, xyvaruse, xzvaruse, yzvaruse;
  double totaltheta, totalmajor, totalminor;
  double totalbackk, totalforwardk, totalbackkdist, totalforwardkdist, ak2;
  int endpoint;
  double endlx, endly, endlz, endmx, endmy, endmz;
  double totalendtheta, totalendmajor, totalendminor, endellipsedist;

  *clpnt = -1;
  *eldist = -1.;

  koffset = (double *)shMalloc(npts*sizeof(double));
  koffset[0] = 0;
  for (tcnt=1; tcnt<npts; tcnt++) {
    xd = xt[tcnt]-xt[tcnt-1];
    yd = yt[tcnt]-yt[tcnt-1];
    zd = zt[tcnt]-zt[tcnt-1];
    koffset[tcnt] = koffset[tcnt-1]+sqrt(xd*xd+yd*yd+zd*zd);
  }

  /* Find the set of locus points that is between the endpoints */
  backpoint = 0;
  while ((koffset[backpoint]<backk)&&(backpoint<npts-1)) backpoint++;
  forwardpoint = npts-1;
  while ((koffset[forwardpoint]>forwardk)&&(forwardpoint>0)) forwardpoint--;

  /* find out if the point is outside the locus */

    /* Find the value of x,y,z that is closest to the locus and consistent with known 
	information */

    if ((xvar<0.0)&&(yvar<0.0)&&(zvar<0.0)) {
      shFree(koffset);
      return -1;
    }

    /* First, find the closest locus point in the Euclidean sense.  If some of
    the data is missing, it is placed as close to each locus point as possible
    with the constraints of upper or lower limits. */
    closestpoint = backpoint;
    closestdist = atDistanceCalc(x,y,z,xvar,yvar,zvar,xt[backpoint],yt[backpoint],zt[backpoint]);
    /*printf("0 %d %g\n", closestpoint, closestdist);*/
    /* Now that we have calculated the distance to the backpoint-th locus point,
    go through the rest of the locus points and reset ``closestpoint" and
    ``closestdist" as necessary */
    for (tcnt=backpoint+1;tcnt<forwardpoint+1;tcnt++) {
       dist = atDistanceCalc(x,y,z,xvar,yvar,zvar,xt[tcnt],yt[tcnt],zt[tcnt]);
       /* All else being equal, take the redder locus point */
       if (dist<closestdist+EPSILON) {
          closestdist = dist;
          closestpoint = tcnt;
       }
       /*printf("0 %d %g %d %g\n", tcnt, dist, closestpoint, closestdist);*/
    }
    *clpnt = closestpoint;

    /* Return an error if the major and minor axes are reversed or negative. */
    if ((distmax[closestpoint]<0)||(distmin[closestpoint]<0)||
	(distmax[closestpoint]<distmin[closestpoint])) {
      shFree(koffset);
      return -2;
    }

  /* Return an error if the closest locus point, or one of the endpoints, 
     has kx=ky=0 - our definition of i and j is undefined in this case. */ 
    if ((kx[closestpoint]==0)&&(ky[closestpoint]==0)) {
      shFree( koffset);
      return -3;
    }
    if ((kx[backpoint]==0)&&(ky[backpoint]==0)) {
      shFree(koffset);
      return -3;
    }
    if ((kx[forwardpoint]==0)&&(ky[forwardpoint]==0)) {
      shFree(koffset);
      return -3;
    }

    /* First, find the major axis, minor axis and position angle of the error
	ellipse in the plane of the cross section at the closest point.  Compute
	the three elements of the variance - covariance matrix in the lm plane,
	then compute the ellipse. */

    atEllipseConvolve (kx[closestpoint], ky[closestpoint], kz[closestpoint], theta[closestpoint], distmax[closestpoint], distmin[closestpoint], xvar, xyvar, xzvar, yvar, yzvar, zvar, nsigma, &totalmajor, &totalminor, &totaltheta);

  /* Return an error if the convolved axes are reversed or negative. Can happen when covariances are large and variances are small or zero, not necessarily physically possible*/
    if ((totalmajor<0)||(totalminor<0)||
        (totalmajor<totalminor)) {
      shFree(koffset);
      return -2;
    }

    atCalcLm (kx[closestpoint], ky[closestpoint], kz[closestpoint], totaltheta, &lx, &ly, &lz, &mx, &my, &mz);

    /* Compute the totalbackkdist and totalforwardkdist */
    xyvaruse = xyvar; xzvaruse = xzvar; yzvaruse = yzvar;
    if (xvar<0) {xvaruse = 0.0; xyvaruse = 0.0; xzvaruse = 0.0;} else {xvaruse = xvar;}
    if (yvar<0) {yvaruse = 0.0; xyvaruse = 0.0; yzvaruse = 0.0;} else {yvaruse = yvar;}
    if (zvar<0) {zvaruse = 0.0; xzvaruse = 0.0; yzvaruse = 0.0;} else {zvaruse = zvar;}
    vk = kx[backpoint]*kx[backpoint]*xvaruse + ky[backpoint]*ky[backpoint]*yvaruse + 
	kz[backpoint]*kz[backpoint]*zvaruse + 2.0*kx[backpoint]*ky[backpoint]*xyvaruse 
	+ 2.0*kx[backpoint]*kz[backpoint]*xzvaruse + 2.0*ky[backpoint]*kz[backpoint]*yzvaruse;
    totalbackkdist = sqrt(backkdist*backkdist+nsigma*nsigma*vk);
    vk = kx[forwardpoint]*kx[forwardpoint]*xvaruse + ky[forwardpoint]*ky[forwardpoint]*yvaruse + 
	kz[forwardpoint]*kz[forwardpoint]*zvaruse + 2.0*kx[forwardpoint]*ky[forwardpoint]*xyvaruse 
	+ 2.0*kx[forwardpoint]*kz[forwardpoint]*xzvaruse + 2.0*ky[forwardpoint]*kz[forwardpoint]*yzvaruse;
    totalforwardkdist = sqrt(forwardkdist*forwardkdist+nsigma*nsigma*vk);

    /* In the case that information is missing about some of the colors, calculate
	the allowed point in color space that is most likely to be included in the
	locus.  This is not necessarily the closest Euclidean distance to the
	closest locus point, since the excluded section is a right elliptical
	cylinder, not a sphere centered at the locus point. */

    if (xvar<0.0) {
       if (yvar<0.0) {
          /* x and y are missing */
          zd = z-zt[closestpoint];
          /* Calculate the values for xd and yd that will put the point exactly on the locus */
	  atMoveToLocus (x-xt[closestpoint], y-yt[closestpoint], kx[closestpoint], ky[closestpoint], kz[closestpoint], xvar, yvar, totalmajor, totalminor, lx, ly, lz, mx, my, mz, &xd, &yd, zd);
       } else {
          yd = y-yt[closestpoint];
          if (zvar<0.0) {
            /* x and z are missing */
            /* Calculate the values for zd and xd that will put the point exactly on the locus */
	    atMoveToLocus (z-zt[closestpoint], x-xt[closestpoint], kz[closestpoint], kx[closestpoint], ky[closestpoint], zvar, xvar, totalmajor, totalminor, lz, lx, ly, mz, mx, my, &zd, &xd, yd);
          } else {
            /* Only x is missing */
            zd = z-zt[closestpoint];
	    xd = atCalcOptimalX (totalmajor, totalminor, lx, ly, lz, mx, my, mz, yd, zd);
	    xd = atApplyLimits (xvar, xd, x-xt[closestpoint]);
          }
       }
    } else {
       xd = x-xt[closestpoint];
       if (yvar<0.0) {
          if (zvar<0.0) {
            /* y and z are both missing */
            /* Calculate the values for yd and zd that will put the point exactly on the locus */
	    atMoveToLocus (y-yt[closestpoint], z-zt[closestpoint], ky[closestpoint], kz[closestpoint], kx[closestpoint], yvar, zvar, totalmajor, totalminor, ly, lz, lx, my, mz, mx, &yd, &zd, xd);
          } else {
            /* Only y is missing */
            zd = z-zt[closestpoint];
	    yd = atCalcOptimalY (totalmajor, totalminor, lx, ly, lz, mx, my, mz, xd, zd);
	    yd = atApplyLimits (yvar, yd, y-yt[closestpoint]);
          }
       } else {
          yd = y-yt[closestpoint];
          if (zvar<0.0) {
            /* Only z is missing */
            zd = atCalcOptimalZ (totalmajor,totalminor,lx,ly,lz,mx,my,mz,xd,yd);
	    zd = atApplyLimits (zvar, zd, z-zt[closestpoint]);
          } else {
            /* x, y and z are all there */
            zd = z-zt[closestpoint];
          }
       }
    }

    /* Find out if the star is in the locus or not */

    inlocus = 0;	/* Default to being outside the locus */

    kd = kx[closestpoint]*xd + ky[closestpoint]*yd + kz[closestpoint]*zd;
    id = ( -kx[closestpoint]*kz[closestpoint]*xd - ky[closestpoint]*kz[closestpoint]*yd + (kx[closestpoint]*kx[closestpoint] +ky[closestpoint]*ky[closestpoint])*zd ) / sqrt(kx[closestpoint]*kx[closestpoint] + ky[closestpoint]*ky[closestpoint]);
    jd = ( ky[closestpoint]*xd - kx[closestpoint]*yd ) / sqrt(kx[closestpoint]*kx[closestpoint] + ky[closestpoint]*ky[closestpoint]);
    ld = cos(totaltheta)*id + sin(totaltheta)*jd;
    md = -sin(totaltheta)*id + cos(totaltheta)*jd;

    ellipsedist = atCalcEllipseDist (ld, md, totalmajor, totalminor);
    *eldist = ellipsedist;
    /*printf("closest %d, ld %g, md %g, maj %g, min %g, theta %g\n",closestpoint, ld, md, totalmajor, totalminor, totaltheta);*/
    if (ellipsedist<1.0+EPSILON) {
      /* The comparators for totalbackk and totalforwardk were changed to
	>= and <= from > and < to handle the limits cases */
      if (kd < 0) {
        if ((kd+koffset[closestpoint]+EPSILON)>backk) {
          inlocus = 1;
        } else {
	  endpoint = backpoint;
          kendpoint = backk;
          ak2 = totalbackkdist*totalbackkdist;
          atEllipseConvolve (kx[endpoint], ky[endpoint], kz[endpoint], theta[endpoint], distmax[endpoint], distmin[endpoint], xvar, xyvar, xzvar, yvar, yzvar, zvar, nsigma, &totalendmajor, &totalendminor, &totalendtheta);
          atCalcLm (kx[endpoint], ky[endpoint], kz[endpoint], totalendtheta, &endlx, &endly, &endlz, &endmx, &endmy, &endmz);
          endellipsedist = atCalcEllipseDist (ld, md, totalendmajor, totalendminor);
          if (endellipsedist<1.0+EPSILON) {
            totalbackk = backk-totalbackkdist*sqrt(1.0-endellipsedist);
            if ((kd+koffset[closestpoint]+EPSILON)>totalbackk) {
              inlocus = 1;
            }
          }
        }
      } else {
        if ((kd+koffset[closestpoint])<forwardk+EPSILON) {
          inlocus = 1;
        } else {
	  endpoint = forwardpoint;
          kendpoint = forwardk;
          ak2 = totalforwardkdist*totalforwardkdist;
          atEllipseConvolve (kx[endpoint], ky[endpoint], kz[endpoint], theta[endpoint], distmax[endpoint], distmin[endpoint], xvar, xyvar, xzvar, yvar, yzvar, zvar, nsigma, &totalendmajor, &totalendminor, &totalendtheta);
          atCalcLm (kx[endpoint], ky[endpoint], kz[endpoint], totalendtheta, &endlx, &endly, &endlz, &endmx, &endmy, &endmz);
          endellipsedist = atCalcEllipseDist (ld, md, totalendmajor, totalendminor);
          if (endellipsedist<1+EPSILON) {
            totalforwardk = kendpoint+totalforwardkdist*sqrt(1.0-endellipsedist);
            if ((kd+koffset[closestpoint])<totalforwardk+EPSILON) {
              inlocus = 1;
            }
          }
        }
      }
         
      if (inlocus == 0) {
	 /* We were close enough to the center of the locus, but failed the end conditions.
            Try moving the star in the allowed direction(s) to be on the endplane (or in the 
	    end ellipsoid), and then ask if the new ellipsedist <=1.0 */

	 if ((xvar<0)||(yvar<0)||(zvar<0)) {
            if (xvar<0.0) {
               if (yvar<0.0) {
                  /* x and y are missing */
                  /* Calculate the values for yd and xd that will put the point exactly on end plane */
	          zd = z-zt[endpoint];
	          atMoveToEllipsoid (kx[endpoint], ky[endpoint], kz[endpoint], endlx, endly, endlz, endmx, endmy, endmz, totalendmajor*totalendmajor, totalendminor*totalendminor, ak2, kendpoint - koffset[endpoint], &xd, &yd, zd, xvar, yvar, x-xt[endpoint], y-yt[endpoint]);
               } else {
                  if (zvar<0.0) {
                    /* x and z are missing */
                    /* Calculate the values for zd and xd that will put the point exactly on the locus */
	            yd = y-yt[endpoint];
	            atMoveToEllipsoid (kz[endpoint], kx[endpoint], ky[endpoint], endlz, endlx, endly, endmz, endmx, endmy, totalendmajor*totalendmajor, totalendminor*totalendminor, ak2, kendpoint - koffset[endpoint], &zd, &xd, yd, zvar, xvar, z-zt[endpoint], x-xt[endpoint]);
                  } else {
                    /* Only x is missing */
		    zd = z-zt[endpoint];
		    yd = y-yt[endpoint];
		    atMoveXToEllipsoid (kx[endpoint], ky[endpoint], kz[endpoint], endlx, endly, endlz, endmx, endmy, endmz, totalendmajor*totalendmajor, totalendminor*totalendminor, ak2, kendpoint - koffset[endpoint], &xd, yd, zd);
	            xd = atApplyLimits (xvar, xd, x-xt[endpoint]);
                  }
               }
            } else {
               if (yvar<0.0) {
                  if (zvar<0.0) {
                    /* y and z are both missing */
                    /* Calculate the values for yd and zd that will put the point exactly on the locus */
		    xd = x-xt[endpoint];
	            atMoveToEllipsoid (ky[endpoint], kz[endpoint], kx[endpoint], endly, endlz, endlx, endmy, endmz, endmx, totalendmajor*totalendmajor, totalendminor*totalendminor, ak2, kendpoint - koffset[endpoint], &yd, &zd, xd, yvar, zvar, y-yt[endpoint], z-zt[endpoint]);
                  } else {
                    /* Only y is missing */
		    xd = x-xt[endpoint];
		    zd = z-zt[endpoint];
		    atMoveXToEllipsoid (ky[endpoint], kz[endpoint], kx[endpoint], endly, endlz, endlx, endmy, endmz, endmx, totalendmajor*totalendmajor, totalendminor*totalendminor, ak2, kendpoint - koffset[endpoint], &yd, zd, xd);
	            yd = atApplyLimits (yvar, yd, y-yt[endpoint]);
                  }
               } else {
                  if (zvar<0.0) {
                    /* Only z is missing */
		    yd = y-yt[endpoint];
		    xd = x-xt[endpoint];
		    atMoveXToEllipsoid (kz[endpoint], kx[endpoint], ky[endpoint], endlz, endlx, endly, endmz, endmx, endmy, totalendmajor*totalendmajor, totalendminor*totalendminor, ak2, kendpoint - koffset[endpoint], &zd, xd, yd);
	            zd = atApplyLimits (zvar, zd, z-zt[endpoint]);
                  }
               }
            }
   
            /* Now that we've recalculated the position, check the distance */
            kd = kx[endpoint]*xd + ky[endpoint]*yd + kz[endpoint]*zd;
    	    id = ( -kx[endpoint]*kz[endpoint]*xd - ky[endpoint]*kz[endpoint]*yd + (kx[endpoint]*kx[endpoint] +ky[endpoint]*ky[endpoint])*zd ) / sqrt(kx[endpoint]*kx[endpoint] + ky[endpoint]*ky[endpoint]);
    	    jd = ( ky[endpoint]*xd - kx[endpoint]*yd ) / sqrt(kx[endpoint]*kx[endpoint] + ky[endpoint]*ky[endpoint]);
    	    ld = cos(totalendtheta)*id + sin(totalendtheta)*jd;
    	    md = -sin(totalendtheta)*id + cos(totalendtheta)*jd;
            endellipsedist = atCalcEllipseDist (ld, md, totalendmajor, totalendminor);
	    if (endellipsedist<1+EPSILON) {
	       if (endpoint==forwardpoint) {
                  totalforwardk = kendpoint+totalforwardkdist*sqrt(1.0-endellipsedist);
                  if ((kd+koffset[endpoint])<totalforwardk+EPSILON) {
                    inlocus = 1;
                  }
               } else {
                  totalbackk = kendpoint-totalbackkdist*sqrt(1.0-endellipsedist);
                  if ((kd+koffset[endpoint]+EPSILON)>totalbackk) {
                    inlocus = 1;
                  }
	       }
	    }

         }
      }
    }

    shFree(koffset);
  

    return inlocus;

}

/*<AUTO EXTRACT>

  ROUTINE: atQuLocusKlmFromXyz

  DESCRIPTION: Given a set of npts locus points xt,yt,zt,kx,ky,kz,theta, 
	calculate the <k,l,m> values for point <x,y,z>
<HTML>
</HTML>
</AUTO>*/

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
		double *m) {
  int tcnt;
  double xd,yd,zd,kd,id,jd;
  int closestpoint;
  double closestdist, dist;
  double *koffset;

  koffset = (double *)shMalloc(npts*sizeof(double));
  koffset[0] = 0;
  for (tcnt=1; tcnt<npts; tcnt++) {
    xd = xt[tcnt]-xt[tcnt-1];
    yd = yt[tcnt]-yt[tcnt-1];
    zd = zt[tcnt]-zt[tcnt-1];
    koffset[tcnt] = koffset[tcnt-1]+sqrt(xd*xd+yd*yd+zd*zd);
  }

  /* Find the locus point it is the closest to */
  closestpoint = 0;
  xd = x-xt[0];
  yd = y-yt[0];
  zd = z-zt[0];
  closestdist = sqrt(xd*xd + yd*yd + zd*zd);
  for(tcnt=1;tcnt<npts;tcnt++){
    xd = x-xt[tcnt];
    yd = y-yt[tcnt];
    zd = z-zt[tcnt];
    dist = sqrt(xd*xd + yd*yd + zd*zd);
    if (dist<closestdist) {
      closestdist = dist;
      closestpoint = tcnt;
    }
  }

  /* Calculate k,l,m */
  xd = x-xt[closestpoint];
  yd = y-yt[closestpoint];
  zd = z-zt[closestpoint];
  kd = kx[closestpoint]*xd + ky[closestpoint]*yd + kz[closestpoint]*zd;

  id = ( -kx[closestpoint]*kz[closestpoint]*xd -
       ky[closestpoint]*kz[closestpoint]*yd +
       (kx[closestpoint]*kx[closestpoint]
       +ky[closestpoint]*ky[closestpoint])*zd )
       /
       sqrt(kx[closestpoint]*kx[closestpoint]
       +ky[closestpoint]*ky[closestpoint]);
  jd = ( ky[closestpoint]*xd - kx[closestpoint]*yd ) /
       sqrt(kx[closestpoint]*kx[closestpoint]
       +ky[closestpoint]*ky[closestpoint]);
  *l = cos(theta[closestpoint])*id + sin(theta[closestpoint])*jd;
  *m = -sin(theta[closestpoint])*id + cos(theta[closestpoint])*jd;
  *k = kd+koffset[closestpoint];

  shFree(koffset);

}
