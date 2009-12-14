/*
 * calculate linear predictive coding coefficients used by photo to
 * interpolate over bad columns
 *
 * The seeing is taken to be Gaussian
 *
 * Optionally, generate the code to patch up a defect, ready to be
 * cut-and-pasted into correctFrames.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define BAD_NOISE 1e20			/* a variance implying ignore pixel */
#define NIN 6				/* desired initial indentation */

void *shMalloc(int);
void shFree(void *);

static void analyse_defect(char *defect, int *x1, int *x2);
static int do_defect(char *defect, double noise2, double sigma,
		     int print_coeffs, int print_variance, int print_as_C);
static int get_type(char *defect);
static double invert(double **iarr, double **arr, int n);
static void setS(double **S, double **N, int dimen, double sigma,
		 double noise2, char *defect);

static int left_defect = 0, right_defect = 0; /* do the bad columns extend
						 to the {left,right} edge? */

static void
usage(void)
{
   char **ptr;
   char *msgs[] = {
      "Usage: make_interp [options] defect [defect...]",
      "Defects can be specified as numbers (e.g. 0142) or "
	"strings (e.g. \"##...#.\")",
	"Options:",
	"   -h, -?       Print this message",
	"   -c           Print the coefficients (default)",
	"   -C           Produce output that can be cut-and-pasted into C",
	"   -l	       Assume that defect is at left side of data",
	"   -n #         Set the noise variance to # (default: 0)",
	"   -r	       Assume that defect is at right side of data",
	"   -s #         Set the PSF to an N(0,#) Gaussian (default: 1)",
	"   -v           Print the coefficients' variance",
	"Note that -l is only useful with numeric defects, when it specifies",
	"that the defect be reversed (so -l 014 is equivalent to ..##)",
      NULL
   };

   for(ptr = msgs;*ptr != NULL;ptr++) {
      fprintf(stderr,"%s\n",*ptr);
   }
}      

