#include <string.h>
#include "atSlalib_c.h"

/*
   Prototypes for the calls to the FORTRAN library.
   The only place that these are called directly are in THIS file.
   */

/* String Decoding */
void sla_intin_(char *string, int *nstrt, int *result, 
		int *jflag, size_t strlength); 
void sla_flotin_(char *string, int *nstrt, float *result, 
		 int *jflag, size_t strlength); 
void sla_dfltin_(char *string, int *nstrt, double *result, 
		 int *jflag, size_t strlength); 

/*Sexagesimal Conversions */ 
void sla_ctf2d_(int *hour, int *min, float *sec, float *days, int *jflag); 
void sla_dtf2r_(int *hour, int *min, double *sec, double *radian, int *jflag);
void sla_daf2r_(int *deg, int *min, double *sec, double *radian, int *jflag);
void sla_dr2af_(int *ndp, double *angle, char *sign, int *idmsf,int);
void sla_dr2tf_(int *ndp, double *angle, char *sign, int *ihmsf,int);

/* Angles, Vectors, and Rotation Matrices */
double sla_drange_(double *angle);
double sla_dranrm_(double *angle);
double sla_dsep_(double *a1, double *b1, double *a2, double *b2);
double sla_dbear_(double *a1, double *b1, double *a2, double *b2);

/* Calendars */
void sla_cldj_(int *iy, int *im, int *id, double *djm, int *j);
void sla_caldj_(int *iy, int *im, int *id, double *djm, int *j);
void sla_djcl_(double *djm, int *iy, int *im, int *id, double *fd, int *j);
void sla_calyd_(int *iy, int *im, int *id, int *nd, int *j);
double sla_epb_(double *date);
double sla_epb2d_(double *epb);
double sla_epj_(double *date);
double sla_epj2d_(double *epj);

/* Timescales */
double sla_gmst_(double *ut1);
double sla_eqeqx_(double *date);
double sla_dat_(double *dju);
double sla_dtt_(double *dju);
double sla_rcc_(double *tdb, double *ut1, double *wl, double *u, double *v);

/* Precession and Nutation */
void sla_nut_(double *date, double *rmatn);
void sla_nutc_(double *date, double *dpsi, double *deps, double *eps0);
void sla_prec_(double *ep0, double *ep1, double *rmatp);
void sla_prenut_(double *epoch, double *date, double *rmatpn);
void sla_prebn_(double *bep0, double *bep1, double *rmatp);
void sla_preces_(char *sys, double *ep0, double *ep1, 
		 double *ra, double *dc, int);

/* Proper Motion */
void sla_pm_(double *r0, double *d0, double *pr, double *pd, double *px,
	     double *rv, double *ep0, double *ep1, double *r1, double *d1);

/* Observed Place */
double sla_zd_(double* ha, double* dec, double* phi);

/* Refraction and Air Mass */
double sla_airmas_(double* zd);

/* Astrometry */
void sla_ds2tp_(double* ra, double* dec, double* raz, double* decz, double* xi,
		double* eta, int* j);
void sla_fitxy_(int *itype, int *np, double *xye, double *xym,
		double *coeffs, int *jflag);
void sla_invf_(double *forwardCoeffs, double *backwardsCoeffs, int *jflag);
void sla_xy2xy_(double *xe, double *ye, double *coeff, double *xm, double *ym);

/*
   c bindings for SLALIB routines
   */

/* String Decoding */
void sla_intin(char *string, int *nstrt, int *result, int *resultFlag)
{ sla_intin_(string, nstrt, result, resultFlag, strlen(string)); }

void sla_flotin(char *string, int *nstrt, float *result, int *resultFlag)
{ sla_flotin_( string, nstrt, result, resultFlag, strlen(string)); }

void sla_dfltin(char *string, int *nstrt, double *result, int *resultFlag)
{ sla_dfltin_( string, nstrt, result, resultFlag, strlen(string)); }

/* Sexagesimal Conversions */
void sla_ctf2d(int *hour, int *minute, float *second, float *days, int *jflag)
{ sla_ctf2d_(hour, minute, second, days, jflag); }

void sla_dtf2r(int *hour, int *min, double *sec, double *radian, int *jflag)
{ sla_dtf2r_(hour, min, sec, radian, jflag); }

void sla_daf2r(int *deg, int *min, double *sec, double *radian, int *jflag) 
{ sla_daf2r_(deg, min, sec, radian, jflag); }

