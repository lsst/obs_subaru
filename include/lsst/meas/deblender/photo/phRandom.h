#if !defined(PHRANDOM_H)
#define PHRANDOM_H
#if !defined(STAND_ALONE)
#include "phRanlib.h"
#endif

/*
 * Type to hold state information for random number tables
 */
typedef struct {
   REGION *reg;
   int *base;
   int *top;
   int *ptr;
} RANDOM;

/*
 * phRandom uses a pre-calculated set random numbers, which are read from disk
 * or calculated on the fly.
 *
 * You can get the same random numbers as phRandom with the macro PHRANDOM;
 * this is significantly faster.
 *
 * N.b. The random numbers returned by phRandom and phRandom_u16 are
 * independent, but they are NOT independent of those returned by PHRANDOM
 * (in fact, they are a (possibly identical) part of the same sequence).
 *
 * PHRANDOM expects that the variables rand_{base,ptr,top} have been unpacked
 * from a RANDOM
 * structure, which was initialised with phRandomNew()
 *   int *rand_base = rand->base;
 *   int *rand_top = rand->top;
 *   int *rand_ptr = rand->ptr;
 * You can do this with DECLARE_PHRANDOM:
 *   DECLARE_PHRANDOM(rand);
 * When you are done you _must_ say
 *   END_PHRANDOM(rand);
 * to repack the variables
 */
int phRandomIsInitialised(void);
RANDOM *phRandomGet(void);
RANDOM *phRandomNew(char *file, unsigned int offset);
void phRandomDel(RANDOM *rand);
unsigned int phRandomSeedSet(RANDOM *rand, unsigned int seed);
int phRandom(void);
short phRandom_u16(void);

#define DECLARE_PHRANDOM(R)	/* unpack rand into local variables */	      \
{				/* use { to force use of END_PHRANDOM */      \
   int *rand_base = ((R) == NULL) ? NULL : (R)->base;			      \
   int *rand_top = ((R) == NULL) ? NULL : (R)->top;			      \
   int *rand_ptr = ((R) == NULL) ? NULL : (R)->ptr;

#define PHRANDOM \
   ((++rand_ptr == rand_top) ? *(rand_ptr = rand_base) : *rand_ptr)
#define END_PHRANDOM(R)		/* pack local pointer back into rand */	      \
   if((R) != NULL) { (R)->ptr = rand_ptr; } 				      \
}

float phGaussdev(void);
#if !defined(STAND_ALONE)
float phPoissondev(const double mu);
float phRandomUniformdev(void);

int phRegIntGaussianAdd(REGION *reg, RANDOM *rand,
			const float mean, const float sigma);
int phRegIntNoiseAdd(REGION *reg, RANDOM *rand,
		     const int bkgd, const float gain, const int poisson);
#endif
#endif
