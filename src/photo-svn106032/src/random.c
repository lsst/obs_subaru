/*
 * Create a random (uniform distribution) image, and use such an
 * image as a random number generator
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>
#include "dervish.h"
#include "phRandom.h"
/*
 * Do we have a Gaussian deviate squirreled away?
 */
static int phGaussdev_iset = 0;

/*****************************************************************************/
/*
 * These random number generators come from SM
 */
static void sran_sm(long seed);
static int rand_sm(void);

/*****************************************************************************/
/*
 * The random number generators sran, ran[12], and iran[12] are based
 * on NR2, section 7.1. But it's so simple that this cannot be a problem, 
 * surely?
 */
#define NTAB 32

#define IM0 2147483647
#define IA0 16807
#define IQ0 127773
#define IR0 2386
#define IM1 2147483563
#define IA1 40014
#define IQ1 53668
#define IR1 12211
#define IM2 2147483399
#define IA2 40692
#define IQ2 52774
#define IR2 3791

#define MULTMOD(SEED,N)			/* evaluate (SEED*IAN)%IMN */   \
{					/* using Schrage's method */	\
   register unsigned int l = SEED/IQ##N; 				\
   SEED = IA##N*(SEED - l*IQ##N) - l*IR##N;				\
   if(SEED < 0) SEED += IM##N;						\
}

static int i_seed1 = 1, i_seed2 = 123456789;
static int i_iy = 0;
static unsigned int i_iv[NTAB];

static void
sran1(int iseed)
{
   int i;

   if(iseed == 0) {
      i_seed1 = 1;
   } else {
      i_seed1 = iseed;
   }
   i_seed2 = i_seed1;

   for(i = NTAB + 7;i >= 0;i--) {
      MULTMOD(i_seed1,1);		/* we could use IM0 etc. here for
					   seeding ran1, but why bother? */
      if(i < NTAB) {
	 i_iv[i] = i_seed1;
      }
   }
   i_iy = i_iv[0];
}

/*****************************************************************************/

static int
iran1_16(void)
{
   int i;

   MULTMOD(i_seed1,0);			/* a multiplicative-congruent rand # */

   i = i_iy/(1 + (IM0 - 1)/NTAB);	/* shuffle answers */
   i_iy = i_iv[i];
   i_iv[i] = i_seed1;

   return((i_iy - 1) & 0xffff);
}

static int
iran2_16(void)
{
   int i;

   MULTMOD(i_seed1,1);		/* two multiplicative-congruent */
   MULTMOD(i_seed2,2);		/* random numbers */

   i = i_iy/(1 + (IM1 - 1)/NTAB);	/* shuffle answers */
   i_iy = i_iv[i] - i_seed2;
   if(i_iy < 1) i_iy += IM1 - 1;
   i_iv[i] = i_seed1;

   return((i_iy - 1) & 0xffff);
}

/*****************************************************************************/
/*
 * <EXTRACT AUTO>
 * These are the functions to initialise the random numbers used by photo.
 *
 * phRandomNew reads a table from a file, assumed to be a U16 fits file
 * with one row. If the filename is of the form ###[:#] (e.g. 1000:2)
 * phRandomNew will calculate ### random numbers using a linear-congruent
 * generator; specifically, the default algorithm is ran1 from Press et al.,
 * but you can specify the better but slower ran2 by putting a :2 at the
 * end of the ### (so 1000:2 meant 1000 random numbers using ran2).
 *
 * The RANDOM returned is designed to be fed to the macro PHRANDOM to
 * generate 32-bit random numbers; the numbers can be written to disk
 * with shRegWriteAsFits(rand->reg,"file",...);
 *
 * When you have no further need for random numbers, you should call
 * phRandomDel to free all resources used by the these functions
 */
static RANDOM *i_random = NULL;		/* used by phRandom and phRandom_u16 */

/*
 * <EXTRACT AUTO>
 */
