/*
 * Calculate coefficients for aperture photometry in a band-limited image
 *
 * This program calculates the coefficients for a single cell, delimited by
 * two radii and two angles. If you want to calculate a complete set of
 * cells, two shell scripts are given at the end of this source file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alloca.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "phMathUtils.h"

#if !defined(M_PI)
#  define M_PI 3.14159265358979323846
#  define M_SQRT2 1.41421356237309504880
#endif

void *shMalloc(int);
void shFree(void *);

double erf(double);			/* usually in libm.a, but not in
					   <math.h> when in strict ansi mode */

void
apodize_coeffs(float asigma,		/* apodize with N(0,asigma^2) filter */
	       float dx, float dy,	/* centre of aperture is (dx, dy) */
	       float **coeffs,		/* the coefficients */
	       int ncoeff);		/* dimen of coeffs is [ncoeff][ncoeff]*/

void
calc_coeffs(float r1, float r2,		/*            from r1 to r2 in r */
	    float t1, float t2,		/*             and t1 to t2 in theta */
	    float half_bell,		/* half-width of bell for sinc filter*/
	    int deconvolve,		/* deconvolve pixel response? */
	    int stokes,			/* calculate stokes coeffs? */
	    float **coeffs,		/* desired coefficients */
	    int nc);			/* coeffs[-nc..nc][-nc..nc] */
static int
find_coeffs(int i, int j,		/* For the pixel at (i,j), integrate */
	    float r1, float r2,		/* the sinc coeffs from r1 to r2 in r*/
	    float t1, float t2,		/*             and t1 to t2 in theta */
	    float half_bell,		/* half-width of cosine bell */
	    int decon,			/* deconvolve pixel repsonse? */
	    int stokes,			/* calculate U, I, or Q? */
	    float *ans);		/* desired answer */

static void
write_file(char *file,
	   float *s_coeffs,
	   int ncoeff,
	   float r1, float r2,
	   float t1, float t2);

/*****************************************************************************/

static float eps = 1e-6;		/* desired accuracy */
static int verbose = 0;			/* should I be chatty? */

/*****************************************************************************/

static void
usage(void)
{
   char **ptr;
   char *msgs[] = {
      "Usage: make_photom_coeffs [options] r1 r2 theta1 theta2",
      "Integrate from r1--r2, and theta1--theta2 (in degrees)",
      "(If r1 or r2 is -ve, it'll taken to be -(area of equivalent circle)",
      "so -1 will be converted to approximately 0.5641)",
      "Options:",
      "   -h, -?       Print this message",
      "   -a sigma     Apodize coeffs with an N(0,sigma^2) Gaussian",
      "   -a sigma,dx,dy Shift centre to (dx, dy) for apodized filter",
      "   -A file      Produce imagefile file of apodised coeffs for SM "
	"(file_type coeffs).",
      "   -b #         Half-width of bell for sinc-coefficients (default: 10)",
      "   -c           Print the coefficients (default)",
      "   -C           Produce output that can be cut-and-pasted into C",
      "   -d           Deconvolve pixellation (default: 0)",
      "   -e #         Calculate to an accuracy # (default: 1e-6)",
      "   -g sigma     Evaluate sum of coeffs*image for an N(0,sigma^2)",
      "                object to see how well we do (sigma == 0 => return sum)",
      "   -i           Use geometric intersection of pixels, not a sinc",
      "   -L           Print results as a LaTeX table",
      "   -n #         Calculate coeffs in region of size 2*# + 1",
      "                centred at (0,0) (default: 10)",
      "   -r           Print coefficients in reverse order",
      "   -Q           Calculate coefficients for Stokes' Q coefficients",
      "   -S str       Produce imagefile str for SM (file_type coeffs).",
      "                If the str looks like \"<file\", read coeffs from file",
      "   -U           Calculate coefficients for Stokes' U coefficients",
      "   -v           Be verbose",
      NULL
   };

   for(ptr = msgs;*ptr != NULL;ptr++) {
      fprintf(stderr,"%s\n",*ptr);
   }
}      

