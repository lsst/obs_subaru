/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* qwrite.c Write binary data
 *
 * Programmer: R. White     Date: 11 March 1991
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "ftcl.h"
#include "dervish.h"
#include "region.h"
#include "math.h"
#include "dervish_msg_c.h"
#include "errStack.h"
#include "phStructs.h"
#include "phCompUtils.h"

#define COMP_DEBUG 10

extern int writeshort(MYFILE *outfile, int a);
extern int writeint(MYFILE *outfile, int a);
extern int qwrite(MYFILE *outfile, unsigned char *a, int n);
extern int  mywrite(MYFILE *file, unsigned char *buffer, int n);

extern int
writeshort(MYFILE *outfile, int a)
{
  int i;
  unsigned char b[2];
  
  /* Write integer A one byte at a time to outfile.
   *
   * This is portable from Vax to Sun since it eliminates the
   * need for byte-swapping.
   */
  for (i=1; i>=0; i--) {
    b[i] = a & 0x000000ff;
    a >>= 8;
  }
  qwrite(outfile,b,2);
  return (SH_SUCCESS);
}

extern int
writeint(MYFILE *outfile, int a)
{
  int i;
  unsigned char b[4];
  
  /* Write integer A one byte at a time to outfile.
   *
   * This is portable from Vax to Sun since it eliminates the
   * need for byte-swapping.
   */
  for (i=3; i>=0; i--) {
    b[i] = a & 0x000000ff;
    a >>= 8;
  }
  qwrite(outfile,b,4);
  return (SH_SUCCESS);
}

extern int
qwrite(MYFILE *outfile, unsigned char *a, int n)
{
  if(mywrite(outfile, a, n) != n) {
    shError("qwrite");
    return(SH_GENERIC_ERROR);
  }
  return(SH_GENERIC_ERROR);
}

/*
 * write n bytes from buffer into file
 * returns number of bytes written (=n) if successful, <=0 if not
 */
extern int
mywrite(MYFILE *file, unsigned char buffer[], int n)
{
#ifdef TO_A_BUFFER
  int nmax;
  /*
   * this version used when doing I/O to a buffer
   */
  if (file->current+n > file->end) {
    nmax = file->end - file->current;
    if (nmax > 0) memcpy((void *)file->current,(void *) buffer, nmax);
    file->current = file->end;
    shError("mywrite: buffer overflow\n");
    return (SH_GENERIC_ERROR);
  }
  memcpy((void *)file->current, (void *)buffer, n);
  file->current += n;
  return (n);
#else
  /*
   * this version used when doing I/O to a file
   */
  return ( fwrite(buffer, 1, n, file) );
#endif
}
