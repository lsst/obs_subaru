#ifndef ATSLALIB_C_H
#define ATSLALIB_C_H

#ifdef __cplusplus
extern "C" {
#endif

/* HTML
<HTML>
<HEAD>
<TITLE>SDSSMATH SLALIB C Bindings</TITLE>
</HEAD>

<BODY>

<H1>SLALIB C Bindings</H1>

<MENU>
<LI><A HREF=#stringDecoding>String Decoding</A>
<LI><A HREF=#angles>Angles, Vectors, and Rotation Matrices</a>
<LI><A HREF=#calendars>Calendars</a>
<LI><A HREF=#timescales>Timescales</a>
<LI><A HREF=#precession>Precession and Nutation</a>
<LI><A HREF=#properMotion>Proper Motion</a>
<LI><A HREF=#observedPlace>Observed Place</a>
<LI><A HREF=#refraction>Refraction and Air Mass</a>
<LI><A HREF=#astrometry>Astrometry</a>
</MENU>
*/

/* HTML 
<H2><A NAME=stringDecoding>String Decoding</A></H2>
*/
/* <PRE> */
void sla_intin( char *string, int *nstrt, int *result, int *jflag); 
void sla_flotin( char *string, int *nstrt, float *result, int *jflag);
void sla_dfltin( char *string, int *nstrt, double *result, int *jflag);
/* </PRE> */

/* HTML
<H2><A NAME=sexagesimalConversions>Sexagesimal Conversions</A></H2>
*/
/* <PRE> */
void sla_ctf2d(int *hour, int *min, float *sec, float *days, int *jflag); 
void sla_dtf2r(int *hour, int *min, double *sec, double *radian, int *jflag);
void sla_daf2r(int *deg, int *min, double *sec, double *radian, int *jflag);
void sla_dr2af(int *ndp, double *angle, char *sign, int *idmsf, int nidmsf);
void sla_dr2tf(int *ndp, double *angle, char *sign, int *ihmsf, int nihmsf);
/* </PRE> */

/* HTML 
<H2><A NAME=angles>Angles, Vectors, and Rotation Matrices</A></H2>
*/
/* <PRE> */
double sla_drange(double *angle);
double sla_dranrm(double *angle);
double sla_dsep(double *a1, double *b1, double *a2, double *b2);
double sla_dbear(double *a1, double *b1, double *a2, double *b2);
/* </PRE> */

/* HTML 
<H2><A NAME=calendars>Calendars</a></H2>
*/
/* <PRE> */
void sla_cldj(int *iy, int *im, int *id, double *djm, int *j);
void sla_caldj(int *iy, int *im, int *id, double *djm, int *j);
void sla_djcl(double *djm, int *iy, int *im, int *id, double *fd, int *j);
void sla_calyd(int *iy, int *im, int *id, int *nd, int *j);
double sla_epb(double *date);
double sla_epb2d(double *epb);
double sla_epj(double *date);
double sla_epj2d(double *epj);
/* /<PRE> */

/* HTML 
<H2><A NAME=timescales>Timescales</a></H2>
*/
/* <PRE> */
double sla_gmst(double *ut1);
double sla_eqeqx(double *date);
double sla_dat(double *dju);
double sla_dtt(double *dju);
double sla_rcc(double *tdb, double *ut1, double *wl, double *u, double *v);
/* /<PRE> */

/* HTML 
<H2><A NAME=precession>Precession and Nutation</a></H2>
*/
/* <PRE> */
void sla_nut(double *date, double *rmatn);
void sla_nutc(double *date, double *dpsi, double *deps, double *eps0);
void sla_prec(double *ep0, double *ep1, double *rmatp);
void sla_prenut(double *epoch, double *date, double *rmatpn);
void sla_prebn(double *bep0, double *bep1, double *rmatp);
void sla_preces(char *sys, double *ep0, double *ep1, 
		double *ra, double *dc);
/* </PRE> */

/* HTML 
<H2><A NAME=properMotion>Proper Motion</A></H2>
*/
/* <PRE> */
void sla_pm(double *r0, double *d0, double *pr, double *pd, double *px,
	    double *rv, double *ep0, double *ep1, double *r1, double *d1);
/* </PRE> */

/* HTML 
<H2><A NAME=observedPlace>Observed Place</a></H2>
*/
/* <PRE> */
double sla_zd(double *ha, double *dec, double *phi);
/* </PRE> */

/* HTML 
<H2><A NAME=refraction>Refraction and Air Mass</a></H2>
*/
/* <PRE> */
double sla_airmas(double *zd);
/* </PRE> */

/* HTML 
<H2><A NAME=astrometry>Astrometry</a></H2>
*/
/* <PRE> */
void sla_ds2tp(double* ra, double* dec, double* raz, double* decz, double* xi,
	       double* eta, int* j);
void slaDtp2s ( double xi, double eta, double raz, double decz,
                double *ra, double *dec );
void sla_fitxy(int *itype, int *np, double *xye, double *xym,
	       double *coeffs, int *jflag);
void sla_invf( double *fCoeffs, double *bCoeffs, int *jflag);
void sla_xy2xy(double *xe, double *ye, double *coeff, double *xm, double *ym);
/* </PRE> */

/* HTML
</BODY>
</HTML>
*/

#ifdef __cplusplus
}
#endif

#endif
