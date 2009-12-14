/* Copyright (c) 1994 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* rice_encode.c		Do Rice encoding for an array.  Writes codes
 *						to outfile.  Leaves array unchanged (unlike
 *						encode.c, which modifies A.)
 *
 * Programmer: R. White		Date: 27 July 1994
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "ftcl.h"
#include "dervish.h"
#include "region.h"
#include "math.h"
#include "dervish_msg_c.h"
#include "errStack.h"
#include "phStructs.h"
#include "phCompUtils.h"

#define COMP_DEBUG 10

static int verbose;

/* note this code is different from the qtree code */

static char code_magic[2] = { (char)0xEE, (char)0xAA };

static int doriceenc( MYFILE *outfile, int a[], int nx, int ny, int ndim);

extern int writeshort(MYFILE *outfile, int a);
extern int writeint(MYFILE *outfile, int a);
extern int qwrite(MYFILE *outfile, char *a, int n);
extern int start_outputing_bits(void);
extern int done_outputing_bits(MYFILE *outfile);
extern int output_nbits(MYFILE *outfile, int bits, int n);

extern int
rice_encode(MYFILE *outfile, int a[] , int nx, int ny, int scale)
{
  int nx2, ny2, save0;
  int nbytes, nbits;
  
  /*
   * write magic value
   */
  qwrite(outfile, code_magic, sizeof(code_magic));
  writeshort(outfile, nx);			/* size of image					*/
  writeshort(outfile, ny);
  writeshort(outfile, scale);			/* scale factor for digitization	*/
  nbytes = 2 + 2 + 2 + 2;
  /*
   * write first value of A (sum of all pixels -- the only value
   * which does not compress well)
   */
  writeint(outfile, a[0]);
  nbytes += 4;
  save0 = a[0];
  a[0] = 0;
  /*
   * Initialize bit output
   */
  start_outputing_bits();
  /*
   * do the Rice encoding for each quadrant
   */
  nx2 = (nx+1)/2;
  ny2 = (ny+1)/2;
  
  nbits = 0;
  nbits += doriceenc( outfile,  a,                  nx2,      ny2, ny);
  nbits += doriceenc( outfile, &a[ny2],             nx2, ny - ny2, ny);
  nbits += doriceenc( outfile, &a[ny*nx2],     nx - nx2,      ny2, ny);
  nbits += doriceenc( outfile, &a[ny*nx2+ny2], nx - nx2, ny - ny2, ny);
  
  /*
   * Add zero as an EOF symbol
   */
  output_nbits(outfile, 0, 4);
  nbits += 4;
  done_outputing_bits(outfile);
  
  nbytes += (nbits + 7)/8;

  if (verbose) {
    /*
     * total number of bits written to file
     */
    fprintf(stderr, "Rice:  %6.3f bits/pixel, compression factor %5.1f\n",
	    ((float) 8*nbytes)/(nx*ny),
	    16.0*(nx*ny)/((float) 8*nbytes));
  }
  /* restore a[0] */
  a[0] = save0;
  return(SH_SUCCESS);
}

/*
 * count number of bits for Rice encoding of this array section
 *
 * Note that right now I'm not using the ability to do coding in blocks; I pick
 * a single value for the bit-splitting parameter and use that for the entire
 * image.  Eventually will want to change this.
 */
static int
doriceenc( MYFILE *outfile, int a[], int nx, int ny, int ndim)
{
  int i, j, *p, v, bits[16], bmin, fs, top;

  /*
   * try splitting off 0,1,... least significant bits
   */
  for (j = 0; j<16; j++) bits[j] = j*nx*ny;
  for (i=0; i<nx; i++) {
    for (p = &a[i*ndim]; p < &a[i*ndim+ny]; p++) {
      /*
       * try various number of split bits
       */
      v = 2*abs(*p) - (*p > 0);
      for (j=0; j<16; j++) bits[j] += (v>>j) + 1;
    }
  }
  /*
   * pick the smallest
   */
  bmin = bits[0];
  fs = 0;
  for (j=1; j<16; j++) {
    if (bmin > bits[j]) {
      fs = j;
      bmin = bits[j];
    }
  }
  /*
   * 4 ID bits used to indicate split level
   */
  bmin = bmin + 4;
  /*
   * write the codes
   */
  output_nbits(outfile, fs, 4);
  for (i=0; i<nx; i++) {
    for (p = &a[i*ndim]; p < &a[i*ndim+ny]; p++) {
      v = 2*abs(*p) - (*p > 0);
      top = v >> fs;
      /*
       * top is coded by top zeros + 1
       * can write up to 24 bits at a time with output_nbits
       */
      for ( ; top>24 ; top -= 24) output_nbits(outfile, 0, 24);
      output_nbits(outfile, 1, top+1);
      /*
       * bottom FS bits are written without coding
       * output_nbits picks off just those bits
       */
      if (fs > 0) output_nbits(outfile, v, fs);
    }
  }
  return (bmin);
}
