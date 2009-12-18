/* <AUTO>
  FILE: atImSim
<HTML>
  Routines to simulate images
</HTML>
  </AUTO>
*/

/***************************************************************
 *
 *  C routines to simulate galaxies
 *
 **************************************************************/

#include "smaValues.h"
#include "dervish.h"
#include "atImSim.h"
#include "slalib.h"
#include "atGauss.h"
#include "atConversions.h"

static double deVauc(double Izero, double rs, double row, double col, double r, double c, double theta, double axisRatio) {
      double rad;
      double ct,st,xp,yp;
	ct = cos(-theta*3.14159265/180.0);
	st = sin(-theta*3.14159265/180.0);
	xp = (c-col)*ct + (r-row)*st;
	yp = (-(c-col)*st + (r-row)*ct)/axisRatio;
	rad = sqrt(xp*xp+yp*yp);
      /*rad = sqrt((r-row)*(r-row)+(c-col)*(c-col));*/
      return (Izero*(exp(-(pow(rad/rs,0.25)))));
}

/* This routine is not adapted from Numerical Recipes */
/* It calculates the rectangular integral with nxn boxes */
static double recDeVauc(double Izero, double rs, double row, double col, double ra, double rb, double ca, double cb, double theta, double axisRatio, int n) {
  double x,y, tnm, sum, delr, delc;
  int it, j, k;

  if (n==1) {
    return (sum=deVauc(Izero,rs,row,col,(ra+rb)/2,(ca+cb)/2,theta, axisRatio));
  } else {
    for (it=1,j=0;j<n-1;j++) it <<= 1;
    tnm=it;
    delr=(rb-ra)/tnm;
    delc=(cb-ca)/tnm;
    x=ra+0.5*delr;
    y=ca+0.5*delc;
    sum = 0.0;
    for (j=0;j<it;j++,x+=delr,y=ca+0.5*delc)
      for (k=0;k<it;k++,y+=delc) sum += deVauc(Izero,rs,row,col,x,y, theta, axisRatio);
    sum=sum/(tnm*tnm);
  }
  return sum;
}

#define EPS 0.01
#define JMAX 20

static double intDeVauc(double Izero, double rs, double row, double col, double ra, double rb, double ca, double cb,double theta, double axisRatio) {
  int j;
  double s, olds;

  olds = -1.0e30;
  for (j=1;j<=JMAX;j++) {
    s=recDeVauc(Izero,rs,row,col,ra,rb,ca,cb,theta, axisRatio,j);
    if (fabs(s-olds)<EPS*fabs(olds)) return s;
    olds = s;
  }
  return -1;			/* We should never get here */
}

/*<AUTO EXTRACT>

  ROUTINE: atExpDiskAdd

  DESCRIPTION:
<HTML>
	This routine adds an exponential disk to the image.
</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS
</HTML>
</AUTO>*/

RET_CODE atExpDiskAdd (
		       REGION *region, 
		       double row, 
		       double col, 
		       double Izero, 
		       double rs, 
		       double theta, 
		       double axisRatio,
		       int peak,
		       int fast,
		       double *countsAdded
		       ) {
  int rstart,rend,cstart,cend;	/* limits of disk */
  int r,c;
  float rad, newval, gal,xp,yp,ct,st;

  if (rs == 0) return SH_SUCCESS;

  /* Find extent of a box within region which contains 
     4*rs in every direction */
   if ( axisRatio > 1 ) axisRatio = 1;
   if ( axisRatio < 0.1 ) axisRatio = 0.1;
  if(peak==0) {
   Izero = Izero*9.0/44118.0*32.0*32.0/axisRatio/rs/rs;
 }
  rstart = floor(row-4*rs);
  rend = row+4*rs;
  cstart = floor(col-4*rs);
  cend = ceil(col+4*rs);
  if (rstart<0) rstart = 0;
  if (cstart<0) cstart = 0;
  if (rend>region->nrow) rend = region->nrow;
  if (cend>region->ncol) cend = region->ncol;

  /* For each pixel in the box, add the exponential disk */
  *countsAdded = 0;
	ct = cos(-theta*3.1415926535/180.);
	st = sin(-theta*3.1415926535/180.);
  for (r=rstart;r<rend;r++) {
    for (c=cstart;c<cend;c++) {
	xp = (c+0.5-col)*ct + (r+0.5-row)*st;
	yp =  (-(c+0.5-col)*st + (r+0.5-row)*ct)/axisRatio;
	rad = sqrt(xp*xp+yp*yp);
      /*rad = sqrt((r+0.5-row)*(r+0.5-row)+(c+0.5-col)*(c+0.5-col));*/
      gal = Izero*(exp(-rad/rs)-exp(-4.0));
      if (gal<0) gal=0;
      *countsAdded += gal;
	if (fast)
	  region->rows_fl32[r][c] += gal;
	else {
	  newval = shRegPixGetAsDbl(region, r, c)+gal;
	  shRegPixSetWithDbl(region, r, c, newval);
	}
      }
  }

  /* Successful return */

  return SH_SUCCESS;

}