int
main(int ac, char **av)
{
   char *afile = NULL;			/* output file for apodised coeffs */
   float asigma = 0;			/* sigma for apodization */
   float **coeffs, *s_coeffs;		/* desired coefficients */
   int deconvolve = 0;			/* deconvolve pixellation? */
   float dx = 0.0, dy = 0.0;		/* shift coefficients by (dx, dy)
					   when apodizing */
   float half_bell = 10;		/* half-width of cosbell */
   int i,j;
   int LaTeX = 0;			/* print results as a LaTeX table? */
   int ncoeff = 10;			/* max i,j in coeffs array */
   int print_coeffs = 0;		/* print the coefficients? */
   int print_as_C = 0;			/* output be cut-and-pasted into C? */
   char *ptr;				/* pointer to av[2] */
   float r1 = -1, r2;			/* inner and outer radii */
   float t1,t2;				/* lower and higher angular boundary */
   float sigma = -1;			/* sigma for Gaussian PSF */
   char *file = NULL;			/* name of file to hold an SM image */
   int reverse = 0;			/* print coeffs in reverse order? */
   int stokes = 0;			/* calculate Stokes coeffs Q or U? */
/*
 * process arguments
 */
   while(ac > 1 && av[1][0] == '-' && !isdigit(av[1][1])) {
      switch(av[1][1]) {
       case '?': case 'h':		/* help */
	 usage();
	 return(0);
       case 'a':			/* apodize coeffs with N(0,#^2) filter*/
	 if(ac == 2) {
	    fprintf(stderr,"You must specify a value with -a\n");
	    exit(1);
	 }

	 if((asigma = atof(av[2])) <= 0) {
	    fprintf(stderr,"The value specified with -a must be > 0\n");
	    exit(1);
	 }

	 ptr = av[2];
	 if(ptr != NULL && (ptr = strchr(ptr, ',')) != NULL) {
	    ptr++; dx = atof(ptr);
	 }
	 if(ptr != NULL && (ptr = strchr(ptr, ',')) != NULL) {
	    ptr++; dy = atof(ptr);
	 }
	 
	 ac--; av++;
	 break;
       case 'A':			/* generate an SM image file
					   of apodized coeffs */
	 if(ac == 2) {
	    fprintf(stderr,"You must specify a filename with -A\n");
	    exit(1);
	 }
	 afile = av[2];
	 ac--; av++;
	 break;
       case 'b':			/* set half-width of cosine bell */
	 if(ac == 2) {
	    fprintf(stderr,"You must specify a value with -b\n");
	    exit(1);
	 }
	 if((half_bell = atof(av[2])) <= 0) {
	    fprintf(stderr,"The value specified with -b must be > 0\n");
	    exit(1);
	 }
	 
	 ac--; av++;
	 break;
       case 'c':			/* print coefficients */
	 print_coeffs = 1;
	 break;
       case 'd':			/* deconvolve pixellation */
	 deconvolve = 1;
	 break;
       case 'C':			/* output is in format of legal C */
	 print_as_C = 1;
	 break;
       case 'e':			/* specify accuracy */
	 if(ac == 2) {
	    fprintf(stderr,"You must specify a value with -e\n");
	    exit(1);
	 }
	 if((eps = atof(av[2])) <= 0) {
	    fprintf(stderr,"The value specified with -e must be > 0\n");
	    exit(1);
	 }
	 
	 ac--; av++;
	 break;
       case 'g':			/* sum coeffs for an N(0,#^2) PSF */
	 if(ac == 2) {
	    fprintf(stderr,"You must specify a value with -g\n");
	    exit(1);
	 }
	 if((sigma = atof(av[2])) < 0) {
	    fprintf(stderr,"The value specified with -g must be > 0\n");
	    exit(1);
	 }
	 
	 ac--; av++;
	 break;
       case 'i':			/* use geometric intersection not a
					   sinc interpolation */
	 fprintf(stderr,"-i is not currently implemented\n");
	 break;
       case 'L':			/* Print results as a LaTeX table */
	 LaTeX = 1;
	 break;
       case 'n':			/* 1st quadrant is ncoeff*ncoeff
					   (excluding 0,0) */
	 if(ac == 2) {
	    fprintf(stderr,"You must specify a value with -n\n");
	    exit(1);
	 }
	 if((ncoeff = atoi(av[2])) <= 0) {
	    fprintf(stderr,"The value specified with -n must be > 0\n");
	    exit(1);
	 }
	 
	 ac--; av++;
	 break;
       case 'Q':			/* calculate Q */
	 stokes = 1;
	 break;
       case 'r':			/* print coeffs in reverse order */
	 reverse = 1;
	 break;
       case 'S':			/* generate an SM image file */
	 if(ac == 2) {
	    fprintf(stderr,"You must specify a filename with -S\n");
	    exit(1);
	 }
	 file = av[2];
	 ac--; av++;
	 break;
       case 'U':			/* calculate U */
	 stokes = -1;
	 break;
       case 'v':			/* be chatty */
	 verbose = 1;
	 break;
       default:
	 fprintf(stderr,"Unknown option \"%s\"\n",av[1]);
	 break;
      }
      ac--; av++;
   }
/*
 * check inputs
 */
   if(!print_coeffs && !print_as_C && (file == NULL || *file == '<') &&
						  afile == NULL && sigma < 0) {
      print_coeffs = 1;
   }
   if(ac == 5) {
      r1 = atof(av[1]); r2 = atof(av[2]);
      if(r1 < 0) {
	 r1 = sqrt(-r1/M_PI);
      }
      if(r2 < 0) {
	 r2 = sqrt(-r2/M_PI);
      }
      t1 = atof(av[3])*M_PI/180; t2 = atof(av[4])*M_PI/180;
   } else {
      if(file == NULL || file[0] != '<') { /* read file */
	 fprintf(stderr,
		 "You must specify two radii and two angles (in degrees)\n");
	 usage();
	 exit(1);
      }
   }
/*
 * Allocate and evaluate or read coefficients
 */
   coeffs = (float **)shMalloc((2*ncoeff + 1)*sizeof(float *)) + ncoeff;
   s_coeffs = shMalloc((2*ncoeff + 1)*(2*ncoeff + 1)*sizeof(float));
   for(i = -ncoeff;i <= ncoeff;i++) {
      coeffs[i] = s_coeffs + (ncoeff + i)*(2*ncoeff + 1) + ncoeff;
   }
   if(file == NULL || file[0] != '<') {
      calc_coeffs(r1,r2,t1,t2,half_bell,deconvolve,stokes,coeffs,ncoeff);
   } else {
      FILE *fil;
      int n;
      
      if((fil = fopen(&file[1],"r")) == NULL) {
	 fprintf(stderr,"Cannot open %s ",&file[1]);
	 perror("");
	 exit(1);
      }
      fread(&n,sizeof(int),1,fil);
      if(n != 2*ncoeff + 1) {
	 fprintf(stderr,"File has ncoeff == %d not %d\n",(n - 1)/2,ncoeff);
	 exit(1);
      }
      fread(&n,sizeof(int),1,fil);
      fread(&r1,sizeof(float),1,fil);
      fread(&r2,sizeof(float),1,fil);
      fread(&t1,sizeof(float),1,fil);
      fread(&t2,sizeof(float),1,fil);
      fread(s_coeffs,sizeof(float),n*n,fil);
      fclose(fil);

      file = NULL;			/* don't overwrite */
   }
/*
 * apodize coefficients if so desired
 */
   if(asigma > 0) {
      apodize_coeffs(asigma,dx,dy,coeffs,ncoeff);
   } else {
      assert(dx == 0.0 && dy == 0.0);
   }
/*
 * Time for output
 */
   if(print_coeffs) {
      const int i0 = reverse ? 0 : ncoeff;
      const int i1 = reverse ? ncoeff + 1 : -1;
      const int di = reverse ? 1 : -1;
      if(LaTeX) {
	 printf("\\begin{table}\n");
	 printf("\\begin{tabular}{c|*{%d}{r}}%%\n", 1 + (ncoeff + 1));
	 for(i = i0; i != i1; i += di) {
	     printf("%-3d & ",i);
	     for(j = 0;j <= ncoeff;j++) {
		 printf("%6.3f %s",coeffs[i][j], (j == ncoeff ? "\\\\" : "&"));
	     }
	     printf("\n");
	 }
	 printf("\\hline\n");
	 printf("   & ");
	 for(i = 0;i <= ncoeff;i++) {
	     printf("%4d   %s", i, (i == ncoeff ? "\\\\" : "&"));
	 }
	 printf("\n");
	 printf("\n");
	 printf("\\end{tabular}\n");
	 printf("\\caption[]{}\n");
	 printf("\\label{XXX}\n");
	 printf("\\end{table}\n");
      } else {
	 for(i = i0; i != i1; i += di) {
	     printf("%-3d ",i);
	     for(j = 0;j <= ncoeff;j++) {
		printf("%6.3f ",coeffs[i][j]);
	     }
	     printf("\n");
	 }
	 printf("    ");
	 for(i = 0;i <= ncoeff;i++) {
	    printf("%4d   ",i);
	 }
	 printf("\n");
      }
   }
   if(file != NULL) {
      write_file(file, s_coeffs, ncoeff, r1, r2, t1, t2);
   }
   if(afile != NULL) {
      write_file(afile, s_coeffs, ncoeff, r1, r2, t1, t2);
   }

   if(print_as_C) {
      int *start = shMalloc(2*(2*ncoeff + 1)*sizeof(int));
      int *end = start + (2*ncoeff + 1);
      double fac;			/* multiply output coeffs by fac */
      int nmax;				/* maximum number of coeffs */
      int imin, imax;			/* min and max values of y */
      int jmin, jmax;			/* min and max values of x */
      int nrun;				/* num of runs with non-zero coeffs */
      float tol = 1e-3;			/* minimum coeff to consider */
      double true_sum;			/* true value of sum of coeffs */
      double coeff_sum;			/* sum of output coefficients */
      double acoeff_sum;		/* sum of |output coefficients| */

      start += ncoeff; end += ncoeff;

      nmax = 0;
      jmin = imin = 10000; jmax = imax = -10000;
      coeff_sum = acoeff_sum = 0;
      for(i = -ncoeff;i <= ncoeff;i++) {
	 start[i] = -10000;
	 end[i] = start[i] - 1;
	 for(j = -ncoeff;j <= ncoeff;j++) {
	    if(fabs(coeffs[i][j]) < tol) {
	       coeffs[i][j] = 0;
	    } else {
	       coeff_sum += coeffs[i][j];
	       if(coeffs[i][j] > 0) {
		  acoeff_sum += coeffs[i][j];
	       } else {
		  acoeff_sum -= coeffs[i][j];
	       }

	       if(start[i] == -10000) {
		  if(i > imax) imax = i;
		  if(i < imin) imin = i;
		  if(j < jmin) jmin = j;
		  start[i] = j;
	       }
	       end[i] = j;
	    }
	 }

	 if(end[i] > jmax) jmax = end[i];
	 if(end[i] - start[i] + 1 > nmax) {
	    nmax = end[i] - start[i] + 1;
	 }
      }

      nrun = imax - imin + 1;
/*
 * Preserve normalisation of the coefficients. As the sum of the
 * Stokes parameter coeffs can be zero we have to be careful, and add
 * a multiple of the absolute value of the coefficients
 */
      switch (stokes) {
       case -1:				/* U */
	 true_sum = 0.25*(r2*r2 - r1*r1)*(sin(2*t1) - sin(2*t2));
	 break;
       case  0:				/* I */
	 true_sum = 0.5*(r2*r2 - r1*r1)*(t2 - t1);
	 break;
       case  1:				/* Q */
	 true_sum = 0.25*(r2*r2 - r1*r1)*(cos(2*t1) - cos(2*t2));
	 break;
       default:
	 fprintf(stderr,"Illegal value of stokes: %d",stokes); abort();
      }
      if(asigma > 0) {
	 fac = 0.0;			/* don't adjust coeffs */
      } else {
	 fac = (true_sum - coeff_sum)/acoeff_sum;
      }
/*
 * Start output of code; first a struct definition to stderr
 */
      fprintf(stderr,
	  "#define NCOEFF %d\t\t\t/* Max number of coeffs in a row */\n",nmax);
      fprintf(stderr,"#define NMAX %d\t\t\t\t/* Max number of rows */\n",nrun);
      fprintf(stderr,"\n");
      fprintf(stderr,"struct run_T {\n");
      fprintf(stderr,"   int col0;\t\t\t\t/* starting column */\n");
      fprintf(stderr,"   int n;\t\t\t\t/* number of coefficients */\n");
      fprintf(stderr,"   float coeffs[NCOEFF];\t\t/* coefficients */\n");
      fprintf(stderr,"};\t\t\t\t\t/* pragma IGNORE */\n");
      fprintf(stderr,"\n");
      fprintf(stderr,"typedef struct {\t\t\t/* Coefficients for photometry */\n");
      fprintf(stderr,"   float sum;\t\t\t\t/* Sum of coefficients */\n");
      fprintf(stderr,
	  "   int cmin, cmax;\t\t\t/* Min and max cols for needed pixels*/\n");
      fprintf(stderr,
	  "   int rmin, rmax;\t\t\t/* Min and max rows for needed pixels*/\n");
      fprintf(stderr,"   struct run_T run[NMAX];\n");
      fprintf(stderr,"} COEFF;\t\t\t\t/* pragma IGNORE */\n");
      fprintf(stderr,"\n");
/*
 * and now the real coefficients
 */
      printf("   {\t\t\t\t\t");
      if(r1 >= 0) {
	 printf("/* r=%g--%g, theta=%g--%g asigma=%g */",
					 r1,r2,t1*180/M_PI,t2*180/M_PI,asigma);
      }
      printf("\n");
      printf("      %.7f,\n", true_sum);
      printf("      %d, %d, %d, %d,\n", jmin, jmax, imin, imax);
      printf("      {\n");
      for(i = -ncoeff;i <= ncoeff;i++) {
	 if(end[i] < start[i]) {
	    continue;
	 }
	 
	 printf("         { %3d,  %3d,\t\t/* col0, ncoeff */\n",
					       start[i],end[i] - start[i] + 1);
	 printf("            {\n");
	 printf("               ");
	 for(j = start[i];j <= end[i];j++) {
	    coeffs[i][j] += fac*fabs(coeffs[i][j]);
	    printf(" %10.7f,",coeffs[i][j]);
	    if((j - start[i])%5 == 4 && j != end[i]) {
	       printf("\n");
	       printf("               ");
	    }
	 }
	 printf("\n");
	 printf("            },\n");
	 printf("         },\n");
      }

      printf("      },\n");
      printf("   },\n");

      shFree(start - ncoeff);
   }

   if(sigma >= 0) {
      float sum = 0;
      const float sig_sq2 = sigma*M_SQRT2;
      float val;
      
      for(i = -ncoeff;i <= ncoeff;i++) {
	 for(j = -ncoeff;j <= ncoeff;j++) {
	    if(coeffs[j][i] != 0.0f) {
	       if(sigma == 0) {
		  val = 1.0;
	       } else {
		  if(deconvolve) {
		     val =  erf((i + 0.5)/sig_sq2) - erf((i - 0.5)/sig_sq2);
		     val *= erf((j + 0.5)/sig_sq2) - erf((j - 0.5)/sig_sq2);
		     val *= M_PI/2;
		  } else {
		     val = exp(-pow(j/sig_sq2,2) - pow(i/sig_sq2,2))/pow(sigma,2);
		  }
	       }
	       sum += val*coeffs[j][i];
	    }
	 }
      }
      
      if(sigma == 0) {
	  val = 0.5*(t2 - t1)*(pow(r2,2) - pow(r1,2));
      } else {
	 sum *= sigma*sigma;
	 val = sigma*sigma*(t2 - t1)*
	     (exp(-pow(r1/sigma,2)/2) - exp(-pow(r2/sigma,2)/2));
      }
      fprintf(stderr,"Aperture sum: %g (== %g*(expected == %g))\n",
							      sum,sum/val,val);
   }
/*
 * clean up
 */
   shFree(coeffs - ncoeff);
   shFree(s_coeffs);

   return(0);
}