RANDOM *
phRandomNew(
	    char *file,			/* file of random numbers. */
	    unsigned int offset	/* starting offset in file */
	    )
{
   int nval;				/* number of 32 bit values */
   RANDOM *rand = shMalloc(sizeof(RANDOM));

   if(isdigit(*file)) {			/* generate region on the fly */
      char *cptr;
      int c;
      unsigned short *ptr;		/* pointer to row of region */
      int type = 1;			/* type of generator */
      
      nval = atol(file)*2;		/* we generate 16 bit values */
      if(type == 3) {
	 nval = 2*(int)(nval/2);	/* we're generating 32-bit values */
      }
      if((cptr = strchr(file,':')) != NULL) {
	 type = atoi(cptr + 1);
      }

      if(type != 1 && type != 2 && type != 3) {
	 shError("I only know random numbers of types 1, 2, or 3, not %d\n",
		 type);
	 shFree(rand);
	 return(NULL);
      }

      rand->reg = shRegNew("phRandom",1,nval,TYPE_U16);

      sran1(offset);
      switch(type) {
       case 3:
	 sran_sm(offset);
	 break;
      }
      
      ptr = rand->reg->rows_u16[0];
      c = 0;
      switch (type) {
       case 1:
	 do {
#if defined(SDSS_LITTLE_ENDIAN)
	    ptr[c + 1] = iran1_16();
	    c++;
	    ptr[c - 1] = iran1_16();
	    c++;
#else
	    ptr[c] = iran1_16();
	    c++;
	    ptr[c] = iran1_16();
	    c++;
#endif
	 } while(c != nval);
	 break;
       case 2:
	 do {
#if defined(SDSS_LITTLE_ENDIAN)
	    ptr[c + 1] = iran2_16();
	    c++;
	    ptr[c - 1] = iran2_16();
	    c++;
#else
	    ptr[c] = iran2_16();
	    c++;
	    ptr[c] = iran2_16();
	    c++;
#endif
	 } while(c != nval);
	 break;
       case 3:
	 do {
	    *(int *)&ptr[c] = rand_sm();
	    c += sizeof(int)/sizeof(*ptr);
	 } while(c != nval);
	 break;
       default:
	 shFatal("Impossible error %s %d",__FILE__,__LINE__);
	 break;
      }
   } else {
      rand->reg = shRegNew("phRandom",0,0,TYPE_U16);
      
      if(shRegReadAsFits(rand->reg, file, 1, 0, DEF_NONE, NULL, 0, 0, 0) != 
	 SH_SUCCESS) {

	 shRegDel(rand->reg);
	 shFree(rand);
	 return(NULL);
      }
   }

   if(rand->reg->nrow != 1) {
      shErrStackPush("Expected a random region with 1 row; found %d\n",
	      rand->reg->nrow);
      phRandomDel(rand);
      return(NULL);
   }
   nval = rand->reg->ncol;

   if(rand->reg->type != TYPE_U16) {
      shErrStackPush("Expected a noise region of shorts\n");
      phRandomDel(rand);
      return(NULL);
   } else {
      nval /= 2;			/* treat it as 32 bit ints */
   }

   shAssert(sizeof(int) == 4);		/* as specified in survey standards */
   offset = sizeof(int)*(int)(offset/sizeof(int)); /* align on 4by boundary */
   offset = offset - nval*(int)(offset/nval);
   
   rand->base = (int *)rand->reg->rows_u16[0];
   rand->top = rand->base + nval;
   rand->ptr = rand->base + offset;

   if(i_random == NULL) {
      i_random = rand;			/* used by phRandom and phRandom_u16 */
   }
   
   return(rand);
}

/*
 * <EXTRACT AUTO>
 */
void
phRandomDel(RANDOM *rand)
{
   if(rand != NULL) {
      if(rand == i_random) {
	 i_random = NULL;
      }
      shRegDel(rand->reg);
      shFree(rand);
   }
}

