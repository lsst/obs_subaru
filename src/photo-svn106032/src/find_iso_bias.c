/*
 * This standalone programme is used to find the bias in the ellipses
 * that measure objects fits to isophotes, due to the finite angular
 * width of the sectors that photo uses to extrace radial profiles
 */
#include <stdio.h> 
#include "dervish.h"
#include "phExtract.h"
#include "phMathUtils.h"
#include "phMeasureObj.h"

static VEC *fit_ellipse(float a, float b, float phi);
static float get_area(float a, float b, float phi, float theta);
static void find_params(const float a, const float b, float *ac, float *bc);

static void
usage(void)
{
   char **ptr;
   char *msgs[] = {
      "Usage: find_iso_bias [options]",
      "Options:",
      "   -h, -?       Print this message",
      "   -a val       Use a major axis of val",
      "   -b val       Use a minor axis of val",
      "   -d           Debias estimated coefficients",
      "   -c           Print coefficients of elliptical fit",
      "   -C           Like -c, but also print estimated radii",
      "   -l           Fit Lorentzian to a and b as a fn of phi",
      "   -m           Take a and b to be measured, not true, values",
      "   -p val       Use a position angle val degrees clockwise of north",
      NULL
   };

   for(ptr = msgs;*ptr != NULL;ptr++) {
      fprintf(stderr,"%s\n",*ptr);
   }
}      

/*
 * variables related to parsing arguments
 */
static int debias = 0;			/* debias coefficients? */
static int estimate_lorentzian = 0;	/* fit Lorentzian as fn of phi? */
static int print_coeffs = 0;		/* print radii? */
static int use_true = 1;		/* use true, not measured, a and b */

int
main(int ac, char **av)
{
   float a = 1, b = 1, phi = 0;		/* parameters of ellipse */
/*
 * process arguments
 */
   while(ac > 1 && av[1][0] == '-') {
      switch(av[1][1]) {
       case '?': case 'h':		/* help */
	 usage();
	 return(0);
       case 'a': case 'b': case 'p':
	 if(ac == 2) {
	    fprintf(stderr,"Please provide a value with %s\n",av[1]);
	 } else {
	    char *ptr; float val;
	    
	    val = strtod(av[2],&ptr);
	    if(ptr == av[2]) {
	       fprintf(stderr,"I need a number with %s, not \"%s\"\n",
		       av[1], av[2]);
	    }
	    switch (av[1][1]) {
	     case 'a':
	       a = val; break;
	     case 'b':
	       b = val; break;
	     case 'p':
	       phi = val; break;
	     default:
	       fprintf(stderr,"Impossible case\n");
	       abort();
	    }
	    ac--; av++;
	 }
	 break;
       case 'c':			/* estimate and print coefficients */
	 print_coeffs = 1;
	 break;
       case 'C':			/* like 'c', but also print radii */
	 print_coeffs = 2;
	 break;
       case 'd':			/* debias coefficients */
	 debias = 1;
	 break;
       case 'l':			/* fit Lorentzian to a and b */
	 estimate_lorentzian = 1;
	 break;
       case 'm':			/* use measured, not true, a and b */
	 use_true = 0;
	 break;
       default:
	 fprintf(stderr,"Unknown option \"%s\"\n",av[1]);
	 break;
      }
      ac--; av++;
   }
/*
 * setup
 */
   p_phInitEllipseFit();
/*
 * work
 */
   if(print_coeffs) {
      printf("a = %g b = %g phi = %g ", a, b, phi);
      phVecDel(fit_ellipse(a, b, phi));
   }
   if(estimate_lorentzian) {
      float ac[3], bc[3];
      find_params(a, b, ac, bc);

      printf("a: %g %g %g  ", ac[0], ac[1], ac[2]);
      printf("b: %g %g %g\n", bc[0], bc[1], bc[2]);
   }
   
   p_phFiniEllipseFit();

   return(0);
}

/*****************************************************************************/
/*
 * fit a set of coefficients to an ellipse sampled as if by phProfileExtract()
 */
static VEC *
fit_ellipse(float a,
	    float b,
	    float phi)
{
   VEC *coeffs;				/* fitted coefficients */
   const float dtheta = M_PI*2/NSEC;	/* angular width of sector */
   int i;
   float r[NSEC];			/* radii to fit */
   float rt[NSEC];			/* true radii */

   phi = (phi - 90)*M_PI/180;		/* measured clockwise from x axis */
   for(i = 0; i < NSEC; i++) {
      r[i] = sqrt(2/dtheta*(get_area(a, b, phi, i*dtheta + dtheta/2) -
				     get_area(a, b, phi, i*dtheta - dtheta/2)));
      rt[i] = a*sqrt((1 + pow(tan(i*dtheta - phi),2))/
		     (1 + pow(a/b*tan(i*dtheta - phi),2)));
   }
   coeffs = p_phEllipseFit(r, NULL, debias);

   if(print_coeffs) {
      printf("rowc = %g colc = %g a = %g b = %g phi = %g\n", coeffs->ve[0],
	     coeffs->ve[1], coeffs->ve[2], coeffs->ve[3], coeffs->ve[4]);
      if(print_coeffs > 1) {
	 for(i = 0; i < NSEC; i++) {
	    printf("%g %g %g\n", i*dtheta, rt[i], r[i]);
	 }
      }
   }

   return(coeffs);
}

/*****************************************************************************/
/*
 * return the area of a sector of an ellipse, measured clockwise from the x axis
 */