/*****************************************************************************/
/*
 * Write coeffs out to a file
 */
static void
write_file(char *file,
	   float *s_coeffs,
	   int ncoeff,
	   float r1, float r2,
	   float t1, float t2)
{
   FILE *fil;
   int n = 2*ncoeff + 1;
      
   if((fil = fopen(file,"w")) == NULL) {
      fprintf(stderr,"Cannot open %s ",file);
      perror("");
      exit(1);
   }
   fwrite(&n,sizeof(int),1,fil);
   fwrite(&n,sizeof(int),1,fil);
   fwrite(&r1,sizeof(float),1,fil);
   fwrite(&r2,sizeof(float),1,fil);
   fwrite(&t1,sizeof(float),1,fil);
   fwrite(&t2,sizeof(float),1,fil);
   fwrite(s_coeffs,sizeof(float),n*n,fil);
   fclose(fil);
}

/*****************************************************************************/
/*
 * calculate a normalised Gaussian, N(dx, sigma^2)
 */
static void
set_filter(float *f,			/* filter to set */
	   int nf,			/* filter is f[-nf/2] ... f[nf/2] */
	   float sigma,			/* desired sigma */
	   float dx)			/* centre of Gaussian */
{
   int i;
   float sum = 0;
/*
 * calculate filter...
 */
   for(i = -nf/2; i <= nf/2; i++) {
      f[i] = exp(-0.5*pow((i + dx)/sigma, 2));
      sum += f[i];
   }
/*
 * ... and normalise
 */
   for(i = -nf/2; i <= nf/2; i++) {
      f[i] /= sum;
   }
}