/*****************************************************************************/
/*
 * <AUTO>
 * Set the seed of a pre-existing RANDOM
 *
 * Returns the previous seed, or 0 if the RANDOM is NULL
 */
unsigned int
phRandomSeedSet(RANDOM *rand,
		unsigned int seed)
{
   unsigned int old;

   phGaussdev_iset = 0;			/* clear phGaussdev's saved value */

   if(rand == NULL) return(0);
   
   old = (rand->ptr - rand->base);
   rand->ptr = rand->base + seed%(rand->top - rand->base);

   return(old);
}

/*****************************************************************************/
/*
 * <EXTRACT AUTO>
 * These are two random number generators, but you probably don't want to
 * call them. The usual way to get sets of random numbers is to use the
 * macro PHRANDOM, as declared and described in phRandom.h --- it's
 * significantly faster
 *
 * PHRANDOM expects that the variables rand_{base,ptr,top} have been unpacked
 * from a RANDOM
 * structure, which was initialised with phRandomNew()
 *   int *rand_base = rand->base;
 *   int *rand_top = rand->top;
 *   int *rand_ptr = rand->ptr;
 * You can do this with DECLARE_PHRANDOM:
 *   DECLARE_PHRANDOM(rand);
 * (This must follow all declarations in the block).
 *
 * When you are done you _must_ say
 *   END_PHRANDOM(rand);
 * to repack the variables. So the code looks like:
 *   DECLARE_PHRANDOM(rand);
 *   ...
 *   val = PHRANDOM;
 *   ...
 *   END_PHRANDOM(rand);
 *
 * You must call phRandomNew before using any of these generators, and
 * phRandomDel when you are done 
 *
 * The random numbers returned by phRandom and phRandom_u16 are independent,
 * but they are NOT independent of those returned by PHRANDOM (in fact, they
 * are a (possibly identical) part of the same sequence).
 */
int
phRandom(void)
{
   int r;
   DECLARE_PHRANDOM(i_random);
   
   r = PHRANDOM;
   END_PHRANDOM(i_random);
   return(r);
}

short
phRandom_u16(void)
{
   short r;
   DECLARE_PHRANDOM(i_random);
   
   r = PHRANDOM & 0xffff;
   END_PHRANDOM(i_random);
   return(r);
}

/*****************************************************************************/
/*
 * <EXTRACT AUTO>
 * Is the random number generator initialised?
 */
int
phRandomIsInitialised(void)
{
   return(i_random == NULL ? 0 : 1);
}

/*****************************************************************************/
/*
 * <EXTRACT AUTO>
 * Return the `internal' RANDOM
 */
RANDOM *
phRandomGet(void)
{
   return(i_random);
}

/*****************************************************************************/
/*
 * return an N(0,1) variate.  Note that this saves an `extra' random
 * number internally, and uses it if phGaussdev_iset is true
 */
float
phGaussdev(void)
{
   const float inorm = 1.0/(float)((1U<<(8*sizeof(int)-1)) - 1);
   static float gset;			/* saved Gaussian deviate */
   float fac,r = 0,v1,v2;		/* initialise r to appease
					   the IRIX 5.3 cc */

   if(phGaussdev_iset) {
      phGaussdev_iset = 0;
      return(gset);
   } else {
      do {
	 v1 = phRandom()*inorm;
	 v2 = phRandom()*inorm;
	 r = v1*v1+v2*v2;
      } while (r >= 1.0);
      fac = sqrt(-2.0*log(r)/r);
      gset = v1*fac;
      phGaussdev_iset = 1;

      return(v2*fac);
   }
}

#if !defined(STAND_ALONE)
#if 0
/*****************************************************************************/
/*
 * Use Lanczos' version of Stirling's formula to calculate a log factorial.
 */