/*<AUTO EXTRACT>

  ROUTINE: atDeVaucAdd

  DESCRIPTION:
<HTML>
	This routine adds an de Vaucouleurs profile
		to the image.  Stops when the counts fall
		to 0.1 per pixel.
</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS
</HTML>
</AUTO>*/

RET_CODE atDeVaucAdd (
		      REGION *region, 
		      double row, 
		      double col, 
		      double Izero, 
		      double rs, 
		      double theta,
		      double axisRatio,
		      int peak,
		      int fast,
		      double *countsAdded
		      ) {
  int rstart,rend,cstart,cend;	/* limits of bulge */
  int r,c;
  float rad, newval, gal,ct,st,xp,yp,icorr,extent;

  /* Find extent of a box within region which contains 
     the bulge until it falls to 0.1 counts */
   if ( axisRatio > 1 ) axisRatio = 1;
   if ( axisRatio < 0.10 ) axisRatio = 0.10;
   icorr = 1 - atan2(1.0/axisRatio - 1.0,1.0)/14.0;
  /*extent = rs*pow(log(10.0*Izero*10),4.0);*/
  if(peak==0){
   Izero = Izero*8000/57716.0*icorr/axisRatio*0.008*0.008/rs/rs;
 }
  extent = rs*pow(log(800.0*Izero),4.0)/4.0;
/*printf("extent %f\n",extent);*/
  rstart = floor(row-extent);
  rend = row+extent;
  cstart = floor(col-extent);
  cend = ceil(col+extent);
  if (rstart<0) rstart = 0;
  if (cstart<0) cstart = 0;
  if (rend>region->nrow) rend = region->nrow;
  if (cend>region->ncol) cend = region->ncol;
	/*printf("%f %d %d %d %d\n",extent,rstart,rend,cstart,cend);*/


  /* For each pixel in the box, add the de Vaucoulers profile */
  *countsAdded = 0;
   ct = cos(-theta*3.1415926535/180.);
   st = sin(-theta*3.1415926535/180.);
  for (r=rstart;r<rend;r++) {
    for (c=cstart;c<cend;c++) {
	if(fabs(row-r-0.5)<=1 && fabs(col-c-0.5)<=1) {
      /*gal = Izero*(exp(-pow(rad/rs,0.25)));*/
      gal = intDeVauc(Izero,rs,row,col,r,r+1,c,c+1,theta, axisRatio);
	} else {
        xp = (c+0.5-col)*ct + (r+0.5-row)*st;
        yp =  (-(c+0.5-col)*st + (r+0.5-row)*ct)/axisRatio;
	rad = sqrt(xp*xp+yp*yp);
      gal = Izero*(exp(-pow(rad/rs,0.25)));
	}
      /*rad = 
	sqrt((r+0.5-row)*(r+0.5-row)+(c+0.5-col)*(c+0.5-col));
      gal = Izero*(exp(-pow(rad/rs,0.25)));*/
      /*gal = intDeVauc(Izero,rs,row,col,r,r+1,c,c+1);*/
      if (gal<0) gal=0;
      *countsAdded += gal;
	if(fast)
	region->rows_fl32[r][c] += gal;
	else { 
	  newval = shRegPixGetAsDbl(region, r, c)+gal;
	  shRegPixSetWithDbl(region, r, c, newval);
	    }
      }
  }

  /* Successful return */

  return SH_SUCCESS;

}

/*<AUTO EXTRACT>

  ROUTINE: atDeVaucAddSlow

  DESCRIPTION:
<HTML>
	This routine adds a de Vaucouleurs profile
		to the image.  Goes out to rmax and
		only works on FL32 regions.
</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS
</HTML>
</AUTO>*/