static float
get_area(float a, float b,		/* major- and minor-axes */
	 float phi,			/* p.a. of major axis, +ve from x */
	 float theta)			/* desired angle */
{
   float area;				/* desired area */

   theta -= phi;

   area = 0.5*a*b*atan2(a*sin(theta), b*cos(theta));
   if(theta <= -M_PI) {
      area -= a*b*M_PI;
   } else if(theta >= M_PI) {
      area += a*b*M_PI;
   }

   return(area);
}

/*****************************************************************************/
#define NPHI 30				/* number of position angles */
static float p[NPHI];			/* desired angles */
static float *data = NULL;		/* measured values of data to fit */

static void fit_lorentzian(float ab, float *cm, float *cc);
static double find_aratio(double arat);
static double g_aratio;			/* global copy of desired aratio */
static double g_phi;

static void
find_params(const float a0,		/* desired major axis of ellipse */
	    const float b0,		/* desired minor axis of ellipse */
	    float *ac,			/* fitted coeffs for a */
	    float *bc)			/* fitted coeffs for b */
{
   float a, b;				/* current values of a, b */
   double aratio;			/* value of a/b */
   float am[NPHI], bm[NPHI], phim[NPHI]; /* measured a, b, phi */
   VEC *coeffs;				/* fitted coefficients */
   int i;

   for(i = 0;i < NPHI;i++) {
      p[i] = i*30.0/NPHI;
      a = a0; b = b0;

      if(!use_true) {
	 g_aratio = a/b; g_phi = p[i];
      	 if(phZeroFind(1, 1e4, find_aratio, 1e-5, &aratio) != 0) {
	    shError("Cannot find axis ratio resulting in a/b == %g", a/b);
	 }
	 coeffs = fit_ellipse(b*aratio, b, p[i]);
	 b /= coeffs->ve[3];
	 a = b*aratio;

	 phVecDel(coeffs);
      }

      coeffs = fit_ellipse(a, b, p[i]);
      am[i] = a/coeffs->ve[2];
      bm[i] = b/coeffs->ve[3];
      phim[i] = coeffs->ve[4] - p[i];
      phVecDel(coeffs);
   }

   fit_lorentzian(aratio, am, ac);
   fit_lorentzian(aratio, bm, bc);
}

/*
 * Function used by find_params to find the true axis ratio that leads
 * to the specified value, g_aratio
 */
static double
find_aratio(double arat)
{
   VEC *coeffs = fit_ellipse(arat, 1, g_phi);
   double aratio = coeffs->ve[2]/coeffs->ve[3];
   
   phVecDel(coeffs);

   return(aratio - g_aratio);
}

/*****************************************************************************/
/*
 * fit a Lorentzian curve to some data in the globals p[] and data[]
 */
static double lorentz_c0, lorentz_c1, lorentz_c2; /* values of cn if fixed */

static void
fit_lorentzian_func(int ndata,
		    int nparam,
		    double *param,
		    double *fvec,
		    int *iflag)
{
   double c0, c1, c2;
   int i;

   shAssert(*iflag >= 0 && nparam >= 1);

   c0 = param[0];

   if(nparam <= 1) {
      c1 = lorentz_c1;
   } else {
      c1 = param[1];
   }

   if(nparam <= 2) {
      c2 = lorentz_c2;
   } else {
      c2 = param[2];
   }

   for(i = 0; i < ndata; i++) {
      fvec[i] = data[i] - (c0 + c1/(1 + pow((p[i] - 15)/c2,2)));
   }
}

static void
fit_lorentzian(float ab,		/* a/b */
	       float *cm,		/* measured values of c */
	       float *cc)		/* desired coefficients */
{
   int info;				/* return code from LM minimiser */
   int nfev = 0;			/* number of function evaluations */
   double norm = 0;			/* p[]'s norm */
   int npar;				/* number of parameters to fit */
   double param[3];			/* fitted parameters */
   double tol = 1e-6;			/* desired tolerance on parameters */
   double sumsq_tol = 0;		/* tolerance in sum of squares */

   data = cm;				/* transfer to global */

   lorentz_c0 = 1;
   lorentz_c1 = 0;
#if 0
   npar = 3;
   lorentz_c2 = 1;
#else
   npar = 2;
   lorentz_c2 = 15*pow(1 + ab/4.5, -0.6);
   if(ab > 0.4) {
      lorentz_c2 = 11;
   } else {
      lorentz_c2 = -2.578 + ab*(106.662 + ab*(-279.419 + ab*244.074));
   }
#endif
   param[0] = lorentz_c0;
   param[1] = lorentz_c1;
   param[2] = lorentz_c2;

   shAssert(npar <= sizeof(param)/sizeof(param[0]));

   info = phLMMinimise(fit_lorentzian_func, NPHI, npar, param, &norm,
		       &nfev, tol, sumsq_tol, 0, NULL);
   
   cc[0] = param[0]; cc[1] = param[1]; cc[2] = param[2];

   if(info < 0) {                      /* user exit */
      shFatal("fit_lorentzian: user exit");
   }
   if(info == 0) {                     /* improper input params */ 
      shError("fit_lorentzian: improper inputs to LM Minimiser");
      return;
   } else if(info == 12) {
      ;					/* fvec is orthogonal to Jacobian */
   } else if(info & ~07) {
      shError("fit_lorentzian: LM Minimiser returns %d",info);
      return;
   }
}

