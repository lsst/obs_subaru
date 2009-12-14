#include <stdio.h>
#include <assert.h>
#include <phMathUtils.h> 

#define N 5				/* number of variables */
#define M 41				/* number of `data' points */

/*
 * Return a (not very good) random number in [0,1]
 */
static float
uniform(void)
{
   static int first = 1;
   static unsigned long val;

   if(first) {
      first = 0;
      val = 1234;			/* could be a call to e.g. time() */
   }

   val = (val*1103515245 + 12345) & 0xffffffff;

   return((float)val/(float)0xffffffff);
}


const double ptrue[N] = {
   1000,				/* I0     p[0] */
   10.0,				/* xc     p[1] */
   1.0,					/* sigma0 p[2] */
   100,					/* I1     p[3] */
   3.0,					/* sigma1 p[4] */
};

static int nfunc = 0;

void
func(int m, int n, double *p, double *f, int *flg)
{
   static double pos[M];		/* position of an observation */
   int i;
   static double data[M];		/* a data value */
   static double sig[M];		/* model s.d. */
   double model;			/* model values */
   static int first = 1;		/* is this the first call? */
   double sky = 1000;			/* sky level */

   assert(*flg >= 0 && n >= 0);

   if(first) {
      first = 0;
      
      for(i = 0;i < m;i++) {
	 pos[i] = i*40.0/(M - 1) - 20;

	 data[i] = sky +
	   ptrue[0]*exp(-pow((pos[i] - ptrue[1])/ptrue[2],2)/2) + 
	   ptrue[3]*exp(-pow((pos[i] - ptrue[1])/ptrue[4],2)/2);
	 sig[i] = sqrt(data[i])/400;
	 data[i] += (2*uniform() - 1)*sig[i]*sqrt(3);
      }
   }

   nfunc++;
#if 0
   printf("Estimated (%3dth):",nfunc);
   for(i = 0;i < n;i++) {
      printf(" %9g",p[i]);
   }
   printf("\n");
#endif

   for(i = 0;i < m;i++) {
      model = sky + p[0]*exp(-pow((pos[i] - p[1])/p[2],2)/2) + 
	p[3]*exp(-pow((pos[i] - p[1])/p[4],2)/2);
      f[i] = (data[i] - model)/sig[i];
   }
}

int
main(void)
{
   double chisq = -1e10;
   int i;
   int nfunc;
   double params[N];
   int stat;
   double tol;
   double sumsq_tol = M - 1;		/* quit when chisq reaches this value*/

   params[0] = 0;			/* I0_1 */
   params[1] = 8;			/* x0 */
   params[2] = 1.5;			/* sigma1 */
   params[3] = 0;			/* I0_2 */
   params[4] = 5;			/* sigma2 */

   tol = 1e-4;
   
   stat = phLMMinimise(func,M,N,params,&chisq,&nfunc,tol,sumsq_tol,0,NULL);
   chisq *= chisq;			/* we are returned the norm */

   printf("lmdif1 returns %d, chisq = %g, nfunc = %d\n",stat,chisq,nfunc);
   printf("Params: True:");
   for(i = 0;i < N;i++) {
      printf(" %9g",ptrue[i]);
   }
   printf("\n   Estimated:");
   for(i = 0;i < N;i++) {
      printf(" %9g",params[i]);
   }
   printf("\n");

   if(stat == 0 || stat > 07) {
      printf("TEST-ERR: lmdif1 failed to converge\n");
      return(1);
   } else {
      return(0);
   }
}