int
main(int ac, char **av)
{
   double sigma = 1.0;			/* PSF is an N(0,sigma^2) Gaussian */
   double noise2 = 0;			/* variance of noise */
   int print_coeffs = 0;		/* print the coefficients? */
   int print_variance = 0;		/* print the coefficients' variance? */
   int print_as_C = 0;			/* output be cut-and-pasted into C? */
   char *ptr;
   double val;
/*
 * process arguments
 */
   while(ac > 1 && av[1][0] == '-') {
      switch(av[1][1]) {
       case '?': case 'h':		/* help */
	 usage();
	 return(0);
       case 'c':			/* print coefficients */
	 print_coeffs = 1;
	 break;
       case 'C':			/* output is in format of legal C */
	 print_as_C = 1;
	 break;
       case 'l':			/* defect abuts left of data */
	 left_defect = 1;
	 break;
       case 'n':			/* noise variance */
       case 's':			/* sigma */
	 if(ac == 2) {
	    fprintf(stderr,"Please provide a value with %s\n",av[1]);
	 } else {
	    val = strtod(av[2],&ptr);
	    if(ptr == av[2]) {
	       fprintf(stderr,"I need a number with -s, not \"%s\"\n",av[2]);
	    }
	    switch (av[1][1]) {
	     case 'n':
	       noise2 = val; break;
	     case 's':
	       sigma = val; break;
	     default:
	       fprintf(stderr,"Impossible case\n");
	       abort();
	    }
	    ac--; av++;
	 }
	 break;
       case 'r':			/* defect abuts right side of data */
	 right_defect = 1;
	 break;
       case 'v':			/* print coefficient variance */
	 print_variance = 1;
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
   if(!print_coeffs && !print_variance && !print_as_C) {
      print_coeffs = 1;
   }
   if(ac == 1) {
      fprintf(stderr,"You must specify at least one defect pattern\n");
      usage();
      exit(1);
   }
   if(sigma <= 0.0) {
      fprintf(stderr,"Sigma must be >= 0\n");
      sigma = 1.0;
   }

   if(print_as_C) {
      if(left_defect) {
	 printf("%*s double out2_1 = out[bad_x2 + 1];\n",NIN - 3,"");
	 printf("%*s double out2_2 = out[bad_x2 + 2];\n",NIN - 3,"");
	 printf("\n");
	 printf("%*s case BAD_LEFT:\n",NIN - 3,"");
      } else if(right_defect) {
	 printf("%*s double out1_2 = out[bad_x1 - 2];\n",NIN - 3,"");
	 printf("%*s double out1_1 = out[bad_x1 - 1];\n",NIN - 3,"");
	 printf("\n");
	 printf("%*s case BAD_RIGHT:\n",NIN - 3,"");
      } else {
	 printf("%*s double out1_2 = out[bad_x1 - 2];\n",NIN - 3,"");
	 printf("%*s double out1_1 = out[bad_x1 - 1];\n",NIN - 3,"");
	 printf("%*s double out2_1 = out[bad_x2 + 1];\n",NIN - 3,"");
	 printf("%*s double out2_2 = out[bad_x2 + 2];\n",NIN - 3,"");
	 printf("\n");
	 printf("%*s case BAD_MIDDLE:\n",NIN - 3,"");
      }
      printf("\n");
   }

   while(ac > 1) {
      do_defect(av[1],noise2,sigma,print_coeffs, print_variance, print_as_C);
      ac--; av++;
   }

   return(0);
}

/*****************************************************************************/
/*
 * Calculate LP coefficients for a defect, assuming a gaussian PSF N(0,sigma^2)
 * The <noise^2> is taken to be noise2. 
 */
static int
do_defect(char *defect,
	  double noise2,
	  double sigma,
	  int print_coeffs,
	  int print_variance,
	  int print_as_C)
{
   int i,j,k;
   double lambda;			/* lagrange multiplier */
   int ncoeff;				/* number of coefficients desired */
   double sum, sum2;
   double **S, **N;			/* <S_i S_j>, <N_i N_j> */
   double **SN;				/* (S*N)^{-1} */
   double *temp;				/* a temp vector */
   double **d;				/* LP coefficients */
   int x1, x2;				/* first and last patched column */

   if(isxdigit(defect[0])) {		/* a hex digit, convert to [.#]* */
      static char defect_s[100];
      int idefect = strtol(defect,NULL,0);

      if(left_defect) {
	 defect = defect_s;
	 while(idefect > 0) {
	    *defect++ = (idefect%2) ? '#' : '.';
	    idefect /= 2;
	 }
	 *defect = '\0';
	 defect = defect_s;
      } else {
	 defect = defect_s + sizeof(defect_s) - 1;
	 *defect-- = '\0';
	 while(idefect > 0) {
	    *defect-- = (idefect%2) ? '#' : '.';
	    idefect /= 2;
	 }
	 
	 defect++;
      }
   }

   ncoeff = strlen(defect);
   for(i = 0;i < ncoeff;i++) {
      if(defect[i] != '.' && defect[i] != '#') {
	 fprintf(stderr,"Invalid defect; must contain only . and #\n");
	 return(-1);
      }
   }
/*
 * Allocate the memory
 */
   S = shMalloc(4*ncoeff*sizeof(double *));
   N = S + ncoeff;
   SN = N + ncoeff;
   d = SN + ncoeff;
   S[0] = shMalloc((4*ncoeff*ncoeff + ncoeff)*sizeof(double));
   N[0] = S[0] + ncoeff*ncoeff;
   SN[0] = N[0] + ncoeff*ncoeff;
   d[0] = SN[0] + ncoeff*ncoeff;
   temp = d[0] + ncoeff*ncoeff;

   for(i = 0;i < ncoeff;i++) {
      S[i] = S[0] + i*ncoeff;
      N[i] = N[0] + i*ncoeff;
      SN[i] = SN[0] + i*ncoeff;
      d[i] = d[0] + i*ncoeff;
   }
/*
 * then do the work
 */
   setS(S, N, ncoeff, sigma, noise2, defect);

   for(i = 0;i < ncoeff;i++) {		/* N = S + N */
      for(j = 0;j < ncoeff;j++) {
	 N[i][j] += S[i][j];
      }
   }
   (void)invert(SN,N,ncoeff);		/* SN = N^{-1} (i.e. (S + N)^{-1} */

   for(i = 0;i < ncoeff;i++) {
      if(defect[i] == '#') {		/* no need to interpolate */
	 continue;
      }
/*
 * estimate lagrange multiplier to ensure that sum(d) == 1.
 * in the comments, E is a vector with all components == 1
 */
      for(j = 0;j < ncoeff;j++) {	/* temp = SN*S[i] */
	 sum = 0;
	 for(k = 0;k < ncoeff;k++) {
	    sum += SN[j][k]*S[i][k];
	 }
	 temp[j] = sum;
      }

      sum = 0;
      for(j = 0;j < ncoeff;j++) {	/* sum = sum(temp) */
	 sum += temp[j];
      }
      lambda = sum - 1;
      
      for(j = 0;j < ncoeff;j++) {	/* temp = SN*E */
	 sum = 0;
	 for(k = 0;k < ncoeff;k++) {
	    sum += SN[j][k];
	 }
	 temp[j] = sum;
      }
      sum = 0;
      for(j = 0;j < ncoeff;j++) {	/* sum = sum(temp*E) */
	 sum += temp[j];
      }
      lambda /= sum;
/*
 * and estimate coefficients, d
 */
      for(j = 0;j < ncoeff;j++) {	/* temp = S[i] - lambda*E */
	 temp[j] = S[i][j] - lambda;
      }

      for(j = 0;j < ncoeff;j++) {	/* d[i] = SN*temp */
	 sum = 0;
	 for(k = 0;k < ncoeff;k++) {
	    sum += SN[k][j]*temp[k];
	 }
	 d[i][j] = sum;
      }
   }
/*
 * done. Output time
 */
   analyse_defect(defect, &x1, &x2);
      
   if(print_as_C) {
      int nc;				/* number of coeffs on this line */

      printf("%*s    case 0%o:\t\t/* %s, <noise^2> = %g, sigma = %g */\n",
	     NIN,"", get_type(defect), defect, noise2, sigma);
      for(i = 0;i < ncoeff;i++) {
	 if(i < x1 || i > x2) {
	    continue;
	 }
	 printf("%*s      val = ",NIN,"");
	 for(j = nc = 0;j < ncoeff;j++) {
	    if(fabs(d[i][j]) < 1e-4) {
	       continue;
	    }
	    if(nc > 0) {
	       if(d[i][j] < 0) {
		  printf(" - "); d[i][j] = -d[i][j];
	       } else {
		  printf(" + ");
	       }
	       if(nc%3 == 0) {
		  printf("\n%*s            ",NIN,"");
	       }
	    }
	    printf("%.4f*",d[i][j]);
	    if(j == x1 - 2) {
	       printf("out1_2");	/* out[bad_x1 - 2] */
	    } else if(j == x1 - 1) {
	       printf("out1_1");
	    } else if(j == x2 + 1) {
	       printf("out2_1");
	    } else if(j == x2 + 2) {
	       printf("out2_2");
	    } else {
	       fprintf(stderr,"Impossible condition at line %d\n",__LINE__);
	       abort();
	    }
	    nc++;
	 }
	 printf(" + 0.5;\n");

	 if(i - x1 < x2 - i) {
	    printf("%*s      out[bad_x1",NIN,"");
	    if(i > x1) {
	       printf(" + %d",i - x1);
	    }
	 } else {
	    printf("%*s      out[bad_x2",NIN,"");
	    if(i < x2) {
	       printf(" - %d",x2 - i);
	    }
	 }
	 printf("] = (val < min) ? ");

	 if(left_defect) {
	    printf("out2_1");
	 } else if(right_defect) {
	    printf("out1_1");
	 } else {
	    printf("0.5*(out1_1 + out2_1) + 0.5");
	 }

	 printf(": \n\t\t\t\t\t (val > MAX_U16 ? MAX_U16 : val);\n\n");
      }
      printf("%*s      break;\n",NIN,"");
   } else {
      printf("\t%s (0%o) %d -- %d\n",defect,get_type(defect),x1,x2);
      for(i = 0;i < ncoeff;i++) {
	 if(i < x1 || i > x2) {
	    continue;
	 }
	 if(print_coeffs) {
	    printf("%-3d",i);
	    for(j = 0;j < ncoeff;j++) {
	       printf("%7.4f%s",d[i][j],(j%9 == 8 ? "\n   " : " "));
	    }
	    printf("\r");		/* backup over those spaces */
	    if(j%9 != 0) {
	       printf("\n");
	    }
	 }
	 if(print_variance) {
	    sum = sum2 = 0;
	    for(j = 0;j < ncoeff;j++) {
	       sum += d[i][j];
	       sum2 += pow(d[i][j],2);
	    }
	    printf("Sum(d_%d) = %.4f  Sum(d_%d^2) = %.4f\n", i,sum, i, sum2);
	 }
      }
   }

   shFree(S[0]); shFree(S);

   return(0);
}

/*****************************************************************************/
/*
 * figure out where the fixable part of a defect is. Because there may
 * be bad columns in the `good' part this is sometimes at best a guess.
 */
#define NSEG 10				/* max number of bad segments */

static void
analyse_defect(char *defect,		/* the defect */
	       int *x1,			/* first fixable column */
	       int *x2)			/* last fixable column */
{
   int i;
   int bad_start[NSEG];
   double delta;				/* distance of segment start from
					   centre of defect */
   const int len = strlen(defect);
   int nbad_seg;			/* number of bad segments */

   nbad_seg = 0;
   for(i = 0;;) {
      while(defect[i] == '#') i++;
      if(defect[i] == '\0' || nbad_seg == NSEG) {
	 if(nbad_seg < 0) {
	    fprintf(stderr,"No bad columns in defect %s\n",defect);
	    *x1 = 0;
	    *x2 = -1;
	    return;
	 }

	 if(nbad_seg == NSEG) {
	    fprintf(stderr,"Too many bad segments in defect %s (max %d)\n",
		    defect,NSEG);
	 }
/*
 * find the bad segment nearest to the centre of the defect string
 */
	 delta = 2*len;
	 for(i = 0;i < nbad_seg;i++) {
	    if(fabs(bad_start[i] - len/2.0) < delta) {
	       *x1 = bad_start[i];
	       delta = fabs(bad_start[i] - len/2.0);
	    }
	 }
	 for(i = *x1;defect[i] == '.'; i++) continue;
	 *x2 = i - 1;

	 if(right_defect) {
	    if(*x2 + 1 != len) {
	       fprintf(stderr,
		 "\aDefect %s doesn't extend to right edge of data\n", defect);
	    }
	 } else if(left_defect) {
	    if(*x1 != 0) {
	       fprintf(stderr,
		  "\aDefect %s doesn't extend to left edge of data\n", defect);
	    }
	 } else {
	    if(*x1 == 0 || *x2 + 1 == len) {
	       fprintf(stderr,"\aDefect %s extends to edge of data\n", defect);
	    }
	 }
	 return;
      }

      bad_start[nbad_seg++] = i;

      while(defect[i] == '.') i++;
   }
}

/*****************************************************************************/
/*
 * set S and N matrices of size dimen, with sigma. The noise variance is noise2
 * There are nbad bad values that should be ignored
 */
static void
setS(double **S,
     double **N,
     int dimen,
     double sigma,
     double noise2,
     char *defect)
{
   int i,j;

   for(i = 0;i < dimen;i++) {
      for(j = 0;j < dimen;j++) {
	 S[i][j] = exp(-0.25*pow((j - i)/sigma,2));
	 N[i][j] = 0;
      }
      N[i][i] = noise2;
   }
/*
 * Set the noise in the bad pixels very high
 */
   for(i = 0;i < dimen;i++) {
      if(defect[i] == '.') {
	 N[i][i] = BAD_NOISE;
      }
   }
}

/*****************************************************************************/
/*
 * return a defect's type as an int
 */
static int
get_type(char *defect)
{
   int type;

   if(left_defect) {
      char *ptr;
      
      for(type = 0, ptr = defect + strlen(defect) - 1;ptr >= defect;ptr--) {
	 type = (type << 1) + (*ptr == '#' ? 1 : 0);
      }
   } else {
      for(type = 0;*defect != '\0';defect++) {
	 type = (type << 1) + (*defect == '#' ? 1 : 0);
      }
   }

   return(type);
}

/*****************************************************************************/
/*
 * returns absolute max of double array a, pointer to index of max element
 * in pj. Used in inverse()
 */
static double
afamax(double *a,
       int n,
       int *pj)
{
   int j = 0;
   int i;
   double b,c;
   
   b = 0.0;
   for (i=0 ; i < n ; i++ ) {
      if((c = fabs(a[i])) > b) {
	 b=c;
	 j=i;
      }
   }
   *pj = j;

   return b;
}

/*****************************************************************************/
/*
 * Invert a matrix
 */
static double
invert(double **iarr, double **arr, int n)
{
   double am, c, m, akk, r, *sc, *xt;
   int im, *rp, i, j, k, l, rpi, rpk, it;
   double det;
   
   sc = (double *)malloc(2*n*sizeof(double));
   xt = sc + n;
   rp = (int *)malloc(n*sizeof(int));
   
   for( i = 0 ; i < n ; i++) {
      rp[i] = i ;			/* initialize row ptrs */
      sc[i] = afamax( arr[i], n, &im ) ;	/* scale facs=max row elts */
   }
   for( k = 0 ; k < n ; k++) {
      /* first find max abs scaled element in col k, row >= k */
      am = 0.;
      for ( i = k ; i < n ; i++ ) {
	 rpi = rp[i];
	 c = fabs(arr[rpi][k])/sc[rpi];
	 if(c > am ){
	    am = c;
	    im = i;
	 }
      }
      it = rp[im];
      rp[im] = rp[k];
      rp[k] = it;
      akk = arr[it][k];
      for ( i = k+1 ; i < n ; i++ ) {
	 rpi = rp[i];
	 m = arr[rpi][k] = arr[rpi][k]/akk;
	 
	 for( j = k + 1 ; j < n ; j++ ) {
	    arr[rpi][j] -= m*arr[it][j];
	 }
      }
   }
/*
 * matrix now decomposed. compute determinant
 */
    det = 1.;
    for ( i=0 ; i < n ; i++) {
       det *= arr[rp[i]][i];
    }
/*
 * now compute inverse. first set RHS to identity
 */
    for( i=0 ; i < n ; i++ ) {
       memset(iarr[i], n*sizeof(double), '\0' );
    }
    for( i=0 ; i < n ; i++) {
       rpi = rp[i];
       for ( l=0 ; l < n ; l++ ){
	  iarr[rpi][l] = (rpi == l) ? 1.0 : 0.0;
       }
    }
/*
 * now run multipliers through rhs
 */
   for( i=0 ; i<n ; i++ ) {
      rpi = rp[i];
      for ( l=0 ; l < n ; l++ ){
	 for ( k= i+1; k < n; k++) {
	    rpk = rp[k];
	    iarr[rpk][l] -= iarr[rpi][l]*arr[rpk][i];
	 }
      }
   }
/*
 * now do back substitution
 */
   for ( l= 0 ; l < n ; l++ ) {
      for ( i = n-1 ; i >= 0 ; i-- ){
	 rpi = rp[i];
	 r = iarr[rpi][l];
	 for (j = i+1 ; j <n ; j++) {
	    r  -= arr[rpi][j]*xt[j];
	 }
	 xt[i] = r/arr[rpi][i];
      }
      for ( i=0 ; i < n ; i++) {
	 iarr[i][l] = xt[i];
      }
   }

   free(sc); free(rp);

   return (det);
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
