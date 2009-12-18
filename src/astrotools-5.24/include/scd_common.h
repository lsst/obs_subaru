/*
 * coding common to rice compress and decompress
 */
/*
 * commpression uses some unix things like stat() that are not POSIX
 */
#undef _POSIX_SOURCE

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <unistd.h>
#include <getopt.h>
#include <utime.h>

/*
 * 16 bit pixels are read and written into a PIXEL_VALUE from a fits file
 * that holds them in big ended order.
 */
#define BYTES_PER_PIX 2
#define BITS_PER_PIX 16
typedef union {
  unsigned short pixel;
  unsigned char  pixbyte[BYTES_PER_PIX];	/* unsigned for histogram */
} PIXEL_VALUE;

/*
 * Type for compressed data.  A string of bits are packed into a
 * compressed element using << and | and when a element is full its 
 * characters are put one at a time according to its endedness.
 * It is big win to use longs because it saves testing and shifting.  
 */
typedef long COMPTYPE;

typedef union
{
  COMPTYPE compval;
  char compbyte[sizeof(long)];
} COMP_UNION;

#define COMP_ELT_BITS 8*sizeof(COMPTYPE)       /* size of an elt in bits */
#define COMP_ELT_BYTES sizeof(COMPTYPE)        /* size of an elt in bytes */

/* for sunos - otherwise it comes from limits.h */
#ifndef PATH_MAX
#define PATH_MAX 255
#endif  

/*
 * The version number put in the first byte after the fits header
 * to identify which decompress algorithm to use.
 */
#define SRICE_VERSION 1

/*
 * Whether to use difference from mode or difference from prceding value
 */
#define SRICE_MODE_ALGORITHM 0
#define SRICE_DIF_ALGORITHM 1