/*****************************************************************************/
/*
 * Given a set of coefficients, apodize them with a Gaussian, and shift the
 * centre of the aperture to (dx, dy)
 */
void
apodize_coeffs(float asigma,		/* apodize with N(0,asigma^2) filter */
	       float dx, float dy,	/* centre of aperture is (dx, dy) */
	       float **coeffs,		/* the coefficients */
	       int ncoeff)		/* dimen of coeffs is [ncoeff][ncoeff]*/
{
   float **c, *c_s;			/* augmented array */
   float *f;				/* smoothing filter */
   int i,j,k;
   float max;				/* largest coefficient */
   int n;				/* size of augmented array */
   int nf;				/* size of smoothing filter */
   float **scr, *scr_s;			/* scratch array */
   double sum;				/* sum for convolution */

   nf = 1 + 2*(int)(3*asigma + 0.5 + fabs(fabs(dx) > fabs(dy) ? dx : dy));
   n = 2*ncoeff + 2*nf + 1;
   assert(n%2 == 1 && nf%2 == 1);
/*
 * set up augmented array; note that origin of coeffs is c[0][0]
 */
   f = alloca(nf*sizeof(float)); f += nf/2;

   c = alloca(2*n*sizeof(float *)); scr = c + n;
   c_s = alloca(2*n*n*sizeof(float)); scr_s = c_s + n*n;
   memset(c_s, '\0', 2*n*n*sizeof(float));
   assert(c_s[0] == 0.0);		/* check that 0.0 is all bits 0 */
   for(i = 0;i < n; i++) {
      c[i] = c_s + i*n + n/2;
      scr[i] = scr_s + i*n + n/2;
   }
   c += n/2; scr += n/2;
/*
 * fill out c
 */
   for(i = 0; i < ncoeff; i++) {
      for(j = 0; j < ncoeff; j++) {
	 c[i][j] = c[-i][j] = c[i][-j] = c[-i][-j] = coeffs[i][j];
      }
   }
/*
 * set x-filter and convolve with it
 */
   set_filter(f, nf, asigma, dx);

   for(i = -ncoeff - nf/2 + 1; i < ncoeff + nf/2; i++) {
      for(j = -ncoeff - nf/2 + 1; j < ncoeff + nf/2; j++) {
	 sum = 0;
	 for(k = -nf/2; k <= nf/2; k++) {
	    sum += c[i][j + k]*f[k];
	 }
	 scr[i][j] = sum;
      }
   }
/*
 * set y-filter and convolve with _it_
 */
   set_filter(f, nf, asigma, dy);

   for(i = -ncoeff + 1; i < ncoeff; i++) {
      for(j = -ncoeff + 1; j < ncoeff; j++) {
	 sum = 0;
	 for(k = -nf/2; k <= nf/2; k++) {
	    sum += scr[i + k][j]*f[k];
	 }
	 c[i][j] = sum;
      }
   }
/*
 * Find the largest coefficient
 */
   max = 0;
   for(i = -ncoeff + 1; i < ncoeff; i++) {
      for(j = -ncoeff + 1; j < ncoeff; j++) {
	 if(c[i][j] > max) {
	    max = c[i][j];
	 }
      }
   }
/*
 * pack that back into coeffs, setting the maximum value to 1.0
 */
   for(i = -ncoeff + 1; i < ncoeff; i++) {
      for(j = -ncoeff + 1; j < ncoeff; j++) {
	 coeffs[i][j] = c[i][j]/max;
      }
   }
}