RET_CODE atDeVaucAddSlow (
			  REGION *region, /* region to add profile to */
			  double row, /* row position */
			  double col, /* column position */
			  double counts, /* total counts */
			  double rs, /* scale length */
			  double maxRad, /* maximum distance from center */
			  double theta,	/* position angle */
			  double axisRatio, /* b/a */
			  int ngrid, /* size of grid to sample pixels */
			  double *countsOut /* counts actually added */
			  ) {
  int r, r0, r1, c, c0, c1, ir, ic;
  double sum, xp, yp, st, ct;
  double gs = 1.0/(ngrid+1);
  int nsum;
  double mra, i0;
  double gr, gc, rad;
  double gr0, gc0, xp0, yp0, rad0;
  *countsOut = 0;
  ct = cos(-theta*at_deg2Rad);
  st = sin(-theta*at_deg2Rad);

  mra = maxRad/axisRatio;
  r0 = row-mra; 
  r1 = row+mra+1; 
  c0 = col-mra; 
  c1 = col+mra+1;

  if (r0 < 0) r0=0; if (r1 > region->nrow) r1=region->nrow;
  if (c0 < 0) c0=0; if (c1 > region->ncol) c1=region->ncol;

  i0 = counts/(0.0106*axisRatio*rs*rs);
  for (r=r0; r<r1; r++) {
    gr0 = r+0.5-row;
    for (c=c0; c<c1; c++) {
      gc0 = c+0.5-col;
      xp0 =   gc0*ct + gr0*st; 
      yp0 = (-gc0*st + gr0*ct)/axisRatio;
      rad0 = sqrt(xp0*xp0+yp0*yp0);
      if (rad0/rs < 2.0) {
	sum=0; nsum=0;
	for (ir=1; ir<ngrid+1; ir++) {
	  gr = r+ir*gs-row;
	  for (ic=1; ic<ngrid+1; ic++) {
	    gc = c+ic*gs-col;
	    xp =   gc*ct + gr*st; 
	    yp = (-gc*st + gr*ct)/axisRatio;
	    rad = sqrt(xp*xp + yp*yp);
	    if (rad < mra) {
	      sum += i0*pow(10, (-3.33*pow(rad/rs,0.25)));
	      nsum++;
	    }
	  } 
	}
	if (nsum > 0) {
	  region->rows_fl32[r][c] += sum/nsum;
	  *countsOut += sum/nsum;
	}
      } else {
	if (rad0 < mra) {
	  sum = i0*pow(10, (-3.33*pow(rad0/rs,0.25)));
	  region->rows_fl32[r][c] += sum;
	  *countsOut += sum;
	}
      }
    }
  }

  return SH_SUCCESS;
}
/*<AUTO EXTRACT>

  ROUTINE: atExpDiskAddSlow

  DESCRIPTION:
<HTML>
	This routine adds an exponential disk profile
		to the image.  Goes out to rmax and
		only works on FL32 regions.
</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS
</HTML>
</AUTO>*/

RET_CODE atExpDiskAddSlow(
			  REGION *region, /* region to add profile to */
			  double row, /* row position */
			  double col, /* column position */
			  double counts, /* total counts */
			  double rs, /* scale length */
			  double maxRad, /* maximum distance from center */
			  double theta,	/* position angle */
			  double axisRatio, /* b/a */
			  int ngrid, /* size of grid to sample pixels */
			  double *countsOut /* counts actually added */
			  ) {
  int r, r0, r1, c, c0, c1, ir, ic;
  double sum, xp, yp, st, ct;
  double gs = 1.0/(ngrid+1);
  int nsum;
  double mra, i0;
  double gr, gc, rad;
  double gr0, gc0, xp0, yp0, rad0;
  *countsOut = 0;
  ct = cos(-theta*at_deg2Rad);
  st = sin(-theta*at_deg2Rad);

  if (axisRatio > 1.0) axisRatio = 1.0;
  if (axisRatio < 0.1) axisRatio = 0.1;

  mra = maxRad/axisRatio;
  r0 = row-mra; 
  r1 = row+mra+1; 
  c0 = col-mra; 
  c1 = col+mra+1;
  if (r0 < 0) r0=0; if (r1 > region->nrow) r1=region->nrow;
  if (c0 < 0) c0=0; if (c1 > region->ncol) c1=region->ncol;

  i0 = counts/(2.0*M_PI*axisRatio*rs*rs);
  for (r=r0; r<r1; r++) {
    gr0 = r+0.5-row;
    for (c=c0; c<c1; c++) {
      gc0 = c+0.5-col;
      xp0 =   gc0*ct + gr0*st; 
      yp0 = (-gc0*st + gr0*ct)/axisRatio;
      rad0 = sqrt(xp0*xp0+yp0*yp0);
      if (rad0/rs < 2.0) {
	sum=0; nsum=0;
	for (ir=1; ir<ngrid+1; ir++) {
	  gr = r+ir*gs-row;
	  for (ic=1; ic<ngrid+1; ic++) {
	    gc = c+ic*gs-col;
	    xp =   gc*ct + gr*st; 
	    yp = (-gc*st + gr*ct)/axisRatio;
	    rad = sqrt(xp*xp + yp*yp);
	    if (rad < mra) {
	      sum += i0*exp(-rad/rs);
	      nsum++;
	    }
	  } 
	}
	if (nsum > 0) {
	  region->rows_fl32[r][c] += sum/nsum;
	  *countsOut += sum/nsum;
	}
      } else {
	if (rad0 < mra) {
	  sum = i0*exp(-rad0/rs);
	  region->rows_fl32[r][c] += sum;
	  *countsOut += sum;
	}
      }
    }
  }
  return SH_SUCCESS;
}