static float
ln_factorial(double x)
{
#define LOG_2_PI_2 0.91893853320467274180L /* == log(2*M_PI)/2 */

   double s = 1.000000000190015L +
     76.18009172947146L/(x + 1) + -86.50532032941677L/(x + 2) +
       24.01409824083091L/(x + 3) + -1.231739572450155L/(x + 4) +
	 0.1208650973867e-2L/(x + 5) + -0.539523938495e-5L/(x + 6);
   return((x + 0.5)*log(x + 5.5) - (x + 5.5) + LOG_2_PI_2 + log(s));
}

/*****************************************************************************/
/*
 * Return a Poisson variate with mean mu.
 */
float
phPoissondev(const double mu)
{
   static float sq = 0;			/* set when */
   static float almu = 0;		/*          a new value of */
   static float g = 0;			/*                        mu appears */	
   static float oldmu = -1.0;		/* old value of mu */
   float var,t,y;

   if(mu < 12.0) {
      if(mu != oldmu) {
	 shAssert(mu >= 0);		/* so oldmu == -1 initialisation works*/
	 oldmu = mu;
	 g = exp(-mu);
      }
      var = -1;
      t = 1.0;
      do {
	 ++var;
	 t *= phRandomUniformdev();
      } while (t > g);
   } else {
      if(mu != oldmu) {
	 shAssert(mu >= 0);		/* so oldmu == -1 initialisation works*/
	 oldmu = mu;
	 sq = sqrt(2*mu);
	 almu = log(mu);
	 g = mu*almu - ln_factorial(mu);
      }
      do {
	 do {
	    y = tan(M_PI*phRandomUniformdev());
	    var = sq*y + mu;
	 } while(var < 0.0);
	 var = floor(var);
	 t = 0.9*(1 + y*y)*exp(var*almu - ln_factorial(var) - g);
      } while(phRandomUniformdev() > t);
   }

   return(var);
}
#else
float
phPoissondev(const double mu)
{
   static int first = 1;

   if(first) {
      first = 0;
      phPoissonSeedSet(12345L, 678910L);
   }

   return(phPoissonDevGet(mu));
   
}

#endif
#endif
/*****************************************************************************/
/*
 * return a variate in [0,1).
 */
float
phRandomUniformdev(void)
{
   const float inorm = 1.0/(2.0*(1U<<(8*sizeof(int) - 1)) - 1);
   return(phRandom()*inorm + 0.5);
}

/*****************************************************************************/
/*
 * These random number generators are stolen from SM with the
 * permission of the author; they're based on the ones suggested by
 * Marsaglia and Zaman in Computers in Physics; Bill Press pointed
 * them out to SM.
 *
 * Note that SM doesn't not claim a copyright on this code
 */
#define RNMAX (1 - 1.2e-7)		/* ~ largest float less than 1 */

static unsigned int x = 521288629;
static unsigned int y = 362436069; 
static unsigned int z = 16163801;
static unsigned int c = 1;
static unsigned int n = 1131199299;

/*
 * This generator uses 32-bit arithmetic. On most machines, an int is
 * 4 bytes so the ANDing with 0xffffffff is a NOP, but in that case the
 * optimiser should catch it. Increasingly, long is a 64-bit type so
 * it is not used here.
 */
static void
mzranset(int xs, int ys, int zs, int ns)
{
   x = xs; y = ys; z = zs; n = ns;
   c = (y > z) ? 1 : 0;
}

static int
mzran(void)
{
   unsigned int s;

   if(y > x + c) {
      s = y - (x + c);
      c = 0;
   } else {
      s = y - (x + c) - 18;
      c = 1;
   }
   s &= 0xffffffff;
   x = y;
   y = z;
   z = s;
   n = 69069*n + 1013904243;
   n &= 0xffffffff;

   s = (s + n) & 0xffffffff;		/* here's a 32-bit random number */

   return(s);
}

/*****************************************************************************/
/*
 * Set the random seed 
 */
static void
sran_sm(long seed)
{
   srand(seed);
   mzranset((int)seed, rand(), rand(), rand());
}

static int
rand_sm(void)
{
   return(mzran());
}