/*****************************************************************************/
/*
 * Calculate a whole set of coefficients. We can either calculate the simple
 * integral over the cell (stokes == 0), Q (stokes == 1), or U (stokes == -1)
 */
void
calc_coeffs(float r1, float r2,		/*            from r1 to r2 in r */
	    float t1, float t2,		/*             and t1 to t2 in theta */
	    float half_bell,		/* half-width of bell for sinc filter*/
	    int deconvolve,		/* deconvolve pixel response? */
	    int stokes,			/* calculate stokes coeffs? */
	    float **coeffs,		/* desired coefficients */
	    int nc)			/* coeffs[-nc..nc][-nc..nc] */
{
   int i,j;
   float sum = 0.0;
   
   for(i = -nc;i <= nc;i++) {
      if(verbose) {
	 fprintf(stderr,"Column %d\n",i);
      }
      for(j = -nc;j <= nc;j++) {
	 if(find_coeffs(i,j,r1,r2,t1,t2,
			half_bell,deconvolve,stokes,&coeffs[j][i]) < 0) {
	    fprintf(stderr,
		    "coeff() failed to converge for (x,y) = (%d,%d)\n",i,j);
	 }
	 sum += coeffs[j][i];
      }
   }
/*
 * fix up normalisation (for positive-definite case, namely stokes == 0)
 */
   if(stokes == 0) {
      if(sum == 0.0) {
	 fprintf(stderr,"Sum of coefficients is 0.0\n");
      } else {
	 float fac = 0.5*(t2 - t1)*(r2*r2 - r1*r1)/sum;

	 if(verbose) {
	    fprintf(stderr,"Sum of coefficients: %.3f (== %6g*expected)\n",
		    sum,1/fac);
	 }
	 for(i = -nc;i <= nc;i++) {
	    for(j = -nc;j <= nc;j++) {
	       coeffs[j][i] *= fac;
	    }
	 }
      }
   }
}