/*<AUTO EXTRACT>

  ROUTINE: atDeltaAdd

  DESCRIPTION: 
<HTML>
 	This routine adds a delta function to an image.
	The image can then be convolved with a gaussian or other
	PSF to result in a stellar profile.
	The routine actually distributes light in 3 pixels 
	arranged in a 'L' shape so that sub-pixel
	centroiding of the final star will result.  The
	weighting of the light is such that the standard moment
	of the light has the indicated centroid, but more
	complicated weighting of the light (gaussian weighted)
	will be biased at the 50 milli-pixel level.
</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS
</HTML>
</AUTO>*/

RET_CODE atDeltaAdd (
		     REGION *region, 
		     double row, 
		     double col, 
		     double Izero, 
		     double *countsAdded
		     ) {
	
	int xi,yi;
	double xf,yf,i,j,a1,a2,a3,a4;

	/* distribute light over 4 pixels to get fractional pixel 'delta func' */
	xi= col;
	xf= col-xi;
	xi--;

	if(xf>0.5){
	xf = xf-1;
	xi++;
	}

	yi= row;
	yf= row-yi;
	yi--;

	if(yf>0.5){
	yf = yf-1;
	yi++;
	}

	i=xf; j=yf;

	if(i<=j && j<=0){ 
	 a2=0;
	 a4=0.5+i;
	 a3=j-i;
	 a1=0.5-j;
	 /*q="1l";*/
	} else if (i<=j && i>=0) {
	 a2=0;
	 a1=0.5-j;
	 a3= -i+j;
	 a4=0.5+i;
	 /*q="4l";*/
	} else if (i<=j && j <= -i) { 
	 a4=0;
	 a2=0.5+i;
	 a1=-j-i;
	 a3=0.5+j;
	 /*q="3l";*/
	} else if (i<=j && j > -i) {
	 a1=0;
	 a2=0.5-j;
	 a4= i+j;
	 a3=0.5-i;
	 /*q="3r";*/
	} else if (i>j && i <= 0) {
	 a3=0;
	 a4=0.5+j;
	 a2=i-j;
	 a1=0.5-i;
	 /*q="1r";*/
	} else if (i>j && j >= 0) {
	 a3=0;
	 a1=0.5-i;
	 a2= -j+i;
	 a4=0.5+j;
	 /*q="4r";*/
	} else if (i>j && j <= -i) {
	 a4=0;
	 a3=0.5+j;
	 a1= -i-j;
	 a2=0.5+i;
	 /*q="2l";*/
	} else if (i>j && j > -i) {
	 a1=0;
	 a3=0.5-i;
	 a4=j+i;
	 a2=0.5-j;
	 /*q="2r";*/
	} else {
	printf("screwy octant??\n");
        a1 = a2 = a3 = a4 = 0;
	}


	a1 *= Izero;
	a2 *= Izero;
	a3 *= Izero;
	a4 *= Izero;

	*countsAdded=0;
	if(xi>=0 && yi>=0 && xi<region->ncol && yi < region->nrow){
	shRegPixSetWithDbl(region,yi,xi,a1+shRegPixGetAsDbl(region,yi,xi));
	*countsAdded += a1;
	}
	if((xi+1)>=0 && yi>=0 && (xi+1)<region->ncol && yi < region->nrow){
	shRegPixSetWithDbl(region,yi,(xi+1),a2+shRegPixGetAsDbl(region,yi,xi+1));
	*countsAdded += a2;
	}
	if((xi+1)>=0 && (yi+1)>=0 && (xi+1)<region->ncol && (yi+1) < region->nrow){
	shRegPixSetWithDbl(region,(yi+1),(xi+1),a4+shRegPixGetAsDbl(region,yi+1,xi+1));
	*countsAdded += a4;
	}
	if(xi>=0 && (yi+1)>=0 && xi<region->ncol && (yi+1) < region->nrow){
	shRegPixSetWithDbl(region,(yi+1),xi,a3+shRegPixGetAsDbl(region,yi+1,xi));
	*countsAdded += a3;
	}

	return SH_SUCCESS;
}