void sla_dr2af(int *ndp, double *angle, char *sign, int *idmsf, int nidmsf)
{ sla_dr2af_(ndp,angle,sign,idmsf,nidmsf); }

void sla_dr2tf(int *ndp, double *angle, char *sign, int *ihmsf, int nihmsf)
{ sla_dr2tf_(ndp,angle,sign,ihmsf,nihmsf); }

/* Angles, Vectors, and Matrices */
double sla_drange(double *angle)
{ return sla_drange_(angle); }

double sla_dranrm(double *angle)
{ return sla_drange_(angle); }

double sla_dsep(double *a1, double *b1, double *a2, double *b2)
{ return sla_dsep_(a1, b1, a2, b2); }

double sla_dbear(double *a1, double *b1, double *a2, double *b2)
{ return sla_dbear_(a1, b1, a2, b2); }

/* Calendars */
void sla_cldj(int *iy, int *im, int *id, double *djm, int *j)
{ sla_cldj_(iy, im, id, djm, j); }

void sla_caldj(int *iy, int *im, int *id, double *djm, int *j)
{ sla_caldj_(iy, im, id, djm, j); }

void sla_djcl(double *djm, int *iy, int *im, int *id, double *fd, int *j)
{ sla_djcl_(djm, iy, im, id, fd, j); }

void sla_calyd(int *iy, int *im, int *id, int *nd, int *j)
{ sla_calyd_(iy, im, id, nd, j); }

double sla_epb(double *date)
{ return sla_epb_(date); }

double sla_epb2d(double *epb)
{ return sla_epb2d_(epb); }

double sla_epj(double *date)
{ return sla_epj_(date); }

double sla_epj2d(double *epj)
{ return sla_epj2d_(epj); }

/* Timescales */
double sla_gmst(double *ut1)
{ return sla_gmst_(ut1); }

double sla_eqeqx(double *date)
{ return  sla_eqeqx_(date); }

double sla_dat(double *dju)
{ return sla_dat_(dju); }

double sla_dtt(double *dju)
{ return sla_dtt_(dju); }

/* The following routine causes a load error on sunOs:  it can't find
   ___d_mod
double sla_rcc(double *tdb, double *ut1, double *wl, double *u, double *v)
{ return sla_rcc_(tdb, ut1, wl, u, v); }
*/


/* Precession and Nutation */
void sla_nut(double *date, double *rmatn)
{ sla_nut_(date, rmatn); }

void sla_nutc(double *date, double *dpsi, double *deps, double *eps0)
{ sla_nutc_(date, dpsi, deps, eps0); }

void sla_prec(double *ep0, double *ep1, double *rmatp)
{ sla_prec_(ep0, ep1, rmatp); }

void sla_prenut(double *epoch, double *date, double *rmatpn)
{ sla_prenut_(epoch, date, rmatpn); }

void sla_prebn(double *bep0, double *bep1, double *rmatp)
{ sla_prebn_(bep0, bep1, rmatp); }

void sla_preces(char *sys, double *ep0, double *ep1, double *ra, double *dc)
{ sla_preces_(sys, ep0, ep1, ra, dc, 3); }

/* Proper Motion */
void sla_pm(double *r0, double *d0, double *pr, double *pd, double *px,
	    double *rv, double *ep0, double *ep1, double *r1, double *d1)
{ sla_pm_(r0, d0, pr, pd, px, rv, ep0, ep1, r1, d1); }

/* Observed Place */
double sla_zd(double* ha, double* dec, double* phi)
{ return sla_zd_(ha, dec, phi); }

/* Refraction and Air Mass */
double sla_airmas(double* zd)
{ return sla_airmas_(zd); }

/* Astrometry */
void sla_ds2tp(double* ra, double* dec, double* raz, double* decz, double* xi,
             double* eta, int* j)
{
   sla_ds2tp_(ra, dec, raz, decz, xi, eta,j);
}
void sla_fitxy(int *itype, int *np, double *xye, double *xym,
	       double *coeffs, int *jflag)
{ sla_fitxy_(itype, np, xye, xym, coeffs, jflag); }


void sla_invf(double *forwardCoeffs, double *backwardsCoeffs, int *jflag)
{ sla_invf_(forwardCoeffs, backwardsCoeffs, jflag); }


void sla_xy2xy(double *xe, double *ye, double *coeff, double *xm, double *ym)
{ sla_xy2xy_(xe, ye, coeff, xm, ym); }