/*****************************************************************************/

static int romberg(float (*func)(float), float a, float b, float *ans);
static int dromberg(float (*func)(float,float),
		 float a, float b, float c, float d, float *ans);

static int deconvolve;			/* deconvolve pixel response? */
static float hbell;			/* half-width of cosbell for filter */
static int stokes;			/* calculate U, I, or Q */
static float xi, yi;			/* position of centre of pixel whose
					   coefficient is being calculated */
/*
 * Actually evaluate the sinc-interpolated value. We add 1 to the result
 * so as not to confuse the convergence criterion used in the integration
 * whenever the real integral vanishes
 *
 * If the half-width of the cosbell is negative, use the geometric intersection
 * of the pixels with the aperture
 */
static float
func(float r, float t)
{
   float x,y;				/* (x,y) value at point (r,theta) */
   float fx, fy;			/* sinc-filters in x and y */

   x = r*cos(t) - xi;
   y = r*sin(t) - yi;

   if(fabs(x) < hbell) {
      x *= M_PI;
      if(deconvolve) {
	 fx = (x == 0.0f ? 1 + M_PI*M_PI/72 :
	      sin(x)/x + M_PI*M_PI/(24*x*x*x)*((x*x - 2)*sin(x) + 2*x*cos(x)));
      } else {
	 fx = (x == 0.0f ? 1 : sin(x)/x);
      }
      fx *= 0.5*(1 + cos(x/hbell));
   } else {
      fx = 0.0;
   }
   if(fabs(y) < hbell) {
      y *= M_PI;
      if(deconvolve) {
	 fy = (y == 0.0f ? 1 + M_PI*M_PI/72 :
	      sin(y)/y + M_PI*M_PI/(24*y*y*y)*((y*y - 2)*sin(y) + 2*y*cos(y)));
      } else {
	 fy = (y == 0.0f ? 1 : sin(y)/y);
      }
      fy *= 0.5*(1 + cos(y/hbell));
   } else {
      fy = 0.0;
   }

   return(1 + r*fx*fy*
	  (stokes == -1 ? sin(2*t) : (stokes == 1) ? cos(2*t) : 1));
}