/*<AUTO EXTRACT>

  ROUTINE: atWingAdd

  DESCRIPTION:
<HTML>
	 This routine adds a power law wing to an image.
	It stops when the counts in the wing decrease to
	0.1 DN. The central pixel or so of the wing may be excluded
	to avoid an infinite density cusp at r=0 (set the minR param).
	The axial ratio of the wing may be specified for an
	elliptical wing and the power law and peak of the wing
	may be specified.

</HTML>

  RETURN VALUES:
<HTML>
	SH_SUCCESS
</HTML>
</AUTO>*/

RET_CODE atWingAdd (
		    REGION *region, 
		    double row, 
		    double col, 
		    double Izero, 
		    double alpha, 
		    double theta,
		    double axisRatio,
		    double minR,
		    int peak,
		    int fast,
		    double *countsAdded
		    ) {
  int rstart,rend,cstart,cend;	/* limits of bulge */
  int r,c;
  float rad, newval, gal,ct,st,xp,yp,extent;

  /* Find extent of a box within region which contains 
     the bulge until it falls to 0.1 counts */
   if ( axisRatio > 1 ) axisRatio = 1;
   if ( axisRatio < 0.01 ) axisRatio = 0.01;
  if(peak==0){
   Izero = Izero/axisRatio/30.0;
 }
   if (alpha > -2) alpha = -2.0;
   if (alpha < -6) alpha = -6.0;
   if(minR<=0.0) minR = 0.0;
  extent = pow(0.1/Izero,1.0/alpha);
/*printf("extent %f\n",extent);*/
  rstart = floor(row-extent);
  rend = row+extent;
  cstart = floor(col-extent);
  cend = ceil(col+extent);
  if (rstart<0) rstart = 0;
  if (cstart<0) cstart = 0;
  if (rend>region->nrow) rend = region->nrow;
  if (cend>region->ncol) cend = region->ncol;
	/*printf("%f %d %d %d %d\n",extent,rstart,rend,cstart,cend);*/


  /* For each pixel in the box, add the de Vaucoulers profile */
  *countsAdded = 0;
   ct = cos(-theta*3.1415926535/180.);
   st = sin(-theta*3.1415926535/180.);
  for (r=rstart;r<rend;r++) {
    for (c=cstart;c<cend;c++) {

        xp = (c+0.5-col)*ct + (r+0.5-row)*st;
        yp =  (-(c+0.5-col)*st + (r+0.5-row)*ct)/axisRatio;
	rad = sqrt(xp*xp+yp*yp);
	if(rad>minR) {
      gal = Izero*pow((double)rad,alpha);
    } else {
      gal = 0;
    }
      if (gal<0) gal=0;
      *countsAdded += gal;

	if(fast)
	  region->rows_fl32[r][c] += gal;
	else {
      newval = shRegPixGetAsDbl(region, r, c)+gal;
      shRegPixSetWithDbl(region, r, c, newval);
	}
    }
  }

  /* Successful return */

  return SH_SUCCESS;

}

/*<AUTO EXTRACT>
  ROUTINE: atRegStatNoiseAdd

  DESCRIPTION:  Add statistical noise to a region, assuming gain=1

  RETURN VALUES: SH_SUCCESS

</AUTO>*/
RET_CODE atRegStatNoiseAdd(
                           REGION *reg
                           ) {
  int r, c;
  double value, r1, r2, delta, width;
  double twoPi = 2.0*M_PI;

  for (r=0; r<reg->nrow; r++) {
    for (c=0; c<reg->ncol; c++) {
      value = shRegPixGetAsDbl(reg, r, c);
      width = sqrt(value);
      r1 = (double) rand() / (double) RAND_MAX;
      r2 = 0;
      while (r2 == 0) {
        r2 = (double) rand() / (double) RAND_MAX;
      }
      delta = sin(twoPi*r1) * sqrt(-2.0*log(r2));
      shRegPixSetWithDbl(reg, r, c, value+width*delta);
    }
  }
  return SH_SUCCESS;
}