static int
find_coeffs(int i, int j,
	    float r1, float r2,		/*            from r1 to r2 in r */
	    float t1, float t2,		/*             and t1 to t2 in theta */
	    float half_bell,		/* half-width of bell for sinc filter*/
	    int decon,			/* deconvolve pixel reponse? */
	    int i_stokes,		/* calculate U, I, or Q? */
	    float *ans)			/* desired answer */
{
   int ret;				/* return code */
   deconvolve = decon;
   hbell = half_bell;
   stokes = i_stokes;
   xi = i; yi = j;
   
   ret = dromberg(func,r1,r2,t1,t2,ans);
   *ans -= (t2 - t1)*(r2 - r1);

   return(ret);
}

/*****************************************************************************/
/*
 * Perform a numerical quadrature. This code is taken from Numerical Recipes
 * (with the function trapzd() inlined to make the call reentrant)
 */
#define IMAX 20
#define K 5

static void
polint(float xa[],
       float ya[],
       int n,
       float x,
       float *y,
       float *dy)
{
   int i,m,ns=1;
   float den,dif,dift,ho,hp,w;
   float *c,*d;
   
   dif=fabs(x-xa[0]);
   c=alloca(2*n*sizeof(float));
   d = c + n;
/*
 * find index of closest table entry, ns
 */
   for(i = 0;i < n;i++) {
      if((dift = fabs(x-xa[i])) < dif) {
	 ns=i;
	 dif=dift;
      }
      c[i]=ya[i];
      d[i]=ya[i];
   }
   *y=ya[ns--];				/* initial approximation */
   
   for(m = 0;m < n-1;m++) {
      for(i=0;i < n - m - 1;i++) {
	 ho=xa[i] - x;
	 hp=xa[i + m + 1] - x;
	 w=c[i+1]-d[i];
	 if ((den=ho-hp) == 0.0) {
	    fprintf(stderr,"Error in routine POLINT");
	    abort();
	 }
	 den=w/den;
	 d[i]=hp*den;
	 c[i]=ho*den;
      }
      *y += (*dy=(2*ns < (n-m) ? c[ns+1] : d[ns--]));
   }
}

/*****************************************************************************/
/*
 * Perform an integral using Romberg's method.
 *
 * Return 0 if all is well, -1 if the integral fails to converge.
 */
static int
romberg(float (*func)(float),		/* integrate func() */
	float a,			/*                  from a */
	float b,			/*                         to b */
	float *ans)			/* The value of the integral */
{
   float ss,dss;
   float s[IMAX + 1],h[IMAX + 1];
   int i;
   float st;				/* previous sum in integrator */
   int it;

   h[0]=1.0;
   for(i=0;i < IMAX;i++) {
/*
 * Evaluate the integral using the Trapezoid Rule
 */
      if(i == 0) {
	 it = 1;
	 st = s[i] = 0.5*(b-a)*(func(a) + func(b));
      } else {
	 float del = (b-a)/it;
	 int j;
	 float sum = 0.0;
	 float x=a + 0.5*del;
	 
	 for(j = 1;j <= it;j++) {
	    sum += func(x);
	    x += del;
	 }
	 s[i] = st = 0.5*(st + (b-a)*sum/it);
	 it *= 2;
      }
/*
 * If we have enough points, extrapolate to zero stepsize
 */
      if(i >= K) {
	 polint(&h[i - K],&s[i - K],K,0.0,&ss,&dss);
	 if (fabs(dss) < eps*fabs(ss) || fabs(dss) < EPSILON_f) {
	    break;
	 }
      }
      s[i+1]=s[i];
      h[i+1]=0.25*h[i];
   }

   *ans = ss;
   return(i < IMAX ? 0 : -1);
}

/*****************************************************************************/
/*
 * evaluate a double integral where the limits are simple constants
 */
static float e_t1, e_t2;		/* extern copies of limits */
static float (*e_func)(float,float);	/* extern copy of (*integrand) */
static float e_r;			/* extern copy of r */

/*
 * a wrapper to call the real function to be evaluated
 */
static float
call_func(float t)			/* current value of t */
{
   return((*e_func)(e_r,t));
}

/*
 * The function called to perform the r integration
 */
static float
func1(float r)				/* current value of r */
{
   float ans;

   e_r = r;
   if(romberg(call_func,e_t1,e_t2,&ans) < 0) {
      fprintf(stderr,
	   "Theta integral failed to converge for r == %g (val = %g)\n",r,ans);
   }
   return(ans);
}

/*
 * Here's the call to do the double integral
 */
static int
dromberg(float (*func)(float,float),	/* integrate func(r,theta) */
	 float r1, float r2,		/*            from r1 to r2 in r */
	 float t1, float t2,		/*             and t1 to t2 in theta */
	 float *ans)			/* desired answer */
{
   e_t1 = t1; e_t2 = t2;
   e_func = func;
   
   return(romberg(func1,r1,r2,ans));
}

/*****************************************************************************/
/*
 * manage memory
 */
void *
shMalloc(int n)
{
   void *ptr = malloc(n);
   if(ptr == NULL) {
      fprintf(stderr,"Cannot allocate %d bytes\n",n);
      abort();
   }

   return(ptr);
}

void
shFree(void *ptr)
{
   free(ptr);
}

/*****************************************************************************/
/*
 * Shell Scripts
 *
 * First, do_cells which calculates a complete set of coefficients, storing
 * them in the directory ./cells
 *
#!/bin/sh
#
# Make the sinc-interpolated aperture coefficients for a set of cells
#
# Radii as in the "for r" loop, angles of width 30degrees starting at
# -15,345,30 (and also 0--360 for all annuli)
#
cosbell=100
nc=15
base=coeffs; type=""
#base=dcoeffs15; dflg=-d; nc=15
#radii="0.56 1.69 2.58 4.41 7.51 11.58"
radii="   -1   -9  -21  -61 -177  -421"
#
r2=0
for r in $radii; do
	r1=$r2;
	r2=$r
	src/make_photom_coeffs $type $dflg -b $cosbell -e 1e-5 -n $nc\
		    -S cells/$base:$cosbell:$r1:$r2:0:360 $r1 $r2 0 360

	if [ $r1 != 0 ]; then
		t2=-15
		for t in 15 45 75 105 135 165 195 225 255 285 315 345; do
			t1=$t2
			t2=$t
			src/make_photom_coeffs $type $dflg -b $cosbell -e 1e-5\
				    -n $nc\
				    -S cells/$base:$cosbell:$r1:$r2:$t1:$t2 \
					$r1 $r2 $t1 $t2
		done
	fi
done
 */

/*****************************************************************************/
/*
 * Now do_code, that generates the initialisation code for a complete set
 * of cells, ready to be pasted into C
 *
#!/bin/sh
#
# Make the sinc-interpolated aperture coefficients for a set of cells. We
# assume that we can read the actual values from the cells directory
#
# Radii as in the "for r" loop, angles of width 30degrees starting at
# -15,345,30 (and also 0--360 for all annuli)
#
cosbell=5
nc=15
base=coeffs
#radii="0.56 1.69 2.58 4.41 7.51 11.58"
radii="   -1   -9  -21  -61 -177  -421"
#
# Parse arguments
#
while [ X"$1" != X"" ]; do
	case "$1" in
	  -Q)
		base=Qcoeffs; type="-Q";;
	  -U)
		base=Ucoeffs; type="-U";;
	  *)
		echo "Unknown flag $1" 1>&2;;
	esac
	shift
done
#
r2=0
for r in $radii; do
	r1=$r2;
	r2=$r

	if [ $r1 = 0 ]; then
		src/make_photom_coeffs -C $type -b $cosbell -e 1e-5 -n $nc\
			    -S "<cells/$base:$cosbell:$r1:$r2:0:360" \
				$r1 $r2 0 360 2>&1
	elif [ X"$type" != X"" ]; then
		src/make_photom_coeffs -C $type -b $cosbell -e 1e-5 -n $nc\
			    -S "<cells/$base:$cosbell:$r1:$r2:0:360" \
				$r1 $r2 0 360 2> /dev/null
	else 
		t2=-15
		for t in 15 45 75 105 135 165 195 225 255 285 315 345; do
			t1=$t2
			t2=$t
			src/make_photom_coeffs -C $type -b $cosbell -e 1e-5 \
				    -n $nc\
				    -S "<cells/$base:$cosbell:$r1:$r2:$t1:$t2"\
					$r1 $r2 $t1 $t2 2> /dev/null
		done
	fi
done
 */
